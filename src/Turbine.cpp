/*
 * Turbine.cpp
 * -----------
 * Implementation of the Turbine component solver.
 * See Turbine.h for interface documentation.
 */

#include "components/Turbine.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <stdexcept>

/* ---------------------------------------------------------------------------
 * Turbine::solve
 * ---------------------------------------------------------------------------*/
ComponentResult Turbine::solve(
    const ThermodynamicState& inlet,
    double P_out_kPa,
    double eta,
    DataSource source)
{
    if (eta <= 0.0 || eta > 1.0)
        throw std::invalid_argument(
            "Turbine::solve: isentropic efficiency must be in (0, 1]");

    ComponentResult result;
    result.componentType = "Turbina (Turbine)";
    result.fluid         = inlet.fluid;

    /* ------------------------------------------------------------------
     * Step 1: Record inlet state
     * ------------------------------------------------------------------ */
    result.inlet = inlet;
    {
        std::ostringstream vals;
        vals << std::fixed << std::setprecision(3)
             << "T1=" << inlet.T_C << "°C  P1=" << inlet.P_kPa
             << " kPa  h1=" << inlet.h << " kJ/kg  s1=" << inlet.s
             << " kJ/(kg·K)";
        result.addStep(
            "Estado 1 — entrada de la turbina",
            "",
            vals.str(),
            inlet.h,
            "kJ/kg"
        );
    }

    /* ------------------------------------------------------------------
     * Step 2: Find isentropic outlet state (State 2s)
     * s2s = s1, P2 = P_out
     * ------------------------------------------------------------------ */
    ThermodynamicState outlet_s;
    try {
        outlet_s = ThermodynamicState::fromPS(
            inlet.fluid, P_out_kPa, inlet.s, source);
    } catch (const std::exception& e) {
        throw std::runtime_error(
            std::string("Turbine::solve: cannot find isentropic outlet: ") +
            e.what());
    }

    result.outletS = outlet_s;
    {
        std::ostringstream eq, vals;
        eq   << "s2s = s1  (proceso isentropico)";
        vals << std::fixed << std::setprecision(3)
             << "s2s=" << outlet_s.s << " kJ/(kg·K)  "
             << "P2=" << P_out_kPa << " kPa  "
             << "T2s=" << outlet_s.T_C << "°C  "
             << "h2s=" << outlet_s.h << " kJ/kg";
        result.addStep(
            "Estado 2s — salida isentropica (proceso ideal)",
            eq.str(), vals.str(),
            outlet_s.h, "kJ/kg"
        );
    }

    /* ------------------------------------------------------------------
     * Step 3: Isentropic work
     * w_s = h1 - h2s
     * ------------------------------------------------------------------ */
    double w_s = inlet.h - outlet_s.h;
    {
        std::ostringstream eq, vals;
        eq   << "w_s = h1 - h2s";
        vals << std::fixed << std::setprecision(3)
             << inlet.h << " - " << outlet_s.h;
        result.addStep(
            "Trabajo isentropico especifico",
            eq.str(), vals.str(), w_s, "kJ/kg"
        );
    }

    /* ------------------------------------------------------------------
     * Step 4: Actual outlet state
     * eta_T = w_actual / w_s  →  h2 = h1 - eta*w_s
     * ------------------------------------------------------------------ */
    double h2_actual = inlet.h - eta * w_s;
    ThermodynamicState outlet_actual;

    try {
        outlet_actual = ThermodynamicState::fromPH(
            inlet.fluid, P_out_kPa, h2_actual, source);
    } catch (...) {
        /* Fallback: use isentropic state with adjusted h */
        outlet_actual        = outlet_s;
        outlet_actual.h      = h2_actual;
        outlet_actual.P_kPa  = P_out_kPa;
    }

    result.outlet = outlet_actual;
    {
        std::ostringstream eq, vals;
        if (eta < 1.0) {
            eq   << "h2 = h1 - eta_T * w_s";
            vals << std::fixed << std::setprecision(3)
                 << inlet.h << " - " << eta << " * " << w_s;
        } else {
            eq   << "h2 = h2s  (turbina ideal, eta=1)";
            vals << std::fixed << std::setprecision(3) << h2_actual;
        }
        result.addStep(
            "Estado 2 — salida real de la turbina",
            eq.str(), vals.str(),
            h2_actual, "kJ/kg"
        );
    }

    /* ------------------------------------------------------------------
     * Step 5: Actual work output
     * w_actual = h1 - h2
     * ------------------------------------------------------------------ */
    double w_actual = inlet.h - h2_actual;
    result.specificWork = w_actual;
    result.specificHeat = 0.0;  /* adiabatic turbine */
    result.isentropicEfficiency = eta;
    result.converged = true;
    {
        std::ostringstream eq, vals;
        eq   << "w_salida = h1 - h2";
        vals << std::fixed << std::setprecision(3)
             << inlet.h << " - " << h2_actual;
        result.addStep(
            "Trabajo de salida de la turbina",
            eq.str(), vals.str(), w_actual, "kJ/kg"
        );
    }

    /* ------------------------------------------------------------------
     * Step 6: Isentropic efficiency (if eta < 1 given, verify it)
     * eta_T = w_actual / w_s = (h1-h2) / (h1-h2s)
     * ------------------------------------------------------------------ */
    if (eta < 1.0) {
        double eta_calc = w_actual / w_s;
        std::ostringstream eq, vals;
        eq   << "eta_T = w_actual / w_s = (h1-h2) / (h1-h2s)";
        vals << std::fixed << std::setprecision(3)
             << w_actual << " / " << w_s
             << " = " << eta_calc;
        result.addStep(
            "Verificacion eficiencia isentropica",
            eq.str(), vals.str(), eta_calc * 100.0, "%"
        );
    }

    /* ------------------------------------------------------------------
     * Energy balance: h1 = w + h2  (adiabatic steady-flow)
     * ------------------------------------------------------------------ */
    result.balance.E_in       = inlet.h;
    result.balance.E_out      = w_actual + h2_actual;
    result.balance.imbalance  = std::fabs(result.balance.E_in -
                                           result.balance.E_out);
    result.balance.balanced   = result.balance.imbalance < 0.01;

    std::ostringstream balDesc;
    balDesc << std::fixed << std::setprecision(3)
            << "h1 = w_salida + h2  →  "
            << inlet.h << " = " << w_actual << " + " << h2_actual;
    result.balance.description = balDesc.str();

    return result;
}

