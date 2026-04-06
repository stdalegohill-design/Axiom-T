/*
 * ThermodynamicState.cpp
 * ----------------------
 * Implementation of ThermodynamicState factory methods.
 * See ThermodynamicState.h for interface documentation.
 */

#include "components/ThermodynamicState.h"
#include "solvers/NumericalSolver.h"
#include <cmath>
#include <limits>

/* ---------------------------------------------------------------------------
 * Phase determination helper
 * ---------------------------------------------------------------------------*/
FluidPhase ThermodynamicState::determinePhase(
    double T_C, double P_kPa, double T_sat, double quality)
{
    if (!std::isnan(quality)) {
        if (quality <= 0.0) return FluidPhase::SATURATED_LIQUID;
        if (quality >= 1.0) return FluidPhase::SATURATED_VAPOR;
        return FluidPhase::TWO_PHASE;
    }
    if (T_C > T_sat + 0.01) return FluidPhase::SUPERHEATED_VAPOR;
    if (T_C < T_sat - 0.01) return FluidPhase::COMPRESSED_LIQUID;
    return FluidPhase::SATURATED_VAPOR;
}

/* ---------------------------------------------------------------------------
 * fromTP — from temperature and pressure
 * ---------------------------------------------------------------------------*/
ThermodynamicState ThermodynamicState::fromTP(
    const std::string& fluid,
    double T_C,
    double P_kPa,
    DataSource source)
{
    ThermodynamicState st;
    st.fluid  = fluid;
    st.source = source;
    st.T_C    = T_C;
    st.P_kPa  = P_kPa;
    st.x      = UNKNOWN;

    /* Try superheated steam table first */
    try {
        SuperheatedProps sp = DataCore::superheatedAt(T_C, P_kPa, source);
        st.h     = sp.h;
        st.u     = sp.u;
        st.s     = sp.s;
        st.v     = sp.v;
        st.Cp    = UNKNOWN;
        st.Cv    = UNKNOWN;
        st.phase = FluidPhase::SUPERHEATED_VAPOR;
        return st;
    } catch (...) {}

    /* Try ideal gas table */
    try {
        double T_K = T_C + 273.15;
        IdealGasProps ig = DataCore::idealGasAt(fluid, T_K, source);
        st.h     = ig.h;
        st.u     = ig.u;
        st.s     = ig.s0;   /* s at P_ref — caller adjusts for actual P */
        st.v     = UNKNOWN;
        st.Cp    = ig.Cp;
        st.Cv    = ig.Cv;
        st.phase = FluidPhase::IDEAL_GAS;
        return st;
    } catch (...) {}

    /* Try saturated liquid/vapor */
    try {
        SatWaterProps sat = DataCore::satWaterAtP(P_kPa, source);
        double T_sat = sat.T_C;
        if (T_C >= T_sat - 0.5) {
            /* Treat as saturated vapor */
            st.h     = sat.hg;
            st.s     = sat.sg;
            st.v     = sat.vg;
            st.u     = sat.hg - P_kPa * sat.vg;
            st.phase = FluidPhase::SATURATED_VAPOR;
        } else {
            st.h     = sat.hf;
            st.s     = sat.sf;
            st.v     = sat.vf;
            st.u     = sat.hf - P_kPa * sat.vf;
            st.phase = FluidPhase::COMPRESSED_LIQUID;
        }
        return st;
    } catch (...) {}

    throw std::runtime_error(
        "ThermodynamicState::fromTP: cannot determine state for fluid '" +
        fluid + "' at T=" + std::to_string(T_C) +
        "°C P=" + std::to_string(P_kPa) + " kPa");
}

/* ---------------------------------------------------------------------------
 * fromPS — from pressure and entropy (isentropic outlet)
 * Iterates to find T such that s(T, P) = s_target.
 * ---------------------------------------------------------------------------*/
