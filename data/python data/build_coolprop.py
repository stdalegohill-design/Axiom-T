"""
build_coolprop_db.py
--------------------
Axiom-T — Generador de base de datos CoolProp.

Genera: C:\\Users\\alego\\projects\\Axiom-T\\data\\coolprop.db

Tablas:
    fluids              Metadatos de cada fluido (Tc, Pc, M, T_min, T_max...)
    grid_saturation     Curva de saturación completa (hf, hg, hfg, muf, mug...)
    grid_twophase       Zona bifásica por (T, x)
    grid_subcooled      Líquido comprimido (T, P)
    grid_superheated    Vapor sobrecalentado (T, P)
    grid_supercritical  Zona supercrítica T>Tc, P>Pc
    cache_query         Caché on-demand para consultas arbitrarias en runtime

Uso:
    python build_coolprop_db.py                        # genera todo
    python build_coolprop_db.py --fluids Water R134a   # solo esos fluidos
    python build_coolprop_db.py --rebuild              # borra grids y regenera
    python build_coolprop_db.py --skip-grid            # solo esquema + metadatos

NOTA — Clasificación de fases:
    Este script NO usa PropsSI("Phase") — ese enum es inestable entre versiones
    de CoolProp. En su lugar se usa lógica termodinámica directa:
        Sobrecalentado : T > T_sat(P)  con P < Pc
        Subcooled      : T < T_sat(P)  con P < Pc
        Supercrítico   : T > Tc  Y  P > Pc
        Bifásico       : T_triple < T < Tc, variable Q

Requisito:
    pip install coolprop
"""

import sqlite3
import sys
import os
import argparse
import time
from datetime import datetime

# ============================================================================
# CONFIGURACIÓN
# ============================================================================
DEFAULT_DB = r"C:\Users\alego\projects\Axiom-T\data\coolprop.db"

PREBUILT_FLUIDS = {
    "Water":         "Water",
    "R134a":         "R134a",
    "R410A":         "R410A",
    "R22":           "R22",
    "R32":           "R32",
    "R1234yf":       "R1234yf",
    "Methane":       "Methane",
    "Propane":       "Propane",
    "CarbonDioxide": "CarbonDioxide",
    "Ammonia":       "Ammonia",
    "Hydrogen":      "Hydrogen",
    "Nitrogen":      "Nitrogen",
    "Oxygen":        "Oxygen",
    "Argon":         "Argon",
    "Helium":        "Helium",
    "Air":           "Air",
}

# Resolución de grids
N_SAT   = 300           # puntos en curva de saturación
N_X     = 21            # calidades: 0.00, 0.05, ..., 1.00
N_T_SH  = 60            # T para sobrecalentado
N_P_SH  = 40            # P para sobrecalentado  → ~2400 puntos por fluido
N_T_SUB = 40            # T para subcooled
N_P_SUB = 30            # P para subcooled        → ~1200 puntos por fluido
N_T_SC  = 40            # T para supercrítico
N_P_SC  = 30            # P para supercrítico     → 1200 puntos por fluido

