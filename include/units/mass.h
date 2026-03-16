#ifndef MASS_H
#define MASS_H

/*
Mass.h
-----
Type-safe encapsulation of mass as a physical quantity.
All values stored internally in kilograms (SI).

Supported units: kilogram, gram, pound-mass, ounce, ton (metric).
*/

#include <string>
#include <stdexcept>

class Mass {
public:

    // Factory methods

    static Mass fromKilogram(double kg);
    static Mass fromGram(double g);
    static Mass fromPoundMass(double lbm);
    static Mass fromOunce(double oz);
    static Mass fromMetricTon(double t);

    // Output conversions

    double toKilogram()     const;
    double toGram()         const;
    double toPoundMass()    const;
    double toOunce()        const;
    double toMetricTon()    const;

    // Arithmetic operators

    Mass    operator+(const Mass& other)    const;
    Mass    operator-(const Mass& other)    const;
    Mass    operator*(double scalar)        const;
    Mass    operator/(double scalar)        const;
    double  operator/(const Mass& other)    const;

    // Comparison operators

    bool operator==(const Mass& other) const;
    bool operator!=(const Mass& other) const;
    bool operator< (const Mass& other) const;
    bool operator> (const Mass& other) const;
    bool operator<=(const Mass& other) const;
    bool operator>=(const Mass& other) const;

    // Utilities

    double      value()   const { return m_kilogram; }
    bool        isValid() const { return m_kilogram >= 0.0; }
    std::string toString() const;

private:

    explicit Mass(double kilogram);

    double m_kilogram;

    // Conversion factors to kilogram — sourced from NIST
    static constexpr double GRAM_TO_KG       = 1.0e-3;
    static constexpr double LBM_TO_KG        = 0.45359237;
    static constexpr double OUNCE_TO_KG      = 0.028349523125;
    static constexpr double METRIC_TON_TO_KG = 1.0e3;

    static constexpr double EPSILON = 1.0e-9;
};

Mass operator*(double scalar, const Mass& m);

#endif // MASS_H