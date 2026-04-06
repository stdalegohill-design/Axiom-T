/*
 * ComponentResult.cpp
 * -------------------
 * Implementation of ComponentResult pedagogical output.
 */

#include "components/ComponentResult.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cmath>

static const std::string LINE_D(65, '=');
static const std::string LINE_S(65, '-');

/* ---------------------------------------------------------------------------
 * addStep
 * ---------------------------------------------------------------------------*/
void ComponentResult::addStep(
    const std::string& description,
    const std::string& equation,
    const std::string& values,
    double             result,
    const std::string& unit)
{
    CalculationStep step;
    step.stepNumber  = static_cast<int>(steps.size()) + 1;
    step.description = description;
    step.equation    = equation;
    step.values      = values;
    step.result      = result;
    step.unit        = unit;
    steps.push_back(step);
}

/* ---------------------------------------------------------------------------
 * phaseLabel helper
 * ---------------------------------------------------------------------------*/
static const char* phaseLabel(FluidPhase p) {
    switch (p) {
        case FluidPhase::SUPERHEATED_VAPOR:  return "vapor sobrecalentado";
        case FluidPhase::SATURATED_VAPOR:    return "vapor saturado (x=1)";
        case FluidPhase::TWO_PHASE:          return "mezcla liquido-vapor";
        case FluidPhase::SATURATED_LIQUID:   return "liquido saturado (x=0)";
        case FluidPhase::COMPRESSED_LIQUID:  return "liquido comprimido";
        case FluidPhase::SUPERCRITICAL:      return "supercritico";
        case FluidPhase::IDEAL_GAS:          return "gas ideal";
        default:                             return "fase desconocida";
    }
}

/* ---------------------------------------------------------------------------
 * print state helper
 * ---------------------------------------------------------------------------*/
static void printState(const std::string& label,
                       const ThermodynamicState& st) {
    auto row = [](const std::string& name, double val, const std::string& unit) {
        if (!std::isnan(val))
            std::cout << "    " << std::left << std::setw(24) << name
                      << std::right << std::setw(10) << std::fixed
                      << std::setprecision(4) << val
                      << "  " << unit << "\n";
    };

    std::cout << "  " << label << " [" << phaseLabel(st.phase) << "]\n";
    row("P",   st.P_kPa, "kPa");
    row("T",   st.T_C,   "deg C");
    row("h",   st.h,     "kJ/kg");
    row("s",   st.s,     "kJ/(kg·K)");
    row("v",   st.v,     "m3/kg");
    if (!std::isnan(st.x) && st.phase == FluidPhase::TWO_PHASE)
        row("x (calidad)", st.x, "-");
    if (!std::isnan(st.Cp))
        row("Cp", st.Cp, "kJ/(kg·K)");
}

/* ---------------------------------------------------------------------------
 * print
 * ---------------------------------------------------------------------------*/
void ComponentResult::print() const {
    std::cout << "\n" << LINE_D << "\n";
    std::cout << "  AXIOM-T | " << componentType << "\n";
    if (!fluid.empty())
        std::cout << "  Fluido: " << fluid << "\n";
    if (!reference.empty())
        std::cout << "  Referencia: " << reference << "\n";
    std::cout << LINE_D << "\n";

    /* States */
    std::cout << "\n" << LINE_S << "\n";
    std::cout << "  Estados termodinamicos\n";
    std::cout << LINE_S << "\n\n";
    printState("Estado 1 (entrada)", inlet);
    std::cout << "\n";
    printState("Estado 2 (salida actual)", outlet);
    if (!std::isnan(outletS.h)) {
        std::cout << "\n";
        printState("Estado 2s (salida isentropica)", outletS);
    }

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

    /* Energy balance */
    std::cout << LINE_S << "\n";
    std::cout << "  Balance de energia (Primera Ley)\n";
    std::cout << LINE_S << "\n";
    std::cout << "  " << balance.description << "\n";
    std::cout << "  E_entrada = " << std::fixed << std::setprecision(4)
              << balance.E_in << " kJ/kg\n";
    std::cout << "  E_salida  = " << balance.E_out << " kJ/kg\n";
    std::cout << "  Desbalance = " << balance.imbalance << " kJ/kg  ["
              << (balance.balanced ? "OK" : "REVISAR") << "]\n";

    /* Final results */
    std::cout << "\n" << LINE_S << "\n";
    std::cout << "  Resultados finales\n";
    std::cout << LINE_S << "\n";

    auto printResult = [](const std::string& name, double val,
                          const std::string& unit) {
        if (!std::isnan(val))
            std::cout << "  " << std::left << std::setw(30) << name
                      << std::right << std::setw(10) << std::fixed
                      << std::setprecision(4) << val
                      << "  " << unit << "\n";
    };

    printResult("Trabajo especifico w", specificWork, "kJ/kg");
    printResult("Calor especifico q",   specificHeat,  "kJ/kg");
    if (!std::isnan(isentropicEfficiency))
        printResult("Eficiencia isentropica eta",
                    isentropicEfficiency * 100.0, "%");

    std::cout << "\n" << LINE_D << "\n\n";
}

/* ---------------------------------------------------------------------------
 * summary
 * ---------------------------------------------------------------------------*/
std::string ComponentResult::summary() const {
    std::ostringstream oss;
    oss << componentType << " [" << fluid << "]"
        << "  w=" << std::fixed << std::setprecision(2) << specificWork << " kJ/kg";
    if (!std::isnan(isentropicEfficiency))
        oss << "  eta=" << std::setprecision(1)
            << isentropicEfficiency * 100.0 << "%";
    return oss.str();
}