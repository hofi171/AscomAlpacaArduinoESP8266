/**
 * @file IFilterWheel.h
 * @brief ASCOM Alpaca FilterWheel Device Interface
 * 
 * This interface defines the methods and properties for controlling a filter wheel device
 * according to the ASCOM Alpaca API specification.
 * 
 * Reference: https://ascom-standards.org/newdocs/filterwheel.html
 */

#ifndef IFILTERWHEEL_H
#define IFILTERWHEEL_H

#include <string>
#include <vector>

/**
 * @class IFilterWheel
 * @brief Interface for ASCOM Alpaca FilterWheel device
 * 
 * This pure virtual interface defines all methods required to implement
 * an ASCOM-compliant filter wheel device driver.
 */
class IFilterWheel {
public:
    virtual ~IFilterWheel() {}

    // ==================== Properties (GET) ====================
    
    /**
     * @brief Get filter focus offsets
     * @return Vector of focus offsets for each filter
     */
    virtual std::vector<int> GetFocusOffsets() = 0;
    
    /**
     * @brief Get filter names
     * @return Vector of filter names
     */
    virtual std::vector<std::string> GetNames() = 0;
    
    /**
     * @brief Get current filter wheel position
     * @return Current position (0 to N-1)
     */
    virtual int GetPosition() = 0;
    
    /**
     * @brief Set filter wheel position
     * @param position Target position (0 to N-1)
     */
    virtual void SetPosition(int position) = 0;
};

#endif // IFILTERWHEEL_H
