#ifndef DATACORE_H
#define DATACORE_H

/*
 * DataCore.h
 * ----------
 * Wrapper C++ sobre dos bases de datos SQLite:
 *
 *   DataSource::CENGEL
 *     thermodata.db — valores digitalizados de Cengel & Boles 7ma ed.
 *     Tabla: cengel_ideal_gas (gas_name, T_K, h_kJkg, u_kJkg, s0_kJkgK, Pr, vr)
 *     Unidades: kJ/kg, kJ/(kg·K)
 *     Gases: Air, Nitrogen, Oxygen, CarbonDioxide, CarbonMonoxide,
 *            Hydrogen, WaterVapor, OxygenAtomic, Hydroxyl
 *     Uso: ejercicios académicos que deben coincidir con el libro.
 *
 *   DataSource::COOLPROP
 *     coolprop.db — generado desde CoolProp (ecuaciones de estado NIST).
 *     Tablas: fluids, grid_saturation, grid_superheated, grid_subcooled,
 *             grid_supercritical, grid_twophase
 *     Unidades internas: J/kg (DataCore convierte a kJ/kg en la salida)
 *     Fluidos: Water, Air, Nitrogen, Oxygen, R134a, R410A, Ammonia, ...
 *     Uso: mayor precisión, refrigerantes, problemas de ingeniería.
 *
 * Uso:
 *   DataCore::open("data/thermodata.db", "data/coolprop.db");
 *
 *   // Ejercicio del libro — fuente Cengel
 *   IdealGasProps p = DataCore::idealGasAt("Air", 500.0, DataSource::CENGEL);
 *
 *   // Vapor de agua — fuente CoolProp
 *   SatWaterProps s = DataCore::satWaterAtT(100.0, DataSource::COOLPROP);
 *
 *   // Cambio de entropía Ejemplo 7-9 Cengel (método exacto Ec. 7-39)
 *   double ds = DataCore::idealGasDeltaEntropy(
 *       "Air", 290.0, 100e3, 330.0, 600e3, 0.2870, DataSource::CENGEL);
 *
 *   DataCore::close();
 */

#include <string>
#include <stdexcept>

/* ---------------------------------------------------------------------------
 * DataSource
 * ---------------------------------------------------------------------------*/
enum class DataSource {
    CENGEL,    /* thermodata.db — valores exactos del libro Cengel    */
    COOLPROP   /* coolprop.db   — CoolProp, mayor precisión y fluidos */
};

/* ---------------------------------------------------------------------------
 * IdealGasProps — propiedades de gas ideal a temperatura T
 * Todas las magnitudes en kJ/kg o kJ/(kg·K)
 * ---------------------------------------------------------------------------*/
struct IdealGasProps {
    double     T_K;     /* temperatura [K]                              */
    double     h;       /* entalpía específica [kJ/kg]                  */
    double     u;       /* energía interna específica [kJ/kg]           */
    double     s0;      /* entropía a P_ref=101325 Pa [kJ/(kg·K)]       */
    double     Cp;      /* calor específico Cp [kJ/(kg·K)] (0 si no disponible) */
    double     Cv;      /* calor específico Cv [kJ/(kg·K)] (0 si no disponible) */
    DataSource source;
};

/* ---------------------------------------------------------------------------
 * SatWaterProps — propiedades de agua saturada (tablas A-4 / A-5)
 * ---------------------------------------------------------------------------*/
struct SatWaterProps {
    double     T_C;     /* temperatura de saturación [°C]  */
    double     P_kPa;   /* presión de saturación [kPa]     */
    double     vf;      /* vol. esp. líquido [m³/kg]       */
    double     vg;      /* vol. esp. vapor [m³/kg]         */
    double     hf;      /* entalpía líquido [kJ/kg]        */
    double     hfg;     /* calor de vaporización [kJ/kg]   */
    double     hg;      /* entalpía vapor [kJ/kg]          */
    double     sf;      /* entropía líquido [kJ/(kg·K)]    */
    double     sfg;     /* entropía de vaporización        */
    double     sg;      /* entropía vapor [kJ/(kg·K)]      */
    DataSource source;
};

/* ---------------------------------------------------------------------------
 * SuperheatedProps — vapor sobrecalentado (tabla A-6)
 * ---------------------------------------------------------------------------*/
struct SuperheatedProps {
    double     T_C;     /* temperatura [°C]          */
    double     P_kPa;   /* presión [kPa]             */
    double     v;       /* vol. esp. [m³/kg]         */
    double     u;       /* energía interna [kJ/kg]   */
    double     h;       /* entalpía [kJ/kg]          */
    double     s;       /* entropía [kJ/(kg·K)]      */
    DataSource source;
};