ThermodynamicState ThermodynamicState::fromPS(
    const std::string& fluid,
    double P_kPa,
    double s_kJkgK,
    DataSource source)
{
    ThermodynamicState st;
    st.fluid  = fluid;
    st.source = source;
    st.P_kPa  = P_kPa;
    st.s      = s_kJkgK;
    st.x      = UNKNOWN;

    /* Check if point is in two-phase region */
    try {
        SatWaterProps sat = DataCore::satWaterAtP(P_kPa, source);
        if (s_kJkgK <= sat.sg && s_kJkgK >= sat.sf) {
            /* Two-phase mixture */
            double x = (s_kJkgK - sat.sf) / sat.sfg;
            st.T_C   = sat.T_C;
            st.x     = x;
            st.h     = sat.hf + x * sat.hfg;
            st.s     = s_kJkgK;
            st.v     = sat.vf + x * (sat.vg - sat.vf);
            st.u     = st.h - P_kPa * st.v;
            st.phase = (x <= 0.0) ? FluidPhase::SATURATED_LIQUID :
                       (x >= 1.0) ? FluidPhase::SATURATED_VAPOR  :
                                    FluidPhase::TWO_PHASE;
            return st;
        }
    } catch (...) {}

    /* Superheated region — iterate on T using bisection */
    /* Search T in range [T_sat, 800°C] for the superheated region */
    double T_lo = 0.0, T_hi = 800.0;

    /* Get T_sat as lower bound */
    try {
        SatWaterProps sat = DataCore::satWaterAtP(P_kPa, source);
        T_lo = sat.T_C + 1.0;
    } catch (...) {
        T_lo = -50.0;
    }

    /* Bisection to find T where s(T,P) = s_target */
    auto s_at_T = [&](double T) -> double {
        try {
            SuperheatedProps sp = DataCore::superheatedAt(T, P_kPa, source);
            return sp.s;
        } catch (...) {
            return UNKNOWN;
        }
    };

    double s_lo = s_at_T(T_lo);
    double s_hi = s_at_T(T_hi);

    if (std::isnan(s_lo) || std::isnan(s_hi))
        throw std::runtime_error(
            "ThermodynamicState::fromPS: cannot bracket s=" +
            std::to_string(s_kJkgK) + " at P=" + std::to_string(P_kPa) + " kPa");

    /* Expand upper bound if needed */
    while (s_hi < s_kJkgK && T_hi < 1500.0) {
        T_hi += 100.0;
        s_hi = s_at_T(T_hi);
    }

    /* Bisection */
    for (int i = 0; i < 60; ++i) {
        double T_mid = (T_lo + T_hi) / 2.0;
        double s_mid = s_at_T(T_mid);
        if (std::isnan(s_mid)) break;
        if (s_mid < s_kJkgK)
            T_lo = T_mid;
        else
            T_hi = T_mid;
        if (T_hi - T_lo < 0.001) break;
    }

    double T_final = (T_lo + T_hi) / 2.0;

    try {
        SuperheatedProps sp = DataCore::superheatedAt(T_final, P_kPa, source);
        st.T_C   = T_final;
        st.h     = sp.h;
        st.s     = sp.s;
        st.u     = sp.u;
        st.v     = sp.v;
        st.phase = FluidPhase::SUPERHEATED_VAPOR;
        return st;
    } catch (...) {}

    throw std::runtime_error(
        "ThermodynamicState::fromPS: failed to find state for fluid '" +
        fluid + "' at P=" + std::to_string(P_kPa) +
        " kPa, s=" + std::to_string(s_kJkgK));
}

/* ---------------------------------------------------------------------------
 * fromPH — from pressure and enthalpy
 * ---------------------------------------------------------------------------*/
ThermodynamicState ThermodynamicState::fromPH(
    const std::string& fluid,
    double P_kPa,
    double h_kJkg,
    DataSource source)
{
    ThermodynamicState st;
    st.fluid  = fluid;
    st.source = source;
    st.P_kPa  = P_kPa;
    st.h      = h_kJkg;
    st.x      = UNKNOWN;

    /* Check two-phase */
    try {
        SatWaterProps sat = DataCore::satWaterAtP(P_kPa, source);
        if (h_kJkg >= sat.hf && h_kJkg <= sat.hg) {
            double x = (h_kJkg - sat.hf) / sat.hfg;
            st.T_C   = sat.T_C;
            st.x     = x;
            st.s     = sat.sf + x * sat.sfg;
            st.v     = sat.vf + x * (sat.vg - sat.vf);
            st.u     = h_kJkg - P_kPa * st.v;
            st.phase = (x <= 0.0) ? FluidPhase::SATURATED_LIQUID :
                       (x >= 1.0) ? FluidPhase::SATURATED_VAPOR  :
                                    FluidPhase::TWO_PHASE;
            return st;
        }
    } catch (...) {}

    /* Superheated — bisect on T */
    double T_lo = 100.0, T_hi = 800.0;
    try {
        SatWaterProps sat = DataCore::satWaterAtP(P_kPa, source);
        T_lo = sat.T_C + 1.0;
    } catch (...) {}

    for (int i = 0; i < 60; ++i) {
        double T_mid = (T_lo + T_hi) / 2.0;
        try {
            SuperheatedProps sp = DataCore::superheatedAt(T_mid, P_kPa, source);
            if (sp.h < h_kJkg) T_lo = T_mid;
            else                T_hi = T_mid;
        } catch (...) { break; }
        if (T_hi - T_lo < 0.001) break;
    }

    double T_final = (T_lo + T_hi) / 2.0;
    try {
        SuperheatedProps sp = DataCore::superheatedAt(T_final, P_kPa, source);
        st.T_C   = T_final;
        st.s     = sp.s;
        st.u     = sp.u;
        st.v     = sp.v;
        st.phase = FluidPhase::SUPERHEATED_VAPOR;
        return st;
    } catch (...) {}

    throw std::runtime_error(
        "ThermodynamicState::fromPH: failed for fluid '" + fluid +
        "' at P=" + std::to_string(P_kPa) + " kPa, h=" +
        std::to_string(h_kJkg) + " kJ/kg");
}

