/**
 * @file AscomTypes.h
 * @brief ASCOM Alpaca Common Types and Structures
 * 
 * This file defines common types, enumerations, and structures used across
 * ASCOM Alpaca device interfaces.
 * 
 * References: 
 * - https://ascom-standards.org/newdocs/rate.html
 * - https://ascom-standards.org/newdocs/statevalue.html
 */

#ifndef ASCOMTYPES_H
#define ASCOMTYPES_H

#include <string>

/**
 * @enum GuideDirection
 * @brief Enumeration of guide directions for pulse guiding
 */
enum GuideDirection {
    GUIDE_NORTH = 0,
    GUIDE_SOUTH = 1,
    GUIDE_EAST = 2,
    GUIDE_WEST = 3
};

/**
 * @struct Rate
 * @brief Represents a rate range with minimum and maximum values
 * 
 * Used to describe axis movement rates for telescope devices.
 * Reference: https://ascom-standards.org/newdocs/rate.html
 */
struct Rate {
    /**
     * @brief Minimum rate value in degrees per second
     */
    double minimum;
    
    /**
     * @brief Maximum rate value in degrees per second
     */
    double maximum;
    
    /**
     * @brief Default constructor
     */
    Rate() : minimum(0.0), maximum(0.0) {}
    
    /**
     * @brief Parameterized constructor
     * @param min Minimum rate
     * @param max Maximum rate
     */
    Rate(double min, double max) : minimum(min), maximum(max) {}
};

/**
 * @struct StateValue
 * @brief Represents a device state value with name and value
 * 
 * Used to describe device state in the DeviceState property.
 * Reference: https://ascom-standards.org/newdocs/statevalue.html
 */
struct StateValue {
    /**
     * @brief Name of the state value
     */
    std::string name;
    
    /**
     * @brief Value of the state (can be numeric or string)
     */
    std::string value;
    
    /**
     * @brief Default constructor
     */
    StateValue() : name(""), value("") {}
    
    /**
     * @brief Parameterized constructor
     * @param n State name
     * @param v State value
     */
    StateValue(const std::string& n, const std::string& v) : name(n), value(v) {}
};

#endif // ASCOMTYPES_H
