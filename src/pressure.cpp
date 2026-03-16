/*
 Pressure.cpp
 ------------
 Implementation of the Pressure class.
 See Pressure.h for interface documentation.
 */

#include "C:\Users\alego\projects\Axiom-T\include\units\pressure.h"
#include <sstream>
#include <iomanip>
#include <cmath>

// Private constructor — validates physical constraint

Pressure::Pressure(double pascal) : m_pascal(pascal) {
    if (pascal < 0.0) {
        throw std::invalid_argument(
            "Pressure value cannot be negative. Received: "
            + std::to_string(pascal) + " Pa"
        );
    }
}

// Factory methods

Pressure Pressure::fromPascal(double pa)   { return Pressure(pa); }
Pressure Pressure::fromKPa(double kpa)     { return Pressure(kpa  * KPA_TO_PA);  }
Pressure Pressure::fromMPa(double mpa)     { return Pressure(mpa  * MPA_TO_PA);  }
Pressure Pressure::fromBar(double bar)     { return Pressure(bar  * BAR_TO_PA);  }
Pressure Pressure::fromAtm(double atm)     { return Pressure(atm  * ATM_TO_PA);  }
Pressure Pressure::fromPSI(double psi)     { return Pressure(psi  * PSI_TO_PA);  }
Pressure Pressure::fromMmHg(double mmhg)  { return Pressure(mmhg * MMHG_TO_PA); }

// Output conversions

double Pressure::toPascal() const { return m_pascal; }
double Pressure::toKPa()    const { return m_pascal / KPA_TO_PA;  }
double Pressure::toMPa()    const { return m_pascal / MPA_TO_PA;  }
double Pressure::toBar()    const { return m_pascal / BAR_TO_PA;  }
double Pressure::toAtm()    const { return m_pascal / ATM_TO_PA;  }
double Pressure::toPSI()    const { return m_pascal / PSI_TO_PA;  }
double Pressure::toMmHg()   const { return m_pascal / MMHG_TO_PA; }

// Arithmetic operators

Pressure Pressure::operator+(const Pressure& other) const {
    return Pressure(m_pascal + other.m_pascal);
}

Pressure Pressure::operator-(const Pressure& other) const {
    if (m_pascal < other.m_pascal) {
        throw std::invalid_argument(
            "Pressure subtraction would yield a negative result."
        );
    }
    return Pressure(m_pascal - other.m_pascal);
}

Pressure Pressure::operator*(double scalar) const {
    if (scalar < 0.0) {
        throw std::invalid_argument(
            "Cannot multiply Pressure by a negative scalar."
        );
    }
    return Pressure(m_pascal * scalar);
}

Pressure Pressure::operator/(double scalar) const {
    if (scalar == 0.0) {
        throw std::invalid_argument("Cannot divide Pressure by zero.");
    }
    if (scalar < 0.0) {
        throw std::invalid_argument(
            "Cannot divide Pressure by a negative scalar."
        );
    }
    return Pressure(m_pascal / scalar);
}

double Pressure::operator/(const Pressure& other) const {
    if (other.m_pascal == 0.0) {
        throw std::invalid_argument(
            "Cannot compute pressure ratio: denominator is zero."
        );
    }
    return m_pascal / other.m_pascal;
}

// Comparison operators

bool Pressure::operator==(const Pressure& other) const {
    return std::fabs(m_pascal - other.m_pascal) < EPSILON;
}

bool Pressure::operator!=(const Pressure& other) const {
    return !(*this == other);
}

bool Pressure::operator<(const Pressure& other) const {
    return m_pascal < other.m_pascal - EPSILON;
}

bool Pressure::operator>(const Pressure& other) const {
    return m_pascal > other.m_pascal + EPSILON;
}

bool Pressure::operator<=(const Pressure& other) const {
    return !(*this > other);
}

bool Pressure::operator>=(const Pressure& other) const {
    return !(*this < other);
}

// Non-member operator

Pressure operator*(double scalar, const Pressure& p) {
    return p * scalar;
}

// Utilities

std::string Pressure::toString() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4)
        << "Pressure { "
        << m_pascal  << " Pa | "
        << toKPa()   << " kPa | "
        << toBar()   << " bar | "
        << toAtm()   << " atm | "
        << toPSI()   << " PSI"
        << " }";
    return oss.str();
}