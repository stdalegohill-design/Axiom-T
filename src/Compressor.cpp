/*
 * Compressor.cpp
 * --------------
 * Implementation of the Compressor component solver.
 */

#include "components/Compressor.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <stdexcept>

/* ---------------------------------------------------------------------------
 * Compressor::solve
 * ---------------------------------------------------------------------------*/
ComponentResult Compressor::solve(
    const ThermodynamicState& inlet,
    double P_out_kPa,
    double eta,
    DataSource source)
{
    if (eta <= 0.0 || eta > 1.0)
        throw std::invalid_argument(
            "Compressor::solve: isentropic efficiency must be in (0, 1]");

    ComponentResult result;
    result.componentType = "Compresor (Compressor)";
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
            "Estado 1 — entrada del compresor",
            "", vals.str(), inlet.h, "kJ/kg");
    }

    /* ------------------------------------------------------------------
     * Step 2: Isentropic outlet state (State 2s)
     * s2s = s1, P2 = P_out
     * ------------------------------------------------------------------ */
    ThermodynamicState outlet_s;

    /* For ideal gas: use adiabaticTemperature relation */
    if (inlet.phase == FluidPhase::IDEAL_GAS) {
        /* T2s = T1 * (P2/P1)^((gamma-1)/gamma) */
        double gamma = (inlet.Cp > 0 && inlet.Cv > 0)
                       ? inlet.Cp / inlet.Cv : 1.4;
        double T1_K  = inlet.T_K();
        double T2s_K = T1_K * std::pow(P_out_kPa / inlet.P_kPa,
                                        (gamma - 1.0) / gamma);
        outlet_s = ThermodynamicState::fromIdealGas(
            inlet.fluid, T2s_K - 273.15, P_out_kPa, source);
        outlet_s.phase = FluidPhase::IDEAL_GAS;
    } else {
        try {
            outlet_s = ThermodynamicState::fromPS(
                inlet.fluid, P_out_kPa, inlet.s, source);
        } catch (const std::exception& e) {
            throw std::runtime_error(
                std::string("Compressor::solve: cannot find isentropic "
                            "outlet: ") + e.what());
        }
    }

    result.outletS = outlet_s;
    {
        std::ostringstream eq, vals;
        eq   << "s2s = s1  (proceso isentropico)";
        vals << std::fixed << std::setprecision(3)
             << "T2s=" << outlet_s.T_C << "°C  "
             << "h2s=" << outlet_s.h   << " kJ/kg  "
             << "P2="  << P_out_kPa    << " kPa";
        result.addStep(
            "Estado 2s — salida isentropica (compresor ideal)",
            eq.str(), vals.str(), outlet_s.h, "kJ/kg");
    }

    /* ------------------------------------------------------------------
     * Step 3: Isentropic work input
     * w_s = h2s - h1
     * ------------------------------------------------------------------ */
    double w_s = outlet_s.h - inlet.h;
    {
        std::ostringstream eq, vals;
        eq   << "w_s = h2s - h1";
        vals << std::fixed << std::setprecision(3)
             << outlet_s.h << " - " << inlet.h;
        result.addStep(
            "Trabajo isentropico especifico",
            eq.str(), vals.str(), w_s, "kJ/kg");
    }

    /* ------------------------------------------------------------------
     * Step 4: Actual outlet state
     * eta_C = w_s / w_actual  →  h2 = h1 + w_s / eta
     * ------------------------------------------------------------------ */
    double h2_actual = inlet.h + w_s / eta;
    ThermodynamicState outlet_actual;

    if (inlet.phase == FluidPhase::IDEAL_GAS) {
        /* For ideal gas: find T2 from h2 */
        double Cp    = (inlet.Cp > 0) ? inlet.Cp : 1.005;
        double dT    = (h2_actual - inlet.h) / Cp;
        double T2_C  = inlet.T_C + dT;
        outlet_actual = ThermodynamicState::fromIdealGas(
            inlet.fluid, T2_C, P_out_kPa, source);
        outlet_actual.h     = h2_actual;
        outlet_actual.phase = FluidPhase::IDEAL_GAS;
    } else {
        try {
            outlet_actual = ThermodynamicState::fromPH(
                inlet.fluid, P_out_kPa, h2_actual, source);
        } catch (...) {
            outlet_actual       = outlet_s;
            outlet_actual.h     = h2_actual;
            outlet_actual.P_kPa = P_out_kPa;
        }
    }

    result.outlet = outlet_actual;
    {
        std::ostringstream eq, vals;
        if (eta < 1.0) {
            eq   << "h2 = h1 + w_s / eta_C";
            vals << std::fixed << std::setprecision(3)
                 << inlet.h << " + " << w_s << " / " << eta;
        } else {
            eq   << "h2 = h2s  (compresor ideal, eta=1)";
            vals << std::fixed << std::setprecision(3) << h2_actual;
        }
        result.addStep(
            "Estado 2 — salida real del compresor",
            eq.str(), vals.str(), h2_actual, "kJ/kg");
    }

    /* ------------------------------------------------------------------
     * Step 5: Actual work input
     * w_actual = h2 - h1
     * ------------------------------------------------------------------ */
    double w_actual = h2_actual - inlet.h;
    result.specificWork = w_actual;   /* positive = work input */
    result.specificHeat = 0.0;
    result.isentropicEfficiency = eta;
    result.converged = true;
    {
        std::ostringstream eq, vals;
        eq   << "w_entrada = h2 - h1";
        vals << std::fixed << std::setprecision(3)
             << h2_actual << " - " << inlet.h;
        result.addStep(
            "Trabajo de entrada al compresor",
            eq.str(), vals.str(), w_actual, "kJ/kg");
    }

    /* ------------------------------------------------------------------
     * Step 6: Verify efficiency
     * ------------------------------------------------------------------ */
    if (eta < 1.0) {
        double eta_calc = w_s / w_actual;
        std::ostringstream eq, vals;
        eq   << "eta_C = w_s / w_actual = (h2s-h1) / (h2-h1)";
        vals << std::fixed << std::setprecision(3)
             << w_s << " / " << w_actual << " = " << eta_calc;
        result.addStep(
            "Verificacion eficiencia isentropica",
            eq.str(), vals.str(), eta_calc * 100.0, "%");
    }

    /* ------------------------------------------------------------------
     * Energy balance: h1 + w = h2  (steady-flow, adiabatic)
     * ------------------------------------------------------------------ */
    result.balance.E_in      = inlet.h + w_actual;
    result.balance.E_out     = h2_actual;
    result.balance.imbalance = std::fabs(result.balance.E_in -
                                          result.balance.E_out);
    result.balance.balanced  = result.balance.imbalance < 0.01;

    std::ostringstream balDesc;
    balDesc << std::fixed << std::setprecision(3)
            << "h1 + w_entrada = h2  →  "
            << inlet.h << " + " << w_actual << " = " << h2_actual;
    result.balance.description = balDesc.str();

    return result;
}

