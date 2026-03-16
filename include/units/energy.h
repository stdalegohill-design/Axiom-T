#ifndef ENERGY_H
#define ENERGY_H

/*
 Energy.h
 --------
 Type-safe encapsulation of energy as a physical quantity.
 All values stored internally in Joules (SI).

 Supported units: Joule, kilojoule, megajoule, calorie, kilocalorie,
                  BTU (British Thermal Unit), kWh.

 Note: Energy may be negative to represent heat rejected or work
 done on/by the system in thermodynamic sign conventions.
*/

#include <string>
#include <stdexcept>

class Energy {
public:

    // Factory methods

    static Energy fromJoule(double j);
    static Energy fromKJoule(double kj);
    static Energy fromMJoule(double mj);
    static Energy fromCalorie(double cal);
    static Energy fromKCalorie(double kcal);
    static Energy fromBTU(double btu);
    static Energy fromKWh(double kwh);

    // Output conversions

    double toJoule()    const;
    double toKJoule()   const;
    double toMJoule()   const;
    double toCalorie()  const;
    double toKCalorie() const;
    double toBTU()      const;
    double toKWh()      const;

    // Arithmetic operators

    Energy operator+(const Energy& other) const;
    Energy operator-(const Energy& other) const;
    Energy operator*(double scalar)       const;
    Energy operator/(double scalar)       const;
    double operator/(const Energy& other) const;

    // Comparison operators

    bool operator==(const Energy& other) const;
    bool operator!=(const Energy& other) const;
    bool operator< (const Energy& other) const;
    bool operator> (const Energy& other) const;
    bool operator<=(const Energy& other) const;
    bool operator>=(const Energy& other) const;

    // Utilities

    double      value()    const { return m_joule; }
    std::string toString() const;

private:

    explicit Energy(double joule);

    double m_joule;

    // Conversion factors to Joule — sourced from NIST
    static constexpr double KJ_TO_J   = 1.0e3;
    static constexpr double MJ_TO_J   = 1.0e6;
    static constexpr double CAL_TO_J  = 4.184;
    static constexpr double KCAL_TO_J = 4184.0;
    static constexpr double BTU_TO_J  = 1055.05585262;
    static constexpr double KWH_TO_J  = 3.6e6;

    static constexpr double EPSILON = 1.0e-6;
};

Energy operator*(double scalar, const Energy& e);

#endif // ENERGY_H