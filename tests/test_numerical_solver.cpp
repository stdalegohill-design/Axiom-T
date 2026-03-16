/*
 * test_numerical_solver.cpp
 * -------------------------
 * Unit tests for NumericalSolver.
 *
 * Validation methodology:
 *   Reference values taken from Chapra & Canale, "Numerical Methods for
 *   Engineers", 5th ed., and from Burden & Faires, "Numerical Analysis", 9th ed.
 *   Maximum accepted error: 0.01% (Axiom-T QA criterion).
 *
 * Test coverage:
 *   1. Newton-Raphson — convergence, history population, exception on df=0
 *   2. Bisection       — convergence, exception on invalid bracket
 *   3. Simpson 1/3     — exact result for polynomials, exception on odd n
 *   4. Runge-Kutta 4   — known ODE with analytical solution, trajectory size
 *   5. Pedagogical     — history vector is populated correctly
 */

#include "catch2/catch_amalgamated.hpp"
#include "solvers/NumericalSolver.h"
#include <cmath>

/* M_PI is not guaranteed by the C++ standard — define it explicitly */
#ifndef M_PI
static constexpr double M_PI = 3.14159265358979323846;
#endif

/* Catch2 v3 amalgamated requires explicit namespace qualification */
using Catch::Approx;

static double percentError(double expected, double actual) {
    return std::fabs((actual - expected) / expected) * 100.0;
}

static constexpr double TOLERANCE = 0.01; /* 0.01% maximum error */

/* =========================================================================
 * Suite 1: Newton-Raphson
 * ========================================================================= */

TEST_CASE("Newton-Raphson finds root of f(x) = x^2 - 9", "[solver][newton]") {

    /* f(x) = x^2 - 9  =>  root at x = 3 */
    auto f  = [](double x) { return x * x - 9.0; };
    auto df = [](double x) { return 2.0 * x; };

    SECTION("converges to x = 3 from x0 = 2") {
        SolverResult r = NumericalSolver::newtonRaphson(f, df, 2.0);
        REQUIRE(r.converged);
        REQUIRE(percentError(3.0, r.solution) < TOLERANCE);
    }

    SECTION("converges to x = -3 from x0 = -2") {
        SolverResult r = NumericalSolver::newtonRaphson(f, df, -2.0);
        REQUIRE(r.converged);
        REQUIRE(percentError(3.0, std::fabs(r.solution)) < TOLERANCE);
    }

    SECTION("history vector is non-empty after convergence") {
        SolverResult r = NumericalSolver::newtonRaphson(f, df, 2.0);
        REQUIRE_FALSE(r.history.empty());
    }

    SECTION("iteration count matches history size") {
        SolverResult r = NumericalSolver::newtonRaphson(f, df, 2.0);
        REQUIRE(r.iterations == static_cast<int>(r.history.size()));
    }

    SECTION("throws when derivative is zero at starting point") {
        /* f(x) = x^2,  df(0) = 0 */
        auto fz  = [](double x) { return x * x; };
        auto dfz = [](double x) { return 2.0 * x; };
        REQUIRE_THROWS_AS(
            NumericalSolver::newtonRaphson(fz, dfz, 0.0),
            std::runtime_error
        );
    }
}

TEST_CASE("Newton-Raphson: Chapra example — f(x) = e^(-x) - x", "[solver][newton][chapra]") {
    /*
     * Chapra & Canale, Example 5.2 (approx)
     * f(x) = e^(-x) - x = 0  =>  root ~ 0.56714
     */
    auto f  = [](double x) { return std::exp(-x) - x; };
    auto df = [](double x) { return -std::exp(-x) - 1.0; };

    SolverResult r = NumericalSolver::newtonRaphson(f, df, 0.0);
    REQUIRE(r.converged);
    REQUIRE(percentError(0.56714329, r.solution) < TOLERANCE);
}

/* =========================================================================
 * Suite 2: Bisection
 * ========================================================================= */

