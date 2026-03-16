/*
 Volume.cpp
 ----------
 Implementation of the Volume class.
 See Volume.h for interface documentation.
*/

#include "units/Volume.h"
#include <sstream>
#include <iomanip>
#include <cmath>

// Private constructor

Volume::Volume(double cubicMeter) : m_cubicMeter(cubicMeter) {
    if (cubicMeter < 0.0) {
        throw std::invalid_argument(
            "Volume cannot be negative. Received: "
            + std::to_string(cubicMeter) + " m³"
        );
    }
}

// Factory methods

Volume Volume::fromCubicMeter(double m3)  { return Volume(m3);               }
Volume Volume::fromLiter(double l)        { return Volume(l   * LITER_TO_M3);      }
Volume Volume::fromMilliliter(double ml)  { return Volume(ml  * MILLILITER_TO_M3); }
Volume Volume::fromCubicFoot(double ft3)  { return Volume(ft3 * FT3_TO_M3);        }
Volume Volume::fromCubicInch(double in3)  { return Volume(in3 * IN3_TO_M3);        }
Volume Volume::fromGallonUS(double gal)   { return Volume(gal * GAL_US_TO_M3);     }

// Output conversions

double Volume::toCubicMeter() const { return m_cubicMeter; }
double Volume::toLiter()      const { return m_cubicMeter / LITER_TO_M3;      }
double Volume::toMilliliter() const { return m_cubicMeter / MILLILITER_TO_M3; }
double Volume::toCubicFoot()  const { return m_cubicMeter / FT3_TO_M3;        }
double Volume::toCubicInch()  const { return m_cubicMeter / IN3_TO_M3;        }
double Volume::toGallonUS()   const { return m_cubicMeter / GAL_US_TO_M3;     }

// Arithmetic operators

Volume Volume::operator+(const Volume& other) const {
    return Volume(m_cubicMeter + other.m_cubicMeter);
}

Volume Volume::operator-(const Volume& other) const {
    if (m_cubicMeter < other.m_cubicMeter) {
        throw std::invalid_argument(
            "Volume subtraction would yield a negative result."
        );
    }
    return Volume(m_cubicMeter - other.m_cubicMeter);
}

Volume Volume::operator*(double scalar) const {
    if (scalar < 0.0) {
        throw std::invalid_argument(
            "Cannot multiply Volume by a negative scalar."
        );
    }
    return Volume(m_cubicMeter * scalar);
}

Volume Volume::operator/(double scalar) const {
    if (scalar == 0.0) {
        throw std::invalid_argument("Cannot divide Volume by zero.");
    }
    if (scalar < 0.0) {
        throw std::invalid_argument(
            "Cannot divide Volume by a negative scalar."
        );
    }
    return Volume(m_cubicMeter / scalar);
}

double Volume::operator/(const Volume& other) const {
    if (other.m_cubicMeter == 0.0) {
        throw std::invalid_argument(
            "Cannot compute volume ratio: denominator is zero."
        );
    }
    return m_cubicMeter / other.m_cubicMeter;
}

// Comparison operators

bool Volume::operator==(const Volume& other) const {
    return std::fabs(m_cubicMeter - other.m_cubicMeter) < EPSILON;
}

bool Volume::operator!=(const Volume& other) const { return !(*this == other); }

bool Volume::operator<(const Volume& other) const {
    return m_cubicMeter < other.m_cubicMeter - EPSILON;
}

bool Volume::operator>(const Volume& other) const {
    return m_cubicMeter > other.m_cubicMeter + EPSILON;
}

bool Volume::operator<=(const Volume& other) const { return !(*this > other); }
bool Volume::operator>=(const Volume& other) const { return !(*this < other); }

Volume operator*(double scalar, const Volume& v) { return v * scalar; }

// Utilities

std::string Volume::toString() const {
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(6)
        << "Volume { "
        << toCubicMeter() << " m³ | "
        << toLiter()      << " L | "
        << toCubicFoot()  << " ft³ | "
        << toGallonUS()   << " gal"
        << " }";
    return oss.str();
}