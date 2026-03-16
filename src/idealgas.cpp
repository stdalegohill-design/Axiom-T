/*
 * IdealGas.cpp
 * ------------
 * Implementation of IdealGas.
 * See IdealGas.h for interface documentation and references.
 */

#include "substances/IdealGas.h"
#include "solvers/NumericalSolver.h"
#include <cmath>
#include <stdexcept>

/* ---------------------------------------------------------------------------
 * Private constructor
 * ---------------------------------------------------------------------------*/
IdealGas::IdealGas(const GasProperties& props) : m_props(props) {}

/* ---------------------------------------------------------------------------
 * Factory methods
 * All constants from NIST Chemistry WebBook (2023) and
 * Cengel & Boles, Thermodynamics, 8th ed., Table A-2.
 *
 * Note on units: Cp, Cv, R are stored in kJ/(kg·K).
 * The engine operates in SI (Pa, K, kg, J) but thermodymanic calculations
 * use kJ because Cengel & Boles tabulates in kJ — conversion happens at
 * the output boundary (Energy class stores Joules internally).
 * ---------------------------------------------------------------------------*/
/*
 * Internal helper: derives Cv and gamma from Cp and R.
 *
 * Cp and R are the primary constants (sourced from NIST).
 * Cv and gamma are derived via the Mayer relation and definition:
 *   Cv    = Cp - R
 *   gamma = Cp / Cv
 *
 * This ensures perfect internal consistency regardless of rounding
 * in the source tables, and makes Cp - Cv == R and gamma == Cp/Cv
 * exact by construction.
 */
static GasProperties makeProps(
    const std::string& name, double M, double R, double Cp)
{
    GasProperties p;
    p.name  = name;
    p.M     = M;
    p.R     = R;
    p.Cp    = Cp;
    p.Cv    = Cp - R;
    p.gamma = Cp / (Cp - R);
    return p;
}

IdealGas IdealGas::Air() {
    /* M=28.97 kg/kmol, R=0.2870 kJ/(kg·K), Cp=1.005 kJ/(kg·K)
     * Source: Cengel & Boles, Table A-2 / NIST */
    return IdealGas(makeProps("Air", 28.97, 0.28700, 1.00500));
}

IdealGas IdealGas::Nitrogen() {
    /* M=28.014 kg/kmol, R=0.2968 kJ/(kg·K), Cp=1.039 kJ/(kg·K) */
    return IdealGas(makeProps("Nitrogen", 28.014, 0.29680, 1.03900));
}

IdealGas IdealGas::Oxygen() {
    /* M=32.000 kg/kmol, R=0.2598 kJ/(kg·K), Cp=0.918 kJ/(kg·K) */
    return IdealGas(makeProps("Oxygen", 32.000, 0.25980, 0.91800));
}

IdealGas IdealGas::CarbonDioxide() {
    /* M=44.010 kg/kmol, R=0.1889 kJ/(kg·K), Cp=0.846 kJ/(kg·K) */
    return IdealGas(makeProps("Carbon Dioxide", 44.010, 0.18890, 0.84600));
}

/* ---------------------------------------------------------------------------
 * Equation of state
 * ---------------------------------------------------------------------------*/
double IdealGas::specificVolume(const Pressure& P, const Temperature& T) const {
    /*
     * Pv = RT  =>  v = RT/P
     *
     * R  [kJ/(kg·K)]  = [kPa·m³/(kg·K)]
     * T  [K]
     * P  [Pa] → convert to kPa: divide by 1000
     *
     * v = R[kJ/(kg·K)] * T[K] / P[kPa]  →  [m³/kg]
     */
    double P_kPa = P.toKPa();
    double T_K   = T.toKelvin();

    if (P_kPa <= 0.0) {
        throw std::invalid_argument(
            "IdealGas::specificVolume: pressure must be positive."
        );
    }

    return m_props.R * T_K / P_kPa;
}

Volume IdealGas::volume(const GasState& state) const {
    double v = specificVolume(state.pressure, state.temperature);
    double m = state.mass.toKilogram();
    /* V [m³] = m[kg] * v[m³/kg] */
    return Volume::fromCubicMeter(m * v);
}

/* ---------------------------------------------------------------------------
 * Caloric equations
 * ---------------------------------------------------------------------------*/
double IdealGas::deltaEnthalpy(
    const Temperature& T1,
    const Temperature& T2
) const {
    /*
     * Δh = Cp · (T2 - T1)   [kJ/kg]
     *
     * Temperature difference is independent of scale (ΔT_K = ΔT_C).
     * We use Kelvin directly.
     */
    double dT = T2.toKelvin() - T1.toKelvin();
    return m_props.Cp * dT;
}

double IdealGas::deltaInternalEnergy(
    const Temperature& T1,
    const Temperature& T2
) const {
    double dT = T2.toKelvin() - T1.toKelvin();
    return m_props.Cv * dT;
}

/* ---------------------------------------------------------------------------
 * Entropy change
 * ---------------------------------------------------------------------------*/
