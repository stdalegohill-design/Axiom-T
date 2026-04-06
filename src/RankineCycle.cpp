/*
 * RankineCycle.cpp
 * ----------------
 * Implementation of the Rankine cycle solver.
 */

#include "components/RankineCycle.h"
#include <cmath>
#include <iostream>
#include <iomanip>
#include <sstream>

static const std::string LINE_D(65, '=');
static const std::string LINE_S(65, '-');

/* ---------------------------------------------------------------------------
 * RankineResult::addStep
 * ---------------------------------------------------------------------------*/
void RankineResult::addStep(
    const std::string& desc, const std::string& eq,
    const std::string& vals, double res, const std::string& unit)
{
    CalculationStep step;
    step.stepNumber  = static_cast<int>(steps.size()) + 1;
    step.description = desc;
    step.equation    = eq;
    step.values      = vals;
    step.result      = res;
    step.unit        = unit;
    steps.push_back(step);
}

/* ---------------------------------------------------------------------------
 * RankineResult::print
 * ---------------------------------------------------------------------------*/
void RankineResult::print() const {
    auto row = [](const std::string& label, double val,
                  const std::string& unit) {
        if (!std::isnan(val))
            std::cout << "  " << std::left << std::setw(32) << label
                      << std::right << std::setw(10) << std::fixed
                      << std::setprecision(4) << val
                      << "  " << unit << "\n";
    };

    std::cout << "\n" << LINE_D << "\n";
    std::cout << "  AXIOM-T | Ciclo Rankine\n";
    std::cout << "  Fluido  : " << fluid << "\n";
    if (!reference.empty())
        std::cout << "  Ref.    : " << reference << "\n";
    std::cout << LINE_D << "\n";

    /* States */
    std::cout << "\n" << LINE_S << "\n";
    std::cout << "  Estados del ciclo\n";
    std::cout << LINE_S << "\n";

    auto printSt = [&](const std::string& label,
                       const ThermodynamicState& st) {
        std::cout << "\n  " << label << "\n";
        row("  P", st.P_kPa, "kPa");
        row("  T", st.T_C,   "deg C");
        row("  h", st.h,     "kJ/kg");
        row("  s", st.s,     "kJ/(kg·K)");
        if (!std::isnan(st.x) && st.x >= 0.0 && st.x <= 1.0)
            row("  x (calidad)", st.x, "-");
    };

    printSt("Estado 1 — entrada turbina (salida caldera)", state1);
    printSt("Estado 2 — salida turbina (entrada condensador)", state2);
    printSt("Estado 3 — salida condensador (entrada bomba)", state3);
    printSt("Estado 4 — salida bomba (entrada caldera)", state4);

    /* Calculation steps */
    std::cout << "\n" << LINE_S << "\n";
    std::cout << "  Calculo paso a paso\n";
    std::cout << LINE_S << "\n\n";

    for (const auto& step : steps) {
        std::cout << "  Paso " << step.stepNumber
                  << ": " << step.description << "\n";
        if (!step.equation.empty())
            std::cout << "    Ecuacion : " << step.equation << "\n";
        if (!step.values.empty())
            std::cout << "    Valores  : " << step.values << "\n";
        std::cout << "    Resultado: "
                  << std::fixed << std::setprecision(4) << step.result
                  << " " << step.unit << "\n\n";
    }

    /* Performance summary */
    std::cout << LINE_S << "\n";
    std::cout << "  Rendimiento del ciclo\n";
    std::cout << LINE_S << "\n\n";
    row("Trabajo turbina  w_t",    w_turbine, "kJ/kg");
    row("Trabajo bomba    w_p",    w_pump,    "kJ/kg");
    row("Trabajo neto     w_net",  w_net,     "kJ/kg");
    row("Calor entrada    q_in",   q_in,      "kJ/kg");
    row("Calor rechazado  q_out",  q_out,     "kJ/kg");
    row("Eficiencia termica eta",  eta_th * 100.0, "%");
    row("Razon trabajo inverso BWR", BWR * 100.0, "%");
    if (!std::isnan(x2) && x2 >= 0.0 && x2 <= 1.0)
        row("Calidad salida turbina x2", x2, "-");
    if (eta_turbine < 1.0)
        row("Eficiencia turbina", eta_turbine * 100.0, "%");
    if (eta_pump < 1.0)
        row("Eficiencia bomba", eta_pump * 100.0, "%");

    std::cout << "\n" << LINE_D << "\n\n";
}

/* ---------------------------------------------------------------------------
 * RankineCycle::solve
 * ---------------------------------------------------------------------------*/
