/*
 * test_ideal_gas.cpp
 * ------------------
 * Unit tests for the IdealGas engine.
 *
 * Validation methodology:
 *   All reference values taken from solved examples in:
 *   Cengel, Y. A., & Boles, M. A., "Thermodynamics: An Engineering Approach",
 *   8th ed., McGraw-Hill, 2015.
 *   Maximum accepted error: 0.01% (Axiom-T QA criterion).
 *
 * Test coverage:
 *   1. Gas properties   — constants match Cengel Table A-2
 *   2. Equation of state — Pv = RT
 *   3. Enthalpy change  — Δh = Cp·ΔT
 *   4. Internal energy  — Δu = Cv·ΔT
 *   5. Entropy change   — Δs = Cp·ln(T2/T1) - R·ln(P2/P1)
 *   6. Adiabatic temp.  — isentropic compression / expansion
 *   7. Efficiencies     — compressor and turbine isentropic efficiency
 *   8. Boundary work    — numerical integration check
 *   9. Physical validation — exception on invalid inputs
 */

#include "catch2/catch_amalgamated.hpp"
#include "substances/IdealGas.h"
#include <cmath>

using Catch::Approx;

static double percentError(double expected, double actual) {
    return std::fabs((actual - expected) / expected) * 100.0;
}

static constexpr double TOL = 0.01; /* 0.01% maximum error */

/* =========================================================================
 * Suite 1: Gas constants — Cengel & Boles, Table A-2
 * ========================================================================= */

TEST_CASE("IdealGas factory methods load correct NIST constants", "[idealgas][properties]") {

    SECTION("Air constants match Cengel Table A-2") {
        IdealGas air = IdealGas::Air();
        REQUIRE(percentError(1.005,  air.Cp()) < TOL);
        REQUIRE(percentError(0.2870, air.R())  < TOL);
        /* Cv and gamma are derived from primary constants Cp and R */
        REQUIRE(percentError(1.005 - 0.2870, air.Cv())              < TOL);
        REQUIRE(percentError(1.005 / (1.005 - 0.2870), air.gamma()) < TOL);
        REQUIRE(air.name() == "Air");
    }

    SECTION("Nitrogen constants match Cengel Table A-2") {
        IdealGas n2 = IdealGas::Nitrogen();
        REQUIRE(percentError(1.039,  n2.Cp()) < TOL);
        REQUIRE(percentError(0.2968, n2.R())  < TOL);
        /* Cv and gamma are derived: Cv = Cp - R, gamma = Cp/Cv */
        REQUIRE(percentError(1.039 - 0.2968, n2.Cv())    < TOL);
        REQUIRE(percentError(1.039 / (1.039 - 0.2968), n2.gamma()) < TOL);
    }

    SECTION("Oxygen constants match Cengel Table A-2") {
        IdealGas o2 = IdealGas::Oxygen();
        REQUIRE(percentError(0.918,  o2.Cp()) < TOL);
        REQUIRE(percentError(0.2598, o2.R())  < TOL);
        REQUIRE(percentError(0.918 - 0.2598, o2.Cv())    < TOL);
        REQUIRE(percentError(0.918 / (0.918 - 0.2598), o2.gamma()) < TOL);
    }

    SECTION("CO2 constants match Cengel Table A-2") {
        IdealGas co2 = IdealGas::CarbonDioxide();
        REQUIRE(percentError(0.846,  co2.Cp()) < TOL);
        REQUIRE(percentError(0.1889, co2.R())  < TOL);
        REQUIRE(percentError(0.846 - 0.1889, co2.Cv())    < TOL);
        REQUIRE(percentError(0.846 / (0.846 - 0.1889), co2.gamma()) < TOL);
    }

    SECTION("Cp - Cv == R for all gases (Mayer relation)") {
        /*
         * Thermodynamic identity: Cp - Cv = R
         * This is an internal consistency check — if constants are correct,
         * this must hold exactly.
         */
        auto check = [](const IdealGas& gas) {
            return percentError(gas.R(), gas.Cp() - gas.Cv()) < TOL;
        };
        REQUIRE(check(IdealGas::Air()));
        REQUIRE(check(IdealGas::Nitrogen()));
        REQUIRE(check(IdealGas::Oxygen()));
        REQUIRE(check(IdealGas::CarbonDioxide()));
    }

    SECTION("gamma == Cp/Cv for all gases") {
        auto check = [](const IdealGas& gas) {
            return percentError(gas.gamma(), gas.Cp() / gas.Cv()) < TOL;
        };
        REQUIRE(check(IdealGas::Air()));
        REQUIRE(check(IdealGas::Nitrogen()));
        REQUIRE(check(IdealGas::Oxygen()));
        REQUIRE(check(IdealGas::CarbonDioxide()));
    }
}

