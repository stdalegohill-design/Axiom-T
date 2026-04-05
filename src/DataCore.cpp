/*
 * DataCore.cpp
 * ------------
 * Implementación de DataCore.
 * Ver DataCore.h para documentación completa de la interfaz.
 *
 * Notas de implementación:
 *   - thermodata.db (CENGEL): unidades en kJ/kg, consulta directa por gas_name
 *   - coolprop.db (COOLPROP): unidades en J/kg, requiere fluid_id lookup
 *     La conversión J→kJ se hace multiplicando por 0.001 al leer
 */

#include "datacore/DataCore.h"
#include "sqlite3.h"
#include <cmath>
#include <sstream>
#include <vector>
#include <string>

/* ---------------------------------------------------------------------------
 * Miembros estáticos
 * ---------------------------------------------------------------------------*/
void* DataCore::m_cengel_db   = nullptr;
void* DataCore::m_coolprop_db = nullptr;

/* ---------------------------------------------------------------------------
 * Utilidades internas
 * ---------------------------------------------------------------------------*/
const char* DataCore::sourceLabel(DataSource src) {
    return (src == DataSource::CENGEL) ? "Cengel 7ma ed." : "CoolProp";
}

void* DataCore::dbHandle(DataSource src) {
    return (src == DataSource::CENGEL) ? m_cengel_db : m_coolprop_db;
}

void DataCore::assertOpen(DataSource src) {
    if (!dbHandle(src)) {
        std::string msg = "DataCore: la base de datos ";
        msg += (src == DataSource::CENGEL)
               ? "Cengel (thermodata.db)" : "CoolProp (coolprop.db)";
        msg += " no está abierta. Llama DataCore::open() primero.";
        throw std::runtime_error(msg);
    }
}

/* ---------------------------------------------------------------------------
 * Helper: ejecuta query y retorna primera fila como vector<double>
 * ---------------------------------------------------------------------------*/
static bool queryRow(void* db_handle,
                     const std::string& sql,
                     std::vector<double>& out)
{
    auto* db = reinterpret_cast<sqlite3*>(db_handle);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
        return false;
    bool found = false;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        int n = sqlite3_column_count(stmt);
        out.resize(n);
        for (int i = 0; i < n; ++i)
            out[i] = sqlite3_column_double(stmt, i);
        found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}

/* ---------------------------------------------------------------------------
 * open / close / isOpen
 * ---------------------------------------------------------------------------*/
static void openOneDB(const std::string& path, void** handle) {
    if (path.empty()) return;
    sqlite3* db = nullptr;
    int rc = sqlite3_open_v2(path.c_str(), &db,
                             SQLITE_OPEN_READONLY | SQLITE_OPEN_FULLMUTEX,
                             nullptr);
    if (rc != SQLITE_OK) {
        std::string msg = "DataCore::open fallo para '";
        msg += path; msg += "': "; msg += sqlite3_errmsg(db);
        sqlite3_close(db);
        throw std::runtime_error(msg);
    }
    *handle = reinterpret_cast<void*>(db);
}

void DataCore::open(const std::string& cengel_path,
                    const std::string& coolprop_path) {
    close();
    openOneDB(cengel_path,   &m_cengel_db);
    openOneDB(coolprop_path, &m_coolprop_db);
}

void DataCore::close() {
    if (m_cengel_db)   { sqlite3_close(reinterpret_cast<sqlite3*>(m_cengel_db));   m_cengel_db   = nullptr; }
    if (m_coolprop_db) { sqlite3_close(reinterpret_cast<sqlite3*>(m_coolprop_db)); m_coolprop_db = nullptr; }
}

bool DataCore::isOpen() {
    return m_cengel_db != nullptr || m_coolprop_db != nullptr;
}

/* ---------------------------------------------------------------------------
 * fluidId() — resuelve el fluid_id en coolprop.db por nombre
 * ---------------------------------------------------------------------------*/
