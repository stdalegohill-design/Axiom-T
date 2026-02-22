/*
 test_pressure.cpp
 -----------------
 Unit tests for the Pressure class.

 Validation methodology:
   Numerical results are verified against conversion tables from
   Cengel & Boles, "Thermodynamics: An Engineering Approach", 8th ed.
   Maximum accepted error: 0.01% (project QA criterion).

 Test coverage:
   1. Factory methods (object creation from each supported unit)
   2. Round-trip conversions (input unit -> Pa -> input unit)
   3. Arithmetic operators
   4. Comparison operators
   5. Physical validation (exception handling)
   6. Real engineering cases
 */

#include "catch2/catch_amalgamated.hpp"
#include "C:\Users\alego\projects\Axiom-T\include\units\pressure.h"
#include <cmath>
using Catch::Approx;

static double percentError(double expected, double actual) {
    return std::fabs((actual - expected) / expected) * 100.0;
}

static constexpr double TOLERANCE = 0.01; // 0.01% maximum error

/* =========================================================================
  Suite 1: Factory Methods
  ========================================================================= */

TEST_CASE("Pressure factory methods produce correct SI values", "[pressure][creation]") {

    SECTION("fromPascal stores the value without transformation") {
        Pressure p = Pressure::fromPascal(101325.0);
        REQUIRE(p.toPascal() == Approx(101325.0).epsilon(1e-9));
    }

    SECTION("fromKPa: 101.325 kPa == 101325 Pa (standard atmosphere)") {
        Pressure p = Pressure::fromKPa(101.325);
        REQUIRE(percentError(101325.0, p.toPascal()) < TOLERANCE);
    }

    SECTION("fromMPa: 1 MPa == 1,000,000 Pa") {
        Pressure p = Pressure::fromMPa(1.0);
        REQUIRE(percentError(1.0e6, p.toPascal()) < TOLERANCE);
    }

    SECTION("fromBar: 1 bar == 100,000 Pa") {
        Pressure p = Pressure::fromBar(1.0);
        REQUIRE(percentError(1.0e5, p.toPascal()) < TOLERANCE);
    }

    SECTION("fromAtm: 1 atm == 101,325 Pa (NIST standard)") {
        Pressure p = Pressure::fromAtm(1.0);
        REQUIRE(percentError(101325.0, p.toPascal()) < TOLERANCE);
    }

    SECTION("fromPSI: 14.696 PSI == 1 atm") {
        Pressure p = Pressure::fromPSI(14.696);
        REQUIRE(percentError(101325.0, p.toPascal()) < TOLERANCE);
    }

    SECTION("fromMmHg: 760 mmHg == 1 atm") {
        Pressure p = Pressure::fromMmHg(760.0);
        REQUIRE(percentError(101325.0, p.toPascal()) < TOLERANCE);
    }
}

/* =========================================================================
  Suite 2: Round-Trip Conversions
  ========================================================================= */

TEST_CASE("Pressure round-trip conversions are lossless within tolerance", "[pressure][conversion]") {

    SECTION("PSI -> Pa -> PSI") {
        double original = 150.0;
        REQUIRE(percentError(original, Pressure::fromPSI(original).toPSI()) < TOLERANCE);
    }

    SECTION("bar -> Pa -> bar") {
        double original = 5.5;
        REQUIRE(percentError(original, Pressure::fromBar(original).toBar()) < TOLERANCE);
    }

    SECTION("atm -> Pa -> atm") {
        double original = 3.2;
        REQUIRE(percentError(original, Pressure::fromAtm(original).toAtm()) < TOLERANCE);
    }

    SECTION("MPa -> kPa cross-unit: 2 MPa == 2000 kPa") {
        REQUIRE(percentError(2000.0, Pressure::fromMPa(2.0).toKPa()) < TOLERANCE);
    }
}

/* =========================================================================
  Suite 3: Arithmetic Operators
  ========================================================================= */

