/**
 * @file ISafetyMonitor.h
 * @brief ASCOM Alpaca SafetyMonitor Device Interface
 * 
 * This interface defines the methods and properties for controlling a safety monitor device
 * according to the ASCOM Alpaca API specification.
 * 
 * Reference: https://ascom-standards.org/newdocs/safetymonitor.html
 */

#ifndef ISAFETYMONITOR_H
#define ISAFETYMONITOR_H

/**
 * @class ISafetyMonitor
 * @brief Interface for ASCOM Alpaca SafetyMonitor device
 * 
 * This pure virtual interface defines all methods required to implement
 * an ASCOM-compliant safety monitor device driver.
 */
class ISafetyMonitor {
public:
    virtual ~ISafetyMonitor() {}

    // ==================== Properties (GET) ====================
    
    /**
     * @brief Check if monitored state is safe for use
     * @return true if safe, false if unsafe
     */
    virtual bool GetIsSafe() = 0;
};

#endif // ISAFETYMONITOR_H