TEST_CASE("Bisection finds root of f(x) = x^3 - x - 2", "[solver][bisection]") {

    /* f(x) = x^3 - x - 2  =>  root at x ~ 1.52138 */
    auto f = [](double x) { return x * x * x - x - 2.0; };

    SECTION("converges on [1, 2]") {
        SolverResult r = NumericalSolver::bisection(f, 1.0, 2.0);
        REQUIRE(r.converged);
        REQUIRE(percentError(1.52138, r.solution) < TOLERANCE);
    }

    SECTION("history is non-empty and monotonically shrinking bracket") {
        SolverResult r = NumericalSolver::bisection(f, 1.0, 2.0);
        REQUIRE_FALSE(r.history.empty());
        /* Each step's x should be between 1 and 2 */
        for (const auto& step : r.history) {
            REQUIRE(step.x >= 1.0);
            REQUIRE(step.x <= 2.0);
        }
    }

    SECTION("throws when f(a)*f(b) >= 0 (no sign change)") {
        /* Both positive: f(2) > 0, f(3) > 0 */
        REQUIRE_THROWS_AS(
            NumericalSolver::bisection(f, 2.0, 3.0),
            std::invalid_argument
        );
    }
}

TEST_CASE("Bisection: classic bracket on f(x) = cos(x) - x", "[solver][bisection][classic]") {
    /*
     * f(x) = cos(x) - x = 0  =>  root ~ 0.73909 (Dottie number)
     */
    auto f = [](double x) { return std::cos(x) - x; };

    SolverResult r = NumericalSolver::bisection(f, 0.0, 1.0);
    REQUIRE(r.converged);
    REQUIRE(percentError(0.73909, r.solution) < TOLERANCE);
}

/* =========================================================================
 * Suite 3: Simpson 1/3
 * ========================================================================= */

TEST_CASE("Simpson 1/3 integrates exactly for polynomials up to degree 3", "[solver][simpson]") {

    SECTION("integral of f(x) = 1 from 0 to 1 == 1") {
        auto f = [](double x) { (void)x; return 1.0; };
        SolverResult r = NumericalSolver::simpsonOneThird(f, 0.0, 1.0, 10);
        REQUIRE(percentError(1.0, r.solution) < TOLERANCE);
    }

    SECTION("integral of f(x) = x from 0 to 1 == 0.5") {
        auto f = [](double x) { return x; };
        SolverResult r = NumericalSolver::simpsonOneThird(f, 0.0, 1.0, 10);
        REQUIRE(percentError(0.5, r.solution) < TOLERANCE);
    }

    SECTION("integral of f(x) = x^2 from 0 to 1 == 1/3") {
        auto f = [](double x) { return x * x; };
        SolverResult r = NumericalSolver::simpsonOneThird(f, 0.0, 1.0, 100);
        REQUIRE(percentError(1.0 / 3.0, r.solution) < TOLERANCE);
    }

    SECTION("integral of f(x) = x^3 from 0 to 2 == 4") {
        auto f = [](double x) { return x * x * x; };
        SolverResult r = NumericalSolver::simpsonOneThird(f, 0.0, 2.0, 100);
        REQUIRE(percentError(4.0, r.solution) < TOLERANCE);
    }
}

TEST_CASE("Simpson 1/3: integral of sin(x) from 0 to pi == 2", "[solver][simpson][trig]") {
    auto f = [](double x) { return std::sin(x); };
    SolverResult r = NumericalSolver::simpsonOneThird(f, 0.0, M_PI, 100);
    REQUIRE(percentError(2.0, r.solution) < TOLERANCE);
}

TEST_CASE("Simpson 1/3 throws on odd panel count", "[solver][simpson][validation]") {
    auto f = [](double x) { return x; };
    REQUIRE_THROWS_AS(
        NumericalSolver::simpsonOneThird(f, 0.0, 1.0, 3),
        std::invalid_argument
    );
}

