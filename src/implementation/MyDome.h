#ifndef MY_DOME_H
#define MY_DOME_H

#include "alpaca_api/Alpaca_Device_Dome.h"

/**
 * @file MyDome.h
 * @brief Example implementation of Dome device
 * 
 * This example demonstrates how to create a concrete Dome device
 * by extending the AlpacaDeviceDome class and implementing the
 * dome control logic for rotation, altitude, shutter, and parking.
 */

class MyDome : public AlpacaDeviceDome {
private:
  // Dome capabilities
  bool canFindHome;
  bool canPark;
  bool canSetAltitude;
  bool canSetAzimuth;
  bool canSetPark;
  bool canSetShutter;
  bool canSlave;
  bool canSyncAzimuth;
  
  // Current state
  double currentAzimuth;           // Current azimuth in degrees (0-360)
  double currentAltitude;          // Current altitude in degrees (0-90)
  double targetAzimuth;            // Target azimuth for slewing
  double targetAltitude;           // Target altitude for slewing
  bool isSlewing;                  // Is dome currently slewing
  bool slaved;                     // Is dome slaved to telescope
  
  // Position tracking
  double homeAzimuth;              // Home position azimuth
  double parkAzimuth;              // Park position azimuth
  double parkAltitude;             // Park position altitude
  bool atHome;                     // Is dome at home position
  bool atPark;                     // Is dome at park position
  
  // Shutter state
  ShutterState shutterStatus;      // Current shutter state
  unsigned long shutterStartTime;  // Time when shutter operation started
  unsigned long shutterDuration;   // Duration for shutter operation (ms)
  
  // Movement parameters
  double azimuthSpeed;             // Degrees per second
  double altitudeSpeed;            // Degrees per second
  unsigned long lastUpdateTime;    // Time of last position update
  
  // Motor control pins (optional)
  int azimuthStepPin;
  int azimuthDirPin;
  int azimuthEnablePin;
  int shutterOpenPin;
  int shutterClosePin;
  int homeSensorPin;
  
  /**
   * @brief Update dome movement
   */
  void updateMovement() {
    if (!isSlewing) return;
    
    unsigned long currentTime = millis();
    if (lastUpdateTime == 0) {
      lastUpdateTime = currentTime;
      return;
    }
    
    double elapsedSec = (currentTime - lastUpdateTime) / 1000.0;
    lastUpdateTime = currentTime;
    
    // Update azimuth
    if (currentAzimuth != targetAzimuth) {
      double azimuthDiff = targetAzimuth - currentAzimuth;
      
      // Handle wrap-around (take shortest path)
      if (azimuthDiff > 180.0) {
        azimuthDiff -= 360.0;
      } else if (azimuthDiff < -180.0) {
        azimuthDiff += 360.0;
      }
      
      double azimuthMove = azimuthSpeed * elapsedSec;
      
      if (abs(azimuthDiff) <= azimuthMove) {
        currentAzimuth = targetAzimuth;
      } else {
        currentAzimuth += (azimuthDiff > 0 ? azimuthMove : -azimuthMove);
        if (currentAzimuth < 0.0) currentAzimuth += 360.0;
        if (currentAzimuth >= 360.0) currentAzimuth -= 360.0;
      }
      
      // Control azimuth motor
      if (azimuthStepPin >= 0 && azimuthDirPin >= 0) {
        digitalWrite(azimuthDirPin, azimuthDiff > 0 ? HIGH : LOW);
        // Pulse step pin (in real implementation, use proper timing)
      }
    }
    
    // Update altitude
    if (canSetAltitude && currentAltitude != targetAltitude) {
      double altitudeDiff = targetAltitude - currentAltitude;
      double altitudeMove = altitudeSpeed * elapsedSec;
      
      if (abs(altitudeDiff) <= altitudeMove) {
        currentAltitude = targetAltitude;
      } else {
        currentAltitude += (altitudeDiff > 0 ? altitudeMove : -altitudeMove);
        if (currentAltitude < 0.0) currentAltitude = 0.0;
        if (currentAltitude > 90.0) currentAltitude = 90.0;
      }
    }
    
    // Check if slewing is complete
    if (currentAzimuth == targetAzimuth && currentAltitude == targetAltitude) {
      isSlewing = false;
      if (azimuthEnablePin >= 0) {
        digitalWrite(azimuthEnablePin, HIGH); // Disable motor
      }
      LOG_DEBUG("Dome slew complete - Az: " + String(currentAzimuth) + " Alt: " + String(currentAltitude));
      
      // Update home/park status
      updatePositionStatus();
    }
  }
  