double IdealGas::deltaEntropy(
    const Temperature& T1, const Pressure& P1,
    const Temperature& T2, const Pressure& P2
) const {
    /*
     * Δs = Cp · ln(T2/T1) - R · ln(P2/P1)   [kJ/(kg·K)]
     *
     * Cengel & Boles, Eq. 7-25.
     * Pressures can be in any consistent unit (ratio is dimensionless).
     */
    double T1_K = T1.toKelvin();
    double T2_K = T2.toKelvin();
    double P1_v = P1.toPascal();
    double P2_v = P2.toPascal();

    if (T1_K <= 0.0 || T2_K <= 0.0) {
        throw std::invalid_argument(
            "IdealGas::deltaEntropy: temperatures must be positive (Kelvin)."
        );
    }
    if (P1_v <= 0.0 || P2_v <= 0.0) {
        throw std::invalid_argument(
            "IdealGas::deltaEntropy: pressures must be positive."
        );
    }

    return m_props.Cp * std::log(T2_K / T1_K)
         - m_props.R  * std::log(P2_v / P1_v);
}

/* ---------------------------------------------------------------------------
 * Boundary work — numerical integration via Simpson 1/3
 * ---------------------------------------------------------------------------*/
double IdealGas::boundaryWork(
    const Pressure& P1, const Temperature& T1,
    const Pressure& P2, const Temperature& T2,
    int n_panels
) const {
    /*
     * w_b = ∫(v1 to v2) P dv
     *
     * For a linear process in the T-P space (which covers isobaric,
     * isothermal, and generic linear processes), we parameterize:
     *
     *   P(ξ) = P1 + (P2 - P1) · ξ          ξ ∈ [0, 1]
     *   T(ξ) = T1 + (T2 - T1) · ξ
     *   v(ξ) = R · T(ξ) / P(ξ)    [m³/kg from kJ/(kg·K) and kPa]
     *
     * dv/dξ = R · [T'(ξ)·P(ξ) - T(ξ)·P'(ξ)] / P(ξ)²
     *
     * w_b = ∫₀¹ P(ξ) · dv/dξ dξ
     *     = R · ∫₀¹ [T'·P - T·P'] / P dξ
     *     = R · ∫₀¹ [T' - T·P'/P] dξ
     *
     * where T' = (T2_K - T1_K), P' = (P2_kPa - P1_kPa) are constants.
     * Result in kJ/kg.
     */
    double P1_kPa = P1.toKPa();
    double P2_kPa = P2.toKPa();
    double T1_K   = T1.toKelvin();
    double T2_K   = T2.toKelvin();

    double dT = T2_K   - T1_K;
    double dP = P2_kPa - P1_kPa;

    auto integrand = [&](double xi) -> double {
        double P_xi = P1_kPa + dP * xi;
        if (std::fabs(P_xi) < 1e-12) return 0.0;
        return m_props.R * (dT - (T1_K + dT * xi) * dP / P_xi);
    };

    SolverResult result = NumericalSolver::simpsonOneThird(
        integrand, 0.0, 1.0, n_panels
    );

    return result.solution;
}

/* ---------------------------------------------------------------------------
 * Isentropic relations
 * ---------------------------------------------------------------------------*/
Temperature IdealGas::adiabaticTemperature(
    const Temperature& T1,
    const Pressure&    P1,
    const Pressure&    P2
) const {
    /*
     * T2/T1 = (P2/P1)^((γ-1)/γ)
     * Cengel & Boles, Eq. 7-44 (cold-air standard).
     */
    double T1_K      = T1.toKelvin();
    double ratio     = P2.toPascal() / P1.toPascal();
    double exponent  = (m_props.gamma - 1.0) / m_props.gamma;
    double T2_K      = T1_K * std::pow(ratio, exponent);
    return Temperature::fromKelvin(T2_K);
}

Pressure IdealGas::adiabaticPressure(
    const Temperature& T1,
    const Temperature& T2,
    const Pressure&    P1
) const {
    /*
     * P2/P1 = (T2/T1)^(γ/(γ-1))
     */
    double T1_K    = T1.toKelvin();
    double T2_K    = T2.toKelvin();
    double exponent = m_props.gamma / (m_props.gamma - 1.0);
    double P2_Pa   = P1.toPascal() * std::pow(T2_K / T1_K, exponent);
    return Pressure::fromPascal(P2_Pa);
}

/* ---------------------------------------------------------------------------
 * Isentropic efficiencies
 * ---------------------------------------------------------------------------*/
double IdealGas::isentropicEfficiencyCompressor(
    const Temperature& T1,
    const Temperature& T2_actual,
    const Temperature& T2s
) const {
    double T1_K  = T1.toKelvin();
    double T2a_K = T2_actual.toKelvin();
    double T2s_K = T2s.toKelvin();

    double w_actual     = T2a_K - T1_K;
    double w_isentropic = T2s_K - T1_K;

    if (w_actual <= 0.0) {
        throw std::invalid_argument(
            "IdealGas::isentropicEfficiencyCompressor: "
            "T2_actual must be greater than T1 for a compressor."
        );
    }

    return w_isentropic / w_actual;
}

double IdealGas::isentropicEfficiencyTurbine(
    const Temperature& T1,
    const Temperature& T2_actual,
    const Temperature& T2s
) const {
    double T1_K  = T1.toKelvin();
    double T2a_K = T2_actual.toKelvin();
    double T2s_K = T2s.toKelvin();

    double w_actual     = T1_K - T2a_K;
    double w_isentropic = T1_K - T2s_K;

    if (w_isentropic <= 0.0) {
        throw std::invalid_argument(
            "IdealGas::isentropicEfficiencyTurbine: "
            "T2s must be less than T1 for a turbine."
        );
    }

    return w_actual / w_isentropic;
}