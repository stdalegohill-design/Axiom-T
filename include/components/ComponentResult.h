#ifndef COMPONENT_RESULT_H
#define COMPONENT_RESULT_H

/*
 * ComponentResult.h
 * -----------------
 * Pedagogical output structure for thermodynamic component calculations.
 *
 * Consistent with the SolverResult philosophy from Phase 1: instead of
 * returning only the final answer, ComponentResult records the complete
 * calculation history step by step — equations applied, intermediate
 * values, energy balance verification, and efficiency metrics.
 *
 * This is Axiom-T's core educational differentiator: the student can
 * see exactly how the result was obtained, not just what it is.
 *
 * Usage:
 *   ComponentResult r = Turbine::solve(inlet, P_out, eta);
 *   r.print();                          // formatted console output
 *   double w = r.specificWork;          // final answer
 *   auto steps = r.steps;              // full calculation trace
 */

#include "components/ThermodynamicState.h"
#include <string>
#include <vector>

/* ---------------------------------------------------------------------------
 * CalculationStep
 * One recorded step in the component calculation sequence.
 * ---------------------------------------------------------------------------*/
struct CalculationStep {
    int         stepNumber;   /* sequential step number (1-based)            */
    std::string description;  /* human-readable description of this step     */
    std::string equation;     /* equation applied, e.g. "w = h1 - h2"        */
    std::string values;       /* substituted values, e.g. "3317.2 - 2967.4" */
    double      result;       /* numerical result of this step               */
    std::string unit;         /* unit of the result, e.g. "kJ/kg"           */
};

/* ---------------------------------------------------------------------------
 * EnergyBalance
 * Verification that the first law is satisfied for the component.
 * E_in - E_out = 0 for steady-state devices.
 * ---------------------------------------------------------------------------*/
struct EnergyBalance {
    double E_in;              /* total energy in [kW or kJ/kg]               */
    double E_out;             /* total energy out [kW or kJ/kg]              */
    double imbalance;         /* |E_in - E_out| — should be ~0              */
    bool   balanced;          /* true if imbalance < tolerance               */
    std::string description;  /* e.g. "h1 = w + h2  →  3317.2 = 349.8 + 2967.4" */
};

/* ---------------------------------------------------------------------------
 * ComponentResult
 * ---------------------------------------------------------------------------*/
struct ComponentResult {

    /* -----------------------------------------------------------------------
     * Component identification
     * ----------------------------------------------------------------------- */
    std::string componentType;   /* "Turbine", "Compressor", "Pump", etc.   */
    std::string fluid;           /* fluid name                               */
    std::string reference;       /* Cengel example reference if applicable   */

    /* -----------------------------------------------------------------------
     * Thermodynamic states
     * ----------------------------------------------------------------------- */
    ThermodynamicState inlet;    /* state at component inlet                 */
    ThermodynamicState outlet;   /* state at component outlet (actual)       */
    ThermodynamicState outletS;  /* isentropic outlet (if applicable)        */

    /* -----------------------------------------------------------------------
     * Primary results
     * ----------------------------------------------------------------------- */
    double specificWork;         /* w [kJ/kg] — positive = work output       */
    double specificHeat;         /* q [kJ/kg] — positive = heat added        */
    double isentropicEfficiency; /* η [-] — 0 to 1, UNKNOWN if not computed  */
    bool   converged;            /* true if iterative solution converged     */

    /* -----------------------------------------------------------------------
     * Pedagogical output — the full calculation trace
     * ----------------------------------------------------------------------- */
    std::vector<CalculationStep> steps;   /* step-by-step calculation        */
    EnergyBalance                balance; /* first law verification          */

    /* -----------------------------------------------------------------------
     * Methods
     * ----------------------------------------------------------------------- */

    /*
     * addStep() — append a calculation step to the history.
     * Called internally by component solvers as they compute each quantity.
     */
    void addStep(const std::string& description,
                 const std::string& equation,
                 const std::string& values,
                 double             result,
                 const std::string& unit);

    /*
     * print() — formatted console output showing the full calculation.
     * Prints component type, fluid, states, step-by-step trace,
     * energy balance, and final results with units.
     */
    void print() const;

    /*
     * summary() — one-line result summary for embedding in cycle output.
     */
    std::string summary() const;

    /* Default constructor */
    ComponentResult()
        : specificWork(UNKNOWN), specificHeat(UNKNOWN),
          isentropicEfficiency(UNKNOWN), converged(false) {}
};

#endif // COMPONENT_RESULT_H