#ifndef VOLUME_H
#define VOLUME_H

/*
 Volume.h
 --------
 Type-safe encapsulation of volume as a physical quantity.
 All values stored internally in cubic meters (SI).

 Supported units: cubic meter, liter, milliliter, cubic foot,
                  cubic inch, US gallon.
*/

#include <string>
#include <stdexcept>

class Volume {
public:

    // Factory methods

    static Volume fromCubicMeter(double m3);
    static Volume fromLiter(double l);
    static Volume fromMilliliter(double ml);
    static Volume fromCubicFoot(double ft3);
    static Volume fromCubicInch(double in3);
    static Volume fromGallonUS(double gal);

    // Output conversions

    double toCubicMeter() const;
    double toLiter()      const;
    double toMilliliter() const;
    double toCubicFoot()  const;
    double toCubicInch()  const;
    double toGallonUS()   const;

    // Arithmetic operators

    Volume operator+(const Volume& other) const;
    Volume operator-(const Volume& other) const;
    Volume operator*(double scalar)       const;
    Volume operator/(double scalar)       const;
    double operator/(const Volume& other) const;

    // Comparison operators

    bool operator==(const Volume& other) const;
    bool operator!=(const Volume& other) const;
    bool operator< (const Volume& other) const;
    bool operator> (const Volume& other) const;
    bool operator<=(const Volume& other) const;
    bool operator>=(const Volume& other) const;

    // Utilities

    double      value()    const { return m_cubicMeter; }
    bool        isValid()  const { return m_cubicMeter >= 0.0; }
    std::string toString() const;

private:

    explicit Volume(double cubicMeter);

    double m_cubicMeter;

    // Conversion factors to cubic meter — sourced from NIST
    static constexpr double LITER_TO_M3      = 1.0e-3;
    static constexpr double MILLILITER_TO_M3 = 1.0e-6;
    static constexpr double FT3_TO_M3        = 0.028316846592;
    static constexpr double IN3_TO_M3        = 1.6387064e-5;
    static constexpr double GAL_US_TO_M3     = 3.785411784e-3;

    static constexpr double EPSILON = 1.0e-12;
};

Volume operator*(double scalar, const Volume& v);

#endif // VOLUME_H