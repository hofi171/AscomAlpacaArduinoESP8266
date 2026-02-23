/**
 * @file IFocuser.h
 * @brief ASCOM Alpaca Focuser Device Interface
 * 
 * This interface defines the methods and properties for controlling a focuser device
 * according to the ASCOM Alpaca API specification.
 * 
 * Reference: https://ascom-standards.org/newdocs/focuser.html
 */

#ifndef IFOCUSER_H
#define IFOCUSER_H

/**
 * @class IFocuser
 * @brief Interface for ASCOM Alpaca Focuser device
 * 
 * This pure virtual interface defines all methods required to implement
 * an ASCOM-compliant focuser device driver.
 */
class IFocuser {
public:
    virtual ~IFocuser() {}

    // ==================== Properties (GET) ====================
    
    /**
     * @brief Check if focuser is capable of absolute positioning
     * @return true if focuser supports absolute positioning
     */
    virtual bool GetAbsolute() = 0;
    
    /**
     * @brief Check if focuser is currently moving
     * @return true if focuser is currently moving
     */
    virtual bool GetIsMoving() = 0;
    
    /**
     * @brief Get focuser's maximum increment size
     * @return Maximum increment size allowed
     */
    virtual int GetMaxIncrement() = 0;
    
    /**
     * @brief Get focuser's maximum step position
     * @return Maximum step position
     */
    virtual int GetMaxStep() = 0;
    
    /**
     * @brief Get focuser's current position
     * @return Current position in steps
     */
    virtual int GetPosition() = 0;
    
    /**
     * @brief Get focuser's step size in microns
     * @return Step size in microns
     */
    virtual double GetStepSize() = 0;
    
    /**
     * @brief Get state of temperature compensation mode
     * @return true if temperature compensation is enabled
     */
    virtual bool GetTempComp() = 0;
    
    /**
     * @brief Check if focuser has temperature compensation capability
     * @return true if temperature compensation is available
     */
    virtual bool GetTempCompAvailable() = 0;
    
    /**
     * @brief Get focuser's current temperature
     * @return Temperature in degrees Celsius
     */
    virtual double GetTemperature() = 0;

    // ==================== Properties (PUT) ====================
    
    /**
     * @brief Set temperature compensation mode
     * @param tempComp true to enable temperature compensation, false to disable
     */
    virtual void SetTempComp(bool tempComp) = 0;

    // ==================== Methods ====================
    
    /**
     * @brief Immediately halt focuser motion
     */
    virtual void Halt() = 0;
    
    /**
     * @brief Move focuser to new position
     * @param position Target position (absolute position or relative step count depending on Absolute property)
     */
    virtual void Move(int position) = 0;
};

#endif // IFOCUSER_H
