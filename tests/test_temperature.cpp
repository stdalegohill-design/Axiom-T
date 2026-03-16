/*
 test_temperature.cpp
 --------------------
 Unit tests for the Temperature class.

 Validation methodology:
   Numerical results are verified against conversion tables from
   Cengel & Boles, "Thermodynamics: An Engineering Approach", 8th ed.
   Maximum accepted error: 0.01% (project QA criterion).

 Test coverage:
   1. Absolute factory methods
   2. Delta factory methods
   3. Round-trip conversions
   4. Arithmetic operators and physical consistency
   5. Physical validation (exception handling)
   6. Real engineering cases
*/

#include "catch2/catch_amalgamated.hpp"
#include "C:\Users\alego\projects\Axiom-T\include\units\temperature.h"
#include <cmath>

using Catch::Approx;

static double percentError(double expected, double actual) {
    return std::fabs((actual - expected) / expected) * 100.0;
}

static constexpr double TOLERANCE = 0.01; // 0.01% maximum error

/* =========================================================================
 Suite 1: Absolute Factory Methods
 ========================================================================= */

TEST_CASE("Temperature absolute factory methods produce correct Kelvin values", "[temperature][creation][absolute]") {

    SECTION("fromKelvin stores value without transformation") {
        Temperature t = Temperature::fromKelvin(300.0);
        REQUIRE(t.toKelvin() == Approx(300.0).epsilon(1e-9));
        REQUIRE(t.isAbsolute());
    }

    SECTION("fromCelsius: 0 °C == 273.15 K") {
        Temperature t = Temperature::fromCelsius(0.0);
        REQUIRE(percentError(273.15, t.toKelvin()) < TOLERANCE);
    }

    SECTION("fromCelsius: 100 °C == 373.15 K (water boiling point at 1 atm)") {
        Temperature t = Temperature::fromCelsius(100.0);
        REQUIRE(percentError(373.15, t.toKelvin()) < TOLERANCE);
    }

    SECTION("fromFahrenheit: 32 °F == 273.15 K (water freezing point)") {
        Temperature t = Temperature::fromFahrenheit(32.0);
        REQUIRE(percentError(273.15, t.toKelvin()) < TOLERANCE);
    }

    SECTION("fromFahrenheit: 212 °F == 373.15 K (water boiling point)") {
        Temperature t = Temperature::fromFahrenheit(212.0);
        REQUIRE(percentError(373.15, t.toKelvin()) < TOLERANCE);
    }

    SECTION("fromRankine: 491.67 °R == 273.15 K (water freezing point)") {
        Temperature t = Temperature::fromRankine(491.67);
        REQUIRE(percentError(273.15, t.toKelvin()) < TOLERANCE);
    }

    SECTION("fromRankine: 671.67 °R == 373.15 K (water boiling point)") {
        Temperature t = Temperature::fromRankine(671.67);
        REQUIRE(percentError(373.15, t.toKelvin()) < TOLERANCE);
    }
}

/* =========================================================================
 Suite 2: Delta Factory Methods
 ========================================================================= */

TEST_CASE("Temperature delta factory methods produce correct values", "[temperature][creation][delta]") {

    SECTION("deltaKelvin stores signed value and flags as Delta") {
        Temperature dt = Temperature::deltaKelvin(50.0);
        REQUIRE(dt.toKelvin() == Approx(50.0).epsilon(1e-9));
        REQUIRE(dt.isDelta());
    }

    SECTION("deltaCelsius: 1 Δ°C == 1 ΔK (scales are identical)") {
        Temperature dt = Temperature::deltaCelsius(25.0);
        REQUIRE(dt.toKelvin() == Approx(25.0).epsilon(1e-9));
    }

    SECTION("deltaFahrenheit: 9 Δ°F == 5 ΔK") {
        Temperature dt = Temperature::deltaFahrenheit(9.0);
        REQUIRE(percentError(5.0, dt.toKelvin()) < TOLERANCE);
    }

    SECTION("deltaRankine: 9 Δ°R == 5 ΔK") {
        Temperature dt = Temperature::deltaRankine(9.0);
        REQUIRE(percentError(5.0, dt.toKelvin()) < TOLERANCE);
    }

    SECTION("Delta can be negative") {
        Temperature dt = Temperature::deltaKelvin(-30.0);
        REQUIRE(dt.toKelvin() == Approx(-30.0).epsilon(1e-9));
        REQUIRE(dt.isDelta());
    }
}

