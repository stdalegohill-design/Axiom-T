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

# Core library — compiled as static, promoted to shared DLL in Milestone 4
add_library(ThermoMath STATIC
    src/Pressure.cpp
    src/Temperature.cpp
)

target_include_directories(ThermoMath PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Test executable
add_executable(ThermoTests
    tests/catch2/catch_amalgamated.cpp
    tests/test_pressure.cpp
    tests/test_temperature.cpp
)

target_include_directories(ThermoTests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/tests
)

target_link_libraries(ThermoTests PRIVATE ThermoMath)