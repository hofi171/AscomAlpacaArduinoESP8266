/**
 * @file ICamera.h
 * @brief ASCOM Alpaca Camera Device Interface
 * 
 * This interface defines the methods and properties for controlling a camera device
 * according to the ASCOM Alpaca API specification.
 * 
 * Reference: https://ascom-standards.org/newdocs/camera.html
 */

#ifndef ICAMERA_H
#define ICAMERA_H

#include <string>
#include <vector>
#include "AscomTypes.h"

/**
 * @enum CameraState
 * @brief Enumeration of possible camera states
 */
enum CameraState {
    CAMERA_IDLE = 0,
    CAMERA_WAITING = 1,
    CAMERA_EXPOSING = 2,
    CAMERA_READING = 3,
    CAMERA_DOWNLOAD = 4,
    CAMERA_ERROR = 5
};

/**
 * @enum SensorType
 * @brief Enumeration of possible sensor types
 */
enum SensorType {
    SENSOR_MONOCHROME = 0,
    SENSOR_COLOR = 1,
    SENSOR_RGGB = 2,
    SENSOR_CMYG = 3,
    SENSOR_CMYG2 = 4,
    SENSOR_LRGB = 5
};

/**
 * @class ICamera
 * @brief Interface for ASCOM Alpaca Camera device
 * 
 * This pure virtual interface defines all methods required to implement
 * an ASCOM-compliant camera device driver.
 */
class ICamera {
public:
    virtual ~ICamera() {}

    // ==================== Read-Only Properties (GET) ====================
    
    /**
     * @brief Get the X offset of the Bayer matrix
     * @return X offset of the Bayer matrix
     */
    virtual int GetBayerOffsetX() = 0;
    
    /**
     * @brief Get the Y offset of the Bayer matrix
     * @return Y offset of the Bayer matrix
     */
    virtual int GetBayerOffsetY() = 0;
    
    /**
     * @brief Get the camera operational state
     * @return Current camera state
     */
    virtual CameraState GetCameraState() = 0;
    
    /**
     * @brief Get the width of the CCD camera chip
     * @return Width in pixels
     */
    virtual int GetCameraXSize() = 0;
    
    /**
     * @brief Get the height of the CCD camera chip
     * @return Height in pixels
     */
    virtual int GetCameraYSize() = 0;
    
    /**
     * @brief Check if camera can abort exposures
     * @return true if camera can abort exposures
     */
    virtual bool GetCanAbortExposure() = 0;
    
    /**
     * @brief Check if camera supports asymmetric binning
     * @return true if camera supports asymmetric binning
     */
    virtual bool GetCanAsymmetricBin() = 0;
    
    /**
     * @brief Check if camera has fast readout mode
     * @return true if camera has fast readout mode
     */
    virtual bool GetCanFastReadout() = 0;
    
    /**
     * @brief Check if camera's cooler power can be read
     * @return true if cooler power can be read
     */
    virtual bool GetCanGetCoolerPower() = 0;
    
    /**
     * @brief Check if camera supports pulse guiding
     * @return true if camera supports pulse guiding
     */
    virtual bool GetCanPulseGuide() = 0;
    
    /**
     * @brief Check if camera supports setting CCD temperature
     * @return true if camera can set CCD temperature
     */
    virtual bool GetCanSetCCDTemperature() = 0;
    
    /**
     * @brief Check if camera can stop an exposure in progress
     * @return true if camera can stop exposure
     */
    virtual bool GetCanStopExposure() = 0;
    
    /**
     * @brief Get the current CCD temperature
     * @return Temperature in degrees Celsius
     */
    virtual double GetCCDTemperature() = 0;
    
    /**
     * @brief Get the present cooler power level
     * @return Cooler power (0-100%)
     */
    virtual double GetCoolerPower() = 0;
    
    /**
     * @brief Get the gain of the camera
     * @return Gain in photoelectrons per ADU
     */
    virtual double GetElectronsPerADU() = 0;
    
    /**
     * @brief Get maximum exposure time supported
     * @return Maximum exposure time in seconds
     */
    virtual double GetExposureMax() = 0;
    
