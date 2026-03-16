/*
 * main.cpp
 * --------
 * Axiom-T — Phase 1 demonstration.
 *
 * Problem: Cengel & Boles, Thermodynamics, 7th ed. (Spanish)
 *          Capitulo 7, Ejemplo 7-9
 *          "Cambio de entropia de un gas ideal"
 *
 * Statement:
 *   Air is compressed from an initial state of 100 kPa and 17 C (290 K)
 *   to a final state of 600 kPa and 57 C (330 K).
 *   Determine the entropy change of air using constant specific heats
 *   (cold-air standard, Cp evaluated at average temperature 37 C).
 *
 * Reference solution (Cengel, inciso b):
 *   Ds = Cp,prom * ln(T2/T1) - R * ln(P2/P1)
 *      = 1.006 * ln(330/290) - 0.287 * ln(600/100)
 *      = -0.3842 kJ/(kg·K)
 *
 * Note on Cp:
 *   Cengel uses Cp = 1.006 kJ/(kg·K) evaluated at T_avg = 37 C (Table A-2b).
 *   Axiom-T stores Cp = 1.005 kJ/(kg·K) (standard value, Table A-2).
 *   The small difference (~0.1%) reflects the temperature dependence of Cp
 *   which Phase 2 (variable specific heats) will handle with Tables A-17/A-25.
 */

#include "substances/IdealGas.h"
#include "solvers/NumericalSolver.h"

#include <iostream>
#include <iomanip>
#include <string>
#include <cmath>

/* ---------------------------------------------------------------------------
 * Console formatting helpers
 * ---------------------------------------------------------------------------*/
static const std::string LINE_DOUBLE(65, '=');
static const std::string LINE_SINGLE(65, '-');

static void printHeader(const std::string& title) {
    std::cout << "\n" << LINE_DOUBLE << "\n";
    std::cout << "  " << title << "\n";
    std::cout << LINE_DOUBLE << "\n";
}

static void printSection(const std::string& title) {
    std::cout << "\n" << LINE_SINGLE << "\n";
    std::cout << "  " << title << "\n";
    std::cout << LINE_SINGLE << "\n\n";
}

static void printRow(const std::string& label,
                     double value,
                     const std::string& unit,
                     const std::string& equation = "") {
    std::cout << std::left  << std::setw(30) << ("  " + label)
              << std::right << std::setw(12) << std::fixed
              << std::setprecision(4) << value
              << "  " << std::left << std::setw(14) << unit;
    if (!equation.empty())
        std::cout << "  [" << equation << "]";
    std::cout << "\n";
}

static void printVerify(const std::string& label,
                        double computed,
                        double reference,
                        const std::string& unit) {
    double err    = std::fabs((computed - reference) / reference) * 100.0;
    std::string status = (err < 0.1) ? "PASS" : "FAIL";
    std::cout << std::left  << std::setw(30) << ("  " + label)
              << std::right << std::setw(12) << std::fixed
              << std::setprecision(4) << computed
              << "  " << std::left << std::setw(14) << unit
              << "  [" << status << " | err = "
              << std::fixed << std::setprecision(4) << err << "%]\n";
}

/* ---------------------------------------------------------------------------
 * main
 * ---------------------------------------------------------------------------*/