  /**
   * @brief Update shutter state machine
   */
  void updateShutter() {
    if (shutterStatus == SHUTTER_OPENING || shutterStatus == SHUTTER_CLOSING) {
      unsigned long currentTime = millis();
      if (currentTime - shutterStartTime >= shutterDuration) {
        // Shutter operation complete
        if (shutterStatus == SHUTTER_OPENING) {
          shutterStatus = SHUTTER_OPEN;
          LOG_DEBUG("Shutter is now OPEN");
        } else {
          shutterStatus = SHUTTER_CLOSED;
          LOG_DEBUG("Shutter is now CLOSED");
        }
        
        // Turn off motor pins
        if (shutterOpenPin >= 0) digitalWrite(shutterOpenPin, LOW);
        if (shutterClosePin >= 0) digitalWrite(shutterClosePin, LOW);
      }
    }
  }
  
  /**
   * @brief Update home/park position status
   */
  void updatePositionStatus() {
    // Check if at home position (within 1 degree)
    double azDiff = abs(currentAzimuth - homeAzimuth);
    if (azDiff > 180.0) azDiff = 360.0 - azDiff;
    atHome = (azDiff < 1.0);
    
    // Check if at park position (within 1 degree)
    azDiff = abs(currentAzimuth - parkAzimuth);
    if (azDiff > 180.0) azDiff = 360.0 - azDiff;
    double altDiff = abs(currentAltitude - parkAltitude);
    atPark = (azDiff < 1.0 && altDiff < 1.0);
  }
  
  /**
   * @brief Check home sensor
   */
  void checkHomeSensor() {
    if (homeSensorPin >= 0 && canFindHome) {
      // Read home sensor (active LOW)
      if (digitalRead(homeSensorPin) == LOW) {
        atHome = true;
        currentAzimuth = homeAzimuth;
        LOG_DEBUG("Home sensor triggered");
      }
    }
  }

public:
  /**
   * @brief Constructor for MyDome
   * @param devicename Name of the dome device
   * @param devicenumber Device number for Alpaca API
   * @param description Human-readable description
   * @param server Reference to AsyncWebServer
   * @param has_shutter Does dome have a controllable shutter
   * @param has_altitude_control Does dome have altitude control
   * @param azimuth_step_pin Optional azimuth motor step pin
   * @param azimuth_dir_pin Optional azimuth motor direction pin
   * @param azimuth_enable_pin Optional azimuth motor enable pin
   * @param shutter_open_pin Optional shutter open control pin
   * @param shutter_close_pin Optional shutter close control pin
   * @param home_sensor_pin Optional home sensor input pin
   */
  MyDome(String devicename, int devicenumber, String description, 
         AsyncWebServer &server, bool has_shutter = true, bool has_altitude_control = false,
         int azimuth_step_pin = -1, int azimuth_dir_pin = -1, int azimuth_enable_pin = -1,
         int shutter_open_pin = -1, int shutter_close_pin = -1, int home_sensor_pin = -1)
    : AlpacaDeviceDome(devicename, devicenumber, description, server),
      canFindHome(home_sensor_pin >= 0),
      canPark(true),
      canSetAltitude(has_altitude_control),
      canSetAzimuth(true),
      canSetPark(true),
      canSetShutter(has_shutter),
      canSlave(false),
      canSyncAzimuth(true),
      currentAzimuth(0.0),
      currentAltitude(45.0),
      targetAzimuth(0.0),
      targetAltitude(45.0),
      isSlewing(false),
      slaved(false),
      homeAzimuth(0.0),
      parkAzimuth(180.0),
      parkAltitude(45.0),
      atHome(true),
      atPark(false),
      shutterStatus(has_shutter ? SHUTTER_CLOSED : SHUTTER_OPEN),
      shutterStartTime(0),
      shutterDuration(10000),  // 10 seconds to open/close
      azimuthSpeed(5.0),       // 5 degrees per second
      altitudeSpeed(2.0),      // 2 degrees per second
      lastUpdateTime(0),
      azimuthStepPin(azimuth_step_pin),
      azimuthDirPin(azimuth_dir_pin),
      azimuthEnablePin(azimuth_enable_pin),
      shutterOpenPin(shutter_open_pin),
      shutterClosePin(shutter_close_pin),
      homeSensorPin(home_sensor_pin) {
    
    // Initialize motor control pins
    if (azimuthStepPin >= 0) {
      pinMode(azimuthStepPin, OUTPUT);
      digitalWrite(azimuthStepPin, LOW);
    }
    if (azimuthDirPin >= 0) {
      pinMode(azimuthDirPin, OUTPUT);
      digitalWrite(azimuthDirPin, LOW);
    }
    if (azimuthEnablePin >= 0) {
      pinMode(azimuthEnablePin, OUTPUT);
      digitalWrite(azimuthEnablePin, HIGH); // Disabled
    }
    
    // Initialize shutter control pins
    if (shutterOpenPin >= 0) {
      pinMode(shutterOpenPin, OUTPUT);
      digitalWrite(shutterOpenPin, LOW);
    }
    if (shutterClosePin >= 0) {
      pinMode(shutterClosePin, OUTPUT);
      digitalWrite(shutterClosePin, LOW);
    }
    
    // Initialize home sensor pin
    if (homeSensorPin >= 0) {
      pinMode(homeSensorPin, INPUT_PULLUP);
    }
    
    LOG_DEBUG("MyDome created - Shutter: " + String(has_shutter ? "Yes" : "No") + " Altitude: " + String(has_altitude_control ? "Yes" : "No"));
  }
  