/* ---------------------------------------------------------------------------
 * DataCore — clase estática, conexiones singleton
 * ---------------------------------------------------------------------------*/
class DataCore {
public:

    /*
     * open() — abre ambas BDs. Llamar antes de cualquier consulta.
     *   cengel_db_path   — ruta a thermodata.db  (puede ser "" para omitir)
     *   coolprop_db_path — ruta a coolprop.db    (puede ser "" para omitir)
     * Lanza std::runtime_error si una ruta no vacía no puede abrirse.
     */
    static void open(const std::string& cengel_db_path,
                     const std::string& coolprop_db_path = "");

    /* close() — libera todas las conexiones. */
    static void close();

    /* isOpen() — true si al menos una BD está abierta. */
    static bool isOpen();

    /*
     * idealGasAt() — propiedades a T_K [K], interpolación lineal.
     *
     * Nombres de gas válidos:
     *   CENGEL:   "Air","Nitrogen","Oxygen","CarbonDioxide","CarbonMonoxide",
     *             "Hydrogen","WaterVapor","OxygenAtomic","Hydroxyl"
     *   COOLPROP: "Water","Air","Nitrogen","Oxygen","CarbonDioxide",
     *             "R134a","R410A","Ammonia","Methane","Propane","Argon",
     *             "Hydrogen","Helium","R22","R32","R1234yf"
     *
     * s0 está a P_ref = 101325 Pa.
     * Para s a presión arbitraria P:  s(T,P) = s0(T) - R·ln(P/P_ref)
     */
    static IdealGasProps idealGasAt(
        const std::string& gas_name,
        double             T_K,
        DataSource         source = DataSource::CENGEL
    );

    /*
     * idealGasDeltaEntropy() — método exacto, Cengel Ec. 7-39:
     *   Δs = [s°(T2) - s°(T1)] - R·ln(P2/P1)
     *
     * Más preciso que Cp·ln(T2/T1) - R·ln(P2/P1) porque usa s° tabulada
     * que incorpora la variación de Cp con la temperatura.
     *
     * Parámetros: T1_K, T2_K en [K]; P1_Pa, P2_Pa en [Pa]; R en [kJ/(kg·K)]
     */
    static double idealGasDeltaEntropy(
        const std::string& gas_name,
        double T1_K, double P1_Pa,
        double T2_K, double P2_Pa,
        double R,
        DataSource source = DataSource::CENGEL
    );

    /*
     * idealGasDeltaEnthalpy() — Δh = h(T2) - h(T1) [kJ/kg]
     */
    static double idealGasDeltaEnthalpy(
        const std::string& gas_name,
        double T1_K,
        double T2_K,
        DataSource source = DataSource::CENGEL
    );

    /*
     * satWaterAtT() — agua saturada a T_C [°C], interpolación lineal.
     * CENGEL: usa cengel_sat_water_T (si existe) o grid_saturation.
     * COOLPROP: usa grid_saturation (fluido "Water", fluid_id=1).
     */
    static SatWaterProps satWaterAtT(
        double     T_C,
        DataSource source = DataSource::CENGEL
    );

    /*
     * satWaterAtP() — agua saturada a P_kPa [kPa], interpolación lineal.
     */
    static SatWaterProps satWaterAtP(
        double     P_kPa,
        DataSource source = DataSource::CENGEL
    );

    /*
     * superheatedAt() — vapor sobrecalentado a (T_C [°C], P_kPa [kPa]).
     * Interpolación bilineal sobre la grilla (T, P).
     * COOLPROP: usa grid_superheated (fluido "Water", fluid_id=1).
     */
    static SuperheatedProps superheatedAt(
        double     T_C,
        double     P_kPa,
        DataSource source = DataSource::CENGEL
    );

    /* sourceLabel() — etiqueta legible de la fuente */
    static const char* sourceLabel(DataSource source);

    /* fluidIdFromName() — resuelve fluid_id en coolprop.db por nombre */
    static int fluidId(const std::string& fluid_name);

    /* Interpoladores */
    static IdealGasProps  interpolIG(
        const IdealGasProps&  lo, const IdealGasProps&  hi,
        double T_K, DataSource src);

    static SatWaterProps  interpolSat(
        const SatWaterProps&  lo, const SatWaterProps&  hi,
        double t, double T_C, double P_kPa, DataSource src);

private:
    DataCore() = delete;

    static void* m_cengel_db;    /* sqlite3* — thermodata.db */
    static void* m_coolprop_db;  /* sqlite3* — coolprop.db   */

    static void  assertOpen(DataSource source);
    static void* dbHandle(DataSource source);

    
};

#endif // DATACORE_H