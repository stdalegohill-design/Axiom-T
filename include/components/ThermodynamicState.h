#ifndef THERMODYNAMIC_STATE_H
#define THERMODYNAMIC_STATE_H

/*
 * ThermodynamicState.h
 * --------------------
 * Represents a complete thermodynamic state of a fluid at a given point
 * in a system (inlet, outlet, intermediate state, etc.).
 *
 * Design philosophy:
 *   - Fluid-agnostic: works with any fluid available in coolprop.db or
 *     thermodata.db (water, air, refrigerants, etc.)
 *   - All properties in SI/kJ units consistent with Cengel & Boles notation
 *   - Unknown properties initialized to UNKNOWN (NaN sentinel)
 *   - DataSource tracks which database provided the values
 *
 * Usage:
 *   ThermodynamicState s1 = ThermodynamicState::fromTP(
 *       "Water", 450.0, 5000.0, DataSource::COOLPROP);
 *
 *   ThermodynamicState s2 = ThermodynamicState::fromPS(
 *       "Water", 1400.0, s1.s, DataSource::COOLPROP);
 *
 * References:
 *   Cengel & Boles, Thermodynamics, 7th ed., Chapters 3, 6, 10.
 */

#include "datacore/DataCore.h"
#include <string>
#include <cmath>
#include <stdexcept>

/* Sentinel value for unknown/unset properties */
static constexpr double UNKNOWN = std::numeric_limits<double>::quiet_NaN();

/* ---------------------------------------------------------------------------
 * FluidPhase — thermodynamic phase of the fluid
 * ---------------------------------------------------------------------------*/
enum class FluidPhase {
    SUPERHEATED_VAPOR,   /* T > T_sat at given P, or P < P_sat at given T */
    SATURATED_VAPOR,     /* quality x = 1                                  */
    TWO_PHASE,           /* 0 < x < 1 — liquid-vapor mixture               */
    SATURATED_LIQUID,    /* quality x = 0                                  */
    COMPRESSED_LIQUID,   /* T < T_sat at given P                           */
    SUPERCRITICAL,       /* T > T_crit and P > P_crit                      */
    IDEAL_GAS,           /* modeled as ideal gas (no phase concept)        */
    UNKNOWN_PHASE        /* phase not yet determined                       */
};

/* ---------------------------------------------------------------------------
 * ThermodynamicState
 * ---------------------------------------------------------------------------*/
struct ThermodynamicState {

    /* -----------------------------------------------------------------------
     * Fluid identification
     * ----------------------------------------------------------------------- */
    std::string fluid;     /* fluid name matching coolprop.db/thermodata.db  */
    DataSource  source;    /* which database provided the properties         */

    /* -----------------------------------------------------------------------
     * Primary properties
     * All in kJ/kg or kJ/(kg·K) consistent with Cengel notation.
     * UNKNOWN = NaN when the property has not been set.
     * ----------------------------------------------------------------------- */
    double P_kPa;          /* pressure [kPa]                                 */
    double T_C;            /* temperature [°C]                               */
    double h;              /* specific enthalpy [kJ/kg]                      */
    double s;              /* specific entropy [kJ/(kg·K)]                   */
    double u;              /* specific internal energy [kJ/kg]               */
    double v;              /* specific volume [m³/kg]                        */
    double x;              /* quality [-] — UNKNOWN if not two-phase         */
    double Cp;             /* specific heat at const. pressure [kJ/(kg·K)]  */
    double Cv;             /* specific heat at const. volume [kJ/(kg·K)]    */

    FluidPhase phase;      /* thermodynamic phase                            */

    /* -----------------------------------------------------------------------
     * Convenience accessors
     * ----------------------------------------------------------------------- */
    double T_K()    const { return T_C + 273.15; }
    double P_Pa()   const { return P_kPa * 1000.0; }
    double P_MPa()  const { return P_kPa / 1000.0; }
    double rho()    const { return (v > 0.0) ? 1.0 / v : UNKNOWN; }
    bool   isKnown(double val) const { return !std::isnan(val); }

    /* -----------------------------------------------------------------------
     * Factory methods — construct state from known property pairs
     * ----------------------------------------------------------------------- */

    /*
     * fromTP() — state from temperature and pressure.
     * Works for superheated vapor, compressed liquid, supercritical, ideal gas.
     * T_C in [°C], P_kPa in [kPa].
     */
    static ThermodynamicState fromTP(
        const std::string& fluid,
        double T_C,
        double P_kPa,
        DataSource source = DataSource::COOLPROP
    );

    /*
     * fromPS() — state from pressure and entropy (isentropic processes).
     * Used to find the outlet of an isentropic turbine or compressor.
     * P_kPa in [kPa], s_kJkgK in [kJ/(kg·K)].
     * Requires iteration to find T such that s(T,P) = s_target.
     */
    static ThermodynamicState fromPS(
        const std::string& fluid,
        double P_kPa,
        double s_kJkgK,
        DataSource source = DataSource::COOLPROP
    );

    /*
     * fromPH() — state from pressure and enthalpy.
     * Used after throttling valves (h = const) or mixing chambers.
     */
    static ThermodynamicState fromPH(
        const std::string& fluid,
        double P_kPa,
        double h_kJkg,
        DataSource source = DataSource::COOLPROP
    );

    /*
     * fromSaturatedT() — saturated state at given temperature.
     * x = 0 → saturated liquid, x = 1 → saturated vapor, 0<x<1 → mixture.
     */
    static ThermodynamicState fromSaturatedT(
        const std::string& fluid,
        double T_C,
        double quality,
        DataSource source = DataSource::COOLPROP
    );

    /*
     * fromSaturatedP() — saturated state at given pressure.
     */
    static ThermodynamicState fromSaturatedP(
        const std::string& fluid,
        double P_kPa,
        double quality,
        DataSource source = DataSource::COOLPROP
    );

    /*
     * fromIdealGas() — state using ideal gas model (Fase 1 engine).
     * gas_name must match a gas in thermodata.db or IdealGas class.
     * Uses DataSource::CENGEL by default for academic exercises.
     */
    static ThermodynamicState fromIdealGas(
        const std::string& gas_name,
        double T_C,
        double P_kPa,
        DataSource source = DataSource::CENGEL
    );

    /* -----------------------------------------------------------------------
     * Default constructor — all properties unknown
     * ----------------------------------------------------------------------- */
    ThermodynamicState()
        : fluid(""), source(DataSource::COOLPROP),
          P_kPa(UNKNOWN), T_C(UNKNOWN),
          h(UNKNOWN), s(UNKNOWN), u(UNKNOWN), v(UNKNOWN),
          x(UNKNOWN), Cp(UNKNOWN), Cv(UNKNOWN),
          phase(FluidPhase::UNKNOWN_PHASE) {}

private:
    /* Internal helper to determine phase from sat properties */
    static FluidPhase determinePhase(double T_C, double P_kPa,
                                      double T_sat, double quality);
};

#endif // THERMODYNAMIC_STATE_H