  // ==================== IDome Interface Implementation ====================
  
  double GetAltitude() override {
    return currentAltitude;
  }
  
  bool GetAtHome() override {
    return atHome;
  }
  
  bool GetAtPark() override {
    return atPark;
  }
  
  double GetAzimuth() override {
    return currentAzimuth;
  }
  
  bool GetCanFindHome() override {
    return canFindHome;
  }
  
  bool GetCanPark() override {
    return canPark;
  }
  
  bool GetCanSetAltitude() override {
    return canSetAltitude;
  }
  
  bool GetCanSetAzimuth() override {
    return canSetAzimuth;
  }
  
  bool GetCanSetPark() override {
    return canSetPark;
  }
  
  bool GetCanSetShutter() override {
    return canSetShutter;
  }
  
  bool GetCanSlave() override {
    return canSlave;
  }
  
  bool GetCanSyncAzimuth() override {
    return canSyncAzimuth;
  }
  
  ShutterState GetShutterStatus() override {
    return shutterStatus;
  }
  
  bool GetSlaved() override {
    return slaved;
  }
  
  bool GetSlewing() override {
    return isSlewing || shutterStatus == SHUTTER_OPENING || shutterStatus == SHUTTER_CLOSING;
  }
  
  void SetSlaved(bool slaved) override {
    this->slaved = slaved;
    LOG_DEBUG("Dome slaved to telescope: " + String(slaved ? "Yes" : "No"));
    
    // In a real implementation with slaving:
    // - Subscribe to telescope position updates
    // - Automatically slew dome to keep aperture aligned with telescope
    // - Calculate dome azimuth from telescope RA/Dec and site coordinates
  }
  
  void AbortSlew() override {
    LOG_DEBUG("Abort slew command received");
    targetAzimuth = currentAzimuth;
    targetAltitude = currentAltitude;
    isSlewing = false;
    
    if (azimuthEnablePin >= 0) {
      digitalWrite(azimuthEnablePin, HIGH); // Disable motor
    }
    
    // Also abort shutter movement
    if (shutterStatus == SHUTTER_OPENING || shutterStatus == SHUTTER_CLOSING) {
      shutterStatus = SHUTTER_ERROR;
      if (shutterOpenPin >= 0) digitalWrite(shutterOpenPin, LOW);
      if (shutterClosePin >= 0) digitalWrite(shutterClosePin, LOW);
      LOG_DEBUG("Shutter movement aborted - state set to ERROR");
    }
  }
  
  void CloseShutter() override {
    if (!canSetShutter) {
      LOG_DEBUG("Shutter control not available");
      return;
    }
    
    if (shutterStatus == SHUTTER_CLOSED) {
      LOG_DEBUG("Shutter already closed");
      return;
    }
    
    LOG_DEBUG("Closing shutter");
    shutterStatus = SHUTTER_CLOSING;
    shutterStartTime = millis();
    
    if (shutterClosePin >= 0) {
      digitalWrite(shutterOpenPin, LOW);
      digitalWrite(shutterClosePin, HIGH);
    }
  }
  
  void FindHome() override {
    if (!canFindHome) {
      LOG_DEBUG("Find home not available");
      return;
    }
    
    LOG_DEBUG("Finding home position");
    
    // In a real implementation:
    // - Start rotating dome slowly
    // - Monitor home sensor
    // - Stop when sensor is triggered
    // - Set currentAzimuth to homeAzimuth
    
    // For simulation, just slew to home position
    SlewToAzimuth(homeAzimuth);
  }
  