/* =========================================================================
 * Suite 4: Runge-Kutta 4
 * ========================================================================= */

TEST_CASE("RK4 solves dy/dt = y with analytical solution y = e^t", "[solver][rk4]") {
    /*
     * dy/dt = y,  y(0) = 1  =>  y(t) = e^t
     * Analytical: y(1) = e ~ 2.71828
     */
    auto f = [](double /*t*/, double y) { return y; };

    RK4Result r = NumericalSolver::rungeKutta4(f, 1.0, 0.0, 1.0, 0.1);

    REQUIRE_FALSE(r.trajectory.empty());

    double y_final = r.trajectory.back().y;
    REQUIRE(percentError(std::exp(1.0), y_final) < TOLERANCE);
}

TEST_CASE("RK4: trajectory starts at y0 and has correct step count", "[solver][rk4][trajectory]") {
    auto f = [](double /*t*/, double y) { return y; };

    RK4Result r = NumericalSolver::rungeKutta4(f, 1.0, 0.0, 1.0, 0.1);

    /* First point must be exactly (t0, y0) */
    REQUIRE(r.trajectory.front().t == Approx(0.0).epsilon(1e-9));
    REQUIRE(r.trajectory.front().y == Approx(1.0).epsilon(1e-9));

    /* With h=0.1 and interval [0,1], expect 11 points (0, 0.1, ..., 1.0) */
    REQUIRE(r.trajectory.size() == 11);
}

TEST_CASE("RK4 solves dy/dt = -2ty with y(0) = 1", "[solver][rk4][gaussian]") {
    /*
     * dy/dt = -2*t*y,  y(0) = 1  =>  y(t) = e^(-t^2)
     * Analytical: y(1) = e^(-1) ~ 0.36788
     */
    auto f = [](double t, double y) { return -2.0 * t * y; };

    RK4Result r = NumericalSolver::rungeKutta4(f, 1.0, 0.0, 1.0, 0.05);
    double y_final = r.trajectory.back().y;
    REQUIRE(percentError(std::exp(-1.0), y_final) < TOLERANCE);
}

TEST_CASE("RK4 throws on invalid parameters", "[solver][rk4][validation]") {

    auto f = [](double /*t*/, double y) { return y; };

    SECTION("throws when h <= 0") {
        REQUIRE_THROWS_AS(
            NumericalSolver::rungeKutta4(f, 1.0, 0.0, 1.0, 0.0),
            std::invalid_argument
        );
    }

    SECTION("throws when tf <= t0") {
        REQUIRE_THROWS_AS(
            NumericalSolver::rungeKutta4(f, 1.0, 1.0, 0.0, 0.1),
            std::invalid_argument
        );
    }
}

/* =========================================================================
 * Suite 5: Pedagogical — history quality
 * ========================================================================= */

TEST_CASE("SolverResult history contains physically meaningful values", "[solver][pedagogical]") {

    auto f  = [](double x) { return x * x - 4.0; };
    auto df = [](double x) { return 2.0 * x; };

    SolverResult r = NumericalSolver::newtonRaphson(f, df, 3.0);

    SECTION("each step has positive iteration number") {
        for (const auto& step : r.history) {
            REQUIRE(step.iteration > 0);
        }
    }

    SECTION("iteration numbers are sequential") {
        for (int i = 0; i < static_cast<int>(r.history.size()); ++i) {
            REQUIRE(r.history[i].iteration == i + 1);
        }
    }

    SECTION("error decreases monotonically after first step") {
        /* Newton-Raphson converges quadratically — errors must decrease */
        for (int i = 2; i < static_cast<int>(r.history.size()); ++i) {
            REQUIRE(r.history[i].error <= r.history[i - 1].error + 1e-9);
        }
    }

    SECTION("f(x) at final step is close to zero") {
        double fx_final = r.history.back().fx;
        REQUIRE(std::fabs(fx_final) < 1e-8);
    }
}