# ============================================================================
# ESQUEMA SQL
# ============================================================================
SCHEMA = """
CREATE TABLE IF NOT EXISTS fluids (
    fluid_id      INTEGER PRIMARY KEY AUTOINCREMENT,
    name          TEXT NOT NULL UNIQUE,
    label         TEXT NOT NULL,
    M_kgkmol      REAL, Tc_K     REAL, Pc_Pa    REAL,
    rhoc_kgm3     REAL, Tt_K     REAL, Pt_Pa    REAL,
    T_min_K       REAL, T_max_K  REAL, P_max_Pa REAL,
    has_transport INTEGER DEFAULT 0,
    grid_built    INTEGER DEFAULT 0,
    built_at      TEXT,
    cp_version    TEXT
);

CREATE TABLE IF NOT EXISTS grid_saturation (
    fluid_id   INTEGER NOT NULL REFERENCES fluids(fluid_id),
    T_K        REAL NOT NULL,
    P_sat_Pa   REAL,
    hf_Jkg     REAL, uf_Jkg    REAL, sf_JkgK   REAL,
    vf_m3kg    REAL, rhof_kgm3 REAL, cpf_JkgK  REAL,
    muf_Pas    REAL, kf_WmK    REAL, Prf        REAL,
    hg_Jkg     REAL, ug_Jkg    REAL, sg_JkgK    REAL,
    vg_m3kg    REAL, rhog_kgm3 REAL, cpg_JkgK   REAL,
    mug_Pas    REAL, kg_WmK    REAL, Prg         REAL,
    hfg_Jkg    REAL, sfg_JkgK  REAL,
    PRIMARY KEY (fluid_id, T_K)
);

CREATE TABLE IF NOT EXISTS grid_twophase (
    fluid_id   INTEGER NOT NULL REFERENCES fluids(fluid_id),
    T_K        REAL NOT NULL,
    quality    REAL NOT NULL,
    P_sat_Pa   REAL,
    h_Jkg      REAL, u_Jkg    REAL, s_JkgK   REAL,
    v_m3kg     REAL, rho_kgm3 REAL,
    PRIMARY KEY (fluid_id, T_K, quality)
);

CREATE TABLE IF NOT EXISTS grid_subcooled (
    fluid_id   INTEGER NOT NULL REFERENCES fluids(fluid_id),
    T_K        REAL NOT NULL,
    P_Pa       REAL NOT NULL,
    h_Jkg      REAL, u_Jkg    REAL, s_JkgK   REAL,
    v_m3kg     REAL, rho_kgm3 REAL, cp_JkgK  REAL,
    cv_JkgK    REAL, mu_Pas   REAL, k_WmK    REAL,
    Pr         REAL, w_ms     REAL, Z         REAL,
    PRIMARY KEY (fluid_id, T_K, P_Pa)
);

CREATE TABLE IF NOT EXISTS grid_superheated (
    fluid_id   INTEGER NOT NULL REFERENCES fluids(fluid_id),
    T_K        REAL NOT NULL,
    P_Pa       REAL NOT NULL,
    h_Jkg      REAL, u_Jkg    REAL, s_JkgK   REAL,
    v_m3kg     REAL, rho_kgm3 REAL, cp_JkgK  REAL,
    cv_JkgK    REAL, mu_Pas   REAL, k_WmK    REAL,
    Pr         REAL, w_ms     REAL, Z         REAL,
    PRIMARY KEY (fluid_id, T_K, P_Pa)
);

CREATE TABLE IF NOT EXISTS grid_supercritical (
    fluid_id   INTEGER NOT NULL REFERENCES fluids(fluid_id),
    T_K        REAL NOT NULL,
    P_Pa       REAL NOT NULL,
    h_Jkg      REAL, u_Jkg    REAL, s_JkgK   REAL,
    v_m3kg     REAL, rho_kgm3 REAL, cp_JkgK  REAL,
    cv_JkgK    REAL, mu_Pas   REAL, k_WmK    REAL,
    Pr         REAL, w_ms     REAL, Z         REAL,
    PRIMARY KEY (fluid_id, T_K, P_Pa)
);

CREATE TABLE IF NOT EXISTS cache_query (
    cache_id      INTEGER PRIMARY KEY AUTOINCREMENT,
    fluid         TEXT NOT NULL,
    input1_name   TEXT NOT NULL,
    input1_val    REAL NOT NULL,
    input2_name   TEXT NOT NULL,
    input2_val    REAL NOT NULL,
    query_hash    TEXT NOT NULL UNIQUE,
    T_K           REAL, P_Pa     REAL, h_Jkg    REAL,
    u_Jkg         REAL, s_JkgK   REAL, v_m3kg   REAL,
    rho_kgm3      REAL, cp_JkgK  REAL, cv_JkgK  REAL,
    mu_Pas        REAL, k_WmK    REAL, Pr        REAL,
    w_ms          REAL, Z        REAL, quality   REAL,
    phase_region  TEXT,
    created_at    TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_sat ON grid_saturation    (fluid_id, T_K);
CREATE INDEX IF NOT EXISTS idx_sub ON grid_subcooled     (fluid_id, T_K, P_Pa);
CREATE INDEX IF NOT EXISTS idx_tp  ON grid_twophase      (fluid_id, T_K, quality);
CREATE INDEX IF NOT EXISTS idx_sup ON grid_superheated   (fluid_id, T_K, P_Pa);
CREATE INDEX IF NOT EXISTS idx_sc  ON grid_supercritical (fluid_id, T_K, P_Pa);
CREATE INDEX IF NOT EXISTS idx_ch  ON cache_query        (query_hash);
CREATE INDEX IF NOT EXISTS idx_cf  ON cache_query        (fluid, input1_name, input2_name);
"""

