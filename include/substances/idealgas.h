#ifndef IDEAL_GAS_H
#define IDEAL_GAS_H

/*
 * IdealGas.h
 * ----------
 * Engine for thermodynamic calculations on ideal gases with constant
 * specific heats (cold air standard / perfect gas model).
 *
 * Scope of this module (Phase 1):
 *   - Four reference gases: Air, Nitrogen, Oxygen, Carbon Dioxide
 *   - Constant specific heats (Cp, Cv) evaluated at 300 K reference
 *   - Ideal gas equation of state: Pv = RT
 *   - Caloric equations: Δh = Cp·ΔT, Δu = Cv·ΔT
 *   - Entropy change: Δs = Cp·ln(T2/T1) - R·ln(P2/P1)
 *   - Boundary work for closed systems: w = -∫v dP (computed via Simpson)
 *   - Isentropic relations: T2/T1 = (P2/P1)^((γ-1)/γ)
 *
 * Out of scope (Phase 2 — DataCore):
 *   - Variable specific heats (Tables A-17 to A-25, Cengel & Boles)
 *   - Real gas corrections (compressibility factor Z)
 *   - Steam and refrigerant tables
 *
 * All gas constants sourced from NIST Chemistry WebBook (2023).
 * Validation reference: Cengel & Boles, Thermodynamics, 8th ed., Chapters 3, 7.
 *
 * Design pattern: same as UnitSystem — private constructor + static factory
 * methods. The user cannot instantiate IdealGas directly; they must call
 * IdealGas::Air(), IdealGas::Nitrogen(), etc.
 */

#include "units/Pressure.h"
#include "units/Temperature.h"
#include "units/Mass.h"
#include "units/Energy.h"
#include "units/Volume.h"

#include <string>
#include <stdexcept>

/* ---------------------------------------------------------------------------
 * GasProperties
 * Intrinsic constants for one gas species. All values at standard conditions.
 * ---------------------------------------------------------------------------*/
struct GasProperties {
    std::string name;       /* human-readable name, e.g. "Air"               */
    double M;               /* molar mass [kg/kmol]                          */
    double R;               /* specific gas constant [kJ/(kg·K)]             */
    double Cp;              /* specific heat at constant pressure [kJ/(kg·K)]*/
    double Cv;              /* specific heat at constant volume  [kJ/(kg·K)] */
    double gamma;           /* specific heat ratio Cp/Cv [-]                 */
};

/* ---------------------------------------------------------------------------
 * GasState
 * Complete thermodynamic state of an ideal gas sample.
 * Requires pressure, temperature and mass to be fully determined.
 * ---------------------------------------------------------------------------*/
struct GasState {
    Pressure    pressure;
    Temperature temperature;
    Mass        mass;
};

/* ---------------------------------------------------------------------------
 * IdealGas
 * ---------------------------------------------------------------------------*/
class IdealGas {
public:

    /* -----------------------------------------------------------------------
     * Factory methods — the only valid way to create an IdealGas instance.
     * Constants sourced from NIST Chemistry WebBook (2023).
     * ----------------------------------------------------------------------- */

    /* Dry air (modeled as a mixture: ~78% N2, ~21% O2, ~1% Ar)
     * M = 28.97 kg/kmol, R = 0.2870 kJ/(kg·K)
     * Cp = 1.005 kJ/(kg·K),  Cv = 0.718 kJ/(kg·K),  gamma = 1.400
     * Reference: Cengel & Boles, Table A-2 */
    static IdealGas Air();

    /* Nitrogen (N2)
     * M = 28.014 kg/kmol, R = 0.2968 kJ/(kg·K)
     * Cp = 1.039 kJ/(kg·K),  Cv = 0.743 kJ/(kg·K),  gamma = 1.400
     * Reference: Cengel & Boles, Table A-2 */
    static IdealGas Nitrogen();

    /* Oxygen (O2)
     * M = 32.000 kg/kmol, R = 0.2598 kJ/(kg·K)
     * Cp = 0.918 kJ/(kg·K),  Cv = 0.658 kJ/(kg·K),  gamma = 1.395
     * Reference: Cengel & Boles, Table A-2 */
    static IdealGas Oxygen();

    /* Carbon dioxide (CO2)
     * M = 44.010 kg/kmol, R = 0.1889 kJ/(kg·K)
     * Cp = 0.846 kJ/(kg·K),  Cv = 0.657 kJ/(kg·K),  gamma = 1.289
     * Reference: Cengel & Boles, Table A-2 */
    static IdealGas CarbonDioxide();

    /* -----------------------------------------------------------------------
     * Accessors
     * ----------------------------------------------------------------------- */

    const GasProperties& properties() const { return m_props; }
    std::string          name()        const { return m_props.name; }
    double               R()           const { return m_props.R; }
    double               Cp()          const { return m_props.Cp; }
    double               Cv()          const { return m_props.Cv; }
    double               gamma()       const { return m_props.gamma; }
    double               molarMass()   const { return m_props.M; }

