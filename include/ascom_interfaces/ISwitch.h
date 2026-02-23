/**
 * @file ISwitch.h
 * @brief ASCOM Alpaca Switch Device Interface
 * 
 * This interface defines the methods and properties for controlling a switch device
 * according to the ASCOM Alpaca API specification.
 * 
 * Reference: https://ascom-standards.org/newdocs/switch.html
 */

#ifndef ISWITCH_H
#define ISWITCH_H

#include <string>

/**
 * @class ISwitch
 * @brief Interface for ASCOM Alpaca Switch device
 * 
 * This pure virtual interface defines all methods required to implement
 * an ASCOM-compliant switch device driver.
 * 
 * A switch device can manage multiple individual switches, each identified by
 * an ID number from 0 to MaxSwitch-1.
 */
class ISwitch {
public:
    virtual ~ISwitch() {}

    // ==================== Properties (GET) ====================
    
    /**
     * @brief Get the number of switch devices managed by this driver
     * @return Number of switch devices (0 to MaxSwitch-1)
     */
    virtual int GetMaxSwitch() = 0;

    // ==================== Methods (GET) ====================
    
    /**
     * @brief Check if specified switch can operate asynchronously
     * @param switchNumber Switch number (0 to MaxSwitch-1)
     * @return true if switch supports asynchronous operation (ISwitchV3+)
     */
    virtual bool GetCanAsync(int switchNumber) = 0;
    
    /**
     * @brief Check if specified switch device can be written to
     * @param switchNumber Switch number (0 to MaxSwitch-1)
     * @return true if switch can be written to
     */
    virtual bool GetCanWrite(int switchNumber) = 0;
    
    /**
     * @brief Get state of specified switch device as boolean
     * @param switchNumber Switch number (0 to MaxSwitch-1)
     * @return Current state of the switch (true/false)
     */
    virtual bool GetSwitch(int switchNumber) = 0;
    
    /**
     * @brief Get description of specified switch device
     * @param switchNumber Switch number (0 to MaxSwitch-1)
     * @return Description of the switch
     */
    virtual std::string GetSwitchDescription(int switchNumber) = 0;
    
    /**
     * @brief Get name of specified switch device
     * @param switchNumber Switch number (0 to MaxSwitch-1)
     * @return Name of the switch
     */
    virtual std::string GetSwitchName(int switchNumber) = 0;
    
    /**
     * @brief Get value of specified switch device as double
     * @param switchNumber Switch number (0 to MaxSwitch-1)
     * @return Current value of the switch
     */
    virtual double GetSwitchValue(int switchNumber) = 0;
    
    /**
     * @brief Get minimum value of specified switch device
     * @param switchNumber Switch number (0 to MaxSwitch-1)
     * @return Minimum value the switch can be set to
     */
    virtual double GetMinSwitchValue(int switchNumber) = 0;
    
    /**
     * @brief Get maximum value of specified switch device
     * @param switchNumber Switch number (0 to MaxSwitch-1)
     * @return Maximum value the switch can be set to
     */
    virtual double GetMaxSwitchValue(int switchNumber) = 0;
    
    /**
     * @brief Check if asynchronous state change is complete
     * @param switchNumber Switch number (0 to MaxSwitch-1)
     * @return true if state change is complete (ISwitchV3+)
     */
    virtual bool GetStateChangeComplete(int switchNumber) = 0;
    
    /**
     * @brief Get step size that this switch device supports
     * @param switchNumber Switch number (0 to MaxSwitch-1)
     * @return Step size for the switch value
     */
    virtual double GetSwitchStep(int switchNumber) = 0;

    // ==================== Methods (PUT) ====================
    
    /**
     * @brief Set switch's boolean state asynchronously
     * @param switchNumber Switch number (0 to MaxSwitch-1)
     * @param state Boolean state to set
     */
    virtual void SetAsync(int switchNumber, bool state) = 0;
    
    /**
     * @brief Set switch's double value asynchronously
     * @param switchNumber Switch number (0 to MaxSwitch-1)
     * @param value Double value to set
     */
    virtual void SetAsyncValue(int switchNumber, double value) = 0;
    
    /**
     * @brief Set switch device to specified boolean state
     * @param switchNumber Switch number (0 to MaxSwitch-1)
     * @param state Boolean state to set
     */
    virtual void SetSwitch(int switchNumber, bool state) = 0;
    
    /**
     * @brief Set switch device name
     * @param switchNumber Switch number (0 to MaxSwitch-1)
     * @param name Name to set for the switch
     */
    virtual void SetSwitchName(int switchNumber, const std::string& name) = 0;
    
    /**
     * @brief Set switch device to specified double value
     * @param switchNumber Switch number (0 to MaxSwitch-1)
     * @param value Double value to set
     */
    virtual void SetSwitchValue(int switchNumber, double value) = 0;
};

#endif // ISWITCH_H