GRID_TABLES = [
    "grid_saturation", "grid_twophase", "grid_subcooled",
    "grid_superheated", "grid_supercritical",
]

# ============================================================================
# HELPERS MATEMÁTICOS
# ============================================================================
def linspace(a, b, n):
    if n < 2: return [a]
    return [a + (b - a) * i / (n - 1) for i in range(n)]


def logspace(a, b, n):
    """Escala logarítmica — ideal para presiones que varían órdenes de magnitud."""
    if n < 2: return [a]
    if a <= 0 or b <= 0: return linspace(a, b, n)
    return [a * (b / a) ** (i / (n - 1)) for i in range(n)]


# ============================================================================
# HELPERS COOLPROP
# ============================================================================
def safe(prop, in1, v1, in2, v2, fluid, CP):
    """PropsSI seguro — None en error o NaN."""
    try:
        val = CP.PropsSI(prop, in1, v1, in2, v2, fluid)
        return float(val) if val == val else None
    except Exception:
        return None


def safe1(prop, fluid, CP):
    """Propiedad escalar del fluido (sin estado)."""
    try:
        val = CP.PropsSI(prop, fluid)
        return float(val) if val == val else None
    except Exception:
        return None


def Tsat_at_P(P, fluid, Pc, CP):
    """
    Temperatura de saturación a presión P.
    Devuelve None si P >= Pc (no existe curva de saturación) o si falla.
    """
    if P >= Pc:
        return None
    return safe("T", "P", P, "Q", 0, fluid, CP)


def props_TP(fluid, T, P, CP):
    """
    Calcula las 12 propiedades estándar en (T, P).
    Devuelve tupla de 12: h, u, s, v, rho, cp, cv, mu, k, Pr, w, Z
    """
    rho = safe("D",       "T", T, "P", P, fluid, CP)
    return (
        safe("H",       "T", T, "P", P, fluid, CP),   # h  [J/kg]
        safe("U",       "T", T, "P", P, fluid, CP),   # u  [J/kg]
        safe("S",       "T", T, "P", P, fluid, CP),   # s  [J/kg·K]
        1.0/rho if (rho and rho > 0) else None,        # v  [m³/kg]
        rho,                                            # ρ  [kg/m³]
        safe("CP",      "T", T, "P", P, fluid, CP),   # cp [J/kg·K]
        safe("CV",      "T", T, "P", P, fluid, CP),   # cv [J/kg·K]
        safe("V",       "T", T, "P", P, fluid, CP),   # μ  [Pa·s]
        safe("L",       "T", T, "P", P, fluid, CP),   # k  [W/m·K]
        safe("Prandtl", "T", T, "P", P, fluid, CP),   # Pr [-]
        safe("A",       "T", T, "P", P, fluid, CP),   # w  [m/s]
        safe("Z",       "T", T, "P", P, fluid, CP),   # Z  [-]
    )


def get_meta(fluid, CP):
    M = safe1("M", fluid, CP)
    has_transport = 0
    try:
        v = CP.PropsSI("V", "T", 300, "P", 101325, fluid)
        if v == v and v > 0:
            has_transport = 1
    except Exception:
        pass
    return {
        "M_kgkmol":      M * 1000 if M else None,
        "Tc_K":          safe1("Tcrit",   fluid, CP),
        "Pc_Pa":         safe1("Pcrit",   fluid, CP),
        "rhoc_kgm3":     safe1("rhocrit", fluid, CP),
        "Tt_K":          safe1("Ttriple", fluid, CP),
        "Pt_Pa":         safe1("ptriple", fluid, CP),
        "T_min_K":       safe1("Tmin",    fluid, CP),
        "T_max_K":       safe1("Tmax",    fluid, CP),
        "P_max_Pa":      safe1("pmax",    fluid, CP),
        "has_transport": has_transport,
    }


