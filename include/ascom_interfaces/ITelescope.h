/**
 * @file ITelescope.h
 * @brief ASCOM Alpaca Telescope Device Interface
 * 
 * This interface defines the methods and properties for controlling a telescope mount
 * according to the ASCOM Alpaca API specification.
 * 
 * Reference: https://ascom-standards.org/newdocs/telescope.html
 */

#ifndef ITELESCOPE_H
#define ITELESCOPE_H

#include <string>
#include <vector>
#include "AscomTypes.h"

/**
 * @enum AlignmentMode
 * @brief Enumeration of telescope alignment modes
 */
enum AlignmentMode {
    ALIGNMENT_POLAR = 0,
    ALIGNMENT_ALTAZ = 1,
    ALIGNMENT_GERMAN_POLAR = 2
};

/**
 * @enum EquatorialSystem
 * @brief Enumeration of equatorial coordinate systems
 */
enum EquatorialSystem {
    EQUATORIAL_OTHER = 0,
    EQUATORIAL_TOPOCENTRIC = 1,
    EQUATORIAL_J2000 = 2,
    EQUATORIAL_J2050 = 3,
    EQUATORIAL_B1950 = 4
};

/**
 * @enum PierSide
 * @brief Enumeration of pointing states relative to pier
 */
enum PierSide {
    PIER_EAST = 0,
    PIER_WEST = 1,
    PIER_UNKNOWN = -1
};

/**
 * @enum DriveRate
 * @brief Enumeration of tracking rates
 */
enum DriveRate {
    DRIVE_SIDEREAL = 0,
    DRIVE_LUNAR = 1,
    DRIVE_SOLAR = 2,
    DRIVE_KING = 3
};

/**
 * @enum TelescopeAxis
 * @brief Enumeration of telescope axes
 */
enum TelescopeAxis {
    AXIS_PRIMARY = 0,
    AXIS_SECONDARY = 1,
    AXIS_TERTIARY = 2
};

/**
 * @class ITelescope
 * @brief Interface for ASCOM Alpaca Telescope device
 * 
 * This pure virtual interface defines all methods required to implement
 * an ASCOM-compliant telescope mount driver.
 */
class ITelescope {
public:
    virtual ~ITelescope() {}

    // ==================== Read-Only Properties (GET) ====================
    
    /**
     * @brief Get mount's altitude above horizon
     * @return Altitude in degrees
     */
    virtual double GetAltitude() = 0;
    
    /**
     * @brief Get aperture area
     * @return Aperture area in square meters
     */
    virtual double GetApertureArea() = 0;
    
    /**
     * @brief Get aperture diameter
     * @return Aperture diameter in meters
     */
    virtual double GetApertureDiameter() = 0;
    
    /**
     * @brief Check if mount is at home position
     * @return true if at home position
     */
    virtual bool GetAtHome() = 0;
    
    /**
     * @brief Check if telescope is at park position
     * @return true if at park position
     */
    virtual bool GetAtPark() = 0;
    
    /**
     * @brief Get mount's azimuth
     * @return Azimuth in degrees (North=0, East=90)
     */
    virtual double GetAzimuth() = 0;
    
    /**
     * @brief Get mount's declination
     * @return Declination in degrees
     */
    virtual double GetDeclination() = 0;
    
    /**
     * @brief Check if atmospheric refraction correction is applied
     * @return true if refraction correction is applied
     */
    virtual bool GetDoesRefraction() = 0;
    
    /**
     * @brief Get equatorial coordinate system type
     * @return Equatorial system type
     */
    virtual EquatorialSystem GetEquatorialSystem() = 0;
    
    /**
     * @brief Get focal length
     * @return Focal length in meters
     */
    virtual double GetFocalLength() = 0;
    
    /**
     * @brief Get declination rate offset for guiding
     * @return Rate in degrees per second
     */
    virtual double GetGuideRateDeclination() = 0;
    
    /**
     * @brief Get right ascension rate offset for guiding
     * @return Rate in degrees per second
     */
    virtual double GetGuideRateRightAscension() = 0;
    
    /**
     * @brief Check if executing PulseGuide command
     * @return true if pulse guiding
     */
    virtual bool GetIsPulseGuiding() = 0;
    
    /**
     * @brief Get mount's right ascension
     * @return Right ascension in hours
     */
    virtual double GetRightAscension() = 0;
    
    /**
     * @brief Get mount's pointing state relative to pier
     * @return Pier side
     */
    virtual PierSide GetSideOfPier() = 0;
    