  void OpenShutter() override {
    if (!canSetShutter) {
      LOG_DEBUG("Shutter control not available");
      return;
    }
    
    if (shutterStatus == SHUTTER_OPEN) {
      LOG_DEBUG("Shutter already open");
      return;
    }
    
    LOG_DEBUG("Opening shutter");
    shutterStatus = SHUTTER_OPENING;
    shutterStartTime = millis();
    
    if (shutterOpenPin >= 0) {
      digitalWrite(shutterClosePin, LOW);
      digitalWrite(shutterOpenPin, HIGH);
    }
  }
  
  void Park() override {
    if (!canPark) {
      LOG_DEBUG("Park not available");
      return;
    }
    
    LOG_DEBUG("Parking dome at Az: " + String(parkAzimuth) + " Alt: " + String(parkAltitude));
    
    targetAzimuth = parkAzimuth;
    targetAltitude = parkAltitude;
    isSlewing = true;
    lastUpdateTime = millis();
    
    if (azimuthEnablePin >= 0) {
      digitalWrite(azimuthEnablePin, LOW); // Enable motor
    }
  }
  
  void SetPark() override {
    if (!canSetPark) {
      LOG_DEBUG("Set park not available");
      return;
    }
    
    parkAzimuth = currentAzimuth;
    parkAltitude = currentAltitude;
    LOG_DEBUG("Park position set to Az: " + String(parkAzimuth) + " Alt: " + String(parkAltitude));
  }
  
  void SlewToAltitude(double altitude) override {
    if (!canSetAltitude) {
      LOG_DEBUG("Altitude control not available");
      return;
    }
    
    if (altitude < 0.0 || altitude > 90.0) {
      LOG_DEBUG("Invalid altitude: " + String(altitude) + " (valid range: 0-90)");
      return;
    }
    
    LOG_DEBUG("Slewing to altitude: " + String(altitude));
    targetAltitude = altitude;
    isSlewing = true;
    lastUpdateTime = millis();
  }
  
  void SlewToAzimuth(double azimuth) override {
    if (!canSetAzimuth) {
      LOG_DEBUG("Azimuth control not available");
      return;
    }
    
    // Normalize azimuth to 0-360
    while (azimuth < 0.0) azimuth += 360.0;
    while (azimuth >= 360.0) azimuth -= 360.0;
    
    LOG_DEBUG("Slewing to azimuth: " + String(azimuth));
    targetAzimuth = azimuth;
    isSlewing = true;
    lastUpdateTime = millis();
    
    if (azimuthEnablePin >= 0) {
      digitalWrite(azimuthEnablePin, LOW); // Enable motor
    }
  }
  
  void SyncToAzimuth(double azimuth) override {
    if (!canSyncAzimuth) {
      LOG_DEBUG("Sync azimuth not available");
      return;
    }
    
    // Normalize azimuth to 0-360
    while (azimuth < 0.0) azimuth += 360.0;
    while (azimuth >= 360.0) azimuth -= 360.0;
    
    LOG_DEBUG("Syncing to azimuth: " + String(azimuth) + " (was: " + String(currentAzimuth) + ")");
    currentAzimuth = azimuth;
    targetAzimuth = azimuth;
    
    updatePositionStatus();
  }
  
  // ==================== Additional Methods ====================
  
  /**
   * @brief Update dome state
   * Call this periodically from loop() to handle movement and shutter
   */
  void update() {
    updateMovement();
    updateShutter();
    checkHomeSensor();
  }
  
  /**
   * @brief Set azimuth rotation speed
   * @param degreesPerSecond Speed in degrees per second
   */
  void setAzimuthSpeed(double degreesPerSecond) {
    azimuthSpeed = degreesPerSecond;
    LOG_DEBUG("Azimuth speed set to: " + String(azimuthSpeed) + " deg/sec");
  }
  
  /**
   * @brief Set altitude movement speed
   * @param degreesPerSecond Speed in degrees per second
   */
  void setAltitudeSpeed(double degreesPerSecond) {
    altitudeSpeed = degreesPerSecond;
    LOG_DEBUG("Altitude speed set to: " + String(altitudeSpeed) + " deg/sec");
  }
  
  /**
   * @brief Set shutter operation duration
   * @param durationMs Duration in milliseconds
   */
  void setShutterDuration(unsigned long durationMs) {
    shutterDuration = durationMs;
    LOG_DEBUG("Shutter duration set to: " + String(shutterDuration) + " ms");
  }
  
  /**
   * @brief Get current azimuth target
   * @return Target azimuth
   */
  double getTargetAzimuth() const {
    return targetAzimuth;
  }
  
  /**
   * @brief Get current altitude target
   * @return Target altitude
   */
  double getTargetAltitude() const {
    return targetAltitude;
  }
};

#endif // MY_DOME_H