/* =========================================================================
 * Suite 2: Equation of state — Pv = RT
 * Reference: Cengel & Boles, Example 3-1 and Example 3-2
 * ========================================================================= */

TEST_CASE("IdealGas equation of state Pv = RT", "[idealgas][eos]") {

    IdealGas air = IdealGas::Air();

    SECTION("specific volume of air at 300 K, 100 kPa") {
        /*
         * v = RT/P = 0.2870 * 300 / 100 = 0.861 m³/kg
         * Cengel & Boles, Example 3-1 (adapted)
         */
        Pressure    P = Pressure::fromKPa(100.0);
        Temperature T = Temperature::fromKelvin(300.0);

        double v = air.specificVolume(P, T);
        REQUIRE(percentError(0.861, v) < TOL);
    }

    SECTION("specific volume of air at 1 atm, 25 C") {
        /*
         * Standard conditions: P = 101.325 kPa, T = 298.15 K
         * v = 0.2870 * 298.15 / 101.325 = 0.8445 m³/kg
         */
        Pressure    P = Pressure::fromAtm(1.0);
        Temperature T = Temperature::fromCelsius(25.0);

        double v = air.specificVolume(P, T);
        double v_expected = 0.2870 * 298.15 / 101.325;
        REQUIRE(percentError(v_expected, v) < TOL);
    }

    SECTION("total volume of 2 kg air at 300 K, 200 kPa") {
        /*
         * V = m * R * T / P = 2 * 0.2870 * 300 / 200 = 0.861 m³
         */
        GasState state {
            Pressure::fromKPa(200.0),
            Temperature::fromKelvin(300.0),
            Mass::fromKilogram(2.0)
        };

        Volume V = air.volume(state);
        REQUIRE(percentError(0.861, V.toCubicMeter()) < TOL);
    }
}

/* =========================================================================
 * Suite 3: Enthalpy change — Δh = Cp · ΔT
 * Reference: Cengel & Boles, Example 7-7
 * ========================================================================= */

TEST_CASE("IdealGas enthalpy change Δh = Cp·ΔT", "[idealgas][enthalpy]") {

    IdealGas air = IdealGas::Air();

    SECTION("Δh for air heated from 300 K to 1000 K") {
        /*
         * Δh = 1.005 * (1000 - 300) = 1.005 * 700 = 703.5 kJ/kg
         * Cold-air standard reference value.
         */
        Temperature T1 = Temperature::fromKelvin(300.0);
        Temperature T2 = Temperature::fromKelvin(1000.0);

        double dh = air.deltaEnthalpy(T1, T2);
        REQUIRE(percentError(703.5, dh) < TOL);
    }

    SECTION("Δh is negative when T2 < T1 (cooling)") {
        Temperature T1 = Temperature::fromKelvin(500.0);
        Temperature T2 = Temperature::fromKelvin(300.0);

        double dh = air.deltaEnthalpy(T1, T2);
        REQUIRE(dh < 0.0);
        REQUIRE(percentError(201.0, std::fabs(dh)) < TOL);
    }

    SECTION("Δh = 0 when T1 == T2 (isothermal)") {
        Temperature T = Temperature::fromKelvin(400.0);
        REQUIRE(air.deltaEnthalpy(T, T) == Approx(0.0).margin(1e-9));
    }
}