    /**
     * @brief Get local apparent sidereal time
     * @return Sidereal time in hours
     */
    virtual double GetSiderealTime() = 0;
    
    /**
     * @brief Get site elevation above mean sea level
     * @return Elevation in meters
     */
    virtual double GetSiteElevation() = 0;
    
    /**
     * @brief Get site latitude
     * @return Latitude in degrees
     */
    virtual double GetSiteLatitude() = 0;
    
    /**
     * @brief Get site longitude
     * @return Longitude in degrees (East positive, WGS84)
     */
    virtual double GetSiteLongitude() = 0;
    
    /**
     * @brief Check if telescope is currently slewing
     * @return true if slewing
     */
    virtual bool GetSlewing() = 0;
    
    /**
     * @brief Get post-slew settling time
     * @return Settling time in seconds
     */
    virtual int GetSlewSettleTime() = 0;
    
    /**
     * @brief Get target declination
     * @return Target declination in degrees
     */
    virtual double GetTargetDeclination() = 0;
    
    /**
     * @brief Get target right ascension
     * @return Target right ascension in hours
     */
    virtual double GetTargetRightAscension() = 0;
    
    /**
     * @brief Check if telescope is tracking
     * @return true if tracking
     */
    virtual bool GetTracking() = 0;
    
    /**
     * @brief Get current tracking rate
     * @return Tracking rate
     */
    virtual DriveRate GetTrackingRate() = 0;
    
    /**
     * @brief Get collection of supported tracking rates
     * @return Vector of supported drive rates
     */
    virtual std::vector<DriveRate> GetTrackingRates() = 0;
    
    /**
     * @brief Get UTC date/time
     * @return UTC date/time string
     */
    virtual std::string GetUTCDate() = 0;
    
    /**
     * @brief Get current alignment mode
     * @return Alignment mode
     */
    virtual AlignmentMode GetAlignmentMode() = 0;

    // ==================== Capability Properties (GET) ====================
    
    /**
     * @brief Check if can find home position
     * @return true if can find home
     */
    virtual bool GetCanFindHome() = 0;
    
    /**
     * @brief Check if can be parked
     * @return true if can park
     */
    virtual bool GetCanPark() = 0;
    
    /**
     * @brief Check if can be pulse guided
     * @return true if can pulse guide
     */
    virtual bool GetCanPulseGuide() = 0;
    
    /**
     * @brief Check if declination rate can be changed
     * @return true if declination rate can be set
     */
    virtual bool GetCanSetDeclinationRate() = 0;
    
    /**
     * @brief Check if guide rates can be changed
     * @return true if guide rates can be set
     */
    virtual bool GetCanSetGuideRates() = 0;
    
    /**
     * @brief Check if park position can be set
     * @return true if park position can be set
     */
    virtual bool GetCanSetPark() = 0;
    
    /**
     * @brief Check if side of pier can be set
     * @return true if pier side can be set
     */
    virtual bool GetCanSetPierSide() = 0;
    
    /**
     * @brief Check if right ascension rate can be changed
     * @return true if RA rate can be set
     */
    virtual bool GetCanSetRightAscensionRate() = 0;
    
    /**
     * @brief Check if tracking state can be changed
     * @return true if tracking can be set
     */
    virtual bool GetCanSetTracking() = 0;
    
    /**
     * @brief Check if can slew synchronously
     * @return true if can slew
     */
    virtual bool GetCanSlew() = 0;
    
    /**
     * @brief Check if can slew to Alt/Az synchronously
     * @return true if can slew to Alt/Az
     */
    virtual bool GetCanSlewAltAz() = 0;
    
    /**
     * @brief Check if can slew to Alt/Az asynchronously
     * @return true if can slew to Alt/Az async
     */
    virtual bool GetCanSlewAltAzAsync() = 0;
    
    /**
     * @brief Check if can slew asynchronously to equatorial coordinates
     * @return true if can slew async
     */
    virtual bool GetCanSlewAsync() = 0;
    
    /**
     * @brief Check if can sync to equatorial coordinates
     * @return true if can sync
     */
    virtual bool GetCanSync() = 0;
    
    /**
     * @brief Check if can sync to Alt/Az coordinates
     * @return true if can sync to Alt/Az
     */
    virtual bool GetCanSyncAltAz() = 0;
    
    /**
     * @brief Check if can be unparked
     * @return true if can unpark
     */
    virtual bool GetCanUnpark() = 0;

