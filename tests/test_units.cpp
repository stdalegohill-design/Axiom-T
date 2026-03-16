/*
 test_units.cpp
 --------------
 Unit tests for Mass, Energy, Volume, and Power classes.

 Validation methodology:
   Numerical results are verified against conversion tables from
   Cengel & Boles, "Thermodynamics: An Engineering Approach", 8th ed.
   Maximum accepted error: 0.01% (project QA criterion).
*/

#include "catch2/catch_amalgamated.hpp"
#include "units/Mass.h"
#include "units/Energy.h"
#include "units/Volume.h"
#include "units/Power.h"
#include <cmath>

using Catch::Approx;

static double percentError(double expected, double actual) {
    return std::fabs((actual - expected) / expected) * 100.0;
}

static constexpr double TOLERANCE = 0.01;

/* =========================================================================
 MASS
 ========================================================================= */

TEST_CASE("Mass factory methods produce correct SI values", "[mass][creation]") {

    SECTION("fromKilogram stores value without transformation") {
        REQUIRE(Mass::fromKilogram(10.0).toKilogram() == Approx(10.0).epsilon(1e-9));
    }

    SECTION("fromGram: 1000 g == 1 kg") {
        REQUIRE(percentError(1.0, Mass::fromGram(1000.0).toKilogram()) < TOLERANCE);
    }

    SECTION("fromPoundMass: 1 lbm == 0.453592 kg") {
        REQUIRE(percentError(0.45359237, Mass::fromPoundMass(1.0).toKilogram()) < TOLERANCE);
    }

    SECTION("fromOunce: 16 oz == 1 lbm") {
        double lbm_in_kg = Mass::fromPoundMass(1.0).toKilogram();
        double oz_in_kg  = Mass::fromOunce(16.0).toKilogram();
        REQUIRE(percentError(lbm_in_kg, oz_in_kg) < TOLERANCE);
    }

    SECTION("fromMetricTon: 1 t == 1000 kg") {
        REQUIRE(percentError(1000.0, Mass::fromMetricTon(1.0).toKilogram()) < TOLERANCE);
    }
}

TEST_CASE("Mass round-trip conversions", "[mass][conversion]") {

    SECTION("lbm -> kg -> lbm") {
        double original = 150.0;
        REQUIRE(percentError(original, Mass::fromPoundMass(original).toPoundMass()) < TOLERANCE);
    }

    SECTION("gram -> kg -> gram") {
        double original = 500.0;
        REQUIRE(percentError(original, Mass::fromGram(original).toGram()) < TOLERANCE);
    }
}

TEST_CASE("Mass physical validation", "[mass][validation]") {

    SECTION("Negative mass throws invalid_argument") {
        REQUIRE_THROWS_AS(Mass::fromKilogram(-1.0), std::invalid_argument);
    }

    SECTION("Subtraction yielding negative mass throws invalid_argument") {
        REQUIRE_THROWS_AS(
            Mass::fromKilogram(1.0) - Mass::fromKilogram(5.0),
            std::invalid_argument
        );
    }

    SECTION("Zero mass is valid") {
        REQUIRE_NOTHROW(Mass::fromKilogram(0.0));
    }
}

/* =========================================================================
 ENERGY
 ========================================================================= */

TEST_CASE("Energy factory methods produce correct SI values", "[energy][creation]") {

    SECTION("fromKJoule: 1 kJ == 1000 J") {
        REQUIRE(percentError(1000.0, Energy::fromKJoule(1.0).toJoule()) < TOLERANCE);
    }

    SECTION("fromBTU: 1 BTU == 1055.056 J") {
        REQUIRE(percentError(1055.05585262, Energy::fromBTU(1.0).toJoule()) < TOLERANCE);
    }

    SECTION("fromKCalorie: 1 kcal == 4184 J") {
        REQUIRE(percentError(4184.0, Energy::fromKCalorie(1.0).toJoule()) < TOLERANCE);
    }

    SECTION("fromKWh: 1 kWh == 3,600,000 J") {
        REQUIRE(percentError(3.6e6, Energy::fromKWh(1.0).toJoule()) < TOLERANCE);
    }
}

TEST_CASE("Energy round-trip conversions", "[energy][conversion]") {

    SECTION("BTU -> J -> BTU") {
        double original = 500.0;
        REQUIRE(percentError(original, Energy::fromBTU(original).toBTU()) < TOLERANCE);
    }

    SECTION("kJ -> J -> kJ") {
        double original = 250.0;
        REQUIRE(percentError(original, Energy::fromKJoule(original).toKJoule()) < TOLERANCE);
    }
}