/* ---------------------------------------------------------------------------
 * fromSaturatedT — saturated state at temperature
 * ---------------------------------------------------------------------------*/
ThermodynamicState ThermodynamicState::fromSaturatedT(
    const std::string& fluid,
    double T_C,
    double quality,
    DataSource source)
{
    ThermodynamicState st;
    st.fluid  = fluid;
    st.source = source;
    st.T_C    = T_C;
    st.x      = quality;

    SatWaterProps sat = DataCore::satWaterAtT(T_C, source);
    st.P_kPa = sat.P_kPa;

    if (quality <= 0.0) {
        st.h = sat.hf; st.s = sat.sf; st.v = sat.vf;
        st.phase = FluidPhase::SATURATED_LIQUID;
    } else if (quality >= 1.0) {
        st.h = sat.hg; st.s = sat.sg; st.v = sat.vg;
        st.phase = FluidPhase::SATURATED_VAPOR;
    } else {
        st.h = sat.hf + quality * sat.hfg;
        st.s = sat.sf + quality * sat.sfg;
        st.v = sat.vf + quality * (sat.vg - sat.vf);
        st.phase = FluidPhase::TWO_PHASE;
    }
    st.u = st.h - st.P_kPa * st.v;
    return st;
}

/* ---------------------------------------------------------------------------
 * fromSaturatedP — saturated state at pressure
 * ---------------------------------------------------------------------------*/
ThermodynamicState ThermodynamicState::fromSaturatedP(
    const std::string& fluid,
    double P_kPa,
    double quality,
    DataSource source)
{
    ThermodynamicState st;
    st.fluid  = fluid;
    st.source = source;
    st.P_kPa  = P_kPa;
    st.x      = quality;

    SatWaterProps sat = DataCore::satWaterAtP(P_kPa, source);
    st.T_C = sat.T_C;

    if (quality <= 0.0) {
        st.h = sat.hf; st.s = sat.sf; st.v = sat.vf;
        st.phase = FluidPhase::SATURATED_LIQUID;
    } else if (quality >= 1.0) {
        st.h = sat.hg; st.s = sat.sg; st.v = sat.vg;
        st.phase = FluidPhase::SATURATED_VAPOR;
    } else {
        st.h = sat.hf + quality * sat.hfg;
        st.s = sat.sf + quality * sat.sfg;
        st.v = sat.vf + quality * (sat.vg - sat.vf);
        st.phase = FluidPhase::TWO_PHASE;
    }
    st.u = st.h - st.P_kPa * st.v;
    return st;
}

/* ---------------------------------------------------------------------------
 * fromIdealGas — state from ideal gas tables (Cengel source)
 * ---------------------------------------------------------------------------*/
ThermodynamicState ThermodynamicState::fromIdealGas(
    const std::string& gas_name,
    double T_C,
    double P_kPa,
    DataSource source)
{
    ThermodynamicState st;
    st.fluid  = gas_name;
    st.source = source;
    st.T_C    = T_C;
    st.P_kPa  = P_kPa;
    st.x      = UNKNOWN;
    st.phase  = FluidPhase::IDEAL_GAS;

    double T_K = T_C + 273.15;
    IdealGasProps ig = DataCore::idealGasAt(gas_name, T_K, source);

    static constexpr double P_REF = 101.325; /* kPa */
    st.h  = ig.h;
    st.u  = ig.u;
    st.Cp = ig.Cp;
    st.Cv = ig.Cv;

    /* Look up R from gas properties */
    /* R = Cp - Cv if both available, otherwise use default */
    double R = (ig.Cp > 0 && ig.Cv > 0) ? ig.Cp - ig.Cv : 0.2870;

    /* s(T,P) = s°(T) - R·ln(P/P_ref) */
    st.s = ig.s0 - R * std::log(P_kPa / P_REF);

    /* v = R·T/P (R in kJ/(kg·K), P in kPa → v in m³/kg) */
    st.v = R * T_K / P_kPa;

    return st;
}