    /**
     * @brief Get minimum exposure time supported
     * @return Minimum exposure time in seconds
     */
    virtual double GetExposureMin() = 0;
    
    /**
     * @brief Get smallest increment in exposure time supported
     * @return Exposure resolution in seconds
     */
    virtual double GetExposureResolution() = 0;
    
    /**
     * @brief Get full well capacity
     * @return Full well capacity in electrons
     */
    virtual double GetFullWellCapacity() = 0;
    
    /**
     * @brief Get maximum gain value
     * @return Maximum gain value
     */
    virtual int GetGainMax() = 0;
    
    /**
     * @brief Get minimum gain value
     * @return Minimum gain value
     */
    virtual int GetGainMin() = 0;
    
    /**
     * @brief Get list of gain names
     * @return Vector of gain names
     */
    virtual std::vector<std::string> GetGains() = 0;
    
    /**
     * @brief Check if camera has mechanical shutter
     * @return true if camera has shutter
     */
    virtual bool GetHasShutter() = 0;
    
    /**
     * @brief Get current heat sink temperature
     * @return Temperature in degrees Celsius
     */
    virtual double GetHeatSinkTemperature() = 0;
    
    /**
     * @brief Check if image is ready to be downloaded
     * @return true if image is ready
     */
    virtual bool GetImageReady() = 0;
    
    /**
     * @brief Check if camera is pulse guiding
     * @return true if camera is pulse guiding
     */
    virtual bool GetIsPulseGuiding() = 0;
    
    /**
     * @brief Get duration of the last exposure
     * @return Duration in seconds
     */
    virtual double GetLastExposureDuration() = 0;
    
    /**
     * @brief Get start time of last exposure
     * @return Start time in FITS format
     */
    virtual std::string GetLastExposureStartTime() = 0;
    
    /**
     * @brief Get camera's maximum ADU value
     * @return Maximum ADU value
     */
    virtual int GetMaxADU() = 0;
    
    /**
     * @brief Get maximum binning for X axis
     * @return Maximum X binning
     */
    virtual int GetMaxBinX() = 0;
    
    /**
     * @brief Get maximum binning for Y axis
     * @return Maximum Y binning
     */
    virtual int GetMaxBinY() = 0;
    
    /**
     * @brief Get maximum offset value
     * @return Maximum offset value
     */
    virtual int GetOffsetMax() = 0;
    
    /**
     * @brief Get minimum offset value
     * @return Minimum offset value
     */
    virtual int GetOffsetMin() = 0;
    
    /**
     * @brief Get list of offset names
     * @return Vector of offset names
     */
    virtual std::vector<std::string> GetOffsets() = 0;
    
    /**
     * @brief Get percentage completeness of current operation
     * @return Percentage (0-100%)
     */
    virtual int GetPercentCompleted() = 0;
    
    /**
     * @brief Get width of CCD chip pixels
     * @return Pixel width in microns
     */
    virtual double GetPixelSizeX() = 0;
    
    /**
     * @brief Get height of CCD chip pixels
     * @return Pixel height in microns
     */
    virtual double GetPixelSizeY() = 0;
    
    /**
     * @brief Get list of available readout modes
     * @return Vector of readout mode names
     */
    virtual std::vector<std::string> GetReadoutModes() = 0;
    
    /**
     * @brief Get camera's sensor name
     * @return Sensor name
     */
    virtual std::string GetSensorName() = 0;
    
    /**
     * @brief Get sensor type
     * @return Sensor type
     */
    virtual SensorType GetSensorType() = 0;

    // ==================== Read/Write Properties ====================
    
    /**
     * @brief Get binning factor for X axis
     * @return X binning factor
     */
    virtual int GetBinX() = 0;
    
    /**
     * @brief Set binning factor for X axis
     * @param binX Binning factor (minimum 1)
     */
    virtual void SetBinX(int binX) = 0;
    
    /**
     * @brief Get binning factor for Y axis
     * @return Y binning factor
     */
    virtual int GetBinY() = 0;
    
    /**
     * @brief Set binning factor for Y axis
     * @param binY Binning factor (minimum 1)
     */
    virtual void SetBinY(int binY) = 0;
    
