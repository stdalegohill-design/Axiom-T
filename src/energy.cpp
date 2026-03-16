/*
 Energy.cpp
 ----------
 Implementation of the Energy class.
 See Energy.h for interface documentation.
*/

#include "units/Energy.h"
#include <sstream>
#include <iomanip>
#include <cmath>

// Private constructor — Energy may be negative (heat rejected, work conventions)

Energy::Energy(double joule) : m_joule(joule) {}

// Factory methods

Energy Energy::fromJoule(double j)     { return Energy(j);             }
Energy Energy::fromKJoule(double kj)   { return Energy(kj   * KJ_TO_J);  }
Energy Energy::fromMJoule(double mj)   { return Energy(mj   * MJ_TO_J);  }
Energy Energy::fromCalorie(double cal) { return Energy(cal  * CAL_TO_J); }
Energy Energy::fromKCalorie(double kcal) { return Energy(kcal * KCAL_TO_J); }
Energy Energy::fromBTU(double btu)     { return Energy(btu  * BTU_TO_J); }
Energy Energy::fromKWh(double kwh)     { return Energy(kwh  * KWH_TO_J); }

// Output conversions

double Energy::toJoule()    const { return m_joule; }
double Energy::toKJoule()   const { return m_joule / KJ_TO_J;   }
double Energy::toMJoule()   const { return m_joule / MJ_TO_J;   }
double Energy::toCalorie()  const { return m_joule / CAL_TO_J;  }
double Energy::toKCalorie() const { return m_joule / KCAL_TO_J; }
double Energy::toBTU()      const { return m_joule / BTU_TO_J;  }
double Energy::toKWh()      const { return m_joule / KWH_TO_J;  }

// Arithmetic operators

Energy Energy::operator+(const Energy& other) const {
    return Energy(m_joule + other.m_joule);
}

Energy Energy::operator-(const Energy& other) const {
    return Energy(m_joule - other.m_joule);
}

Energy Energy::operator*(double scalar) const {
    return Energy(m_joule * scalar);
}

Energy Energy::operator/(double scalar) const {
    if (scalar == 0.0) {
        throw std::invalid_argument("Cannot divide Energy by zero.");
    }
    return Energy(m_joule / scalar);
}

double Energy::operator/(const Energy& other) const {
    if (other.m_joule == 0.0) {
        throw std::invalid_argument(
            "Cannot compute energy ratio: denominator is zero."
        );
    }
    return m_joule / other.m_joule;
}

// Comparison operators

bool Energy::operator==(const Energy& other) const {
    return std::fabs(m_joule - other.m_joule) < EPSILON;
}

bool Energy::operator!=(const Energy& other) const { return !(*this == other); }

bool Energy::operator<(const Energy& other) const {
    return m_joule < other.m_joule - EPSILON;
}

bool Energy::operator>(const Energy& other) const {
    return m_joule > other.m_joule + EPSILON;
}

bool Energy::operator<=(const Energy& other) const { return !(*this > other); }
bool Energy::operator>=(const Energy& other) const { return !(*this < other); }

Energy operator*(double scalar, const Energy& e) { return e * scalar; }

// Utilities

std::string Energy::toString() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4)
        << "Energy { "
        << toJoule()    << " J | "
        << toKJoule()   << " kJ | "
        << toBTU()      << " BTU | "
        << toKCalorie() << " kcal | "
        << toKWh()      << " kWh"
        << " }";
    return oss.str();
}