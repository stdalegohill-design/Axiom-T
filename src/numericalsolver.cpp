/*
 * NumericalSolver.cpp
 * -------------------
 * Implementation of NumericalSolver.
 * See NumericalSolver.h for interface documentation and references.
 */

#include "solvers/NumericalSolver.h"
#include <cmath>
#include <stdexcept>

/* ---------------------------------------------------------------------------
 * Internal helper: computes relative approximate error (%).
 * Avoids division by zero when x_new is exactly 0.
 * ---------------------------------------------------------------------------*/
static double relativeError(double x_new, double x_old) {
    if (x_new == 0.0) {
        return std::fabs(x_new - x_old) * 100.0;
    }
    return std::fabs((x_new - x_old) / x_new) * 100.0;
}

/* ---------------------------------------------------------------------------
 * Newton-Raphson
 * ---------------------------------------------------------------------------*/
SolverResult NumericalSolver::newtonRaphson(
    std::function<double(double)> f,
    std::function<double(double)> df,
    double x0,
    double tol,
    int    maxIt
) {
    SolverResult result;
    result.method    = "Newton-Raphson";
    result.converged = false;

    double x_prev = x0;

    for (int i = 1; i <= maxIt; ++i) {
        double fx  = f(x_prev);
        double dfx = df(x_prev);

        if (dfx == 0.0) {
            throw std::runtime_error(
                "Newton-Raphson: derivative is zero at x = "
                + std::to_string(x_prev)
                + ". Cannot continue."
            );
        }

        double x_new = x_prev - fx / dfx;
        double err   = (i == 1) ? 100.0 : relativeError(x_new, x_prev);

        SolverStep step;
        step.iteration = i;
        step.x         = x_new;
        step.fx        = f(x_new);
        step.error     = err;
        result.history.push_back(step);

        if (err < tol && i > 1) {
            result.converged  = true;
            result.solution   = x_new;
            result.iterations = i;
            result.finalError = err;
            return result;
        }

        x_prev = x_new;
    }

    /* Did not converge within maxIt */
    result.solution   = x_prev;
    result.iterations = maxIt;
    result.finalError = result.history.empty() ? 100.0 : result.history.back().error;
    return result;
}

/* ---------------------------------------------------------------------------
 * Bisection
 * ---------------------------------------------------------------------------*/
SolverResult NumericalSolver::bisection(
    std::function<double(double)> f,
    double a,
    double b,
    double tol,
    int    maxIt
) {
    if (f(a) * f(b) >= 0.0) {
        throw std::invalid_argument(
            "Bisection: f(a) and f(b) must have opposite signs. "
            "f(" + std::to_string(a) + ") = " + std::to_string(f(a)) +
            ", f(" + std::to_string(b) + ") = " + std::to_string(f(b))
        );
    }

    SolverResult result;
    result.method    = "Bisection";
    result.converged = false;

    double x_prev = a;

    for (int i = 1; i <= maxIt; ++i) {
        double mid = (a + b) / 2.0;
        double err = (i == 1) ? 100.0 : relativeError(mid, x_prev);

        SolverStep step;
        step.iteration = i;
        step.x         = mid;
        step.fx        = f(mid);
        step.error     = err;
        result.history.push_back(step);

        if (err < tol && i > 1) {
            result.converged  = true;
            result.solution   = mid;
            result.iterations = i;
            result.finalError = err;
            return result;
        }

        if (f(a) * f(mid) < 0.0) {
            b = mid;
        } else {
            a = mid;
        }

        x_prev = mid;
    }

    result.solution   = (a + b) / 2.0;
    result.iterations = maxIt;
    result.finalError = result.history.empty() ? 100.0 : result.history.back().error;
    return result;
}

/* ---------------------------------------------------------------------------
 * Simpson's 1/3 rule (composite)
 * ---------------------------------------------------------------------------*/
SolverResult NumericalSolver::simpsonOneThird(
    std::function<double(double)> f,
    double a,
    double b,
    int    n
) {
    if (n < 2 || n % 2 != 0) {
        throw std::invalid_argument(
            "Simpson 1/3: n must be even and >= 2. Received n = "
            + std::to_string(n)
        );
    }

    SolverResult result;
    result.method    = "Simpson 1/3";
    result.converged = true; /* direct formula, no iteration */

    double h     = (b - a) / static_cast<double>(n);
    double total = f(a) + f(b);

    /*
     * Composite Simpson's rule:
     *   (h/3) * [f(x0) + 4f(x1) + 2f(x2) + 4f(x3) + ... + f(xn)]
     *
     * History records cumulative partial sums so the caller can see
     * how the estimate improves as more panels are added.
     */
    double running = f(a);

    for (int i = 1; i < n; ++i) {
        double xi  = a + i * h;
        double fxi = f(xi);
        double coeff = (i % 2 == 0) ? 2.0 : 4.0;
        total   += coeff * fxi;
        running += coeff * fxi;

        if (i % 2 == 0 || i == n - 1) {
            /* record a step at every even panel and at the last point */
            SolverStep step;
            step.iteration = i;
            step.x         = xi;
            step.fx        = fxi;
            step.error     = 0.0; /* not applicable for direct integration */
            step.x         = (running + f(b)) * h / 3.0; /* partial estimate */
            result.history.push_back(step);
        }
    }

    result.solution   = total * h / 3.0;
    result.iterations = n;
    result.finalError = 0.0;
    return result;
}

/* ---------------------------------------------------------------------------
 * Runge-Kutta 4
 * ---------------------------------------------------------------------------*/
RK4Result NumericalSolver::rungeKutta4(
    std::function<double(double, double)> f,
    double y0,
    double t0,
    double tf,
    double h
) {
    if (h <= 0.0) {
        throw std::invalid_argument(
            "RK4: step size h must be positive. Received h = "
            + std::to_string(h)
        );
    }
    if (tf <= t0) {
        throw std::invalid_argument(
            "RK4: tf must be greater than t0."
        );
    }

    RK4Result result;
    result.method = "Runge-Kutta 4";

    double t = t0;
    double y = y0;

    RK4Step first;
    first.t = t;
    first.y = y;
    result.trajectory.push_back(first);

    while (t < tf - 1e-12) {
        /* Clamp last step to not overshoot tf */
        double step_h = (t + h > tf) ? (tf - t) : h;

        double k1 = f(t,          y);
        double k2 = f(t + step_h / 2.0, y + step_h * k1 / 2.0);
        double k3 = f(t + step_h / 2.0, y + step_h * k2 / 2.0);
        double k4 = f(t + step_h,       y + step_h * k3);

        y = y + (step_h / 6.0) * (k1 + 2.0 * k2 + 2.0 * k3 + k4);
        t = t + step_h;

        RK4Step step;
        step.t = t;
        step.y = y;
        result.trajectory.push_back(step);
    }

    result.steps = static_cast<int>(result.trajectory.size()) - 1;
    return result;
}