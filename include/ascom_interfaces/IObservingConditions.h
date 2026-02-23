/**
 * @file IObservingConditions.h
 * @brief ASCOM Alpaca ObservingConditions Device Interface
 * 
 * This interface defines the methods and properties for controlling an observing conditions device
 * according to the ASCOM Alpaca API specification.
 * 
 * Reference: https://ascom-standards.org/newdocs/observingconditions.html
 */

#ifndef IOBSERVINGCONDITIONS_H
#define IOBSERVINGCONDITIONS_H

#include <string>

/**
 * @class IObservingConditions
 * @brief Interface for ASCOM Alpaca ObservingConditions device
 * 
 * This pure virtual interface defines all methods required to implement
 * an ASCOM-compliant observing conditions device driver.
 */
class IObservingConditions {
public:
    virtual ~IObservingConditions() {}

    // ==================== Properties ====================
    
    /**
     * @brief Get averaging period
     * @return Time period (hours) over which observations are averaged
     */
    virtual double GetAveragePeriod() = 0;
    
    /**
     * @brief Set averaging period
     * @param hours Time period in hours
     */
    virtual void SetAveragePeriod(double hours) = 0;
    
    /**
     * @brief Get cloud cover
     * @return Amount of sky obscured by cloud (0-100%)
     */
    virtual double GetCloudCover() = 0;
    
    /**
     * @brief Get atmospheric dew point
     * @return Dew point in degrees Celsius
     */
    virtual double GetDewPoint() = 0;
    
    /**
     * @brief Get atmospheric humidity
     * @return Humidity percentage (0-100%)
     */
    virtual double GetHumidity() = 0;
    
    /**
     * @brief Get atmospheric pressure
     * @return Pressure in hectopascals (hPa)
     */
    virtual double GetPressure() = 0;
    
    /**
     * @brief Get rain rate
     * @return Rain rate
     */
    virtual double GetRainRate() = 0;
    
    /**
     * @brief Get sky brightness
     * @return Sky brightness at the observatory
     */
    virtual double GetSkyBrightness() = 0;
    
    /**
     * @brief Get sky quality
     * @return Sky quality at the observatory
     */
    virtual double GetSkyQuality() = 0;
    
    /**
     * @brief Get sky temperature
     * @return Sky temperature in degrees Celsius
     */
    virtual double GetSkyTemperature() = 0;
    
    /**
     * @brief Get star FWHM
     * @return Seeing (star FWHM) in arcseconds
     */
    virtual double GetStarFWHM() = 0;
    
    /**
     * @brief Get temperature
     * @return Temperature in degrees Celsius
     */
    virtual double GetTemperature() = 0;
    
    /**
     * @brief Get wind direction
     * @return Wind direction in degrees (0-360, 0=North)
     */
    virtual double GetWindDirection() = 0;
    
    /**
     * @brief Get wind gust
     * @return Peak 3-second wind gust over last 2 minutes
     */
    virtual double GetWindGust() = 0;
    
    /**
     * @brief Get wind speed
     * @return Wind speed
     */
    virtual double GetWindSpeed() = 0;

    // ==================== Methods ====================
    
    /**
     * @brief Refresh sensor values from hardware
     */
    virtual void Refresh() = 0;
    
    /**
     * @brief Get description of specified sensor
     * @param sensorName Name of the sensor
     * @return Sensor description
     */
    virtual std::string GetSensorDescription(const std::string& sensorName) = 0;
    
    /**
     * @brief Get time since sensor was last updated
     * @param sensorName Name of the sensor
     * @return Time in seconds since last update
     */
    virtual double GetTimeSinceLastUpdate(const std::string& sensorName) = 0;
};

#endif // IOBSERVINGCONDITIONS_H
