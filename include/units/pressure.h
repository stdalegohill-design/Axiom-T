#ifndef PRESSURE_H
#define PRESSURE_H

/*
 Pressure.h
 ----------
 Type-safe encapsulation of pressure as a physical quantity.
 
 Design pattern: private constructor + static factory methods.
 All values are stored internally in Pascal (SI). Conversion to other
 units occurs only at input (factory methods) and output (to* methods).
 
 Supported units: Pascal, kilopascal, megapascal, bar, atm, PSI, mmHg.
 
 Conversion constants sourced from NIST (National Institute of Standards
 and Technology), ensuring maximum precision across unit systems.
 */

#include <string>
#include <stdexcept>

class Pressure {
public:

    // Factory methods — the only valid way to construct a Pressure object

    static Pressure fromPascal(double pa);
    static Pressure fromKPa(double kpa);
    static Pressure fromMPa(double mpa);
    static Pressure fromBar(double bar);
    static Pressure fromAtm(double atm);
    static Pressure fromPSI(double psi);
    static Pressure fromMmHg(double mmhg);

    // Output conversions

    double toPascal() const;
    double toKPa()    const;
    double toMPa()    const;
    double toBar()    const;
    double toAtm()    const;
    double toPSI()    const;
    double toMmHg()   const;

    /*
     Arithmetic operators
     
     Permitted operations and their physical rationale:
       P + P  -> P      (absolute pressure accumulation)
       P - P  -> P      (pressure differential, result must be >= 0)
       P * k  -> P      (scalar scaling)
       k * P  -> P      (scalar scaling, commutative form)
       P / k  -> P      (scalar reduction)
       P / P  -> double (dimensionless pressure ratio)
     */

    Pressure operator+(const Pressure& other) const;
    Pressure operator-(const Pressure& other) const;
    Pressure operator*(double scalar)         const;
    Pressure operator/(double scalar)         const;
    double   operator/(const Pressure& other) const;

    // Comparison operators

    bool operator==(const Pressure& other) const;
    bool operator!=(const Pressure& other) const;
    bool operator< (const Pressure& other) const;
    bool operator> (const Pressure& other) const;
    bool operator<=(const Pressure& other) const;
    bool operator>=(const Pressure& other) const;

    // Utilities

    double      value()    const { return m_pascal; }
    bool        isValid()  const { return m_pascal >= 0.0; }
    std::string toString() const;

private:

    explicit Pressure(double pascal);

    double m_pascal;

    // Conversion factors to Pascal — sourced from NIST
    static constexpr double KPA_TO_PA  = 1.0e3;
    static constexpr double MPA_TO_PA  = 1.0e6;
    static constexpr double BAR_TO_PA  = 1.0e5;
    static constexpr double ATM_TO_PA  = 101325.0;
    static constexpr double PSI_TO_PA  = 6894.757293168;
    static constexpr double MMHG_TO_PA = 133.322387415;

    // Tolerance for floating-point equality comparisons
    static constexpr double EPSILON = 1.0e-6;
};

// Non-member operator: allows k * P in addition to P * k 
Pressure operator*(double scalar, const Pressure& p);

#endif // PRESSURE_H