int DataCore::fluidId(const std::string& fluid_name) {
    assertOpen(DataSource::COOLPROP);
    std::string sql = "SELECT fluid_id FROM fluids WHERE name='" + fluid_name + "' LIMIT 1";
    std::vector<double> row;
    if (!queryRow(m_coolprop_db, sql, row))
        throw std::invalid_argument(
            "DataCore::fluidId: fluido '" + fluid_name +
            "' no encontrado en coolprop.db");
    return static_cast<int>(row[0]);
}

/* ---------------------------------------------------------------------------
 * Interpoladores
 * ---------------------------------------------------------------------------*/
IdealGasProps DataCore::interpolIG(
    const IdealGasProps& lo, const IdealGasProps& hi,
    double T_K, DataSource src)
{
    if (hi.T_K == lo.T_K) return lo;
    double t = (T_K - lo.T_K) / (hi.T_K - lo.T_K);
    IdealGasProps p;
    p.T_K = T_K;
    p.h   = lo.h  + t * (hi.h  - lo.h);
    p.u   = lo.u  + t * (hi.u  - lo.u);
    p.s0  = lo.s0 + t * (hi.s0 - lo.s0);
    p.Cp  = lo.Cp + t * (hi.Cp - lo.Cp);
    p.Cv  = lo.Cv + t * (hi.Cv - lo.Cv);
    p.source = src;
    return p;
}

SatWaterProps DataCore::interpolSat(
    const SatWaterProps& lo, const SatWaterProps& hi,
    double t, double T_C, double P_kPa, DataSource src)
{
    SatWaterProps p;
    p.T_C=T_C; p.P_kPa=P_kPa; p.source=src;
    p.vf  = lo.vf  + t*(hi.vf  - lo.vf);
    p.vg  = lo.vg  + t*(hi.vg  - lo.vg);
    p.hf  = lo.hf  + t*(hi.hf  - lo.hf);
    p.hfg = lo.hfg + t*(hi.hfg - lo.hfg);
    p.hg  = lo.hg  + t*(hi.hg  - lo.hg);
    p.sf  = lo.sf  + t*(hi.sf  - lo.sf);
    p.sfg = lo.sfg + t*(hi.sfg - lo.sfg);
    p.sg  = lo.sg  + t*(hi.sg  - lo.sg);
    return p;
}

/* ---------------------------------------------------------------------------
 * idealGasAt — CENGEL
 * Tabla: cengel_ideal_gas, unidades kJ/kg
 * ---------------------------------------------------------------------------*/
static IdealGasProps idealGasCengel(void* db_handle,
                                    const std::string& gas_name,
                                    double T_K)
{
    auto buildQ = [&](const char* op, const char* ord) {
        std::ostringstream q;
        q << "SELECT T_K, h_kJkg, u_kJkg, s0_kJkgK,"
          << " COALESCE(Pr,0), COALESCE(vr,0)"   /* Pr/vr solo en Air */
          << " FROM cengel_ideal_gas"
          << " WHERE gas_name='" << gas_name << "'"
          << " AND T_K " << op << " " << T_K
          << " ORDER BY T_K " << ord << " LIMIT 1";
        return q.str();
    };

    std::vector<double> lo_row, hi_row;
    bool lo_ok = queryRow(db_handle, buildQ("<=","DESC"), lo_row);
    bool hi_ok = queryRow(db_handle, buildQ(">=","ASC"),  hi_row);

    if (!lo_ok && !hi_ok)
        throw std::out_of_range(
            "DataCore::idealGasAt: sin datos para '" + gas_name +
            "' cerca de T=" + std::to_string(T_K) + " K [CENGEL]");

    if (!lo_ok) lo_row = hi_row;
    if (!hi_ok) hi_row = lo_row;

    auto toIG = [](const std::vector<double>& r) {
        IdealGasProps p;
        p.T_K=r[0]; p.h=r[1]; p.u=r[2]; p.s0=r[3];
        p.Cp=0.0; p.Cv=0.0;   /* Cengel no tabula Cp/Cv directamente */
        p.source=DataSource::CENGEL;
        return p;
    };

    return DataCore::interpolIG(toIG(lo_row), toIG(hi_row), T_K,
                                DataSource::CENGEL);
}