    /* -----------------------------------------------------------------------
     * Equation of state: Pv = RT
     *
     * Computes specific volume [m³/kg] at a given state.
     * v = R·T / P   (R in kJ/(kg·K), T in K, P in kPa → v in m³/kg)
     * ----------------------------------------------------------------------- */
    double specificVolume(const Pressure& P, const Temperature& T) const;

    /*
     * Total volume [m³] for a mass sample at given state.
     * V = m · v = m · R · T / P
     */
    Volume volume(const GasState& state) const;

    /* -----------------------------------------------------------------------
     * Caloric equation — enthalpy change [kJ/kg]
     *
     * For an ideal gas, enthalpy depends only on temperature:
     *   Δh = Cp · (T2 - T1)
     *
     * Returns specific enthalpy change in kJ/kg.
     * Sign convention: positive when T2 > T1 (heat addition).
     * ----------------------------------------------------------------------- */
    double deltaEnthalpy(const Temperature& T1, const Temperature& T2) const;

    /* -----------------------------------------------------------------------
     * Caloric equation — internal energy change [kJ/kg]
     *
     *   Δu = Cv · (T2 - T1)
     * ----------------------------------------------------------------------- */
    double deltaInternalEnergy(const Temperature& T1, const Temperature& T2) const;

    /* -----------------------------------------------------------------------
     * Entropy change [kJ/(kg·K)]
     *
     * Gibbs equation for an ideal gas (Cengel & Boles, Eq. 7-25):
     *   Δs = Cp · ln(T2/T1) - R · ln(P2/P1)
     *
     * Returns specific entropy change in kJ/(kg·K).
     * For an isentropic process, Δs = 0 by definition.
     * ----------------------------------------------------------------------- */
    double deltaEntropy(
        const Temperature& T1, const Pressure& P1,
        const Temperature& T2, const Pressure& P2
    ) const;

    /* -----------------------------------------------------------------------
     * Boundary work for a closed system process [kJ/kg]
     *
     * w_b = ∫P dv  (computed numerically via Simpson 1/3)
     *
     * For common processes, analytical results are also available:
     *   Isobaric  (P=const): w_b = P · (v2 - v1) = R · (T2 - T1)
     *   Isochoric (v=const): w_b = 0
     *   Isothermal (T=const): w_b = R·T · ln(v2/v1) = R·T · ln(P1/P2)
     *   Polytropic: w_b = (P2·v2 - P1·v1) / (1 - n)
     *
     * This method computes the general case numerically.
     * n_panels must be even (Simpson requirement).
     * ----------------------------------------------------------------------- */
    double boundaryWork(
        const Pressure& P1, const Temperature& T1,
        const Pressure& P2, const Temperature& T2,
        int n_panels = 100
    ) const;

    /* -----------------------------------------------------------------------
     * Isentropic temperature ratio
     *
     * For a reversible adiabatic process (Cengel & Boles, Eq. 7-44):
     *   T2/T1 = (P2/P1)^((γ-1)/γ)
     *
     * Given T1 and the pressure ratio, returns T2.
     * Fundamental in compressor and turbine analysis.
     * ----------------------------------------------------------------------- */
    Temperature adiabaticTemperature(
        const Temperature& T1,
        const Pressure&    P1,
        const Pressure&    P2
    ) const;

    /* -----------------------------------------------------------------------
     * Isentropic pressure ratio
     *
     * Inverse of adiabaticTemperature: given T1, T2 and P1, returns P2.
     *   P2/P1 = (T2/T1)^(γ/(γ-1))
     * ----------------------------------------------------------------------- */
    Pressure adiabaticPressure(
        const Temperature& T1,
        const Temperature& T2,
        const Pressure&    P1
    ) const;

    /* -----------------------------------------------------------------------
     * Isentropic efficiency of a compressor
     *
     * η_c = w_isentropic / w_actual = (h2s - h1) / (h2 - h1)
     *     = (T2s - T1) / (T2_actual - T1)
     *
     * Parameters:
     *   T1         — inlet temperature
     *   T2_actual  — actual outlet temperature (measured)
     *   T2s        — isentropic outlet temperature (from adiabaticTemperature)
     *
     * Returns efficiency as a dimensionless value in [0, 1].
     * Throws if T2_actual <= T1 (would imply cooling, not compression).
     * ----------------------------------------------------------------------- */
    double isentropicEfficiencyCompressor(
        const Temperature& T1,
        const Temperature& T2_actual,
        const Temperature& T2s
    ) const;

    /* -----------------------------------------------------------------------
     * Isentropic efficiency of a turbine
     *
     * η_t = w_actual / w_isentropic = (h1 - h2) / (h1 - h2s)
     *     = (T1 - T2_actual) / (T1 - T2s)
     * ----------------------------------------------------------------------- */
    double isentropicEfficiencyTurbine(
        const Temperature& T1,
        const Temperature& T2_actual,
        const Temperature& T2s
    ) const;

private:

    explicit IdealGas(const GasProperties& props);

    GasProperties m_props;

    /*
     * Universal gas constant [kJ/(kmol·K)]
     * Source: NIST CODATA 2018 — R_u = 8.31446 kJ/(kmol·K)
     */
    static constexpr double R_UNIVERSAL = 8.31446;
};

#endif // IDEAL_GAS_H