int main() {

    /* ------------------------------------------------------------------
     * Gas and boundary conditions — Cengel Example 7-9
     * ------------------------------------------------------------------ */
    IdealGas air = IdealGas::Air();

    Temperature T1 = Temperature::fromCelsius(17.0);  /* 290 K */
    Pressure    P1 = Pressure::fromKPa(100.0);
    Temperature T2 = Temperature::fromCelsius(57.0);  /* 330 K */
    Pressure    P2 = Pressure::fromKPa(600.0);

    /* ------------------------------------------------------------------
     * Header
     * ------------------------------------------------------------------ */
    printHeader("AXIOM-T  |  Phase 1 — Verificacion Ejemplo 7-9");
    std::cout << "  Cengel & Boles, Termodinamica, 7ma ed.\n";
    std::cout << "  Capitulo 7, Ejemplo 7-9\n";
    std::cout << "  \"Cambio de entropia de un gas ideal\"\n";

    /* ------------------------------------------------------------------
     * Section 1 — Problem statement
     * ------------------------------------------------------------------ */
    printSection("1. Datos del problema");

    printRow("Gas",              0,       "",         "Aire (gas ideal)");
    printRow("T1 (entrada)",     T1.toKelvin(),   "K");
    printRow("T1 (entrada)",     T1.toCelsius(),  "deg C");
    printRow("P1 (entrada)",     P1.toKPa(),      "kPa");
    printRow("T2 (salida)",      T2.toKelvin(),   "K");
    printRow("T2 (salida)",      T2.toCelsius(),  "deg C");
    printRow("P2 (salida)",      P2.toKPa(),      "kPa");

    /* ------------------------------------------------------------------
     * Section 2 — Gas properties
     * ------------------------------------------------------------------ */
    printSection("2. Propiedades del gas  [Cengel Tabla A-2 / NIST]");

    printRow("Cp (Axiom-T, Tabla A-2)", air.Cp(),    "kJ/(kg·K)");
    printRow("Cp (Cengel 7-9, A-2b)",  1.006,        "kJ/(kg·K)",
             "promedio a T_avg=37C");
    printRow("R",                       air.R(),      "kJ/(kg·K)");
    printRow("gamma = Cp/Cv",           air.gamma(),  "[-]");

    std::cout << "\n  Nota: Cengel usa Cp=1.006 evaluado a T_avg=37 C (Tabla A-2b).\n";
    std::cout << "  Axiom-T usa Cp=1.005 (Tabla A-2, valor estandar).\n";
    std::cout << "  La diferencia es esperada — Fase 2 implementara Cp variable.\n";

    /* ------------------------------------------------------------------
     * Section 3 — Step-by-step calculation
     * ------------------------------------------------------------------ */
    printSection("3. Calculo paso a paso");

    double ln_T = std::log(T2.toKelvin() / T1.toKelvin());
    double ln_P = std::log(P2.toPascal() / P1.toPascal());

    std::cout << "  Ecuacion (Cengel Ec. 7-34):\n";
    std::cout << "  Ds = Cp * ln(T2/T1) - R * ln(P2/P1)\n\n";

    std::cout << "  Sustitucion con Axiom-T (Cp = 1.005):\n";
    std::cout << "  Ds = " << std::fixed << std::setprecision(3)
              << air.Cp() << " * ln(" << T2.toKelvin() << "/"
              << T1.toKelvin() << ") - "
              << air.R() << " * ln(" << P2.toKPa() << "/"
              << P1.toKPa() << ")\n";
    std::cout << "  Ds = " << air.Cp() << " * (" << std::setprecision(4)
              << ln_T << ") - " << std::setprecision(3) << air.R()
              << " * (" << std::setprecision(4) << ln_P << ")\n";

    double ds_axiom = air.deltaEntropy(T1, P1, T2, P2);

    std::cout << "  Ds = " << std::setprecision(4)
              << air.Cp() * ln_T << " - "
              << air.R() * ln_P << "\n";
    std::cout << "  Ds = " << std::setprecision(4) << ds_axiom
              << " kJ/(kg·K)\n\n";

    std::cout << "  Sustitucion con Cp=1.006 (Cengel Ejemplo 7-9):\n";
    double ds_cengel_manual = 1.006 * ln_T - 0.287 * ln_P;
    std::cout << "  Ds = 1.006 * (" << std::setprecision(4) << ln_T
              << ") - 0.287 * (" << ln_P << ")\n";
    std::cout << "  Ds = " << std::setprecision(4) << 1.006 * ln_T
              << " - " << 0.287 * ln_P << "\n";
    std::cout << "  Ds = " << std::setprecision(4) << ds_cengel_manual
              << " kJ/(kg·K)\n";

    /* ------------------------------------------------------------------
     * Section 4 — Results summary
     * ------------------------------------------------------------------ */
    printSection("4. Resultados");

    printRow("Ds  (Axiom-T, Cp=1.005)",   ds_axiom,        "kJ/(kg·K)",
             "Ec. 7-34");
    printRow("Ds  (Cengel,  Cp=1.006)",   ds_cengel_manual,"kJ/(kg·K)",
             "Ec. 7-34");
    printRow("Ds  (Cengel libro, ref.)",  -0.3842,          "kJ/(kg·K)",
             "resultado del libro");

    /* ------------------------------------------------------------------
     * Section 5 — Verification against Cengel reference
     * ------------------------------------------------------------------ */
    printSection("5. Verificacion  [referencia: Cengel -0.3842 kJ/(kg·K)]");

    /* tolerance relaxed to 0.1% because Cengel uses Cp=1.006 vs our 1.005 */
    printVerify("Axiom-T vs Cengel libro",  ds_axiom,        -0.3842, "kJ/(kg·K)");
    printVerify("Manual(1.006) vs Cengel",  ds_cengel_manual,-0.3842, "kJ/(kg·K)");

    double diff_cp = std::fabs(ds_axiom - ds_cengel_manual);
    std::cout << "\n  Diferencia entre Cp=1.005 y Cp=1.006:\n";
    std::cout << "  |Ds_axiom - Ds_cengel| = " << std::fixed
              << std::setprecision(6) << diff_cp << " kJ/(kg·K)\n";
    std::cout << "  Esta diferencia es fisica, no un error del programa.\n";
    std::cout << "  Desaparecera en Fase 2 con Cp variable (Tablas A-17).\n";

    /* ------------------------------------------------------------------
     * Section 6 — Additional quantities
     * ------------------------------------------------------------------ */
    printSection("6. Otras magnitudes del proceso");

    printRow("Dh = Cp*(T2-T1)",
             air.deltaEnthalpy(T1, T2),        "kJ/kg",
             "Cp*(330-290)");
    printRow("Du = Cv*(T2-T1)",
             air.deltaInternalEnergy(T1, T2),  "kJ/kg",
             "Cv*(330-290)");
    printRow("v1 = R*T1/P1",
             air.specificVolume(P1, T1),       "m3/kg");
    printRow("v2 = R*T2/P2",
             air.specificVolume(P2, T2),       "m3/kg");

    /* ------------------------------------------------------------------
     * Footer
     * ------------------------------------------------------------------ */
    std::cout << "\n" << LINE_DOUBLE << "\n";
    std::cout << "  Axiom-T Phase 1  |  Ejemplo 7-9 verificado\n";
    std::cout << "  Resultado Axiom-T:  " << std::fixed << std::setprecision(4)
              << ds_axiom << " kJ/(kg*K)\n";
    std::cout << "  Resultado Cengel:  -0.3842 kJ/(kg*K)\n";
    std::cout << "  Diferencia:         causada por Cp=1.005 vs 1.006\n";
    std::cout << LINE_DOUBLE << "\n\n";

    return 0;
}