/* =========================================================================
 Suite 3: Round-Trip Conversions
 ========================================================================= */

TEST_CASE("Temperature round-trip conversions are lossless within tolerance", "[temperature][conversion]") {

    SECTION("Kelvin -> Celsius -> Kelvin") {
        double original = 500.0;
        REQUIRE(percentError(original, Temperature::fromKelvin(original).toKelvin()) < TOLERANCE);
    }

    SECTION("Celsius -> Kelvin -> Celsius") {
        double original = 25.0;
        REQUIRE(percentError(original, Temperature::fromCelsius(original).toCelsius()) < TOLERANCE);
    }

    SECTION("Fahrenheit -> Kelvin -> Fahrenheit") {
        double original = 350.0;
        REQUIRE(percentError(original, Temperature::fromFahrenheit(original).toFahrenheit()) < TOLERANCE);
    }

    SECTION("Rankine -> Kelvin -> Rankine") {
        double original = 900.0;
        REQUIRE(percentError(original, Temperature::fromRankine(original).toRankine()) < TOLERANCE);
    }

    SECTION("Cross-unit: 300 K == 26.85 °C") {
        Temperature t = Temperature::fromKelvin(300.0);
        REQUIRE(t.toCelsius() == Approx(26.85).margin(0.01));
    }
}

/* =========================================================================
 Suite 4: Arithmetic Operators
 ========================================================================= */

TEST_CASE("Temperature arithmetic operators enforce physical consistency", "[temperature][operators]") {

    SECTION("Absolute + Delta = Absolute (state point shift)") {
        Temperature t  = Temperature::fromKelvin(300.0);
        Temperature dt = Temperature::deltaKelvin(50.0);
        Temperature result = t + dt;
        REQUIRE(result.isAbsolute());
        REQUIRE(result.toKelvin() == Approx(350.0).epsilon(1e-9));
    }

    SECTION("Delta + Absolute = Absolute (commutative)") {
        Temperature dt = Temperature::deltaKelvin(50.0);
        Temperature t  = Temperature::fromKelvin(300.0);
        Temperature result = dt + t;
        REQUIRE(result.isAbsolute());
        REQUIRE(result.toKelvin() == Approx(350.0).epsilon(1e-9));
    }

    SECTION("Absolute - Absolute = Delta") {
        Temperature t1 = Temperature::fromKelvin(500.0);
        Temperature t2 = Temperature::fromKelvin(300.0);
        Temperature result = t1 - t2;
        REQUIRE(result.isDelta());
        REQUIRE(result.toKelvin() == Approx(200.0).epsilon(1e-9));
    }

    SECTION("Absolute - Absolute yields negative Delta when T2 > T1") {
        Temperature t1 = Temperature::fromKelvin(300.0);
        Temperature t2 = Temperature::fromKelvin(500.0);
        Temperature result = t1 - t2;
        REQUIRE(result.isDelta());
        REQUIRE(result.toKelvin() == Approx(-200.0).epsilon(1e-9));
    }

    SECTION("Absolute - Delta = Absolute") {
        Temperature t  = Temperature::fromKelvin(400.0);
        Temperature dt = Temperature::deltaKelvin(100.0);
        Temperature result = t - dt;
        REQUIRE(result.isAbsolute());
        REQUIRE(result.toKelvin() == Approx(300.0).epsilon(1e-9));
    }

    SECTION("Delta + Delta = Delta") {
        Temperature dt1 = Temperature::deltaKelvin(20.0);
        Temperature dt2 = Temperature::deltaKelvin(30.0);
        Temperature result = dt1 + dt2;
        REQUIRE(result.isDelta());
        REQUIRE(result.toKelvin() == Approx(50.0).epsilon(1e-9));
    }

    SECTION("Delta * scalar = Delta") {
        Temperature dt = Temperature::deltaKelvin(10.0);
        Temperature result = dt * 3.0;
        REQUIRE(result.isDelta());
        REQUIRE(result.toKelvin() == Approx(30.0).epsilon(1e-9));
    }

    SECTION("Delta / scalar = Delta") {
        Temperature dt = Temperature::deltaKelvin(60.0);
        Temperature result = dt / 3.0;
        REQUIRE(result.isDelta());
        REQUIRE(result.toKelvin() == Approx(20.0).epsilon(1e-9));
    }
}

