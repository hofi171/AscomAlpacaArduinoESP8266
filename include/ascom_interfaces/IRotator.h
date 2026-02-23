/**
 * @file IRotator.h
 * @brief ASCOM Alpaca Rotator Device Interface
 * 
 * This interface defines the methods and properties for controlling a rotator device
 * according to the ASCOM Alpaca API specification.
 * 
 * Reference: https://ascom-standards.org/newdocs/rotator.html
 */

#ifndef IROTATOR_H
#define IROTATOR_H

/**
 * @class IRotator
 * @brief Interface for ASCOM Alpaca Rotator device
 * 
 * This pure virtual interface defines all methods required to implement
 * an ASCOM-compliant rotator device driver.
 */
class IRotator {
public:
    virtual ~IRotator() {}

    // ==================== Properties (GET) ====================
    
    /**
     * @brief Check if rotator supports the Reverse method
     * @return true if Reverse method is supported
     */
    virtual bool GetCanReverse() = 0;
    
    /**
     * @brief Check if rotator is currently moving
     * @return true if rotator is moving
     */
    virtual bool GetIsMoving() = 0;
    
    /**
     * @brief Get rotator's mechanical current position
     * @return Mechanical position in degrees
     */
    virtual double GetMechanicalPosition() = 0;
    
    /**
     * @brief Get rotator's current position
     * @return Current position in degrees
     */
    virtual double GetPosition() = 0;
    
    /**
     * @brief Get rotator's Reverse state
     * @return true if rotator is reversed
     */
    virtual bool GetReverse() = 0;
    
    /**
     * @brief Set rotator's Reverse state
     * @param reverse true to reverse rotator
     */
    virtual void SetReverse(bool reverse) = 0;
    
    /**
     * @brief Get minimum step size
     * @return Minimum step size in degrees
     */
    virtual double GetStepSize() = 0;
    
    /**
     * @brief Get destination position angle
     * @return Target position in degrees
     */
    virtual double GetTargetPosition() = 0;

    // ==================== Methods ====================
    
    /**
     * @brief Immediately stop rotator motion
     */
    virtual void Halt() = 0;
    
    /**
     * @brief Move rotator relative to current position
     * @param position Relative position in degrees
     */
    virtual void Move(double position) = 0;
    
    /**
     * @brief Move rotator to absolute position
     * @param position Absolute position in degrees
     */
    virtual void MoveAbsolute(double position) = 0;
    
    /**
     * @brief Move rotator to raw mechanical position
     * @param position Mechanical position in degrees
     */
    virtual void MoveMechanical(double position) = 0;
    
    /**
     * @brief Sync rotator to position without moving
     * @param position Position to sync to in degrees
     */
    virtual void Sync(double position) = 0;
};

#endif // IROTATOR_H
