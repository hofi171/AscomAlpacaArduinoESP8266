/**
 * @file ICoverCalibrator.h
 * @brief ASCOM Alpaca CoverCalibrator Device Interface
 * 
 * This interface defines the methods and properties for controlling a cover calibrator device
 * according to the ASCOM Alpaca API specification.
 * 
 * Reference: https://ascom-standards.org/newdocs/covercalibrator.html
 */

#ifndef ICOVERCALIBRATOR_H
#define ICOVERCALIBRATOR_H

/**
 * @enum CalibratorStatus
 * @brief Enumeration of possible calibrator states
 */
enum CalibratorStatus {
    CALIBRATOR_NOT_PRESENT = 0,
    CALIBRATOR_OFF = 1,
    CALIBRATOR_NOT_READY = 2,
    CALIBRATOR_READY = 3,
    CALIBRATOR_UNKNOWN = 4,
    CALIBRATOR_ERROR = 5
};

/**
 * @enum CoverStatus
 * @brief Enumeration of possible cover states
 */
enum CoverStatus {
    COVER_NOT_PRESENT = 0,
    COVER_CLOSED = 1,
    COVER_MOVING = 2,
    COVER_OPEN = 3,
    COVER_UNKNOWN = 4,
    COVER_ERROR = 5
};

/**
 * @class ICoverCalibrator
 * @brief Interface for ASCOM Alpaca CoverCalibrator device
 * 
 * This pure virtual interface defines all methods required to implement
 * an ASCOM-compliant cover calibrator device driver.
 */
class ICoverCalibrator {
public:
    virtual ~ICoverCalibrator() {}

    // ==================== Properties (GET) ====================
    
    /**
     * @brief Get current calibrator brightness
     * @return Brightness value (0 to MaxBrightness)
     */
    virtual int GetBrightness() = 0;
    
    /**
     * @brief Check if calibrator brightness is changing
     * @return true if brightness is changing (ICoverCalibratorV2+)
     */
    virtual bool GetCalibratorChanging() = 0;
    
    /**
     * @brief Get state of calibration device
     * @return Current calibrator state
     */
    virtual CalibratorStatus GetCalibratorState() = 0;
    
    /**
     * @brief Check if cover is moving
     * @return true if cover is moving (ICoverCalibratorV2+)
     */
    virtual bool GetCoverMoving() = 0;
    
    /**
     * @brief Get state of device cover
     * @return Current cover state
     */
    virtual CoverStatus GetCoverState() = 0;
    
    /**
     * @brief Get calibrator's maximum brightness value
     * @return Maximum brightness value
     */
    virtual int GetMaxBrightness() = 0;

    // ==================== Methods ====================
    
    /**
     * @brief Turn the calibrator off
     */
    virtual void CalibratorOff() = 0;
    
    /**
     * @brief Turn calibrator on at specified brightness
     * @param brightness Brightness value (0 to MaxBrightness)
     */
    virtual void CalibratorOn(int brightness) = 0;
    
    /**
     * @brief Close the cover
     */
    virtual void CloseCover() = 0;
    
    /**
     * @brief Stop any cover movement
     */
    virtual void HaltCover() = 0;
    
    /**
     * @brief Open the cover
     */
    virtual void OpenCover() = 0;
};

#endif // ICOVERCALIBRATOR_H
