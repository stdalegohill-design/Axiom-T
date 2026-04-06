#ifndef COMPRESSOR_H
#define COMPRESSOR_H

/*
 * Compressor.h
 * ------------
 * Steady-flow compressor component solver.
 *
 * Models an adiabatic compressor where low-pressure fluid is compressed
 * requiring work input. Supports both ideal (isentropic) and real
 * (with isentropic efficiency) compressors.
 *
 * Energy balance (Cengel & Boles, Eq. 6-30):
 *   w_in = h2 - h1          [kJ/kg]
 *
 * Isentropic efficiency (Cengel & Boles, Eq. 7-62):
 *   eta_C = w_isentropic / w_actual
 *         = (h2s - h1) / (h2 - h1)
 *
 * Note: for ideal gases (cold-air standard), this simplifies to:
 *   eta_C = (T2s - T1) / (T2 - T1)
 *
 * Validated against:
 *   Cengel & Boles, Example 7-7 (isentropic compression of air)
 *   Air: T1=300K P1=100kPa → P2=800kPa, w_s=243.3 kJ/kg
 */

#include "components/ThermodynamicState.h"
#include "components/ComponentResult.h"

class Compressor {
public:

    /*
     * solve() — compute compressor outlet state and work input.
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
     *   - specificWork = h2 - h1  [kJ/kg]  (positive = work IN)
     *   - isentropicEfficiency = eta
     *   - full calculation step trace
     *   - energy balance verification
     */
    static ComponentResult solve(
        const ThermodynamicState& inlet,
        double P_out_kPa,
        double eta = 1.0,
        DataSource source = DataSource::COOLPROP
    );

    /*
     * solveFromStates() — compute work given both states directly.
     * Used when states are taken from Cengel tables.
     */
    static ComponentResult solveFromStates(
        const ThermodynamicState& inlet,
        const ThermodynamicState& outlet,
        const std::string& reference = ""
    );

private:
    Compressor() = delete;
};

#endif // COMPRESSOR_H