/* ---------------------------------------------------------------------------
 * idealGasAt — COOLPROP
 * Tabla: grid_superheated (fase gas a baja presión ≈ gas ideal)
 * Unidades en BD: J/kg → convertir a kJ/kg (* 0.001)
 * Usamos P≈0 (comportamiento de gas ideal) o la grilla más baja disponible
 * ---------------------------------------------------------------------------*/
static IdealGasProps idealGasCoolProp(void* db_handle,
                                      const std::string& gas_name,
                                      double T_K)
{
    /* Resolver fluid_id */
    std::string fid_sql = "SELECT fluid_id FROM fluids WHERE name='"
                          + gas_name + "' LIMIT 1";
    std::vector<double> fid_row;
    if (!queryRow(db_handle, fid_sql, fid_row))
        throw std::invalid_argument(
            "DataCore::idealGasAt: fluido '" + gas_name +
            "' no encontrado en coolprop.db");
    int fluid_id = static_cast<int>(fid_row[0]);

    /* Buscar a la presión más baja disponible (comportamiento gas ideal) */
    auto buildQ = [&](const char* op, const char* ord) {
        std::ostringstream q;
        q << "SELECT T_K, h_Jkg, u_Jkg, s_JkgK, cp_JkgK, cv_JkgK"
          << " FROM grid_superheated"
          << " WHERE fluid_id=" << fluid_id
          << " AND h_Jkg IS NOT NULL"
          << " AND T_K " << op << " " << T_K
          << " ORDER BY T_K " << ord << ", P_Pa ASC LIMIT 1";
        return q.str();
    };

    std::vector<double> lo_row, hi_row;
    bool lo_ok = queryRow(db_handle, buildQ("<=","DESC"), lo_row);
    bool hi_ok = queryRow(db_handle, buildQ(">=","ASC"),  hi_row);

    if (!lo_ok && !hi_ok)
        throw std::out_of_range(
            "DataCore::idealGasAt: sin datos para '" + gas_name +
            "' cerca de T=" + std::to_string(T_K) + " K [COOLPROP]");

    if (!lo_ok) lo_row = hi_row;
    if (!hi_ok) hi_row = lo_row;

    /* Convertir J/kg → kJ/kg */
    auto toIG = [](const std::vector<double>& r) {
        IdealGasProps p;
        p.T_K = r[0];
        p.h   = r[1] * 0.001;
        p.u   = r[2] * 0.001;
        p.s0  = r[3] * 0.001;
        p.Cp  = (r[4] != 0.0) ? r[4] * 0.001 : 0.0;
        p.Cv  = (r[5] != 0.0) ? r[5] * 0.001 : 0.0;
        p.source = DataSource::COOLPROP;
        return p;
    };

    return DataCore::interpolIG(toIG(lo_row), toIG(hi_row), T_K,
                                DataSource::COOLPROP);
}

/* ---------------------------------------------------------------------------
 * idealGasAt — dispatcher
 * ---------------------------------------------------------------------------*/
IdealGasProps DataCore::idealGasAt(
    const std::string& gas_name, double T_K, DataSource source)
{
    assertOpen(source);
    if (source == DataSource::CENGEL)
        return idealGasCengel(m_cengel_db, gas_name, T_K);
    else
        return idealGasCoolProp(m_coolprop_db, gas_name, T_K);
}

/* ---------------------------------------------------------------------------
 * idealGasDeltaEntropy  — Cengel Ec. 7-39
 * Δs = [s°(T2) - s°(T1)] - R·ln(P2/P1)
 * ---------------------------------------------------------------------------*/
