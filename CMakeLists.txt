cmake_minimum_required(VERSION 3.16)
project(Axiom-T VERSION 0.3.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# SQLite
add_library(SQLite3 STATIC externals/sqlite3/sqlite3.c)
target_include_directories(SQLite3 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/sqlite3)
target_compile_options(SQLite3 PRIVATE
    $<$<C_COMPILER_ID:GNU>:-w>
    $<$<C_COMPILER_ID:Clang>:-w>
    $<$<C_COMPILER_ID:MSVC>:/w>)
target_link_libraries(SQLite3 PUBLIC
    $<$<PLATFORM_ID:Linux>:dl>
    $<$<PLATFORM_ID:Linux>:pthread>)

# ThermoMath
add_library(ThermoMath STATIC
    src/Pressure.cpp
    src/Temperature.cpp
    src/Mass.cpp
    src/Energy.cpp
    src/Volume.cpp
    src/Power.cpp
    src/NumericalSolver.cpp
    src/IdealGas.cpp
    src/DataCore.cpp
    src/ThermodynamicState.cpp
    src/ComponentResult.cpp
    src/Turbine.cpp
    src/Compressor.cpp
    src/Pump.cpp
    src/RankineCycle.cpp
)

target_include_directories(ThermoMath PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/sqlite3)
target_link_libraries(ThermoMath PUBLIC SQLite3)

# AxiomT executable
add_executable(AxiomT main.cpp)
target_link_libraries(AxiomT PRIVATE ThermoMath)

# ThermoTests
add_executable(ThermoTests
    tests/catch2/catch_amalgamated.cpp
    tests/Test_Pressure.cpp
    tests/Test_Temperature.cpp
    tests/Test_Units.cpp
    tests/Test_NumericalSolver.cpp
    tests/Test_IdealGas.cpp
    tests/Test_DataCore.cpp
    tests/Test_Components.cpp
)
target_include_directories(ThermoTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
    ${CMAKE_CURRENT_SOURCE_DIR}/externals/sqlite3)
target_link_libraries(ThermoTests PRIVATE ThermoMath)