# ============================================================================
# CONSTRUCTORES DE GRIDS
# ============================================================================
def grid_saturation(fid, fluid, meta, CP, conn):
    Tc = meta["Tc_K"]; Tt = meta["Tt_K"] or meta["T_min_K"] or 50.0
    T_lo = max(Tt * 1.001, (meta["T_min_K"] or Tt) + 0.5)
    T_hi = Tc * 0.9999
    rows = []

    for T in linspace(T_lo, T_hi, N_SAT):
        q  = lambda prop, x: safe(prop, "T", T, "Q", x, fluid, CP)
        P  = q("P", 0)
        if P is None: continue

        rhof = q("D", 0); rhog = q("D", 1)
        hf   = q("H", 0); hg   = q("H", 1)
        uf   = q("U", 0); ug   = q("U", 1)
        sf   = q("S", 0); sg   = q("S", 1)

        rows.append((
            fid, T, P,
            hf, uf, sf,
            1/rhof if rhof else None, rhof, q("CP", 0),
            q("V", 0), q("L", 0), q("Prandtl", 0),
            hg, ug, sg,
            1/rhog if rhog else None, rhog, q("CP", 1),
            q("V", 1), q("L", 1), q("Prandtl", 1),
            (hg - hf) if (hg is not None and hf is not None) else None,
            (sg - sf) if (sg is not None and sf is not None) else None,
        ))

    conn.executemany(
        "INSERT OR REPLACE INTO grid_saturation VALUES "
        "(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
        rows)
    return len(rows)


def grid_twophase(fid, fluid, meta, CP, conn):
    Tc = meta["Tc_K"]; Tt = meta["Tt_K"] or meta["T_min_K"] or 50.0
    T_lo = max(Tt * 1.001, (meta["T_min_K"] or Tt) + 0.5)
    T_hi = Tc * 0.9999
    x_list = [i / (N_X - 1) for i in range(N_X)]
    rows = []

    for T in linspace(T_lo, T_hi, N_SAT):
        P = safe("P", "T", T, "Q", 0, fluid, CP)
        if P is None: continue
        for x in x_list:
            rho = safe("D", "T", T, "Q", x, fluid, CP)
            rows.append((
                fid, T, x, P,
                safe("H", "T", T, "Q", x, fluid, CP),
                safe("U", "T", T, "Q", x, fluid, CP),
                safe("S", "T", T, "Q", x, fluid, CP),
                1/rho if rho else None, rho,
            ))

    conn.executemany(
        "INSERT OR REPLACE INTO grid_twophase VALUES (?,?,?,?,?,?,?,?,?)",
        rows)
    return len(rows)


def grid_subcooled(fid, fluid, meta, CP, conn):
    """
    Líquido comprimido: T < T_sat(P), P < Pc.
    Clasificación termodinámica directa — sin Phase enum.
    """
    Tc = meta["Tc_K"]; Pc = meta["Pc_Pa"]
    Tt = meta["Tt_K"] or meta["T_min_K"] or 50.0
    Pt = meta["Pt_Pa"] or 100.0

    T_lo = max(Tt * 1.001, (meta["T_min_K"] or Tt) + 0.5)
    T_hi = Tc * 0.999
    P_lo = max(Pt * 1.01, 1000.0)   # mínimo 1 kPa
    P_hi = Pc * 0.999

    if P_lo >= P_hi: return 0

    rows = []
    for T in linspace(T_lo, T_hi, N_T_SUB):
        for P in logspace(P_lo, P_hi, N_P_SUB):
            Tsat = Tsat_at_P(P, fluid, Pc, CP)
            # Subcooled: temperatura MENOR que la de saturación a esa presión
            if Tsat is None or T >= Tsat:
                continue
            rows.append((fid, T, P) + props_TP(fluid, T, P, CP))

    conn.executemany(
        "INSERT OR REPLACE INTO grid_subcooled VALUES "
        "(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
        rows)
    return len(rows)