double DataCore::idealGasDeltaEntropy(
    const std::string& gas_name,
    double T1_K, double P1_Pa,
    double T2_K, double P2_Pa,
    double R, DataSource source)
{
    static constexpr double P_REF = 101325.0; /* Pa */
    IdealGasProps st1 = idealGasAt(gas_name, T1_K, source);
    IdealGasProps st2 = idealGasAt(gas_name, T2_K, source);
    double s1 = st1.s0 - R * std::log(P1_Pa / P_REF);
    double s2 = st2.s0 - R * std::log(P2_Pa / P_REF);
    return s2 - s1;
}

/* ---------------------------------------------------------------------------
 * idealGasDeltaEnthalpy
 * ---------------------------------------------------------------------------*/
double DataCore::idealGasDeltaEnthalpy(
    const std::string& gas_name,
    double T1_K, double T2_K, DataSource source)
{
    return idealGasAt(gas_name, T2_K, source).h
         - idealGasAt(gas_name, T1_K, source).h;
}

/* ---------------------------------------------------------------------------
 * satWaterAtT
 *
 * CENGEL: busca en cengel_sat_water_T si existe, si no en grid_saturation
 * COOLPROP: usa grid_saturation (Water, fluid_id determinado por nombre)
 * ---------------------------------------------------------------------------*/
SatWaterProps DataCore::satWaterAtT(double T_C, DataSource source) {
    assertOpen(source);

    double T_K = T_C + 273.15;

    if (source == DataSource::CENGEL) {
        /* Intentar tabla dedicada primero */
        auto buildQ = [&](const char* op, const char* ord) {
            std::ostringstream q;
            q << "SELECT T_C,P_kPa,vf,vg,hf,hfg,hg,sf,sfg,sg"
              << " FROM cengel_sat_water_T"
              << " WHERE T_C " << op << " " << T_C
              << " ORDER BY T_C " << ord << " LIMIT 1";
            return q.str();
        };
        std::vector<double> lo_r, hi_r;
        bool lo_ok = queryRow(m_cengel_db, buildQ("<=","DESC"), lo_r);
        bool hi_ok = queryRow(m_cengel_db, buildQ(">=","ASC"),  hi_r);

        if (lo_ok || hi_ok) {
            if (!lo_ok) lo_r=hi_r;
            if (!hi_ok) hi_r=lo_r;
            auto toS = [](const std::vector<double>& r, DataSource s) {
                SatWaterProps p;
                p.T_C=r[0];p.P_kPa=r[1];p.vf=r[2];p.vg=r[3];
                p.hf=r[4];p.hfg=r[5];p.hg=r[6];
                p.sf=r[7];p.sfg=r[8];p.sg=r[9];p.source=s;
                return p;
            };
            SatWaterProps lo=toS(lo_r,source), hi=toS(hi_r,source);
            if (hi.T_C==lo.T_C) return lo;
            double t=(T_C-lo.T_C)/(hi.T_C-lo.T_C);
            return interpolSat(lo,hi,t,T_C,lo.P_kPa+t*(hi.P_kPa-lo.P_kPa),source);
        }
        /* Caer en coolprop.db si no hay tabla dedicada */
        if (!m_coolprop_db)
            throw std::out_of_range(
                "DataCore::satWaterAtT: no hay tabla sat_water_T en CENGEL "
                "y coolprop.db no está abierto.");
    }

    /* COOLPROP — grid_saturation (Water = fluid_id 1) */
    void* db = m_coolprop_db;
    auto buildQ = [&](const char* op, const char* ord) {
        std::ostringstream q;
        q << "SELECT T_K, P_sat_Pa,"
          << " vf_m3kg, vg_m3kg,"
          << " hf_Jkg, hfg_Jkg, hg_Jkg,"
          << " sf_JkgK, sfg_JkgK, sg_JkgK"
          << " FROM grid_saturation"
          << " WHERE fluid_id=1"
          << " AND T_K " << op << " " << T_K
          << " ORDER BY T_K " << ord << " LIMIT 1";
        return q.str();
    };
    std::vector<double> lo_r, hi_r;
    bool lo_ok = queryRow(db, buildQ("<=","DESC"), lo_r);
    bool hi_ok = queryRow(db, buildQ(">=","ASC"),  hi_r);

    if (!lo_ok && !hi_ok)
        throw std::out_of_range(
            "DataCore::satWaterAtT: T=" + std::to_string(T_C) +
            " °C fuera del rango disponible");

    if (!lo_ok) lo_r=hi_r;
    if (!hi_ok) hi_r=lo_r;

    /* J/kg → kJ/kg, Pa → kPa */
    auto toS = [&](const std::vector<double>& r) {
        SatWaterProps p;
        p.T_C   = r[0] - 273.15;
        p.P_kPa = r[1] / 1000.0;
        p.vf = r[2]; p.vg = r[3];
        p.hf  = r[4]*0.001; p.hfg = r[5]*0.001; p.hg  = r[6]*0.001;
        p.sf  = r[7]*0.001; p.sfg = r[8]*0.001; p.sg  = r[9]*0.001;
        p.source = source;
        return p;
    };

    SatWaterProps lo=toS(lo_r), hi=toS(hi_r);
    if (std::fabs(hi.T_C-lo.T_C)<1e-9) return lo;
    double t=(T_C-lo.T_C)/(hi.T_C-lo.T_C);
    return interpolSat(lo,hi,t,T_C,lo.P_kPa+t*(hi.P_kPa-lo.P_kPa),source);
}

