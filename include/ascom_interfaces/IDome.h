/**
 * @file IDome.h
 * @brief ASCOM Alpaca Dome Device Interface
 * 
 * This interface defines the methods and properties for controlling a dome device
 * according to the ASCOM Alpaca API specification.
 * 
 * Reference: https://ascom-standards.org/newdocs/dome.html
 */

#ifndef IDOME_H
#define IDOME_H

/**
 * @enum ShutterState
 * @brief Enumeration of possible shutter states
 */
enum ShutterState {
    SHUTTER_OPEN = 0,
    SHUTTER_CLOSED = 1,
    SHUTTER_OPENING = 2,
    SHUTTER_CLOSING = 3,
    SHUTTER_ERROR = 4
};

/**
 * @class IDome
 * @brief Interface for ASCOM Alpaca Dome device
 * 
 * This pure virtual interface defines all methods required to implement
 * an ASCOM-compliant dome device driver.
 */
class IDome {
public:
    virtual ~IDome() {}

    // ==================== Properties (GET) ====================
    
    /**
     * @brief Get the dome altitude in degrees
     * @return Altitude from 0° (horizon) to 90° (zenith)
     */
    virtual double GetAltitude() = 0;
    
    /**
     * @brief Check if dome is at home position
     * @return true if dome is at home position
     */
    virtual bool GetAtHome() = 0;
    
    /**
     * @brief Check if dome is at park position
     * @return true if dome is at park position
     */
    virtual bool GetAtPark() = 0;
    
    /**
     * @brief Get the dome azimuth in degrees
     * @return Azimuth from 0° (North), 90° (East), 180° (South), 270° (West)
     */
    virtual double GetAzimuth() = 0;
    
    /**
     * @brief Check if dome can find home position
     * @return true if dome can find home position
     */
    virtual bool GetCanFindHome() = 0;
    
    /**
     * @brief Check if dome can be parked
     * @return true if dome can be parked
     */
    virtual bool GetCanPark() = 0;
    
    /**
     * @brief Check if dome altitude can be set
     * @return true if dome altitude can be set
     */
    virtual bool GetCanSetAltitude() = 0;
    
    /**
     * @brief Check if dome azimuth can be set
     * @return true if dome azimuth can be set
     */
    virtual bool GetCanSetAzimuth() = 0;
    
    /**
     * @brief Check if dome park position can be set
     * @return true if dome park position can be set
     */
    virtual bool GetCanSetPark() = 0;
    
    /**
     * @brief Check if dome shutter can be opened
     * @return true if dome shutter can be opened
     */
    virtual bool GetCanSetShutter() = 0;
    
    /**
     * @brief Check if dome supports slaving to a telescope
     * @return true if dome supports slaving to a telescope
     */
    virtual bool GetCanSlave() = 0;
    
    /**
     * @brief Check if dome azimuth can be synced
     * @return true if dome azimuth can be synced
     */
    virtual bool GetCanSyncAzimuth() = 0;
    
    /**
     * @brief Get the shutter status
     * @return Current shutter state
     */
    virtual ShutterState GetShutterStatus() = 0;
    
    /**
     * @brief Check if dome is slaved to telescope
     * @return true if dome is slaved to telescope
     */
    virtual bool GetSlaved() = 0;
    
    /**
     * @brief Check if dome is currently slewing
     * @return true if any part of dome is moving
     */
    virtual bool GetSlewing() = 0;

    // ==================== Properties (PUT) ====================
    
    /**
     * @brief Set dome slaving to telescope
     * @param slaved true to enable slaving, false to disable
     */
    virtual void SetSlaved(bool slaved) = 0;

    // ==================== Methods ====================
    
    /**
     * @brief Immediately abort current dome operation
     */
    virtual void AbortSlew() = 0;
    
    /**
     * @brief Close the shutter
     */
    virtual void CloseShutter() = 0;
    
    /**
     * @brief Start search for dome home position
     */
    virtual void FindHome() = 0;
    
    /**
     * @brief Open the shutter
     */
    virtual void OpenShutter() = 0;
    
    /**
     * @brief Move dome to park position
     */
    virtual void Park() = 0;
    
    /**
     * @brief Set current position as park position
     */
    virtual void SetPark() = 0;
    
    /**
     * @brief Slew dome to specified altitude
     * @param altitude Target altitude in degrees (0-90)
     */
    virtual void SlewToAltitude(double altitude) = 0;
    
    /**
     * @brief Slew dome to specified azimuth
     * @param azimuth Target azimuth in degrees (0-360)
     */
    virtual void SlewToAzimuth(double azimuth) = 0;
    
    /**
     * @brief Sync current position to specified azimuth
     * @param azimuth Azimuth in degrees to sync to (0-360)
     */
    virtual void SyncToAzimuth(double azimuth) = 0;
};

#endif // IDOME_H
