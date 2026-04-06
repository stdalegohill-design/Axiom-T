#ifndef TURBINE_H
#define TURBINE_H

/*
 * Turbine.h
 * ---------
 * Steady-flow turbine component solver.
 *
 * Models an adiabatic turbine where high-pressure fluid expands and
 * produces work output. Supports both ideal (isentropic) and real
 * (with isentropic efficiency) turbines.
 *
 * Energy balance (Cengel & Boles, Eq. 6-30):
 *   w_out = h1 - h2          [kJ/kg]
 *
 * Isentropic efficiency (Cengel & Boles, Eq. 7-63):
 *   eta_T = w_actual / w_isentropic
 *         = (h1 - h2) / (h1 - h2s)
 *
 * Validated against:
 *   Cengel & Boles, Example 7-5 (isentropic steam turbine)
 *   Water: P1=5MPa T1=450°C → P2=1.4MPa, w=349.8 kJ/kg
 */

#include "components/ThermodynamicState.h"
#include "components/ComponentResult.h"
#include <string>

class Turbine {
public:

    /*
     * solve() — compute turbine outlet state and work output.
     *
     * Parameters:
     *   inlet    — fully defined inlet state (State 1)
     *   P_out    — outlet pressure [kPa]
     *   eta      — isentropic efficiency [0-1], default 1.0 (ideal)
     *   source   — data source for property lookups
     *
     * Returns ComponentResult with:
     *   - outlet state (actual, State 2)
     *   - outletS state (isentropic, State 2s)
     *   - specificWork = h1 - h2  [kJ/kg]
     *   - isentropicEfficiency = eta
     *   - full calculation step trace
     *   - energy balance verification
     *
     * Throws std::runtime_error if outlet state cannot be determined.
     */
    static ComponentResult solve(
        const ThermodynamicState& inlet,
        double P_out_kPa,
        double eta = 1.0,
        DataSource source = DataSource::COOLPROP
    );

    /*
     * solveFromStates() — compute work given both inlet and outlet states.
     * Used when the outlet state is already known (e.g. from Cengel tables).
     */
    static ComponentResult solveFromStates(
        const ThermodynamicState& inlet,
        const ThermodynamicState& outlet,
        const std::string& reference = ""
    );

private:
    Turbine() = delete;
};

#endif // TURBINE_H