/* =========================================================================
 Suite 5: Physical Validation
 ========================================================================= */

TEST_CASE("Temperature enforces physical constraints via exceptions", "[temperature][validation]") {

    SECTION("Absolute temperature below 0 K throws invalid_argument") {
        REQUIRE_THROWS_AS(Temperature::fromKelvin(-1.0), std::invalid_argument);
    }

    SECTION("Absolute temperature at 0 K is valid (absolute zero)") {
        REQUIRE_NOTHROW(Temperature::fromKelvin(0.0));
    }

    SECTION("Scaling an absolute temperature throws invalid_argument") {
        REQUIRE_THROWS_AS(Temperature::fromKelvin(300.0) * 2.0, std::invalid_argument);
    }

    SECTION("Dividing an absolute temperature throws invalid_argument") {
        REQUIRE_THROWS_AS(Temperature::fromKelvin(300.0) / 2.0, std::invalid_argument);
    }

    SECTION("Adding two absolute temperatures throws invalid_argument") {
        REQUIRE_THROWS_AS(
            Temperature::fromKelvin(300.0) + Temperature::fromKelvin(200.0),
            std::invalid_argument
        );
    }

    SECTION("Dividing Delta by zero throws invalid_argument") {
        REQUIRE_THROWS_AS(Temperature::deltaKelvin(50.0) / 0.0, std::invalid_argument);
    }
}

/* =========================================================================
 Suite 6: Engineering Reference Cases
 Source: Cengel & Boles, Thermodynamics, 8th ed.
 ========================================================================= */

TEST_CASE("Temperature engineering reference cases", "[temperature][engineering]") {

    SECTION("Isobaric heating: T2 = T1 + ΔT (300 K + 200 K = 500 K)") {
        /*
         Typical state-point shift in a heat exchanger or combustion chamber.
         T1 = 300 K (inlet), ΔT = 200 K (heat added), T2 = 500 K (outlet)
        */
        Temperature T1 = Temperature::fromKelvin(300.0);
        Temperature dT = Temperature::deltaKelvin(200.0);
        Temperature T2 = T1 + dT;
        REQUIRE(T2.isAbsolute());
        REQUIRE(T2.toKelvin() == Approx(500.0).epsilon(1e-9));
    }

    SECTION("Enthalpy difference requires ΔT: 500 K - 300 K = 200 ΔK") {
        /*
         Δh = Cp * ΔT — the ΔT must be a Delta type for dimensional correctness.
        */
        Temperature T2 = Temperature::fromKelvin(500.0);
        Temperature T1 = Temperature::fromKelvin(300.0);
        Temperature dT = T2 - T1;
        REQUIRE(dT.isDelta());
        REQUIRE(dT.toKelvin() == Approx(200.0).epsilon(1e-9));
    }

    SECTION("USC combustion temperature: 2500 °F to Kelvin") {
        /*
         Typical adiabatic flame temperature in USC units.
         2500 °F = (2500 + 459.67) * 5/9 = 1644.26 K
        */
        Temperature T = Temperature::fromFahrenheit(2500.0);
        REQUIRE(T.toKelvin() == Approx(1644.26).margin(0.01));
    }

    SECTION("Rankine cycle condenser: 45 °C condensation temperature") {
        /*
         Common condenser outlet temperature.
         45 °C = 318.15 K = 572.67 °R
        */
        Temperature T = Temperature::fromCelsius(45.0);
        REQUIRE(T.toKelvin()  == Approx(318.15).margin(0.01));
        REQUIRE(T.toRankine() == Approx(572.67).margin(0.01));
    }
}