/* ---------------------------------------------------------------------------
 * satWaterAtP
 * ---------------------------------------------------------------------------*/
SatWaterProps DataCore::satWaterAtP(double P_kPa, DataSource source) {
    assertOpen(source);
    double P_Pa = P_kPa * 1000.0;

    if (source == DataSource::CENGEL) {
        auto buildQ = [&](const char* op, const char* ord) {
            std::ostringstream q;
            q << "SELECT T_C,P_kPa,vf,vg,hf,hfg,hg,sf,sfg,sg"
              << " FROM cengel_sat_water_P"
              << " WHERE P_kPa " << op << " " << P_kPa
              << " ORDER BY P_kPa " << ord << " LIMIT 1";
            return q.str();
        };
        std::vector<double> lo_r, hi_r;
        bool lo_ok = queryRow(m_cengel_db, buildQ("<=","DESC"), lo_r);
        bool hi_ok = queryRow(m_cengel_db, buildQ(">=","ASC"),  hi_r);
        if (lo_ok || hi_ok) {
            if (!lo_ok) lo_r=hi_r;
            if (!hi_ok) hi_r=lo_r;
            auto toS=[&](const std::vector<double>& r){
                SatWaterProps p;
                p.T_C=r[0];p.P_kPa=r[1];p.vf=r[2];p.vg=r[3];
                p.hf=r[4];p.hfg=r[5];p.hg=r[6];
                p.sf=r[7];p.sfg=r[8];p.sg=r[9];p.source=source;
                return p;
            };
            SatWaterProps lo=toS(lo_r),hi=toS(hi_r);
            if (std::fabs(hi.P_kPa-lo.P_kPa)<1e-9) return lo;
            double t=(P_kPa-lo.P_kPa)/(hi.P_kPa-lo.P_kPa);
            return interpolSat(lo,hi,t,lo.T_C+t*(hi.T_C-lo.T_C),P_kPa,source);
        }
        if (!m_coolprop_db)
            throw std::out_of_range(
                "DataCore::satWaterAtP: sin tabla cengel_sat_water_P "
                "y coolprop.db no abierto.");
    }

    /* COOLPROP */
    void* db = m_coolprop_db;
    auto buildQ = [&](const char* op, const char* ord) {
        std::ostringstream q;
        q << "SELECT T_K, P_sat_Pa,"
          << " vf_m3kg, vg_m3kg,"
          << " hf_Jkg, hfg_Jkg, hg_Jkg,"
          << " sf_JkgK, sfg_JkgK, sg_JkgK"
          << " FROM grid_saturation"
          << " WHERE fluid_id=1"
          << " AND P_sat_Pa " << op << " " << P_Pa
          << " ORDER BY P_sat_Pa " << ord << " LIMIT 1";
        return q.str();
    };
    std::vector<double> lo_r, hi_r;
    bool lo_ok = queryRow(db, buildQ("<=","DESC"), lo_r);
    bool hi_ok = queryRow(db, buildQ(">=","ASC"),  hi_r);

    if (!lo_ok && !hi_ok)
        throw std::out_of_range(
            "DataCore::satWaterAtP: P=" + std::to_string(P_kPa) +
            " kPa fuera del rango");

    if (!lo_ok) lo_r=hi_r;
    if (!hi_ok) hi_r=lo_r;

    auto toS=[&](const std::vector<double>& r){
        SatWaterProps p;
        p.T_C=r[0]-273.15; p.P_kPa=r[1]/1000.0;
        p.vf=r[2]; p.vg=r[3];
        p.hf=r[4]*0.001; p.hfg=r[5]*0.001; p.hg=r[6]*0.001;
        p.sf=r[7]*0.001; p.sfg=r[8]*0.001; p.sg=r[9]*0.001;
        p.source=source;
        return p;
    };
    SatWaterProps lo=toS(lo_r),hi=toS(hi_r);
    if (std::fabs(hi.P_kPa-lo.P_kPa)<1e-9) return lo;
    double t=(P_kPa-lo.P_kPa)/(hi.P_kPa-lo.P_kPa);
    return interpolSat(lo,hi,t,lo.T_C+t*(hi.T_C-lo.T_C),P_kPa,source);
}