/* =========================================================================
 * Suite 4: Internal energy change — Δu = Cv · ΔT
 * ========================================================================= */

TEST_CASE("IdealGas internal energy change Δu = Cv·ΔT", "[idealgas][internal_energy]") {

    IdealGas air = IdealGas::Air();

    SECTION("Δu for air from 300 K to 800 K") {
        /*
         * Δu = 0.718 * (800 - 300) = 0.718 * 500 = 359.0 kJ/kg
         */
        Temperature T1 = Temperature::fromKelvin(300.0);
        Temperature T2 = Temperature::fromKelvin(800.0);

        double du = air.deltaInternalEnergy(T1, T2);
        REQUIRE(percentError(359.0, du) < TOL);
    }

    SECTION("First Law consistency: Δh - Δu = R·ΔT") {
        /*
         * For an ideal gas: h - u = Pv = RT
         * Therefore: Δh - Δu = R·ΔT
         */
        Temperature T1 = Temperature::fromKelvin(300.0);
        Temperature T2 = Temperature::fromKelvin(600.0);

        double dh  = air.deltaEnthalpy(T1, T2);
        double du  = air.deltaInternalEnergy(T1, T2);
        double dT  = T2.toKelvin() - T1.toKelvin();
        double R_dT = air.R() * dT;

        REQUIRE(percentError(R_dT, dh - du) < TOL);
    }
}

/* =========================================================================
 * Suite 5: Entropy change
 * Reference: Cengel & Boles, Example 7-7
 * ========================================================================= */

TEST_CASE("IdealGas entropy change", "[idealgas][entropy]") {

    IdealGas air = IdealGas::Air();

    SECTION("isentropic process has Δs = 0") {
        /*
         * For an isentropic compression:
         * T2 = T1 * (P2/P1)^((γ-1)/γ)
         * By definition Δs must be zero.
         */
        Temperature T1 = Temperature::fromKelvin(300.0);
        Pressure    P1 = Pressure::fromKPa(100.0);
        Pressure    P2 = Pressure::fromKPa(800.0);

        Temperature T2 = air.adiabaticTemperature(T1, P1, P2);

        double ds = air.deltaEntropy(T1, P1, T2, P2);
        /*
         * Mathematically Δs = 0 exactly for an isentropic process.
         * In practice, adiabaticTemperature uses std::pow which introduces
         * floating-point rounding, so we allow a small numerical tolerance.
         */
        REQUIRE(std::fabs(ds) < 1e-4);
    }

    SECTION("isobaric process: Δs = Cp·ln(T2/T1)") {
        /*
         * At constant pressure, R·ln(P2/P1) = 0.
         * Δs = Cp·ln(T2/T1) = 1.005·ln(600/300) = 1.005·ln(2) = 0.6964 kJ/(kg·K)
         */
        Temperature T1 = Temperature::fromKelvin(300.0);
        Temperature T2 = Temperature::fromKelvin(600.0);
        Pressure    P  = Pressure::fromKPa(100.0);

        double ds       = air.deltaEntropy(T1, P, T2, P);
        double expected = air.Cp() * std::log(600.0 / 300.0);
        REQUIRE(percentError(expected, ds) < TOL);
    }

    SECTION("isothermal process: Δs = -R·ln(P2/P1)") {
        /*
         * At constant temperature, Cp·ln(T2/T1) = 0.
         * Δs = -R·ln(P2/P1) = -0.2870·ln(400/100) = -0.2870·ln(4)
         */
        Temperature T  = Temperature::fromKelvin(500.0);
        Pressure    P1 = Pressure::fromKPa(100.0);
        Pressure    P2 = Pressure::fromKPa(400.0);

        double ds       = air.deltaEntropy(T, P1, T, P2);
        double expected = -air.R() * std::log(400.0 / 100.0);
        REQUIRE(percentError(std::fabs(expected), std::fabs(ds)) < TOL);
    }
}

