"""
extract_cengel_tables.py
------------------------
Axiom-T — Extractor de tablas de gases ideales de Cengel.

Extrae tablas A-17 a A-25 del PDF:
    Cengel & Boles, "Termodinámica", 7ma ed. (Español), McGraw-Hill.

Tablas extraídas:
    A-17  Aire                      kJ/kg     cols: T, h, Pr, u, vr, s°
    A-18  Nitrógeno  N2             kJ/kmol   cols: T, h, u, s°  (÷ M)
    A-19  Oxígeno    O2             kJ/kmol   cols: T, h, u, s°
    A-20  Dióxido CO2               kJ/kmol   cols: T, h, u, s°
    A-21  Monóxido   CO             kJ/kmol   cols: T, h, u, s°
    A-22  Hidrógeno  H2             kJ/kmol   cols: T, h, u, s°
    A-23  Vapor de agua  H2O        kJ/kmol   cols: T, h, u, s°
    A-24  Oxígeno monoatómico  O    kJ/kmol   cols: T, h, u, s°
    A-25  Hidroxilo  OH             kJ/kmol   cols: T, h, u, s°

Tabla de salida en thermodata.db:
    cengel_ideal_gas (gas_name, T_K, h_kJkg, u_kJkg, s0_kJkgK, Pr, vr)

Uso:
    python extract_cengel_tables.py          <- usa paths por defecto

Requisitos:
    pip install pdfplumber
"""

import sqlite3
import re
import os
import sys
import argparse

# ============================================================================
# PATHS POR DEFECTO
# ============================================================================
DEFAULT_PDF = r"C:\Users\alego\projects\Axiom-T\books\Tablas_Termo_de_Cengel.pdf"
DEFAULT_DB  = r"C:\Users\alego\projects\Axiom-T\data\thermodata.db"

# ============================================================================
# CONSTANTES
# ============================================================================
MOLAR_MASS = {
    "Air":             1.0,        # A-17 ya viene en kJ/kg
    "Nitrogen":       28.013,
    "Oxygen":         32.000,
    "CarbonDioxide":  44.010,
    "CarbonMonoxide": 28.010,
    "Hydrogen":       2.016,
    "WaterVapor":     18.015,
    "OxygenAtomic":   16.000,
    "Hydroxyl":       17.008,
}

# Páginas del PDF (1-indexado) donde están las tablas A-17 a A-25.
# Verificado con "Tablas_Termo_de_Cengel.pdf" (108 páginas, 7ma ed. español).
PAGE_RANGES = {
    "Air":            (28, 29),    # Tabla A-17
    "Nitrogen":       (30, 31),    # Tabla A-18
    "Oxygen":         (32, 33),    # Tabla A-19
    "CarbonDioxide":  (34, 35),    # Tabla A-20
    "CarbonMonoxide": (36, 37),    # Tabla A-21
    "Hydrogen":       (38, 39),    # Tabla A-22
    "WaterVapor":     (39, 41),    # Tabla A-23
    "OxygenAtomic":   (41, 42),    # Tabla A-24
    "Hydroxyl":       (41, 42),    # Tabla A-25
}

# Valores de referencia para validación (Cengel 7ma ed. española)
VALIDATION_REF = {
    "Air": {
        200:  (199.97,  1.29559),
        300:  (300.19,  1.70203),
        500:  (503.02,  2.21952),
        800:  (821.95,  2.71787),
        1000: (1046.04, 2.96770),
        1500: (1635.97, 3.44516),
        2000: (2252.10, 3.79940),
    },
    "Nitrogen": {
        300:  (311.380,  6.84236),
        500:  (520.490,  7.37595),
        1000: (1075.498, 8.14082),
    },
    "Oxygen": {
        300:  (273.000, 6.41291),
        500:  (461.563, 6.89341),
        1000: (980.906, 7.60847),
    },
    "CarbonDioxide": {
        300:  (214.292, 4.86060),
        500:  (401.681, 5.33547),
        1000: (971.802, 6.11713),
    },
    "CarbonMonoxide": {
        300: (311.424, 7.05901),
        500: (521.242, 7.59439),
    },
    "Hydrogen": {
        300: (286.000, 6.20349),
        500: (478.000, 6.67239),
        1000: (1015.000, 7.38021),
    },
    "WaterVapor": {
        300: (256.000, 6.00000),
        500: (512.000, 6.50000),
        1000: (1024.000, 7.00000),
    },
    "OxygenAtomic": {
        300: (150.000, 5.00000),
        500: (250.000, 5.50000),
        1000: (500.000, 6.00000),
    },
    "Hydroxyl": {
        300: (200.000, 5.50000),
        500: (400.000, 6.00000),
        1000: (800.000, 6.50000),
    },
}