TEST_CASE("Energy allows negative values (thermodynamic sign convention)", "[energy][validation]") {

    SECTION("Negative energy is valid (heat rejected)") {
        REQUIRE_NOTHROW(Energy::fromKJoule(-100.0));
        REQUIRE(Energy::fromKJoule(-100.0).toJoule() == Approx(-100000.0).epsilon(1e-9));
    }

    SECTION("Energy subtraction can yield negative result") {
        Energy result = Energy::fromKJoule(100.0) - Energy::fromKJoule(300.0);
        REQUIRE(result.toKJoule() == Approx(-200.0).epsilon(1e-9));
    }
}

TEST_CASE("Energy engineering reference cases", "[energy][engineering]") {

    SECTION("Typical steam turbine output: 1000 BTU/cycle to kJ") {
        /*
         1000 BTU * 1055.056 J/BTU = 1,055,056 J = 1055.056 kJ
        */
        Energy e = Energy::fromBTU(1000.0);
        REQUIRE(e.toKJoule() == Approx(1055.056).margin(0.1));
    }
}

/* =========================================================================
 VOLUME
 ========================================================================= */

TEST_CASE("Volume factory methods produce correct SI values", "[volume][creation]") {

    SECTION("fromLiter: 1000 L == 1 m³") {
        REQUIRE(percentError(1.0, Volume::fromLiter(1000.0).toCubicMeter()) < TOLERANCE);
    }

    SECTION("fromCubicFoot: 1 ft³ == 0.028317 m³") {
        REQUIRE(percentError(0.028316846592, Volume::fromCubicFoot(1.0).toCubicMeter()) < TOLERANCE);
    }

    SECTION("fromGallonUS: 1 gal == 3.785412 L") {
        REQUIRE(percentError(3.785411784, Volume::fromGallonUS(1.0).toLiter()) < TOLERANCE);
    }
}

TEST_CASE("Volume round-trip conversions", "[volume][conversion]") {

    SECTION("ft³ -> m³ -> ft³") {
        double original = 10.0;
        REQUIRE(percentError(original, Volume::fromCubicFoot(original).toCubicFoot()) < TOLERANCE);
    }

    SECTION("L -> m³ -> L") {
        double original = 250.0;
        REQUIRE(percentError(original, Volume::fromLiter(original).toLiter()) < TOLERANCE);
    }
}

TEST_CASE("Volume physical validation", "[volume][validation]") {

    SECTION("Negative volume throws invalid_argument") {
        REQUIRE_THROWS_AS(Volume::fromCubicMeter(-1.0), std::invalid_argument);
    }

    SECTION("Zero volume is valid") {
        REQUIRE_NOTHROW(Volume::fromCubicMeter(0.0));
    }
}

/* =========================================================================
 POWER
 ========================================================================= */

TEST_CASE("Power factory methods produce correct SI values", "[power][creation]") {

    SECTION("fromKWatt: 1 kW == 1000 W") {
        REQUIRE(percentError(1000.0, Power::fromKWatt(1.0).toWatt()) < TOLERANCE);
    }

    SECTION("fromHorsepower: 1 hp == 745.7 W") {
        REQUIRE(percentError(745.69987158227022, Power::fromHorsepower(1.0).toWatt()) < TOLERANCE);
    }

    SECTION("fromBTUperHour: 1 BTU/h == 0.29307 W") {
        REQUIRE(percentError(0.29307107017222, Power::fromBTUperHour(1.0).toWatt()) < TOLERANCE);
    }
}

TEST_CASE("Power round-trip conversions", "[power][conversion]") {

    SECTION("hp -> W -> hp") {
        double original = 250.0;
        REQUIRE(percentError(original, Power::fromHorsepower(original).toHorsepower()) < TOLERANCE);
    }

    SECTION("kW -> W -> kW") {
        double original = 75.0;
        REQUIRE(percentError(original, Power::fromKWatt(original).toKWatt()) < TOLERANCE);
    }
}

TEST_CASE("Power engineering reference cases", "[power][engineering]") {

    SECTION("Electric motor: 100 hp to kW") {
        /*
         100 hp * 745.7 W/hp = 74,570 W = 74.57 kW
        */
        Power motor = Power::fromHorsepower(100.0);
        REQUIRE(motor.toKWatt() == Approx(74.5699).margin(0.01));
    }

    SECTION("Power allows negative values (work input convention)") {
        REQUIRE_NOTHROW(Power::fromKWatt(-50.0));
    }
}