/* ---------------------------------------------------------------------------
 * superheatedAt — interpolación bilineal (T, P)
 * COOLPROP: grid_superheated, Water fluid_id=1, unidades J/kg
 * ---------------------------------------------------------------------------*/
SuperheatedProps DataCore::superheatedAt(
    double T_C, double P_kPa, DataSource source)
{
    assertOpen(source);
    double T_K = T_C + 273.15;
    double P_Pa = P_kPa * 1000.0;

    void* db = (source == DataSource::CENGEL) ? m_cengel_db : m_coolprop_db;

    /* Para CENGEL intentar tabla dedicada */
    if (source == DataSource::CENGEL) {
        auto q4 = [&](const char* Top, const char* Pop,
                      const char* Tord, const char* Pord,
                      std::vector<double>& row) -> bool {
            std::ostringstream q;
            q << "SELECT T_C,P_kPa,v,u,h,s"
              << " FROM cengel_superheated"
              << " WHERE T_C " << Top << " " << T_C
              << " AND P_kPa " << Pop << " " << P_kPa
              << " ORDER BY T_C " << Tord << ", P_kPa " << Pord
              << " LIMIT 1";
            return queryRow(db, q.str(), row);
        };
        std::vector<double> r11,r12,r21,r22;
        bool f11=q4("<=","<=","DESC","DESC",r11);
        bool f12=q4("<=",">=","DESC","ASC", r12);
        bool f21=q4(">=","<=","ASC", "DESC",r21);
        bool f22=q4(">=",">=","ASC", "ASC", r22);

        if (f11||f12||f21||f22) {
            if(!f11) r11=f21?r21:(f12?r12:r22);
            if(!f12) r12=f22?r22:(f11?r11:r21);
            if(!f21) r21=f11?r11:(f22?r22:r12);
            if(!f22) r22=f12?r12:(f21?r21:r11);

            auto toSup=[&](const std::vector<double>& r){
                SuperheatedProps p;
                p.T_C=r[0];p.P_kPa=r[1];p.v=r[2];
                p.u=r[3];p.h=r[4];p.s=r[5];p.source=source;
                return p;
            };
            SuperheatedProps p11=toSup(r11),p12=toSup(r12);
            SuperheatedProps p21=toSup(r21),p22=toSup(r22);
            double dT=p21.T_C-p11.T_C, dP=p12.P_kPa-p11.P_kPa;
            double tT=(dT>1e-9)?(T_C-p11.T_C)/dT:0.0;
            double tP=(dP>1e-9)?(P_kPa-p11.P_kPa)/dP:0.0;
            auto bi=[&](double a,double b,double c,double d){
                return a*(1-tT)*(1-tP)+c*tT*(1-tP)+b*(1-tT)*tP+d*tT*tP;};
            SuperheatedProps res;
            res.T_C=T_C;res.P_kPa=P_kPa;res.source=source;
            res.v=bi(p11.v,p12.v,p21.v,p22.v);
            res.u=bi(p11.u,p12.u,p21.u,p22.u);
            res.h=bi(p11.h,p12.h,p21.h,p22.h);
            res.s=bi(p11.s,p12.s,p21.s,p22.s);
            return res;
        }
        /* Si no hay tabla dedicada, usar coolprop */
        if (!m_coolprop_db)
            throw std::out_of_range("DataCore::superheatedAt: sin tabla CENGEL y coolprop.db no abierto.");
        db = m_coolprop_db;
    }

    /* COOLPROP — grid_superheated Water (fluid_id=1) */
    auto q4 = [&](const char* Top, const char* Pop,
                  const char* Tord, const char* Pord,
                  std::vector<double>& row) -> bool {
        std::ostringstream q;
        q << "SELECT T_K, P_Pa, h_Jkg, u_Jkg, s_JkgK, v_m3kg"
          << " FROM grid_superheated"
          << " WHERE fluid_id=1 AND h_Jkg IS NOT NULL"
          << " AND T_K "  << Top << " " << T_K
          << " AND P_Pa " << Pop << " " << P_Pa
          << " ORDER BY T_K " << Tord << ", P_Pa " << Pord
          << " LIMIT 1";
        return queryRow(db, q.str(), row);
    };

    std::vector<double> r11,r12,r21,r22;
    bool f11=q4("<=","<=","DESC","DESC",r11);
    bool f12=q4("<=",">=","DESC","ASC", r12);
    bool f21=q4(">=","<=","ASC", "DESC",r21);
    bool f22=q4(">=",">=","ASC", "ASC", r22);

    if (!f11&&!f12&&!f21&&!f22)
        throw std::out_of_range(
            "DataCore::superheatedAt: T=" + std::to_string(T_C) +
            "°C P=" + std::to_string(P_kPa) + " kPa fuera de rango [COOLPROP]");

    if(!f11) r11=f21?r21:(f12?r12:r22);
    if(!f12) r12=f22?r22:(f11?r11:r21);
    if(!f21) r21=f11?r11:(f22?r22:r12);
    if(!f22) r22=f12?r12:(f21?r21:r11);

    /* J/kg → kJ/kg */
    auto toSup=[&](const std::vector<double>& r){
        SuperheatedProps p;
        p.T_C=r[0]-273.15; p.P_kPa=r[1]/1000.0;
        p.h=r[2]*0.001; p.u=r[3]*0.001;
        p.s=r[4]*0.001; p.v=r[5];
        p.source=source;
        return p;
    };
    SuperheatedProps p11=toSup(r11),p12=toSup(r12);
    SuperheatedProps p21=toSup(r21),p22=toSup(r22);

    double dT=p21.T_C-p11.T_C, dP=p12.P_kPa-p11.P_kPa;
    double tT=(dT>1e-9)?(T_C-p11.T_C)/dT:0.0;
    double tP=(dP>1e-9)?(P_kPa-p11.P_kPa)/dP:0.0;
    auto bi=[&](double a,double b,double c,double d){
        return a*(1-tT)*(1-tP)+c*tT*(1-tP)+b*(1-tT)*tP+d*tT*tP;};

    SuperheatedProps res;
    res.T_C=T_C; res.P_kPa=P_kPa; res.source=source;
    res.v=bi(p11.v,p12.v,p21.v,p22.v);
    res.u=bi(p11.u,p12.u,p21.u,p22.u);
    res.h=bi(p11.h,p12.h,p21.h,p22.h);
    res.s=bi(p11.s,p12.s,p21.s,p22.s);
    return res;
}