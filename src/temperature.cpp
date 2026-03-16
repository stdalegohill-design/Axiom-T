/*
 Temperature.cpp
 ---------------
 Implementation of the Temperature class.
 See Temperature.h for interface documentation.
*/

#include "C:\Users\alego\projects\Axiom-T\include\units\temperature.h"
#include <sstream>
#include <iomanip>
#include <cmath>

// Private constructor

Temperature::Temperature(double kelvin, TemperatureType type)
    : m_kelvin(kelvin), m_type(type)
{
    if (type == TemperatureType::Absolute && kelvin < 0.0) {
        throw std::invalid_argument(
            "Absolute temperature cannot be below 0 K. Received: "
            + std::to_string(kelvin) + " K"
        );
    }
}

/* =========================================================================
 Factory methods — Absolute
 ========================================================================= */

Temperature Temperature::fromKelvin(double k) {
    return Temperature(k, TemperatureType::Absolute);
}

Temperature Temperature::fromCelsius(double c) {
    return Temperature(c + CELSIUS_OFFSET, TemperatureType::Absolute);
}

Temperature Temperature::fromFahrenheit(double f) {
    // T(K) = (T(°F) + 459.67) * 5/9
    return Temperature((f + FAHRENHEIT_OFFSET) * RANKINE_FACTOR, TemperatureType::Absolute);
}

Temperature Temperature::fromRankine(double r) {
    // T(K) = T(°R) * 5/9
    return Temperature(r * RANKINE_FACTOR, TemperatureType::Absolute);
}

/* =========================================================================
 Factory methods — Delta
 ========================================================================= */

Temperature Temperature::deltaKelvin(double dk) {
    return Temperature(dk, TemperatureType::Delta);
}

Temperature Temperature::deltaCelsius(double dc) {
    // A difference in °C equals the same difference in K
    return Temperature(dc, TemperatureType::Delta);
}

Temperature Temperature::deltaFahrenheit(double df) {
    // A difference in °F: ΔT(K) = ΔT(°F) * 5/9
    return Temperature(df * RANKINE_FACTOR, TemperatureType::Delta);
}

Temperature Temperature::deltaRankine(double dr) {
    return Temperature(dr * RANKINE_FACTOR, TemperatureType::Delta);
}

/* =========================================================================
 Output conversions
 ========================================================================= */

double Temperature::toKelvin() const {
    return m_kelvin;
}

double Temperature::toCelsius() const {
    if (m_type == TemperatureType::Delta)
        return m_kelvin; // ΔT(°C) == ΔT(K)
    return m_kelvin - CELSIUS_OFFSET;
}

double Temperature::toFahrenheit() const {
    if (m_type == TemperatureType::Delta)
        return m_kelvin / RANKINE_FACTOR; // ΔT(°F) = ΔT(K) * 9/5
    return (m_kelvin / RANKINE_FACTOR) - FAHRENHEIT_OFFSET;
}

double Temperature::toRankine() const {
    return m_kelvin / RANKINE_FACTOR;
}

/* =========================================================================
 Arithmetic operators
 ========================================================================= */

Temperature Temperature::operator+(const Temperature& other) const {
    // Absolute + Delta = Absolute
    if (m_type == TemperatureType::Absolute && other.m_type == TemperatureType::Delta)
        return Temperature(m_kelvin + other.m_kelvin, TemperatureType::Absolute);

    // Delta + Delta = Delta
    if (m_type == TemperatureType::Delta && other.m_type == TemperatureType::Delta)
        return Temperature(m_kelvin + other.m_kelvin, TemperatureType::Delta);

    // Delta + Absolute = Absolute (commutative form)
    if (m_type == TemperatureType::Delta && other.m_type == TemperatureType::Absolute)
        return Temperature(m_kelvin + other.m_kelvin, TemperatureType::Absolute);

    // Absolute + Absolute = physically invalid
    throw std::invalid_argument(
        "Adding two absolute temperatures has no physical meaning."
    );
}

Temperature Temperature::operator-(const Temperature& other) const {
    // Absolute - Absolute = Delta
    if (m_type == TemperatureType::Absolute && other.m_type == TemperatureType::Absolute)
        return Temperature(m_kelvin - other.m_kelvin, TemperatureType::Delta);

    // Absolute - Delta = Absolute
    if (m_type == TemperatureType::Absolute && other.m_type == TemperatureType::Delta)
        return Temperature(m_kelvin - other.m_kelvin, TemperatureType::Absolute);

    // Delta - Delta = Delta
    if (m_type == TemperatureType::Delta && other.m_type == TemperatureType::Delta)
        return Temperature(m_kelvin - other.m_kelvin, TemperatureType::Delta);

    // Delta - Absolute = physically invalid
    throw std::invalid_argument(
        "Subtracting an absolute temperature from a delta has no physical meaning."
    );
}

Temperature Temperature::operator*(double scalar) const {
    if (m_type == TemperatureType::Absolute) {
        throw std::invalid_argument(
            "Scaling an absolute temperature by a scalar has no physical meaning. "
            "Use a Delta if scaling a temperature difference."
        );
    }
    return Temperature(m_kelvin * scalar, TemperatureType::Delta);
}

Temperature Temperature::operator/(double scalar) const {
    if (scalar == 0.0) {
        throw std::invalid_argument("Cannot divide Temperature by zero.");
    }
    if (m_type == TemperatureType::Absolute) {
        throw std::invalid_argument(
            "Dividing an absolute temperature by a scalar has no physical meaning."
        );
    }
    return Temperature(m_kelvin / scalar, TemperatureType::Delta);
}

Temperature operator*(double scalar, const Temperature& t) {
    return t * scalar;
}

/* =========================================================================
 Comparison operators
 ========================================================================= */

bool Temperature::operator==(const Temperature& other) const {
    return m_type == other.m_type &&
           std::fabs(m_kelvin - other.m_kelvin) < EPSILON;
}

bool Temperature::operator!=(const Temperature& other) const {
    return !(*this == other);
}

bool Temperature::operator<(const Temperature& other) const {
    return m_kelvin < other.m_kelvin - EPSILON;
}

bool Temperature::operator>(const Temperature& other) const {
    return m_kelvin > other.m_kelvin + EPSILON;
}

bool Temperature::operator<=(const Temperature& other) const {
    return !(*this > other);
}

bool Temperature::operator>=(const Temperature& other) const {
    return !(*this < other);
}

/* =========================================================================
 Utilities
 ========================================================================= */

std::string Temperature::toString() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4);

    if (m_type == TemperatureType::Absolute) {
        oss << "Temperature [Absolute] { "
            << toKelvin()     << " K | "
            << toCelsius()    << " °C | "
            << toFahrenheit() << " °F | "
            << toRankine()    << " °R"
            << " }";
    } else {
        oss << "Temperature [Delta] { "
            << toKelvin()     << " ΔK | "
            << toCelsius()    << " Δ°C | "
            << toFahrenheit() << " Δ°F | "
            << toRankine()    << " Δ°R"
            << " }";
    }

    return oss.str();
}