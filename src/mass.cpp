/*
 Mass.cpp
 --------
 Implementation of the Mass class.
 See Mass.h for interface documentation.
*/

#include "units/Mass.h"
#include <sstream>
#include <iomanip>
#include <cmath>

// Private constructor

Mass::Mass(double kilogram) : m_kilogram(kilogram) {
    if (kilogram < 0.0) {
        throw std::invalid_argument(
            "Mass cannot be negative. Received: "
            + std::to_string(kilogram) + " kg"
        );
    }
}

// Factory methods

Mass Mass::fromKilogram(double kg)   { return Mass(kg); }
Mass Mass::fromGram(double g)        { return Mass(g   * GRAM_TO_KG);       }
Mass Mass::fromPoundMass(double lbm) { return Mass(lbm * LBM_TO_KG);        }
Mass Mass::fromOunce(double oz)      { return Mass(oz  * OUNCE_TO_KG);      }
Mass Mass::fromMetricTon(double t)   { return Mass(t   * METRIC_TON_TO_KG); }

// Output conversions

double Mass::toKilogram()  const { return m_kilogram; }
double Mass::toGram()      const { return m_kilogram / GRAM_TO_KG;       }
double Mass::toPoundMass() const { return m_kilogram / LBM_TO_KG;        }
double Mass::toOunce()     const { return m_kilogram / OUNCE_TO_KG;      }
double Mass::toMetricTon() const { return m_kilogram / METRIC_TON_TO_KG; }

// Arithmetic operators

Mass Mass::operator+(const Mass& other) const {
    return Mass(m_kilogram + other.m_kilogram);
}

Mass Mass::operator-(const Mass& other) const {
    if (m_kilogram < other.m_kilogram) {
        throw std::invalid_argument(
            "Mass subtraction would yield a negative result."
        );
    }
    return Mass(m_kilogram - other.m_kilogram);
}

Mass Mass::operator*(double scalar) const {
    if (scalar < 0.0) {
        throw std::invalid_argument(
            "Cannot multiply Mass by a negative scalar."
        );
    }
    return Mass(m_kilogram * scalar);
}

Mass Mass::operator/(double scalar) const {
    if (scalar == 0.0) {
        throw std::invalid_argument("Cannot divide Mass by zero.");
    }
    if (scalar < 0.0) {
        throw std::invalid_argument(
            "Cannot divide Mass by a negative scalar."
        );
    }
    return Mass(m_kilogram / scalar);
}

double Mass::operator/(const Mass& other) const {
    if (other.m_kilogram == 0.0) {
        throw std::invalid_argument(
            "Cannot compute mass ratio: denominator is zero."
        );
    }
    return m_kilogram / other.m_kilogram;
}

// Comparison operators

bool Mass::operator==(const Mass& other) const {
    return std::fabs(m_kilogram - other.m_kilogram) < EPSILON;
}

bool Mass::operator!=(const Mass& other) const { return !(*this == other); }

bool Mass::operator<(const Mass& other) const {
    return m_kilogram < other.m_kilogram - EPSILON;
}

bool Mass::operator>(const Mass& other) const {
    return m_kilogram > other.m_kilogram + EPSILON;
}

bool Mass::operator<=(const Mass& other) const { return !(*this > other); }
bool Mass::operator>=(const Mass& other) const { return !(*this < other); }

Mass operator*(double scalar, const Mass& m) { return m * scalar; }

// Utilities

std::string Mass::toString() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6)
        << "Mass { "
        << toKilogram()  << " kg | "
        << toGram()      << " g | "
        << toPoundMass() << " lbm | "
        << toMetricTon() << " t"
        << " }";
    return oss.str();
}