#ifndef POWER_H
#define POWER_H

/*
 Power.h
 -------
 Type-safe encapsulation of power as a physical quantity.
 All values stored internally in Watts (SI).

 Supported units: Watt, kilowatt, megawatt, horsepower (mechanical),
                  BTU/hour, BTU/second.

 Note: Power may be negative to represent work input vs output
 depending on thermodynamic sign convention used.
*/

#include <string>
#include <stdexcept>

class Power {
public:

    // Factory methods

    static Power fromWatt(double w);
    static Power fromKWatt(double kw);
    static Power fromMWatt(double mw);
    static Power fromHorsepower(double hp);
    static Power fromBTUperHour(double btuh);
    static Power fromBTUperSecond(double btus);

    // Output conversions

    double toWatt()        const;
    double toKWatt()       const;
    double toMWatt()       const;
    double toHorsepower()  const;
    double toBTUperHour()  const;
    double toBTUperSecond() const;

    // Arithmetic operators

    Power  operator+(const Power& other) const;
    Power  operator-(const Power& other) const;
    Power  operator*(double scalar)      const;
    Power  operator/(double scalar)      const;
    double operator/(const Power& other) const;

    // Comparison operators

    bool operator==(const Power& other) const;
    bool operator!=(const Power& other) const;
    bool operator< (const Power& other) const;
    bool operator> (const Power& other) const;
    bool operator<=(const Power& other) const;
    bool operator>=(const Power& other) const;

    // Utilities

    double      value()    const { return m_watt; }
    std::string toString() const;

private:

    explicit Power(double watt);

    double m_watt;

    // Conversion factors to Watt — sourced from NIST
    static constexpr double KW_TO_W    = 1.0e3;
    static constexpr double MW_TO_W    = 1.0e6;
    static constexpr double HP_TO_W    = 745.69987158227022;
    static constexpr double BTUH_TO_W  = 0.29307107017222;
    static constexpr double BTUS_TO_W  = 1055.05585262;

    static constexpr double EPSILON = 1.0e-6;
};

Power operator*(double scalar, const Power& p);

#endif // POWER_H