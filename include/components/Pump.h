#ifndef PUMP_H
#define PUMP_H

/*
 * Pump.h
 * ------
 * Steady-flow pump component solver.
 *
 * Models an adiabatic pump that raises pressure of an incompressible
 * liquid requiring work input. Used in Rankine cycles between the
 * condenser outlet and boiler inlet.
 *
 * For an incompressible liquid (Cengel & Boles, Eq. 6-32):
 *   w_pump,s = v * (P2 - P1)    [kJ/kg]
 *   h2 = h1 + w_pump,s / eta_P
 *
 * This is a much simpler calculation than a compressor because
 * v ≈ constant for liquids (incompressible assumption).
 *
 * Isentropic efficiency (Cengel & Boles, Eq. 7-62):
 *   eta_P = w_isentropic / w_actual
 *         = v*(P2-P1) / (h2 - h1)
 *
 * Validated against:
 *   Cengel & Boles, Example 10-1 (ideal Rankine cycle)
 *   Water: P1=10kPa (sat liquid) → P2=7MPa
 *   w_pump = 0.001010 * (7000 - 10) = 7.06 kJ/kg
 */

#include "components/ThermodynamicState.h"
#include "components/ComponentResult.h"

class Pump {
public:

    /*
     * solve() — compute pump outlet state and work input.
     *
     * Parameters:
     *   inlet    — inlet state (must be liquid, x=0 or compressed)
     *   P_out    — outlet pressure [kPa]
     *   eta      — isentropic efficiency [0-1], default 1.0 (ideal)
     *   source   — data source for property lookups
     *
     * Returns ComponentResult with:
     *   - outlet state (State 2)
     *   - specificWork = w_pump  [kJ/kg]  (positive = work IN)
     *   - full calculation step trace
     */
    static ComponentResult solve(
        const ThermodynamicState& inlet,
        double P_out_kPa,
        double eta = 1.0,
        DataSource source = DataSource::COOLPROP
    );

private:
    Pump() = delete;
};

#endif // PUMP_H