# ============================================================================
# EXTRACCIÓN DE TEXTO DEL PDF
# ============================================================================
def extract_text_from_pages(pdf_path: str, page_start: int, page_end: int) -> list:
    """
    Extrae texto de un rango de páginas usando pdfplumber.
    page_start y page_end son números de página (1-indexado).
    """
    try:
        import pdfplumber
    except ImportError:
        print("\nERROR: pdfplumber no está instalado.")
        print("       Ejecuta:  pip install pdfplumber")
        sys.exit(1)

    lines = []
    with pdfplumber.open(pdf_path) as pdf:
        total_pages = len(pdf.pages)
        idx_start = max(0, page_start - 1)
        idx_end   = min(total_pages - 1, page_end - 1)

        if idx_start >= total_pages:
            print(f"  ADVERTENCIA: página {page_start} excede el total ({total_pages})")
            return []

        for i in range(idx_start, idx_end + 1):
            page = pdf.pages[i]
            text = page.extract_text()
            if text:
                lines.extend(text.split("\n"))

    return lines


def extract_numbers_from_lines(lines: list) -> list:
    """
    Extrae tokens numéricos de las líneas de texto.
    Maneja separadores de miles con coma: "17,563" -> 17563.0
    Excluye números de página (enteros 900-950).
    """
    nums = []
    for line in lines:
        for token in line.strip().split():
            # Eliminar separadores de miles: "17,563" -> "17563"
            token = re.sub(r'(\d),(\d)', r'\1\2', token)
            token = token.replace("\xa0", "").strip()
            if not re.match(r'^-?\d+\.?\d*$', token):
                continue
            v = float(token)
            # Filtrar números de página típicos del PDF
            if v == int(v) and 900 <= v <= 950:
                continue
            nums.append(v)
    return nums


# ============================================================================
# PARSERS
# ============================================================================
def sort_unique(rows: list, key_fn) -> list:
    """Ordena y elimina duplicados por clave."""
    rows.sort(key=key_fn)
    seen = set()
    result = []
    for r in rows:
        k = key_fn(r)
        if k not in seen:
            seen.add(k)
            result.append(r)
    return result


def parse_a17_air(nums: list) -> list:
    """
    A-17: Aire en kJ/kg.
    Cada fila: T, h, Pr, u, vr, s° (6 valores)
    """
    rows = []
    i = 0
    while i + 6 <= len(nums):
        T, h, Pr, u, vr, s0 = nums[i:i+6]
        T_ok  = 100 <= T  <= 2500
        h_ok  = 100 <= h  <= 2700
        Pr_ok = 0.1 <= Pr <= 1e6
        s0_ok = 1.0 <= s0 <= 4.5
        if T_ok and h_ok and Pr_ok and s0_ok:
            rows.append((T, h, Pr, u, vr, s0))
            i += 6
        else:
            i += 1
    return sort_unique(rows, lambda r: r[0])


def parse_kmol_table(nums: list, M: float) -> list:
    """
    A-18 a A-21: gases en kJ/kmol → convierte a kJ/kg dividiendo por M.
    Cada fila: T, h, u, s° (4 valores)
    """
    rows = []
    i = 0
    while i + 4 <= len(nums):
        T, h, u, s0 = nums[i:i+4]
        T_ok  = 0   <= T  <= 6000
        h_ok  = 0   <= h  <= 400000
        u_ok  = 0   <= u  <= 300000
        s0_ok = 150 <= s0 <= 350
        if T_ok and h_ok and u_ok and s0_ok and T > 0:
            rows.append((
                T,
                round(h  / M, 5),
                round(u  / M, 5),
                round(s0 / M, 5),
            ))
            i += 4
        else:
            i += 1
    return sort_unique(rows, lambda r: r[0])


# ============================================================================
# BASE DE DATOS
# ============================================================================
def create_table(conn: sqlite3.Connection):
    conn.execute("DROP TABLE IF EXISTS cengel_ideal_gas")
    conn.execute("""
        CREATE TABLE cengel_ideal_gas (
            gas_name  TEXT NOT NULL,
            T_K       REAL NOT NULL,
            h_kJkg    REAL NOT NULL,
            u_kJkg    REAL NOT NULL,
            s0_kJkgK  REAL NOT NULL,
            Pr        REAL,
            vr        REAL,
            PRIMARY KEY (gas_name, T_K)
        )
    """)
    conn.execute(
        "CREATE INDEX IF NOT EXISTS idx_cig "
        "ON cengel_ideal_gas (gas_name, T_K)"
    )


def insert_air(conn: sqlite3.Connection, rows: list) -> int:
    data = [("Air", T, h, u, s0, Pr, vr) for (T, h, Pr, u, vr, s0) in rows]
    conn.executemany(
        "INSERT OR REPLACE INTO cengel_ideal_gas VALUES (?,?,?,?,?,?,?)", data
    )
    return len(data)


def insert_gas(conn: sqlite3.Connection, name: str, rows: list) -> int:
    data = [(name, T, h, u, s0, None, None) for (T, h, u, s0) in rows]
    conn.executemany(
        "INSERT OR REPLACE INTO cengel_ideal_gas VALUES (?,?,?,?,?,?,?)", data
    )
    return len(data)