def grid_superheated(fid, fluid, meta, CP, conn):
    """
    Vapor sobrecalentado: T > T_sat(P) con P < Pc.
    Incluye también T >= Tc con P < Pc (gas a presión subcrítica).
    Clasificación termodinámica directa — sin Phase enum.
    """
    Tc = meta["Tc_K"]; Pc = meta["Pc_Pa"]
    T_lo = max(meta["Tt_K"] or 50.0, meta["T_min_K"] or 50.0)
    T_hi = min(meta["T_max_K"] or Tc * 5, Tc * 5)
    P_lo = 100.0        # 100 Pa mínimo
    P_hi = Pc * 0.999   # justo antes del punto crítico en P

    rows = []
    for T in linspace(T_lo, T_hi, N_T_SH):
        for P in logspace(P_lo, P_hi, N_P_SH):
            if T < Tc:
                # Zona subcrítica en T: sobrecalentado solo si T > T_sat(P)
                Tsat = Tsat_at_P(P, fluid, Pc, CP)
                if Tsat is None or T <= Tsat:
                    continue
            # T >= Tc con P < Pc: siempre es gas → incluir sin filtro adicional
            rows.append((fid, T, P) + props_TP(fluid, T, P, CP))

    conn.executemany(
        "INSERT OR REPLACE INTO grid_superheated VALUES "
        "(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
        rows)
    return len(rows)


def grid_supercritical(fid, fluid, meta, CP, conn):
    """
    Zona supercrítica: T > Tc Y P > Pc.
    No requiere clasificación adicional.
    """
    Tc = meta["Tc_K"]; Pc = meta["Pc_Pa"]
    T_lo = Tc * 1.001
    T_hi = min(meta["T_max_K"] or Tc * 5, Tc * 5)
    P_lo = Pc * 1.001
    P_hi = min(meta["P_max_Pa"] or Pc * 20, Pc * 20)

    rows = []
    for T in linspace(T_lo, T_hi, N_T_SC):
        for P in logspace(P_lo, P_hi, N_P_SC):
            rows.append((fid, T, P) + props_TP(fluid, T, P, CP))

    conn.executemany(
        "INSERT OR REPLACE INTO grid_supercritical VALUES "
        "(?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)",
        rows)
    return len(rows)