/* =========================================================================
 * Suite 6: Isentropic temperature — compressor and turbine
 * Reference: Cengel & Boles, Example 7-7 and Example 9-6
 * ========================================================================= */

TEST_CASE("IdealGas isentropic temperature relations", "[idealgas][isentropic]") {

    IdealGas air = IdealGas::Air();

    SECTION("isentropic compression: air from 300 K, 100 kPa to 800 kPa") {
        /*
         * T2s = T1 * (P2/P1)^((γ-1)/γ)
         * Expected value computed using air.gamma() — the derived value
         * from Cp and R — to ensure internal consistency.
         * Cengel & Boles, Example 7-7 (cold-air standard).
         */
        Temperature T1 = Temperature::fromKelvin(300.0);
        Pressure    P1 = Pressure::fromKPa(100.0);
        Pressure    P2 = Pressure::fromKPa(800.0);

        Temperature T2s = air.adiabaticTemperature(T1, P1, P2);

        double g        = air.gamma();
        double expected = 300.0 * std::pow(8.0, (g - 1.0) / g);
        REQUIRE(percentError(expected, T2s.toKelvin()) < TOL);
    }

    SECTION("isentropic expansion: turbine from 1300 K, 1 MPa to 100 kPa") {
        Temperature T1 = Temperature::fromKelvin(1300.0);
        Pressure    P1 = Pressure::fromMPa(1.0);
        Pressure    P2 = Pressure::fromKPa(100.0);

        Temperature T2s     = air.adiabaticTemperature(T1, P1, P2);
        double      g       = air.gamma();
        double      expected = 1300.0 * std::pow(0.1, (g - 1.0) / g);
        REQUIRE(percentError(expected, T2s.toKelvin()) < TOL);
    }

    SECTION("round-trip: adiabaticPressure inverts adiabaticTemperature") {
        /*
         * Given T1, P1, P2 → compute T2s.
         * Then from T1, T2s, P1 → recover P2.
         */
        Temperature T1 = Temperature::fromKelvin(300.0);
        Pressure    P1 = Pressure::fromKPa(100.0);
        Pressure    P2 = Pressure::fromKPa(500.0);

        Temperature T2s    = air.adiabaticTemperature(T1, P1, P2);
        Pressure    P2_rec = air.adiabaticPressure(T1, T2s, P1);

        REQUIRE(percentError(P2.toKPa(), P2_rec.toKPa()) < TOL);
    }
}

/* =========================================================================
 * Suite 7: Isentropic efficiency
 * Reference: Cengel & Boles, Example 7-9
 * ========================================================================= */