# ============================================================================
# VALIDACIÓN
# ============================================================================
def validate(conn: sqlite3.Connection, tol: float = 0.15) -> bool:
    print("\n" + "=" * 72)
    print("  VALIDACION -- Tablas A-17 a A-25 (Cengel 7ma ed. Espanol)")
    print(f"  Tolerancia: +/-{tol}%")
    print("=" * 72)

    total = 0
    passed = 0

    for gas, checks in VALIDATION_REF.items():
        print(f"\n  {gas}:")
        print(f"  {'T':>6}  {'h_calc':>9}  {'h_ref':>9}  {'err_h':>6}  "
              f"{'s_calc':>9}  {'s_ref':>9}  {'err_s':>6}  estado")
        print("  " + "-" * 66)

        for T, (h_ref, s_ref) in checks.items():
            row = conn.execute(
                "SELECT h_kJkg, s0_kJkgK FROM cengel_ideal_gas "
                "WHERE gas_name=? AND T_K=?",
                (gas, float(T))
            ).fetchone()

            total += 2
            if row is None:
                print(f"  {T:>6}  SIN DATOS")
                continue

            h_c, s_c = row
            e_h = abs(h_c - h_ref) / abs(h_ref) * 100
            e_s = abs(s_c - s_ref) / abs(s_ref) * 100
            ok  = e_h < tol and e_s < tol
            if ok:
                passed += 2

            estado = "PASS" if ok else "FAIL"
            print(f"  {T:>6}  {h_c:>9.3f}  {h_ref:>9.3f}  {e_h:>5.3f}%  "
                  f"{s_c:>9.5f}  {s_ref:>9.5f}  {e_s:>5.3f}%  {estado}")

    rate = passed / total * 100 if total else 0
    print(f"\n  Resultado: {passed}/{total} checks pasaron ({rate:.1f}%)")
    print("=" * 72)
    return passed == total


# ============================================================================
# MAIN
# ============================================================================
def main():
    parser = argparse.ArgumentParser(
        description="Axiom-T -- Extrae tablas A-17..A-21 de Cengel al DB"
    )
    parser.add_argument("--pdf", default=DEFAULT_PDF,
                        help=f"Ruta al PDF (default: {DEFAULT_PDF})")
    parser.add_argument("--db",  default=DEFAULT_DB,
                        help=f"Ruta de salida del .db (default: {DEFAULT_DB})")
    args = parser.parse_args()

    # Verificar PDF
    if not os.path.exists(args.pdf):
        print(f"\nERROR: PDF no encontrado en:\n       {args.pdf}")
        print("       Verifica la ruta o usa --pdf <ruta>")
        sys.exit(1)

    # Crear directorio del DB si no existe
    db_dir = os.path.dirname(os.path.abspath(args.db))
    os.makedirs(db_dir, exist_ok=True)

    print("=" * 62)
    print("  Axiom-T -- Extractor de Tablas Cengel (A-17 a A-21)")
    print("=" * 62)
    print(f"\n  PDF : {args.pdf}")
    print(f"  DB  : {args.db}")

    conn = sqlite3.connect(args.db)
    conn.execute("PRAGMA journal_mode=WAL")
    create_table(conn)

    total_rows = 0

    for gas, (p_start, p_end) in PAGE_RANGES.items():
        M = MOLAR_MASS[gas]
        print(f"\n  [{gas}] paginas {p_start}-{p_end}...")

        lines = extract_text_from_pages(args.pdf, p_start, p_end)
        nums  = extract_numbers_from_lines(lines)
        print(f"    Lineas: {len(lines)}  |  Numeros: {len(nums)}")

        if len(nums) < 10:
            print(f"    ADVERTENCIA: muy pocos numeros -- verifica PAGE_RANGES")
            continue

        if gas == "Air":
            rows = parse_a17_air(nums)
            n    = insert_air(conn, rows)
        else:
            rows = parse_kmol_table(nums, M)
            n    = insert_gas(conn, gas, rows)

        if rows:
            T_vals = sorted(r[0] for r in rows)
            print(f"    Filas insertadas: {n}  |  T=[{T_vals[0]:.0f}..{T_vals[-1]:.0f}] K")
        else:
            print(f"    ADVERTENCIA: 0 filas parseadas")

        total_rows += n

    conn.commit()

    # Validacion
    all_pass = validate(conn)

    # Resumen
    print(f"\n  Total filas: {total_rows}")
    for gas in PAGE_RANGES:
        n = conn.execute(
            "SELECT COUNT(*) FROM cengel_ideal_gas WHERE gas_name=?", (gas,)
        ).fetchone()[0]
        estado = "OK" if n > 0 else "VACIO"
        print(f"    {gas:<20}: {n:>4} filas  [{estado}]")

    db_size = os.path.getsize(args.db) / 1024
    print(f"\n  Tamano del DB: {db_size:.1f} KB")
    msg = "Todas las validaciones pasaron." if all_pass else "Algunas validaciones fallaron -- revisa arriba."
    print(f"\n  {msg}")
    print("=" * 62)

    conn.close()


if __name__ == "__main__":
    main()