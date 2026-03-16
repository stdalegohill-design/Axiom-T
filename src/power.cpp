/*
 Power.cpp
 ---------
 Implementation of the Power class.
 See Power.h for interface documentation.
*/

#include "units/Power.h"
#include <sstream>
#include <iomanip>
#include <cmath>

// Private constructor — Power may be negative (sign convention)

Power::Power(double watt) : m_watt(watt) {}

// Factory methods

Power Power::fromWatt(double w)          { return Power(w);            }
Power Power::fromKWatt(double kw)        { return Power(kw   * KW_TO_W);   }
Power Power::fromMWatt(double mw)        { return Power(mw   * MW_TO_W);   }
Power Power::fromHorsepower(double hp)   { return Power(hp   * HP_TO_W);   }
Power Power::fromBTUperHour(double btuh) { return Power(btuh * BTUH_TO_W); }
Power Power::fromBTUperSecond(double btus) { return Power(btus * BTUS_TO_W); }

// Output conversions

double Power::toWatt()         const { return m_watt; }
double Power::toKWatt()        const { return m_watt / KW_TO_W;   }
double Power::toMWatt()        const { return m_watt / MW_TO_W;   }
double Power::toHorsepower()   const { return m_watt / HP_TO_W;   }
double Power::toBTUperHour()   const { return m_watt / BTUH_TO_W; }
double Power::toBTUperSecond() const { return m_watt / BTUS_TO_W; }

// Arithmetic operators

Power Power::operator+(const Power& other) const {
    return Power(m_watt + other.m_watt);
}

Power Power::operator-(const Power& other) const {
    return Power(m_watt - other.m_watt);
}

Power Power::operator*(double scalar) const {
    return Power(m_watt * scalar);
}

Power Power::operator/(double scalar) const {
    if (scalar == 0.0) {
        throw std::invalid_argument("Cannot divide Power by zero.");
    }
    return Power(m_watt / scalar);
}

double Power::operator/(const Power& other) const {
    if (other.m_watt == 0.0) {
        throw std::invalid_argument(
            "Cannot compute power ratio: denominator is zero."
        );
    }
    return m_watt / other.m_watt;
}

// Comparison operators

bool Power::operator==(const Power& other) const {
    return std::fabs(m_watt - other.m_watt) < EPSILON;
}

bool Power::operator!=(const Power& other) const { return !(*this == other); }

bool Power::operator<(const Power& other) const {
    return m_watt < other.m_watt - EPSILON;
}

bool Power::operator>(const Power& other) const {
    return m_watt > other.m_watt + EPSILON;
}

bool Power::operator<=(const Power& other) const { return !(*this > other); }
bool Power::operator>=(const Power& other) const { return !(*this < other); }

Power operator*(double scalar, const Power& p) { return p * scalar; }

// Utilities

std::string Power::toString() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(4)
        << "Power { "
        << toWatt()       << " W | "
        << toKWatt()      << " kW | "
        << toHorsepower() << " hp | "
        << toBTUperHour() << " BTU/h"
        << " }";
    return oss.str();
}