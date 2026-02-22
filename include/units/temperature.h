#ifndef TEMPERATURE_H
#define TEMPERATURE_H

/*
 Temperature.h
 -------------
 Type-safe encapsulation of temperature as a physical quantity.

 Two semantic modes are supported via TemperatureType:
   - Absolute : represents a thermodynamic state point (T >= 0 K enforced)
   - Delta    : represents a temperature difference (ΔT, may be negative)

 All values are stored internally in Kelvin (SI).
 Conversion to other units occurs only at input (factory methods)
 and output (to* methods).

 Supported units: Kelvin, Celsius, Fahrenheit, Rankine.

 Conversion reference: NIST and Cengel & Boles, Appendix.
*/

#include <string>
#include <stdexcept>

enum class TemperatureType { Absolute, Delta };

class Temperature {
public:

    // Factory methods — absolute temperature (state points)

    static Temperature fromKelvin(double k);
    static Temperature fromCelsius(double c);
    static Temperature fromFahrenheit(double f);
    static Temperature fromRankine(double r);

    // Factory methods — temperature difference (ΔT)

    static Temperature deltaKelvin(double dk);
    static Temperature deltaCelsius(double dc);
    static Temperature deltaFahrenheit(double df);
    static Temperature deltaRankine(double dr);

    // Output conversions

    double toKelvin()     const;
    double toCelsius()    const;
    double toFahrenheit() const;
    double toRankine()    const;

    /*
     Arithmetic operators — physically valid combinations only:

       Absolute + Delta    -> Absolute   (shift a state point)
       Absolute - Absolute -> Delta      (difference between two states)
       Absolute - Delta    -> Absolute   (reverse shift)
       Delta    + Delta    -> Delta      (accumulate differences)
       Delta    - Delta    -> Delta
       Delta    * scalar   -> Delta
       Delta    / scalar   -> Delta
       Absolute * scalar   -> invalid    (no physical meaning)
    */

    Temperature operator+(const Temperature& other) const;
    Temperature operator-(const Temperature& other) const;
    Temperature operator*(double scalar)            const;
    Temperature operator/(double scalar)            const;

    bool operator==(const Temperature& other) const;
    bool operator!=(const Temperature& other) const;
    bool operator< (const Temperature& other) const;
    bool operator> (const Temperature& other) const;
    bool operator<=(const Temperature& other) const;
    bool operator>=(const Temperature& other) const;

    // Utilities

    double          value()      const { return m_kelvin; }
    TemperatureType type()       const { return m_type; }
    bool            isAbsolute() const { return m_type == TemperatureType::Absolute; }
    bool            isDelta()    const { return m_type == TemperatureType::Delta; }
    std::string     toString()   const;

private:

    explicit Temperature(double kelvin, TemperatureType type);

    double          m_kelvin;
    TemperatureType m_type;

    // Conversion constants
    static constexpr double CELSIUS_OFFSET    = 273.15;
    static constexpr double RANKINE_FACTOR    = 5.0 / 9.0;
    static constexpr double FAHRENHEIT_OFFSET = 459.67;

    // Tolerance for floating-point equality comparisons
    static constexpr double EPSILON = 1.0e-6;
};

Temperature operator*(double scalar, const Temperature& t);

#endif // TEMPERATURE_H