    // ==================== Read/Write Properties ====================
    
    /**
     * @brief Get declination tracking rate
     * @return Declination rate in arcseconds per SI second
     */
    virtual double GetDeclinationRate() = 0;
    
    /**
     * @brief Set declination tracking rate
     * @param rate Rate in arcseconds per SI second
     */
    virtual void SetDeclinationRate(double rate) = 0;
    
    /**
     * @brief Get right ascension tracking rate
     * @return RA rate in arcseconds per sidereal second
     */
    virtual double GetRightAscensionRate() = 0;
    
    /**
     * @brief Set right ascension tracking rate
     * @param rate Rate in arcseconds per sidereal second
     */
    virtual void SetRightAscensionRate(double rate) = 0;
    
    /**
     * @brief Set target declination
     * @param declination Target declination in degrees
     */
    virtual void SetTargetDeclination(double declination) = 0;
    
    /**
     * @brief Set target right ascension
     * @param rightAscension Target right ascension in hours
     */
    virtual void SetTargetRightAscension(double rightAscension) = 0;
    
    /**
     * @brief Set tracking state
     * @param tracking true to enable tracking
     */
    virtual void SetTracking(bool tracking) = 0;
    
    /**
     * @brief Set tracking rate
     * @param trackingRate Tracking rate to use
     */
    virtual void SetTrackingRate(DriveRate trackingRate) = 0;
    
    /**
     * @brief Set UTC date/time
     * @param utcDate UTC date/time string
     */
    virtual void SetUTCDate(const std::string& utcDate) = 0;

    // ==================== Query Methods ====================
    
    /**
     * @brief Get rates at which telescope can move axis
     * @param axis Telescope axis
     * @return Vector of axis rates in degrees per second
     */
    virtual std::vector<Rate> GetAxisRates(TelescopeAxis axis) = 0;
    
    /**
     * @brief Check if can move specified axis
     * @param axis Telescope axis
     * @return true if axis can be moved
     */
    virtual bool GetCanMoveAxis(TelescopeAxis axis) = 0;
    
    /**
     * @brief Predict pointing state after slew to coordinates
     * @param rightAscension Right ascension in hours
     * @param declination Declination in degrees
     * @return Predicted pier side
     */
    virtual PierSide GetDestinationSideOfPier(double rightAscension, double declination) = 0;

    // ==================== Methods ====================
    
    /**
     * @brief Immediately stop slew in progress
     */
    virtual void AbortSlew() = 0;
    
    /**
     * @brief Move mount to home position
     */
    virtual void FindHome() = 0;
    
    /**
     * @brief Move axis at given rate
     * @param axis Telescope axis
     * @param rate Rate in degrees per second
     */
    virtual void MoveAxis(TelescopeAxis axis, double rate) = 0;
    
    /**
     * @brief Park the mount
     */
    virtual void Park() = 0;
    
    /**
     * @brief Pulse guide in specified direction
     * @param direction Guide direction
     * @param duration Duration in milliseconds
     */
    virtual void PulseGuide(GuideDirection direction, int duration) = 0;
    
    /**
     * @brief Set current position as park position
     */
    virtual void SetPark() = 0;
    
    /**
     * @brief Slew to Alt/Az coordinates asynchronously
     * @param altitude Target altitude in degrees
     * @param azimuth Target azimuth in degrees
     */
    virtual void SlewToAltAzAsync(double altitude, double azimuth) = 0;
    
    /**
     * @brief Slew to equatorial coordinates asynchronously
     * @param rightAscension Target right ascension in hours
     * @param declination Target declination in degrees
     */
    virtual void SlewToCoordinatesAsync(double rightAscension, double declination) = 0;
    
    /**
     * @brief Slew to target RA/Dec asynchronously
     */
    virtual void SlewToTargetAsync() = 0;
    
    /**
     * @brief Sync to Alt/Az coordinates
     * @param altitude Altitude in degrees
     * @param azimuth Azimuth in degrees
     */
    virtual void SyncToAltAz(double altitude, double azimuth) = 0;
    
    /**
     * @brief Sync to equatorial coordinates
     * @param rightAscension Right ascension in hours
     * @param declination Declination in degrees
     */
    virtual void SyncToCoordinates(double rightAscension, double declination) = 0;
    
    /**
     * @brief Sync to target RA/Dec
     */
    virtual void SyncToTarget() = 0;
    
    /**
     * @brief Unpark the mount
     */
    virtual void Unpark() = 0;
};

#endif // ITELESCOPE_H