# ============================================================================
# CACHÉ ON-DEMAND — función lista para usar desde el motor de Axiom-T
# ============================================================================
def query_or_cache(fluid, in1_name, in1_val, in2_name, in2_val, db_path):
    """
    Consulta propiedades de un fluido con dos variables de estado.
    Busca primero en cache_query; si no está, calcula con CoolProp y guarda.

    Parámetros
    ----------
    fluid       : nombre CoolProp del fluido (ej: "Water", "R134a")
    in1_name    : primera variable de entrada (ej: "T", "P", "H", "Q")
    in1_val     : valor de la primera variable (en unidades SI)
    in2_name    : segunda variable de entrada
    in2_val     : valor de la segunda variable
    db_path     : ruta al archivo coolprop.db

    Retorna
    -------
    dict con todas las propiedades calculadas, o None si falla.
    La columna 'from_cache' indica True si vino del caché, False si se calculó.

    Ejemplo
    -------
    # Propiedades del agua a 200°C y 1 MPa
    props = query_or_cache("Water", "T", 473.15, "P", 1e6, "coolprop.db")
    print(props["h_Jkg"], props["s_JkgK"])

    # Propiedades de R134a en la curva de saturación a -20°C
    props = query_or_cache("R134a", "T", 253.15, "Q", 0, "coolprop.db")
    """
    import hashlib, json, sqlite3
    try:
        import CoolProp.CoolProp as CP
    except ImportError:
        return None

    # Hash único para la consulta
    key   = json.dumps([fluid, in1_name, in1_val, in2_name, in2_val], sort_keys=True)
    qhash = hashlib.sha256(key.encode()).hexdigest()

    COLS = ["T_K","P_Pa","h_Jkg","u_Jkg","s_JkgK","v_m3kg",
            "rho_kgm3","cp_JkgK","cv_JkgK","mu_Pas","k_WmK",
            "Pr","w_ms","Z","quality","phase_region"]

    conn = sqlite3.connect(db_path)
    conn.row_factory = sqlite3.Row

    # Buscar en caché
    row = conn.execute(
        "SELECT * FROM cache_query WHERE query_hash=?", (qhash,)
    ).fetchone()

    if row:
        result = dict(row)
        result["from_cache"] = True
        conn.close()
        return result

    # Cache miss → calcular con CoolProp
    def sp(prop):
        return safe(prop, in1_name, in1_val, in2_name, in2_val, fluid, CP)

    rho = sp("D")
    T   = sp("T"); P = sp("P")

    # Determinar región de fase
    phase_region = None
    if T is not None and P is not None:
        try:
            Tc = CP.PropsSI("Tcrit", fluid)
            Pc = CP.PropsSI("Pcrit", fluid)
            if T > Tc and P > Pc:
                phase_region = "supercritical"
            elif T > Tc:
                phase_region = "gas"
            else:
                Tsat = safe("T", "P", P, "Q", 0, fluid, CP)
                if Tsat:
                    if T > Tsat + 0.01:
                        phase_region = "superheated"
                    elif T < Tsat - 0.01:
                        phase_region = "subcooled"
                    else:
                        phase_region = "saturation"
        except Exception:
            pass

    quality = sp("Q")

    vals = {
        "T_K":         T,
        "P_Pa":        P,
        "h_Jkg":       sp("H"),
        "u_Jkg":       sp("U"),
        "s_JkgK":      sp("S"),
        "v_m3kg":      1/rho if (rho and rho > 0) else None,
        "rho_kgm3":    rho,
        "cp_JkgK":     sp("CP"),
        "cv_JkgK":     sp("CV"),
        "mu_Pas":      sp("V"),
        "k_WmK":       sp("L"),
        "Pr":          sp("Prandtl"),
        "w_ms":        sp("A"),
        "Z":           sp("Z"),
        "quality":     quality if (quality is not None and 0 <= quality <= 1) else None,
        "phase_region": phase_region,
    }

    # Guardar en caché
    try:
        conn.execute("""
            INSERT OR IGNORE INTO cache_query
            (fluid, input1_name, input1_val, input2_name, input2_val, query_hash,
             T_K, P_Pa, h_Jkg, u_Jkg, s_JkgK, v_m3kg, rho_kgm3, cp_JkgK,
             cv_JkgK, mu_Pas, k_WmK, Pr, w_ms, Z, quality, phase_region, created_at)
            VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?,?)
        """, (
            fluid, in1_name, in1_val, in2_name, in2_val, qhash,
            vals["T_K"], vals["P_Pa"], vals["h_Jkg"], vals["u_Jkg"],
            vals["s_JkgK"], vals["v_m3kg"], vals["rho_kgm3"], vals["cp_JkgK"],
            vals["cv_JkgK"], vals["mu_Pas"], vals["k_WmK"], vals["Pr"],
            vals["w_ms"], vals["Z"], vals["quality"], vals["phase_region"],
            datetime.now().isoformat()
        ))
        conn.commit()
    except Exception:
        pass

    conn.close()
    vals["from_cache"] = False
    return vals


