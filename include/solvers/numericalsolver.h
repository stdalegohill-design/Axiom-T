#ifndef NUMERICAL_SOLVER_H
#define NUMERICAL_SOLVER_H

/*
  NumericalSolver.h
  -----------------
  Static collection of numerical methods for solving algebraic equations,
  evaluating definite integrals, and integrating ordinary differential equations.
 
  Design decision: all methods are static — NumericalSolver is a namespace-like
  utility class, not an instantiable object. There is no internal state.
 
  Pedagogical design: every method returns a SolverResult containing not only
  the final solution but the complete iteration history (SolverStep vector).
  This allows the caller to inspect convergence behavior step by step, which
  is the core educational feature of Axiom-T.
 
  References:
    Chapra & Canale, Numerical Methods for Engineers, 5th ed.
    Burden & Faires, Numerical Analysis, 9th ed.
 */

#include <functional>
#include <vector>
#include <string>
#include <stdexcept>

/* ---------------------------------------------------------------------------
   SolverStep
   One recorded iteration. Used by all iterative methods.
   ---------------------------------------------------------------------------*/
struct SolverStep {
    int    iteration;   // iteration number (1-based)                        
    double x;           // current approximation of the solution             
    double fx;          // f(x) at this step — shows how close to zero      
    double error;       // relative error |x_new - x_old| / |x_new| * 100   
};

/* ---------------------------------------------------------------------------
   RK4Step
   One recorded step for the Runge-Kutta 4 integrator.
   ---------------------------------------------------------------------------*/
struct RK4Step {
    double t;           // independent variable (time or other parameter)    
    double y;           // solution value at t                               
};

/* ---------------------------------------------------------------------------
   SolverResult
   Returned by all root-finding and integration methods.
   Contains the answer AND the complete process for pedagogical display.
   ---------------------------------------------------------------------------*/
struct SolverResult {
    double                  solution;       // final answer                  
    int                     iterations;     // total iterations performed    
    double                  finalError;     // error at last step (%)        
    bool                    converged;      // true if tolerance was met     
    std::string             method;         // name of the method used       
    std::vector<SolverStep> history;        // full iteration trace          
};

/* ---------------------------------------------------------------------------
   RK4Result
   Returned by rungeKutta4. Contains the complete solution trajectory.
   ---------------------------------------------------------------------------*/
struct RK4Result {
    std::vector<RK4Step>    trajectory;     // all (t, y) points             
    int                     steps;          // total steps taken             
    std::string             method;         // "Runge-Kutta 4"               
};

/* ---------------------------------------------------------------------------
   NumericalSolver
   ---------------------------------------------------------------------------*/
class NumericalSolver {
public:

    /*
      Newton-Raphson method — finds x such that f(x) = 0.
     
      Requires the function and its derivative. Converges quadratically
      when the initial guess is close to the root.
     
      Parameters:
        f     — function to solve: f(x) = 0
        df    — derivative of f
        x0    — initial guess
        tol   — convergence tolerance (relative error %, default 1e-6)
        maxIt — maximum iterations before declaring non-convergence
     
      Throws: std::runtime_error if df(x) = 0 at any step (division by zero).
     */
    static SolverResult newtonRaphson(
        std::function<double(double)> f,
        std::function<double(double)> df,
        double x0,
        double tol   = 1e-6,
        int    maxIt = 100
    );

    /*
      Bisection method — finds x such that f(x) = 0 on interval [a, b].
     
      Requires f(a) and f(b) to have opposite signs. Converges linearly
      but is robust — guaranteed to find a root if one exists.
     
      Parameters:
        f     — function to solve: f(x) = 0
        a, b  — interval endpoints, must satisfy f(a)*f(b) < 0
        tol   — convergence tolerance (relative error %, default 1e-6)
        maxIt — maximum iterations
     
      Throws: std::invalid_argument if f(a)*f(b) >= 0 (no sign change).
     */
    static SolverResult bisection(
        std::function<double(double)> f,
        double a,
        double b,
        double tol   = 1e-6,
        int    maxIt = 100
    );

    /*
      Simpson's 1/3 rule — approximates the definite integral of f from a to b.
     
      Uses composite Simpson's rule with n panels (n must be even).
      Returns the integral value as solution; history records panel-by-panel
      partial sums for pedagogical purposes.
     
      Parameters:
        f  — integrand
        a  — lower bound
        b  — upper bound
        n  — number of panels (must be even, default 100)
     
      Throws: std::invalid_argument if n is odd or n < 2.
     */
    static SolverResult simpsonOneThird(
        std::function<double(double)> f,
        double a,
        double b,
        int    n = 100
    );

    /*
      Runge-Kutta 4 — integrates the ODE dy/dt = f(t, y).
     
      Classical 4th-order method. Balances accuracy and computational cost.
      Used in Axiom-T to solve thermodynamic process paths where state
      variables evolve continuously (e.g. polytropic processes).
     
      Parameters:
        f   — right-hand side: dy/dt = f(t, y)
        y0  — initial condition y(t0)
        t0  — initial time/parameter
        tf  — final time/parameter
        h   — step size
     
      Throws: std::invalid_argument if h <= 0 or tf <= t0.
     */
    static RK4Result rungeKutta4(
        std::function<double(double, double)> f,
        double y0,
        double t0,
        double tf,
        double h
    );

private:
    /* Non-instantiable: all methods are static */
    NumericalSolver() = delete;
};

#endif // NUMERICAL_SOLVER_H