TEST_CASE("Pressure arithmetic operators yield physically correct results", "[pressure][operators]") {

    SECTION("Addition: 2 bar + 3 bar == 5 bar") {
        Pressure result = Pressure::fromBar(2.0) + Pressure::fromBar(3.0);
        REQUIRE(percentError(5.0e5, result.toPascal()) < TOLERANCE);
    }

    SECTION("Subtraction: 500 kPa - 200 kPa == 300 kPa") {
        Pressure result = Pressure::fromKPa(500.0) - Pressure::fromKPa(200.0);
        REQUIRE(percentError(3.0e5, result.toPascal()) < TOLERANCE);
    }

    SECTION("Scalar multiplication P * k: 1 atm * 2 == 2 atm") {
        Pressure result = Pressure::fromAtm(1.0) * 2.0;
        REQUIRE(percentError(2.0 * 101325.0, result.toPascal()) < TOLERANCE);
    }

    SECTION("Scalar multiplication k * P (commutative form)") {
        Pressure result = 2.0 * Pressure::fromAtm(1.0);
        REQUIRE(percentError(2.0 * 101325.0, result.toPascal()) < TOLERANCE);
    }

    SECTION("Scalar division P / k: 4 bar / 2 == 2 bar") {
        Pressure result = Pressure::fromBar(4.0) / 2.0;
        REQUIRE(percentError(2.0e5, result.toPascal()) < TOLERANCE);
    }

    SECTION("Pressure ratio P / P returns dimensionless value: 10 bar / 2 bar == 5.0") {
        double ratio = Pressure::fromBar(10.0) / Pressure::fromBar(2.0);
        REQUIRE(ratio == Approx(5.0).epsilon(1e-9));
    }
}

/* =========================================================================
  Suite 4: Comparison Operators
  ========================================================================= */

TEST_CASE("Pressure comparison operators evaluate correctly", "[pressure][comparison]") {

    Pressure low  = Pressure::fromBar(1.0);
    Pressure high = Pressure::fromBar(5.0);
    Pressure copy = Pressure::fromBar(5.0);

    SECTION("Less than")    { REQUIRE(low  < high); }
    SECTION("Greater than") { REQUIRE(high > low);  }
    SECTION("Equal (same value)")  { REQUIRE(high == copy); }
    SECTION("Not equal")    { REQUIRE(low  != high); }

    SECTION("Equality across units: 1 atm == 101.325 kPa") {
        REQUIRE(Pressure::fromAtm(1.0) == Pressure::fromKPa(101.325));
    }
}

/* =========================================================================
  Suite 5: Physical Validation
  ========================================================================= */

TEST_CASE("Pressure enforces physical constraints via exceptions", "[pressure][validation]") {

    SECTION("Negative pressure throws invalid_argument") {
        REQUIRE_THROWS_AS(Pressure::fromPascal(-1.0), std::invalid_argument);
    }

    SECTION("Subtraction resulting in negative pressure throws invalid_argument") {
        REQUIRE_THROWS_AS(
            Pressure::fromBar(1.0) - Pressure::fromBar(5.0),
            std::invalid_argument
        );
    }

    SECTION("Division by zero scalar throws invalid_argument") {
        REQUIRE_THROWS_AS(Pressure::fromBar(1.0) / 0.0, std::invalid_argument);
    }

    SECTION("Multiplication by negative scalar throws invalid_argument") {
        REQUIRE_THROWS_AS(Pressure::fromBar(1.0) * (-2.0), std::invalid_argument);
    }

    SECTION("Zero pressure is physically valid (perfect vacuum)") {
        REQUIRE_NOTHROW(Pressure::fromPascal(0.0));
        REQUIRE(Pressure::fromPascal(0.0).isValid());
    }
}

/* =========================================================================
  Suite 6: Engineering Reference Cases
  Source: Cengel & Boles, Thermodynamics, 8th ed.
  ========================================================================= */

TEST_CASE("Pressure engineering reference cases", "[pressure][engineering]") {

    SECTION("Gauge + atmospheric = absolute pressure (P_abs = P_gauge + P_atm)") {
        /*
         P_gauge = 200 kPa (manometer reading)
         P_atm   = 101.325 kPa
         P_abs   = 301.325 kPa = 301325 Pa
         */
        Pressure p_abs = Pressure::fromKPa(200.0) + Pressure::fromKPa(101.325);
        REQUIRE(percentError(301325.0, p_abs.toPascal()) < TOLERANCE);
    }

    SECTION("Compression ratio 10:1 (1 atm inlet, 10 atm outlet)") {
        double ratio = Pressure::fromAtm(10.0) / Pressure::fromAtm(1.0);
        REQUIRE(ratio == Approx(10.0).margin(0.001));
    }

    SECTION("3000 PSI high-pressure boiler converts to ~20.684 MPa") {
        /*
         3000 PSI * 6894.757293168 Pa/PSI = 20,684,271.9 Pa = 20.684 MPa
         */
        Pressure boiler = Pressure::fromPSI(3000.0);
        REQUIRE(percentError(20684271.9, boiler.toPascal()) < TOLERANCE);
        REQUIRE(boiler.toMPa() == Approx(20.684).margin(0.01));
    }
}