# ============================================================================
# MAIN
# ============================================================================
def main():
    parser = argparse.ArgumentParser(
        description="Axiom-T -- Genera coolprop.db con grids pre-calculados"
    )
    parser.add_argument("--db", default=DEFAULT_DB,
                        help=f"Ruta del DB (default: {DEFAULT_DB})")
    parser.add_argument("--fluids", nargs="+", default=None,
                        help="Subconjunto de fluidos (ej: Water R134a)")
    parser.add_argument("--skip-grid", action="store_true",
                        help="Solo crear esquema y metadatos, sin generar grids")
    parser.add_argument("--rebuild", action="store_true",
                        help="Borrar todos los grids existentes y regenerar desde cero")
    args = parser.parse_args()

    # ---- CoolProp ----
    try:
        import CoolProp.CoolProp as CP
        cp_version = getattr(CP, "__version__", "?")
    except ImportError:
        print("\nERROR: pip install coolprop")
        sys.exit(1)

    # ---- Fluidos ----
    fluids = ({f: PREBUILT_FLUIDS.get(f, f) for f in args.fluids}
              if args.fluids else PREBUILT_FLUIDS.copy())

    os.makedirs(os.path.dirname(os.path.abspath(args.db)), exist_ok=True)

    print("=" * 66)
    print("  Axiom-T -- CoolProp DB Builder")
    print("=" * 66)
    print(f"  DB        : {args.db}")
    print(f"  CoolProp  : v{cp_version}")
    print(f"  Fluidos   : {len(fluids)}  ({', '.join(fluids.keys())})")
    print(f"  Modo      : {'REBUILD (borra grids existentes)' if args.rebuild else 'INCREMENTAL'}")
    print(f"  Grids     : {'OMITIDOS' if args.skip_grid else 'SI'}")

    conn = sqlite3.connect(args.db)
    conn.execute("PRAGMA journal_mode=WAL")
    conn.execute("PRAGMA synchronous=NORMAL")
    conn.execute("PRAGMA cache_size=-65536")
    conn.executescript(SCHEMA)
    conn.commit()

    # ---- --rebuild: borrar grids de los fluidos seleccionados ----
    if args.rebuild and not args.skip_grid:
        print(f"\n  [REBUILD] Eliminando grids existentes para los fluidos seleccionados...")
        for cp_name in fluids:
            row = conn.execute(
                "SELECT fluid_id FROM fluids WHERE name=?", (cp_name,)
            ).fetchone()
            if row:
                fid = row[0]
                for tbl in GRID_TABLES:
                    conn.execute(f"DELETE FROM {tbl} WHERE fluid_id=?", (fid,))
                conn.execute(
                    "UPDATE fluids SET grid_built=0, built_at=NULL WHERE fluid_id=?",
                    (fid,))
                print(f"    {cp_name}: grids eliminados")
        conn.commit()

    # ---- Procesar fluidos ----
    grand_total = 0
    t_start = time.time()

    for cp_name, label in fluids.items():
        print(f"\n{'─'*66}")
        print(f"  {cp_name}")
        print(f"{'─'*66}")
        t0 = time.time()

        try:
            CP.PropsSI("Tcrit", cp_name)
        except Exception as e:
            print(f"  ERROR: CoolProp no reconoce '{cp_name}': {e}")
            continue

        meta = get_meta(cp_name, CP)
        print(f"  Tc={meta['Tc_K']:.2f}K  Pc={meta['Pc_Pa']/1e6:.3f}MPa  "
              f"M={meta['M_kgkmol']:.3f}kg/kmol  "
              f"Tmin={meta['T_min_K']:.1f}K  Tmax={meta['T_max_K']:.1f}K")

        conn.execute("""
            INSERT INTO fluids
                (name,label,M_kgkmol,Tc_K,Pc_Pa,rhoc_kgm3,
                 Tt_K,Pt_Pa,T_min_K,T_max_K,P_max_Pa,has_transport,cp_version)
            VALUES (?,?,?,?,?,?,?,?,?,?,?,?,?)
            ON CONFLICT(name) DO UPDATE SET
                M_kgkmol=excluded.M_kgkmol,   Tc_K=excluded.Tc_K,
                Pc_Pa=excluded.Pc_Pa,          rhoc_kgm3=excluded.rhoc_kgm3,
                Tt_K=excluded.Tt_K,            Pt_Pa=excluded.Pt_Pa,
                T_min_K=excluded.T_min_K,      T_max_K=excluded.T_max_K,
                P_max_Pa=excluded.P_max_Pa,    has_transport=excluded.has_transport,
                cp_version=excluded.cp_version
        """, (
            cp_name, label,
            meta["M_kgkmol"], meta["Tc_K"], meta["Pc_Pa"], meta["rhoc_kgm3"],
            meta["Tt_K"], meta["Pt_Pa"], meta["T_min_K"], meta["T_max_K"],
            meta["P_max_Pa"], meta["has_transport"], cp_version
        ))
        conn.commit()

        fid = conn.execute(
            "SELECT fluid_id FROM fluids WHERE name=?", (cp_name,)
        ).fetchone()[0]

        if args.skip_grid:
            print("  Grid omitido (--skip-grid).")
            continue

        # Verificar si ya tiene grid (modo incremental)
        if not args.rebuild:
            existing = conn.execute(
                "SELECT grid_built FROM fluids WHERE fluid_id=?", (fid,)
            ).fetchone()[0]
            if existing:
                n = conn.execute(
                    f"SELECT COUNT(*) FROM grid_superheated WHERE fluid_id=?", (fid,)
                ).fetchone()[0]
                print(f"  Ya tiene grid (superheated={n} filas). Usa --rebuild para regenerar.")
                continue

        subtotal = 0

        print("  [1/5] Saturacion      ...", end=" ", flush=True)
        n = grid_saturation(fid, cp_name, meta, CP, conn); conn.commit()
        print(f"{n:>6,} filas"); subtotal += n

        print("  [2/5] Bifasico        ...", end=" ", flush=True)
        n = grid_twophase(fid, cp_name, meta, CP, conn); conn.commit()
        print(f"{n:>6,} filas"); subtotal += n

        print("  [3/5] Subcooled       ...", end=" ", flush=True)
        n = grid_subcooled(fid, cp_name, meta, CP, conn); conn.commit()
        print(f"{n:>6,} filas"); subtotal += n

        print("  [4/5] Sobrecalentado  ...", end=" ", flush=True)
        n = grid_superheated(fid, cp_name, meta, CP, conn); conn.commit()
        print(f"{n:>6,} filas"); subtotal += n

        print("  [5/5] Supercritico    ...", end=" ", flush=True)
        n = grid_supercritical(fid, cp_name, meta, CP, conn); conn.commit()
        print(f"{n:>6,} filas"); subtotal += n

        conn.execute(
            "UPDATE fluids SET grid_built=1, built_at=? WHERE fluid_id=?",
            (datetime.now().isoformat(), fid))
        conn.commit()

        print(f"\n  Subtotal: {subtotal:,} filas  ({time.time()-t0:.1f}s)")
        grand_total += subtotal

    # ---- Resumen ----
    elapsed = time.time() - t_start
    size_mb = os.path.getsize(args.db) / (1024**2)

    print(f"\n{'='*66}")
    print(f"  RESUMEN FINAL")
    print(f"{'='*66}")
    all_tbls = GRID_TABLES + ["cache_query"]
    for tbl in all_tbls:
        n = conn.execute(f"SELECT COUNT(*) FROM {tbl}").fetchone()[0]
        print(f"  {tbl:<26}: {n:>9,} filas")

    print(f"\n  Fluidos registrados:")
    for row in conn.execute(
        "SELECT name, Tc_K, T_min_K, T_max_K, has_transport, grid_built "
        "FROM fluids ORDER BY name"
    ).fetchall():
        estado = "GRID OK" if row[5] else "sin grid"
        tr     = "T+" if row[4] else "   "
        print(f"    {row[0]:<22} {tr}  Tc={row[1]:.1f}K  "
              f"T=[{row[2]:.0f}..{row[3]:.0f}]K  [{estado}]")

    print(f"\n  Filas generadas esta ejecucion : {grand_total:,}")
    print(f"  Tiempo                         : {elapsed:.1f}s")
    print(f"  Tamanio DB                     : {size_mb:.1f} MB")
    print(f"  Ruta                           : {args.db}")
    print(f"{'='*66}")
    print(f"\n  La funcion query_or_cache() en este mismo archivo")
    print(f"  puede importarse directamente en el motor de Axiom-T.")
    conn.close()


if __name__ == "__main__":
    main()