RankineResult RankineCycle::solve(
    const std::string& fluid,
    double T_boiler_C,
    double P_boiler_kPa,
    double P_cond_kPa,
    double eta_turbine,
    double eta_pump,
    DataSource source)
{
    RankineResult result;
    result.fluid        = fluid;
    result.source       = source;
    result.eta_turbine  = eta_turbine;
    result.eta_pump     = eta_pump;

    /* ------------------------------------------------------------------
     * State 1: Turbine inlet (boiler outlet)
     * Superheated steam at T_boiler, P_boiler
     * ------------------------------------------------------------------ */
    result.state1 = ThermodynamicState::fromTP(
        fluid, T_boiler_C, P_boiler_kPa, source);

    {
        std::ostringstream vals;
        vals << std::fixed << std::setprecision(3)
             << "T1=" << result.state1.T_C << "°C  "
             << "P1=" << result.state1.P_kPa << " kPa  "
             << "h1=" << result.state1.h << " kJ/kg  "
             << "s1=" << result.state1.s << " kJ/(kg·K)";
        result.addStep(
            "Estado 1 — vapor sobrecalentado (salida caldera)",
            "", vals.str(), result.state1.h, "kJ/kg");
    }

    /* ------------------------------------------------------------------
     * State 2: Turbine outlet (condenser inlet)
     * Use Turbine::solve for full pedagogical trace
     * ------------------------------------------------------------------ */
    result.turbineResult = Turbine::solve(
        result.state1, P_cond_kPa, eta_turbine, source);
    result.state2 = result.turbineResult.outlet;

    {
        std::ostringstream vals;
        vals << std::fixed << std::setprecision(3)
             << "h2=" << result.state2.h << " kJ/kg  "
             << "s2=" << result.state2.s << " kJ/(kg·K)  "
             << "P2=" << result.state2.P_kPa << " kPa";
        if (!std::isnan(result.state2.x))
            vals << "  x2=" << std::setprecision(4) << result.state2.x;
        result.addStep(
            "Estado 2 — salida turbina (entrada condensador)",
            "", vals.str(), result.state2.h, "kJ/kg");
    }

    result.w_turbine = result.turbineResult.specificWork;
    result.x2        = result.state2.x;

    /* ------------------------------------------------------------------
     * State 3: Condenser outlet (pump inlet)
     * Saturated liquid at P_cond (x=0)
     * ------------------------------------------------------------------ */
    result.state3 = ThermodynamicState::fromSaturatedP(
        fluid, P_cond_kPa, 0.0, source);

    {
        std::ostringstream vals;
        vals << std::fixed << std::setprecision(3)
             << "T3=" << result.state3.T_C << "°C  "
             << "h3=" << result.state3.h << " kJ/kg  "
             << "v3=" << std::setprecision(6) << result.state3.v << " m3/kg";
        result.addStep(
            "Estado 3 — liquido saturado (salida condensador, x=0)",
            "", vals.str(), result.state3.h, "kJ/kg");
    }

    /* Condenser heat rejected */
    double q_out = result.state2.h - result.state3.h;
    {
        std::ostringstream eq, vals;
        eq   << "q_out = h2 - h3";
        vals << std::fixed << std::setprecision(3)
             << result.state2.h << " - " << result.state3.h;
        result.addStep(
            "Calor rechazado en el condensador",
            eq.str(), vals.str(), q_out, "kJ/kg");
    }

    /* ------------------------------------------------------------------
     * State 4: Pump outlet (boiler inlet)
     * Use Pump::solve for full pedagogical trace
     * ------------------------------------------------------------------ */
    result.pumpResult = Pump::solve(
        result.state3, P_boiler_kPa, eta_pump, source);
    result.state4 = result.pumpResult.outlet;

    result.w_pump = result.pumpResult.specificWork;

    {
        std::ostringstream vals;
        vals << std::fixed << std::setprecision(3)
             << "h4=" << result.state4.h << " kJ/kg  "
             << "P4=" << result.state4.P_kPa << " kPa";
        result.addStep(
            "Estado 4 — liquido comprimido (salida bomba, entrada caldera)",
            "", vals.str(), result.state4.h, "kJ/kg");
    }

    /* ------------------------------------------------------------------
     * Boiler heat added
     * q_in = h1 - h4
     * ------------------------------------------------------------------ */
    double q_in = result.state1.h - result.state4.h;
    {
        std::ostringstream eq, vals;
        eq   << "q_in = h1 - h4";
        vals << std::fixed << std::setprecision(3)
             << result.state1.h << " - " << result.state4.h;
        result.addStep(
            "Calor suministrado en la caldera",
            eq.str(), vals.str(), q_in, "kJ/kg");
    }

    /* ------------------------------------------------------------------
     * Cycle performance
     * ------------------------------------------------------------------ */
    double w_net  = result.w_turbine - result.w_pump;
    double eta_th = w_net / q_in;
    double BWR    = result.w_pump / result.w_turbine;

    result.w_net  = w_net;
    result.q_in   = q_in;
    result.q_out  = q_out;
    result.eta_th = eta_th;
    result.BWR    = BWR;

    {
        std::ostringstream eq, vals;
        eq   << "w_net = w_turbina - w_bomba";
        vals << std::fixed << std::setprecision(3)
             << result.w_turbine << " - " << result.w_pump;
        result.addStep("Trabajo neto del ciclo",
                        eq.str(), vals.str(), w_net, "kJ/kg");
    }
    {
        std::ostringstream eq, vals;
        eq   << "eta_th = w_net / q_in";
        vals << std::fixed << std::setprecision(3)
             << w_net << " / " << q_in
             << " = " << std::setprecision(4) << eta_th;
        result.addStep("Eficiencia termica del ciclo",
                        eq.str(), vals.str(), eta_th * 100.0, "%");
    }
    {
        std::ostringstream eq, vals;
        eq   << "BWR = w_bomba / w_turbina";
        vals << std::fixed << std::setprecision(4)
             << result.w_pump << " / " << result.w_turbine;
        result.addStep("Razon de trabajo inverso (BWR)",
                        eq.str(), vals.str(), BWR * 100.0, "%");
    }

    return result;
}