/* ---------------------------------------------------------------------------
 * Turbine::solveFromStates
 * Given states directly (e.g. from Cengel tables)
 * ---------------------------------------------------------------------------*/
ComponentResult Turbine::solveFromStates(
    const ThermodynamicState& inlet,
    const ThermodynamicState& outlet,
    const std::string& reference)
{
    ComponentResult result;
    result.componentType = "Turbina (Turbine)";
    result.fluid         = inlet.fluid;
    result.reference     = reference;
    result.inlet         = inlet;
    result.outlet        = outlet;
    result.converged     = true;

    /* Step 1: inlet */
    {
        std::ostringstream vals;
        vals << std::fixed << std::setprecision(3)
             << "h1=" << inlet.h << " kJ/kg  s1=" << inlet.s
             << " kJ/(kg·K)  P1=" << inlet.P_kPa << " kPa";
        result.addStep("Estado 1 (entrada)", "", vals.str(), inlet.h, "kJ/kg");
    }

    /* Step 2: outlet */
    {
        std::ostringstream vals;
        vals << std::fixed << std::setprecision(3)
             << "h2=" << outlet.h << " kJ/kg  s2=" << outlet.s
             << " kJ/(kg·K)  P2=" << outlet.P_kPa << " kPa";
        result.addStep("Estado 2 (salida)", "", vals.str(), outlet.h, "kJ/kg");
    }

    /* Step 3: work */
    double w = inlet.h - outlet.h;
    result.specificWork = w;
    result.specificHeat = 0.0;
    {
        std::ostringstream eq, vals;
        eq   << "w_salida = h1 - h2";
        vals << std::fixed << std::setprecision(3)
             << inlet.h << " - " << outlet.h;
        result.addStep("Trabajo de salida", eq.str(), vals.str(), w, "kJ/kg");
    }

    /* Energy balance */
    result.balance.E_in      = inlet.h;
    result.balance.E_out     = w + outlet.h;
    result.balance.imbalance = std::fabs(result.balance.E_in -
                                          result.balance.E_out);
    result.balance.balanced  = result.balance.imbalance < 0.01;

    std::ostringstream balDesc;
    balDesc << std::fixed << std::setprecision(3)
            << "h1 = w + h2  →  " << inlet.h
            << " = " << w << " + " << outlet.h;
    result.balance.description = balDesc.str();

    return result;
}