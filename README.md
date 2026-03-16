# Axiom-T: High-Precision Thermofluid Engineering Framework
**Languages:** [English](#english) | [Español](#español) | [Deutsch](#deutsch)

---

**DISCLAIMER / AVISO / HINWEIS**
* **EN:** This project is currently in an **early development stage**. As of the upload date, the framework is **non-functional** and is undergoing core architectural implementation.
* **ES:** Este proyecto se encuentra en una **etapa de desarrollo temprana**. A la fecha de subida, el framework **no es funcional** y está en proceso de implementación de su arquitectura base.
* **DE:** Dieses Projekt befindet sich in einer **frühen Entwicklungsphase**. Zum Zeitpunkt des Hochladens ist das Framework **noch nicht funktionsfähig** und befindet sich in der Implementierung der Basisarchitektur.

---

<a name="english"></a>
## English: Project Philosophy and Technical Architecture

### Development Approach: Physics-First & High Performance
**Axiom-T** is a modular CAE (Computer-Aided Engineering) framework designed for mechanical engineers. Unlike standard calculators, this project prioritizes **computational rigor** and **physical consistency**. Built with C++ for the core engine, it aims to bridge the gap between academic thermodynamics and professional equipment sizing.

* **Unit Safety:** Implementation of a custom `UnitSystem` that prevents dimensional errors at compile-time.
* **Precision Focused:** Designed to maintain a **0.01% error margin**, verified against industry standards like the Chapra numerical methods.

### Technical Stack
* **C++ (Core):** High-performance backend.
* **CMake:** Cross-platform build system.
* **Catch2:** Unit testing for mathematical integrity.



---

<a name="español"></a>
## Español: Filosofía de Desarrollo y Arquitectura Técnica

### Enfoque de Desarrollo: Física-Primero y Alto Rendimiento
**Axiom-T** es un framework modular de ingeniería asistida por computadora (CAE) diseñado para ingenieros mecánicos. Construido con C++ para el motor central, busca cerrar la brecha entre la termodinámica académica y el dimensionamiento profesional de equipos.

* **Seguridad Dimensional:** Implementación de un `UnitSystem` que evita errores de unidades en tiempo de compilación (ej. impide sumar Presión y Temperatura).
* **Lógica de "Caja Abierta":** Cada ecuación física se implementa desde cero para dominar los métodos numéricos subyacentes.

### Stack Tecnológico
* **C++ (Core):** Backend de alto rendimiento.
* **CMake:** Sistema de construcción multiplataforma.
* **Catch2:** Pruebas unitarias para garantizar la integridad matemática.



---

<a name="deutsch"></a>
## Deutsch: Entwicklungsphilosophie und Technische Architektur

### Entwicklungsansatz: Physik-Zuerst & Hochleistung
**Axiom-T** ist ein modulares CAE-Framework (Computer-Aided Engineering), das für Maschinenbauingenieure entwickelt wurde. Im Gegensatz zu Standard-Rechnern priorisiert dieses Projekt **numerische Strenge** und **physikalische Konsistenz**. Mit C++ als Kern-Engine soll die Lücke zwischen akademischer Thermodynamik und professioneller Anlagenauslegung geschlossen werden.

* **Einheitensicherheit:** Implementierung eines benutzerdefinierten `UnitSystem`, das Dimensionsfehler zur Kompilierzeit verhindert (z. B. verhindert das Addieren von Druck und Temperatur).
* **Präzisionsorientiert:** Entwickelt, um eine **Fehlermarge von 0,01 %** einzuhalten, verifiziert gegen Industriestandards wie die numerischen Methoden von Chapra.

### Technologie-Stack
* **C++ (Core):** Hochleistungs-Backend für iterative numerische Lösungen.
* **CMake:** Plattformübergreifendes Build-System (Windows, Linux, macOS).
* **Catch2:** Framework für Unit-Tests zur Gewährleistung der mathematischen Integrität.



---

## Roadmap / Hoja de Ruta / Fahrplan

1.  **Hito 1: ThermoMath Core (Current/En curso):**
    * `UnitSystem` (Mass, Pressure, Temperature, Energy).
    * `NumericalSolver` (Newton-Raphson, Simpson 1/3, RK4).
2.  **Hito 2: DataCore:** SQLite integration for industrial catalogs.
3.  **Hito 3: Process Solver:** Turbine, Pump, and Heat Exchanger logic.
4.  **Hito 4: Axiom-UI:** C# .NET Modern Interface.
