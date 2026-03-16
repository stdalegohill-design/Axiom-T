cmake_minimum_required(VERSION 3.16)
project(Axiom-T VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# =============================================================================
# ThermoMath — static library (the engine)
# =============================================================================
add_library(ThermoMath STATIC
    src/Pressure.cpp
    src/Temperature.cpp
    src/Mass.cpp
    src/Energy.cpp
    src/Volume.cpp
    src/Power.cpp
    src/NumericalSolver.cpp
    src/IdealGas.cpp
)

target_include_directories(ThermoMath PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# =============================================================================
# AxiomT — main application (Phase 1 demo)
# =============================================================================
add_executable(AxiomT
    main.cpp
)

target_link_libraries(AxiomT PRIVATE ThermoMath)

# =============================================================================
# ThermoTests — test suite
# =============================================================================
add_executable(ThermoTests
    tests/catch2/catch_amalgamated.cpp
    tests/test_pressure.cpp
    tests/test_temperature.cpp
    tests/test_units.cpp
    tests/test_numerical_solver.cpp
    tests/test_ideal_gas.cpp
)

target_include_directories(ThermoTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

target_link_libraries(ThermoTests PRIVATE ThermoMath)

# =============================================================================
# Build and run:
#   cd build
#   cmake .. -G "MinGW Makefiles"
#   cmake --build .
#   .\AxiomT.exe              <- main demo
#   .\ThermoTests.exe         <- full test suite
# =============================================================================