/* ---------------------------------------------------------------------------
 * Compressor::solveFromStates
 * ---------------------------------------------------------------------------*/
ComponentResult Compressor::solveFromStates(
    const ThermodynamicState& inlet,
    const ThermodynamicState& outlet,
    const std::string& reference)
{
    ComponentResult result;
    result.componentType = "Compresor (Compressor)";
    result.fluid         = inlet.fluid;
    result.reference     = reference;
    result.inlet         = inlet;
    result.outlet        = outlet;
    result.converged     = true;

    {
        std::ostringstream vals;
        vals << std::fixed << std::setprecision(3)
             << "h1=" << inlet.h << " kJ/kg  T1=" << inlet.T_C << "°C";
        result.addStep("Estado 1 (entrada)", "", vals.str(),
                       inlet.h, "kJ/kg");
    }
    {
        std::ostringstream vals;
        vals << std::fixed << std::setprecision(3)
             << "h2=" << outlet.h << " kJ/kg  T2=" << outlet.T_C << "°C";
        result.addStep("Estado 2 (salida)", "", vals.str(),
                       outlet.h, "kJ/kg");
    }

    double w = outlet.h - inlet.h;
    result.specificWork = w;
    result.specificHeat = 0.0;
    {
        std::ostringstream eq, vals;
        eq   << "w_entrada = h2 - h1";
        vals << std::fixed << std::setprecision(3)
             << outlet.h << " - " << inlet.h;
        result.addStep("Trabajo de entrada", eq.str(), vals.str(),
                       w, "kJ/kg");
    }

    result.balance.E_in      = inlet.h + w;
    result.balance.E_out     = outlet.h;
    result.balance.imbalance = std::fabs(result.balance.E_in -
                                          result.balance.E_out);
    result.balance.balanced  = result.balance.imbalance < 0.01;

    std::ostringstream balDesc;
    balDesc << std::fixed << std::setprecision(3)
            << "h1 + w = h2  →  "
            << inlet.h << " + " << w << " = " << outlet.h;
    result.balance.description = balDesc.str();

    return result;
}