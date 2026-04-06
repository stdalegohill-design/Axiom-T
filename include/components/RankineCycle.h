#ifndef RANKINE_CYCLE_H
#define RANKINE_CYCLE_H

/*
 * RankineCycle.h
 * --------------
 * Ideal and actual Rankine cycle solver.
 *
 * The Rankine cycle consists of four processes:
 *
 *   1→2  Turbine (expansion isentrópica): high-P steam → low-P steam
 *   2→3  Condenser (rechazo de calor): steam → saturated liquid
 *   3→4  Pump (compresión): saturated liquid → compressed liquid
 *   4→1  Boiler (adición de calor): compressed liquid → superheated steam
 *
 * Performance metrics (Cengel & Boles, Chapter 10):
 *   w_net  = w_turbine - w_pump       [kJ/kg]
 *   q_in   = h1 - h4                 [kJ/kg]
 *   q_out  = h2 - h3                 [kJ/kg]
 *   eta_th = w_net / q_in            [-]
 *   BWR    = w_pump / w_turbine      [-]  (back work ratio)
 *
 * Validated against:
 *   Cengel & Boles, Example 10-1 (ideal Rankine cycle)
 *   P_boiler=7MPa T_boiler=500°C, P_condenser=10kPa
 *   eta_th = 38.3%, w_net = 1017.3 kJ/kg
 */

#include "components/ThermodynamicState.h"
#include "components/ComponentResult.h"
#include "components/Turbine.h"
#include "components/Pump.h"
#include <string>
#include <vector>

/* ---------------------------------------------------------------------------
 * RankineResult — complete cycle results with full pedagogical trace
 * ---------------------------------------------------------------------------*/
struct RankineResult {

    /* Four states of the cycle */
    ThermodynamicState state1;   /* turbine inlet (boiler outlet)     */
    ThermodynamicState state2;   /* turbine outlet (condenser inlet)  */
    ThermodynamicState state3;   /* condenser outlet (pump inlet)     */
    ThermodynamicState state4;   /* pump outlet (boiler inlet)        */

    /* Component results */
    ComponentResult turbineResult;
    ComponentResult pumpResult;

    /* Cycle performance */
    double w_turbine;    /* turbine work output [kJ/kg]           */
    double w_pump;       /* pump work input [kJ/kg]               */
    double w_net;        /* net work output [kJ/kg]               */
    double q_in;         /* heat added in boiler [kJ/kg]          */
    double q_out;        /* heat rejected in condenser [kJ/kg]    */
    double eta_th;       /* thermal efficiency [-]                */
    double BWR;          /* back work ratio [-]                   */
    double x2;           /* turbine outlet quality [-]            */

    /* Isentropic efficiencies */
    double eta_turbine;
    double eta_pump;

    /* Source and reference */
    std::string fluid;
    std::string reference;
    DataSource  source;

    /* Calculation steps — full pedagogical trace */
    std::vector<CalculationStep> steps;

    void addStep(const std::string& desc, const std::string& eq,
                 const std::string& vals, double result,
                 const std::string& unit);

    /* print() — formatted console output of the complete cycle */
    void print() const;
};

/* ---------------------------------------------------------------------------
 * RankineCycle
 * ---------------------------------------------------------------------------*/
class RankineCycle {
public:

    /*
     * solve() — compute the complete Rankine cycle.
     *
     * Parameters:
     *   fluid         — working fluid (default "Water")
     *   T_boiler_C    — boiler outlet temperature [°C]
     *   P_boiler_kPa  — boiler pressure [kPa]
     *   P_cond_kPa    — condenser pressure [kPa]
     *   eta_turbine   — turbine isentropic efficiency (default 1.0)
     *   eta_pump      — pump isentropic efficiency (default 1.0)
     *   source        — data source (default COOLPROP)
     *
     * Returns RankineResult with all states, component results,
     * and performance metrics.
     */
    static RankineResult solve(
        const std::string& fluid,
        double T_boiler_C,
        double P_boiler_kPa,
        double P_cond_kPa,
        double eta_turbine = 1.0,
        double eta_pump    = 1.0,
        DataSource source  = DataSource::COOLPROP
    );

private:
    RankineCycle() = delete;
};

#endif // RANKINE_CYCLE_H