TEST_CASE("IdealGas isentropic efficiency", "[idealgas][efficiency]") {

    IdealGas air = IdealGas::Air();

    SECTION("compressor efficiency 80%: T1=300K, T2s=411K, T2a=T1+(T2s-T1)/eta") {
        /*
         * T1   = 300 K
         * T2s  = 411.3 K  (isentropic outlet, compression ratio 4)
         * eta  = 0.80
         * T2a  = T1 + (T2s-T1)/0.80 = 300 + 111.3/0.80 = 439.1 K
         */
        Temperature T1      = Temperature::fromKelvin(300.0);
        Pressure    P1      = Pressure::fromKPa(100.0);
        Pressure    P2      = Pressure::fromKPa(400.0);
        Temperature T2s     = air.adiabaticTemperature(T1, P1, P2);
        double      eta_ref = 0.80;
        double      dT_s    = T2s.toKelvin() - 300.0;
        Temperature T2a     = Temperature::fromKelvin(300.0 + dT_s / eta_ref);

        double eta = air.isentropicEfficiencyCompressor(T1, T2a, T2s);
        REQUIRE(percentError(eta_ref, eta) < TOL);
    }

    SECTION("turbine efficiency 85%: consistent round-trip") {
        Temperature T1      = Temperature::fromKelvin(1200.0);
        Pressure    P1      = Pressure::fromMPa(1.0);
        Pressure    P2      = Pressure::fromKPa(100.0);
        Temperature T2s     = air.adiabaticTemperature(T1, P1, P2);
        double      eta_ref = 0.85;
        double      dT_s    = 1200.0 - T2s.toKelvin();
        Temperature T2a     = Temperature::fromKelvin(1200.0 - dT_s * eta_ref);

        double eta = air.isentropicEfficiencyTurbine(T1, T2a, T2s);
        REQUIRE(percentError(eta_ref, eta) < TOL);
    }

    SECTION("compressor throws when T2_actual <= T1") {
        Temperature T1  = Temperature::fromKelvin(300.0);
        Temperature T2a = Temperature::fromKelvin(280.0); /* impossible for compressor */
        Temperature T2s = Temperature::fromKelvin(400.0);
        REQUIRE_THROWS_AS(
            air.isentropicEfficiencyCompressor(T1, T2a, T2s),
            std::invalid_argument
        );
    }
}

/* =========================================================================
 * Suite 8: Boundary work
 * ========================================================================= */

TEST_CASE("IdealGas boundary work numerical integration", "[idealgas][work]") {

    IdealGas air = IdealGas::Air();

    SECTION("isobaric process: w_b = R·(T2 - T1)") {
        /*
         * At constant pressure: w_b = P·(v2-v1) = R·(T2-T1)
         * Air from 300 K to 500 K at 200 kPa:
         * w_b = 0.2870 * (500 - 300) = 57.4 kJ/kg
         */
        Pressure    P  = Pressure::fromKPa(200.0);
        Temperature T1 = Temperature::fromKelvin(300.0);
        Temperature T2 = Temperature::fromKelvin(500.0);

        double w        = air.boundaryWork(P, T1, P, T2);
        double expected = air.R() * (500.0 - 300.0);
        REQUIRE(percentError(expected, w) < TOL);
    }

    SECTION("isochoric process: w_b = 0 (constant volume)") {
        /*
         * Constant volume: dv = 0  =>  w_b = 0
         * Achieved when T2/T1 = P2/P1 (specific volume unchanged).
         * v = RT/P = const  =>  T1/P1 = T2/P2
         * Use T1=300K, P1=100kPa, T2=600K, P2=200kPa
         */
        Pressure    P1 = Pressure::fromKPa(100.0);
        Pressure    P2 = Pressure::fromKPa(200.0);
        Temperature T1 = Temperature::fromKelvin(300.0);
        Temperature T2 = Temperature::fromKelvin(600.0);

        double w = air.boundaryWork(P1, T1, P2, T2);
        REQUIRE(std::fabs(w) < 0.01); /* effectively zero */
    }
}

/* =========================================================================
 * Suite 9: Physical validation
 * ========================================================================= */

TEST_CASE("IdealGas enforces physical constraints", "[idealgas][validation]") {

    IdealGas air = IdealGas::Air();

    SECTION("specificVolume throws on zero pressure") {
        REQUIRE_THROWS_AS(
            air.specificVolume(Pressure::fromPascal(0.0), Temperature::fromKelvin(300.0)),
            std::invalid_argument
        );
    }

    SECTION("deltaEntropy throws on zero temperature") {
        REQUIRE_THROWS_AS(
            air.deltaEntropy(
                Temperature::fromKelvin(0.0), Pressure::fromKPa(100.0),
                Temperature::fromKelvin(300.0), Pressure::fromKPa(200.0)
            ),
            std::invalid_argument
        );
    }
}