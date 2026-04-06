/*
 * Pump.cpp
 * --------
 * Implementation of the Pump component solver.
 */

#include "components/Pump.h"
#include <cmath>
#include <sstream>
#include <iomanip>
#include <stdexcept>

/* ---------------------------------------------------------------------------
 * Pump::solve
 * ---------------------------------------------------------------------------*/
ComponentResult Pump::solve(
    const ThermodynamicState& inlet,
    double P_out_kPa,
    double eta,
    DataSource source)
{
    if (eta <= 0.0 || eta > 1.0)
        throw std::invalid_argument(
            "Pump::solve: isentropic efficiency must be in (0, 1]");

    if (inlet.v <= 0.0 || std::isnan(inlet.v))
        throw std::invalid_argument(
            "Pump::solve: inlet specific volume must be known and positive. "
            "Use fromSaturatedP() or fromSaturatedT() with x=0.");

    ComponentResult result;
    result.componentType = "Bomba (Pump)";
    result.fluid         = inlet.fluid;

    /* ------------------------------------------------------------------
     * Step 1: Record inlet state
     * ------------------------------------------------------------------ */
    result.inlet = inlet;
    {
        std::ostringstream vals;
        vals << std::fixed << std::setprecision(5)
             << "T1=" << inlet.T_C << "°C  P1=" << inlet.P_kPa
             << " kPa  h1=" << std::setprecision(3) << inlet.h
             << " kJ/kg  v1=" << std::setprecision(6) << inlet.v
             << " m3/kg";
        result.addStep(
            "Estado 1 — entrada de la bomba (liquido saturado)",
            "", vals.str(), inlet.h, "kJ/kg");
    }

    /* ------------------------------------------------------------------
     * Step 2: Isentropic pump work (incompressible liquid)
     * w_s = v * (P2 - P1)   [kJ/kg]
     * Note: v in m³/kg, P in kPa → kJ/kg directly
     * ------------------------------------------------------------------ */
    double dP  = P_out_kPa - inlet.P_kPa;    /* kPa */
    double w_s = inlet.v * dP;               /* m³/kg * kPa = kJ/kg */
    {
        std::ostringstream eq, vals;
        eq   << "w_s = v1 * (P2 - P1)";
        vals << std::fixed << std::setprecision(6) << inlet.v
             << " * (" << std::setprecision(1)
             << P_out_kPa << " - " << inlet.P_kPa << ")"
             << " = " << std::setprecision(4) << inlet.v
             << " * " << std::setprecision(1) << dP;
        result.addStep(
            "Trabajo isentropico de la bomba (liquido incompresible)",
            eq.str(), vals.str(), w_s, "kJ/kg");
    }

    /* ------------------------------------------------------------------
     * Step 3: Actual work (with efficiency)
     * w_actual = w_s / eta_P
     * ------------------------------------------------------------------ */
    double w_actual = w_s / eta;
    result.specificWork = w_actual;
    result.specificHeat = 0.0;
    result.isentropicEfficiency = eta;
    {
        std::ostringstream eq, vals;
        if (eta < 1.0) {
            eq   << "w_p = w_s / eta_P";
            vals << std::fixed << std::setprecision(4)
                 << w_s << " / " << eta;
        } else {
            eq   << "w_p = w_s  (bomba ideal, eta=1)";
            vals << std::fixed << std::setprecision(4) << w_actual;
        }
        result.addStep(
            "Trabajo real de la bomba",
            eq.str(), vals.str(), w_actual, "kJ/kg");
    }

    /* ------------------------------------------------------------------
     * Step 4: Outlet enthalpy
     * h2 = h1 + w_actual
     * ------------------------------------------------------------------ */
    double h2 = inlet.h + w_actual;
    {
        std::ostringstream eq, vals;
        eq   << "h2 = h1 + w_p";
        vals << std::fixed << std::setprecision(3)
             << inlet.h << " + " << w_actual;
        result.addStep(
            "Entalpia a la salida de la bomba",
            eq.str(), vals.str(), h2, "kJ/kg");
    }

    /* ------------------------------------------------------------------
     * Step 5: Build outlet state
     * Approximately: T2 ≈ T1 (small temperature rise), P2 = P_out
     * ------------------------------------------------------------------ */
    ThermodynamicState outlet;
    outlet.fluid  = inlet.fluid;
    outlet.source = source;
    outlet.P_kPa  = P_out_kPa;
    outlet.h      = h2;
    outlet.s      = inlet.s;     /* isentropic approximation */
    outlet.v      = inlet.v;     /* incompressible: v ≈ constant */
    outlet.T_C    = inlet.T_C;   /* temperature rise is negligible */
    outlet.u      = h2 - P_out_kPa * inlet.v;
    outlet.x      = UNKNOWN;
    outlet.phase  = FluidPhase::COMPRESSED_LIQUID;

    result.outlet   = outlet;
    result.converged = true;
    {
        std::ostringstream vals;
        vals << std::fixed << std::setprecision(3)
             << "P2=" << P_out_kPa << " kPa  h2=" << h2
             << " kJ/kg  T2≈" << inlet.T_C << "°C";
        result.addStep(
            "Estado 2 — salida de la bomba (liquido comprimido)",
            "", vals.str(), h2, "kJ/kg");
    }

    /* ------------------------------------------------------------------
     * Energy balance: h1 + w = h2
     * ------------------------------------------------------------------ */
    result.balance.E_in      = inlet.h + w_actual;
    result.balance.E_out     = h2;
    result.balance.imbalance = std::fabs(result.balance.E_in -
                                          result.balance.E_out);
    result.balance.balanced  = result.balance.imbalance < 0.001;

    std::ostringstream balDesc;
    balDesc << std::fixed << std::setprecision(3)
            << "h1 + w_p = h2  →  "
            << inlet.h << " + " << w_actual << " = " << h2;
    result.balance.description = balDesc.str();

    return result;
}