    /**
     * @brief Get cooler on/off state
     * @return true if cooler is on
     */
    virtual bool GetCoolerOn() = 0;
    
    /**
     * @brief Set cooler on/off state
     * @param coolerOn true to turn cooler on
     */
    virtual void SetCoolerOn(bool coolerOn) = 0;
    
    /**
     * @brief Get fast readout mode state
     * @return true if fast readout is enabled
     */
    virtual bool GetFastReadout() = 0;
    
    /**
     * @brief Set fast readout mode
     * @param fastReadout true to enable fast readout
     */
    virtual void SetFastReadout(bool fastReadout) = 0;
    
    /**
     * @brief Get camera's gain
     * @return Gain value or index
     */
    virtual int GetGain() = 0;
    
    /**
     * @brief Set camera's gain
     * @param gain Gain value or index into Gains array
     */
    virtual void SetGain(int gain) = 0;
    
    /**
     * @brief Get subframe width in binned pixels
     * @return Subframe width
     */
    virtual int GetNumX() = 0;
    
    /**
     * @brief Set subframe width in binned pixels
     * @param numX Subframe width
     */
    virtual void SetNumX(int numX) = 0;
    
    /**
     * @brief Get subframe height in binned pixels
     * @return Subframe height
     */
    virtual int GetNumY() = 0;
    
    /**
     * @brief Set subframe height in binned pixels
     * @param numY Subframe height
     */
    virtual void SetNumY(int numY) = 0;
    
    /**
     * @brief Get camera's offset
     * @return Offset value or index
     */
    virtual int GetOffset() = 0;
    
    /**
     * @brief Set camera's offset
     * @param offset Offset value or index into Offsets array
     */
    virtual void SetOffset(int offset) = 0;
    
    /**
     * @brief Get camera's readout mode
     * @return Readout mode index
     */
    virtual int GetReadoutMode() = 0;
    
    /**
     * @brief Set camera's readout mode
     * @param readoutMode Index into ReadoutModes array
     */
    virtual void SetReadoutMode(int readoutMode) = 0;
    
    /**
     * @brief Get camera's cooler setpoint
     * @return Setpoint in degrees Celsius
     */
    virtual double GetSetCCDTemperature() = 0;
    
    /**
     * @brief Set camera's cooler setpoint
     * @param temperature Setpoint in degrees Celsius
     */
    virtual void SetSetCCDTemperature(double temperature) = 0;
    
    /**
     * @brief Get subframe X axis start position
     * @return Start X in binned pixels
     */
    virtual int GetStartX() = 0;
    
    /**
     * @brief Set subframe X axis start position
     * @param startX Start X in binned pixels
     */
    virtual void SetStartX(int startX) = 0;
    
    /**
     * @brief Get subframe Y axis start position
     * @return Start Y in binned pixels
     */
    virtual int GetStartY() = 0;
    
    /**
     * @brief Set subframe Y axis start position
     * @param startY Start Y in binned pixels
     */
    virtual void SetStartY(int startY) = 0;
    
    /**
     * @brief Get sub-exposure duration
     * @return Duration in seconds (ICameraV3+)
     */
    virtual double GetSubExposureDuration() = 0;
    
    /**
     * @brief Set sub-exposure duration
     * @param duration Duration in seconds (ICameraV3+)
     */
    virtual void SetSubExposureDuration(double duration) = 0;

    // ==================== Methods ====================
    
    /**
     * @brief Abort the current exposure
     */
    virtual void AbortExposure() = 0;
    
    /**
     * @brief Pulse guide in specified direction
     * @param direction Guide direction
     * @param duration Duration in milliseconds
     */
    virtual void PulseGuide(GuideDirection direction, int duration) = 0;
    
    /**
     * @brief Start an exposure
     * @param duration Exposure duration in seconds
     * @param light true for light frame, false for dark frame
     */
    virtual void StartExposure(double duration, bool light) = 0;
    
    /**
     * @brief Stop the current exposure
     */
    virtual void StopExposure() = 0;
    
    /**
     * @brief Get the image array
     * @return Vector of pixel values (2D flattened array)
     */
    virtual std::vector<int> GetImageArray() = 0;
};

#endif // ICAMERA_H
