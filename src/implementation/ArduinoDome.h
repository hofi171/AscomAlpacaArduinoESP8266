#ifndef MY_DOME_H
#define MY_DOME_H

#include "alpaca_api/Alpaca_Device_Dome.h"
#include "WiFi_Config.h"
#include <EEPROM.h>
#include <ESP8266WiFi.h>

extern WiFiConfig wifiConfig;

/**
 * @file ArduinoDome.h
 * @brief Example implementation of Dome device
 * 
 * This example demonstrates how to create a concrete Dome device
 * by extending the AlpacaDeviceDome class and implementing the
 * dome control logic for rotation, altitude, shutter, and parking.
 */

class ArduinoDome : public AlpacaDeviceDome {
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
  bool hasShutterSensors;
  
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
  int shutterOpenDrivePin;
  int shutterCloseDrivePin;
  int shutterOpenSensorPin;
  int shutterCloseSensorPin;
  int homeSensorPin;

  // EEPROM storage
  static const int EEPROM_DOME_SETTINGS_VALID_ADDR = 200;
  static const int EEPROM_DOME_HOME_AZ_ADDR = 202;
  static const int EEPROM_DOME_PARK_AZ_ADDR = 210;
  static const int EEPROM_DOME_PARK_ALT_ADDR = 218;
  static const int EEPROM_DOME_AZ_SPEED_ADDR = 226;
  static const int EEPROM_DOME_ALT_SPEED_ADDR = 234;
  static const int EEPROM_DOME_SHUTTER_DURATION_ADDR = 242;
  static const int EEPROM_DOME_AZ_STEP_PIN_ADDR = 246;
  static const int EEPROM_DOME_AZ_DIR_PIN_ADDR = 250;
  static const int EEPROM_DOME_AZ_ENABLE_PIN_ADDR = 254;
  static const int EEPROM_DOME_SHUTTER_OPEN_DRIVE_PIN_ADDR = 258;
  static const int EEPROM_DOME_SHUTTER_CLOSE_DRIVE_PIN_ADDR = 262;
  static const int EEPROM_DOME_SHUTTER_OPEN_SENSOR_PIN_ADDR = 266;
  static const int EEPROM_DOME_SHUTTER_CLOSE_SENSOR_PIN_ADDR = 270;
  static const int EEPROM_DOME_HOME_SENSOR_PIN_ADDR = 274;
  static const uint16_t EEPROM_DOME_VALID_MARKER = 0xD06E;

  static double normalizeAzimuth(double azimuth) {
    while (azimuth < 0.0) azimuth += 360.0;
    while (azimuth >= 360.0) azimuth -= 360.0;
    return azimuth;
  }

  static const char* shutterStateToText(ShutterState state) {
    switch (state) {
      case SHUTTER_OPEN: return "Open";
      case SHUTTER_CLOSED: return "Closed";
      case SHUTTER_OPENING: return "Opening";
      case SHUTTER_CLOSING: return "Closing";
      case SHUTTER_ERROR: return "Error";
      default: return "Unknown";
    }
  }

  bool validateLoadedSettings(double loadedHomeAzimuth,
                              double loadedParkAzimuth,
                              double loadedParkAltitude,
                              double loadedAzimuthSpeed,
                              double loadedAltitudeSpeed,
                              unsigned long loadedShutterDuration) {
    if (isnan(loadedHomeAzimuth) || isinf(loadedHomeAzimuth) || loadedHomeAzimuth < 0.0 || loadedHomeAzimuth >= 360.0) return false;
    if (isnan(loadedParkAzimuth) || isinf(loadedParkAzimuth) || loadedParkAzimuth < 0.0 || loadedParkAzimuth >= 360.0) return false;
    if (isnan(loadedParkAltitude) || isinf(loadedParkAltitude) || loadedParkAltitude < 0.0 || loadedParkAltitude > 90.0) return false;
    if (isnan(loadedAzimuthSpeed) || isinf(loadedAzimuthSpeed) || loadedAzimuthSpeed <= 0.0 || loadedAzimuthSpeed > 60.0) return false;
    if (isnan(loadedAltitudeSpeed) || isinf(loadedAltitudeSpeed) || loadedAltitudeSpeed <= 0.0 || loadedAltitudeSpeed > 30.0) return false;
    if (loadedShutterDuration < 1000 || loadedShutterDuration > 300000) return false;
    return true;
  }

  bool isValidPinOrDisabled(int pin) {
    return pin == -1 || (pin >= 0 && pin <= 16);
  }

  bool hasDuplicateActivePins(const int *pins, int count) {
    for (int i = 0; i < count; ++i) {
      if (pins[i] < 0) continue;
      for (int j = i + 1; j < count; ++j) {
        if (pins[i] == pins[j]) return true;
      }
    }
    return false;
  }

  bool validatePinConfiguration(int newAzimuthStepPin,
                                int newAzimuthDirPin,
                                int newAzimuthEnablePin,
                                int newShutterOpenDrivePin,
                                int newShutterCloseDrivePin,
                                int newShutterOpenSensorPin,
                                int newShutterCloseSensorPin,
                                int newHomeSensorPin) {
    if (!isValidPinOrDisabled(newAzimuthStepPin) ||
        !isValidPinOrDisabled(newAzimuthDirPin) ||
        !isValidPinOrDisabled(newAzimuthEnablePin) ||
        !isValidPinOrDisabled(newShutterOpenDrivePin) ||
        !isValidPinOrDisabled(newShutterCloseDrivePin) ||
        !isValidPinOrDisabled(newShutterOpenSensorPin) ||
        !isValidPinOrDisabled(newShutterCloseSensorPin) ||
        !isValidPinOrDisabled(newHomeSensorPin)) {
      return false;
    }

    int pins[] = {
      newAzimuthStepPin,
      newAzimuthDirPin,
      newAzimuthEnablePin,
      newShutterOpenDrivePin,
      newShutterCloseDrivePin,
      newShutterOpenSensorPin,
      newShutterCloseSensorPin,
      newHomeSensorPin
    };

    return !hasDuplicateActivePins(pins, 8);
  }

  void applyPinConfiguration() {
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
      digitalWrite(azimuthEnablePin, HIGH);
    }

    if (shutterOpenDrivePin >= 0) {
      pinMode(shutterOpenDrivePin, OUTPUT);
      digitalWrite(shutterOpenDrivePin, LOW);
    }
    if (shutterCloseDrivePin >= 0) {
      pinMode(shutterCloseDrivePin, OUTPUT);
      digitalWrite(shutterCloseDrivePin, LOW);
    }

    if (shutterOpenSensorPin >= 0) {
      pinMode(shutterOpenSensorPin, INPUT_PULLUP);
    }
    if (shutterCloseSensorPin >= 0) {
      pinMode(shutterCloseSensorPin, INPUT_PULLUP);
    }

    if (homeSensorPin >= 0) {
      pinMode(homeSensorPin, INPUT_PULLUP);
    }

    hasShutterSensors = (shutterOpenSensorPin >= 0 && shutterCloseSensorPin >= 0);
    canFindHome = (homeSensorPin >= 0);
    canSetShutter = (shutterOpenDrivePin >= 0 && shutterCloseDrivePin >= 0);
  }

  void saveSettingsToEEPROM() {
    EEPROM.put(EEPROM_DOME_HOME_AZ_ADDR, homeAzimuth);
    EEPROM.put(EEPROM_DOME_PARK_AZ_ADDR, parkAzimuth);
    EEPROM.put(EEPROM_DOME_PARK_ALT_ADDR, parkAltitude);
    EEPROM.put(EEPROM_DOME_AZ_SPEED_ADDR, azimuthSpeed);
    EEPROM.put(EEPROM_DOME_ALT_SPEED_ADDR, altitudeSpeed);
    EEPROM.put(EEPROM_DOME_SHUTTER_DURATION_ADDR, shutterDuration);
    EEPROM.put(EEPROM_DOME_AZ_STEP_PIN_ADDR, azimuthStepPin);
    EEPROM.put(EEPROM_DOME_AZ_DIR_PIN_ADDR, azimuthDirPin);
    EEPROM.put(EEPROM_DOME_AZ_ENABLE_PIN_ADDR, azimuthEnablePin);
    EEPROM.put(EEPROM_DOME_SHUTTER_OPEN_DRIVE_PIN_ADDR, shutterOpenDrivePin);
    EEPROM.put(EEPROM_DOME_SHUTTER_CLOSE_DRIVE_PIN_ADDR, shutterCloseDrivePin);
    EEPROM.put(EEPROM_DOME_SHUTTER_OPEN_SENSOR_PIN_ADDR, shutterOpenSensorPin);
    EEPROM.put(EEPROM_DOME_SHUTTER_CLOSE_SENSOR_PIN_ADDR, shutterCloseSensorPin);
    EEPROM.put(EEPROM_DOME_HOME_SENSOR_PIN_ADDR, homeSensorPin);
    EEPROM.write(EEPROM_DOME_SETTINGS_VALID_ADDR, (EEPROM_DOME_VALID_MARKER >> 8) & 0xFF);
    EEPROM.write(EEPROM_DOME_SETTINGS_VALID_ADDR + 1, EEPROM_DOME_VALID_MARKER & 0xFF);
    EEPROM.commit();
  }

  void loadSettingsFromEEPROM() {
    uint16_t marker = (EEPROM.read(EEPROM_DOME_SETTINGS_VALID_ADDR) << 8) |
                      EEPROM.read(EEPROM_DOME_SETTINGS_VALID_ADDR + 1);

    if (marker != EEPROM_DOME_VALID_MARKER) {
      saveSettingsToEEPROM();
      LOG_INFO("No valid dome settings found in EEPROM, defaults saved");
      return;
    }

    double loadedHomeAzimuth = 0.0;
    double loadedParkAzimuth = 0.0;
    double loadedParkAltitude = 45.0;
    double loadedAzimuthSpeed = 5.0;
    double loadedAltitudeSpeed = 2.0;
    unsigned long loadedShutterDuration = 10000;
    int loadedAzimuthStepPin = azimuthStepPin;
    int loadedAzimuthDirPin = azimuthDirPin;
    int loadedAzimuthEnablePin = azimuthEnablePin;
    int loadedShutterOpenDrivePin = shutterOpenDrivePin;
    int loadedShutterCloseDrivePin = shutterCloseDrivePin;
    int loadedShutterOpenSensorPin = shutterOpenSensorPin;
    int loadedShutterCloseSensorPin = shutterCloseSensorPin;
    int loadedHomeSensorPin = homeSensorPin;

    EEPROM.get(EEPROM_DOME_HOME_AZ_ADDR, loadedHomeAzimuth);
    EEPROM.get(EEPROM_DOME_PARK_AZ_ADDR, loadedParkAzimuth);
    EEPROM.get(EEPROM_DOME_PARK_ALT_ADDR, loadedParkAltitude);
    EEPROM.get(EEPROM_DOME_AZ_SPEED_ADDR, loadedAzimuthSpeed);
    EEPROM.get(EEPROM_DOME_ALT_SPEED_ADDR, loadedAltitudeSpeed);
    EEPROM.get(EEPROM_DOME_SHUTTER_DURATION_ADDR, loadedShutterDuration);
    EEPROM.get(EEPROM_DOME_AZ_STEP_PIN_ADDR, loadedAzimuthStepPin);
    EEPROM.get(EEPROM_DOME_AZ_DIR_PIN_ADDR, loadedAzimuthDirPin);
    EEPROM.get(EEPROM_DOME_AZ_ENABLE_PIN_ADDR, loadedAzimuthEnablePin);
    EEPROM.get(EEPROM_DOME_SHUTTER_OPEN_DRIVE_PIN_ADDR, loadedShutterOpenDrivePin);
    EEPROM.get(EEPROM_DOME_SHUTTER_CLOSE_DRIVE_PIN_ADDR, loadedShutterCloseDrivePin);
    EEPROM.get(EEPROM_DOME_SHUTTER_OPEN_SENSOR_PIN_ADDR, loadedShutterOpenSensorPin);
    EEPROM.get(EEPROM_DOME_SHUTTER_CLOSE_SENSOR_PIN_ADDR, loadedShutterCloseSensorPin);
    EEPROM.get(EEPROM_DOME_HOME_SENSOR_PIN_ADDR, loadedHomeSensorPin);

    if (!validateLoadedSettings(loadedHomeAzimuth, loadedParkAzimuth, loadedParkAltitude,
                  loadedAzimuthSpeed, loadedAltitudeSpeed, loadedShutterDuration) ||
      !validatePinConfiguration(loadedAzimuthStepPin, loadedAzimuthDirPin, loadedAzimuthEnablePin,
                    loadedShutterOpenDrivePin, loadedShutterCloseDrivePin,
                    loadedShutterOpenSensorPin, loadedShutterCloseSensorPin,
                    loadedHomeSensorPin)) {
      LOG_WARN("Invalid dome settings in EEPROM, keeping defaults and rewriting");
      saveSettingsToEEPROM();
      return;
    }

    homeAzimuth = loadedHomeAzimuth;
    parkAzimuth = loadedParkAzimuth;
    parkAltitude = loadedParkAltitude;
    azimuthSpeed = loadedAzimuthSpeed;
    altitudeSpeed = loadedAltitudeSpeed;
    shutterDuration = loadedShutterDuration;
    azimuthStepPin = loadedAzimuthStepPin;
    azimuthDirPin = loadedAzimuthDirPin;
    azimuthEnablePin = loadedAzimuthEnablePin;
    shutterOpenDrivePin = loadedShutterOpenDrivePin;
    shutterCloseDrivePin = loadedShutterCloseDrivePin;
    shutterOpenSensorPin = loadedShutterOpenSensorPin;
    shutterCloseSensorPin = loadedShutterCloseSensorPin;
    homeSensorPin = loadedHomeSensorPin;
    updatePositionStatus();
    LOG_INFO("Loaded dome settings from EEPROM");
  }

  bool setPinConfiguration(int newAzimuthStepPin,
                           int newAzimuthDirPin,
                           int newAzimuthEnablePin,
                           int newShutterOpenDrivePin,
                           int newShutterCloseDrivePin,
                           int newShutterOpenSensorPin,
                           int newShutterCloseSensorPin,
                           int newHomeSensorPin) {
    if (!validatePinConfiguration(newAzimuthStepPin, newAzimuthDirPin, newAzimuthEnablePin,
                                  newShutterOpenDrivePin, newShutterCloseDrivePin,
                                  newShutterOpenSensorPin, newShutterCloseSensorPin,
                                  newHomeSensorPin)) {
      return false;
    }

    azimuthStepPin = newAzimuthStepPin;
    azimuthDirPin = newAzimuthDirPin;
    azimuthEnablePin = newAzimuthEnablePin;
    shutterOpenDrivePin = newShutterOpenDrivePin;
    shutterCloseDrivePin = newShutterCloseDrivePin;
    shutterOpenSensorPin = newShutterOpenSensorPin;
    shutterCloseSensorPin = newShutterCloseSensorPin;
    homeSensorPin = newHomeSensorPin;

    applyPinConfiguration();
    saveSettingsToEEPROM();
    LOG_INFO("Dome GPIO configuration updated");
    return true;
  }
  
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

      // Check if shutter operation is complete
      if(hasShutterSensors) {
        if (shutterStatus == SHUTTER_OPENING) {
          if (digitalRead(shutterOpenSensorPin) == LOW && digitalRead(shutterCloseDrivePin) == LOW) {
            shutterStatus = SHUTTER_OPEN;
            LOG_DEBUG("Shutter is now OPEN (sensor)");
            // Turn off motor
            if (shutterOpenDrivePin >= 0) digitalWrite(shutterOpenDrivePin, LOW);
          }
        } else if (shutterStatus == SHUTTER_CLOSING) {
          if (digitalRead(shutterCloseSensorPin) == LOW && digitalRead(shutterOpenDrivePin) == LOW) {
            shutterStatus = SHUTTER_CLOSED;
            LOG_DEBUG("Shutter is now CLOSED (sensor)");
            // Turn off motor
            if (shutterCloseDrivePin >= 0) digitalWrite(shutterCloseDrivePin, LOW);
          }
        }
      } else { // Fallback to time-based shutter control if no sensors
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
        if (shutterOpenDrivePin >= 0) digitalWrite(shutterOpenDrivePin, LOW);
        if (shutterCloseDrivePin >= 0) digitalWrite(shutterCloseDrivePin, LOW);
      }
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
   * @brief Constructor for ArduinoDome
   * @param devicename Name of the dome device
   * @param devicenumber Device number for Alpaca API
   * @param description Human-readable description
   * @param server Reference to AsyncWebServer
   * @param has_shutter Does dome have a controllable shutter
   * @param has_altitude_control Does dome have altitude control
   * @param azimuth_step_pin Optional azimuth motor step pin
   * @param azimuth_dir_pin Optional azimuth motor direction pin
   * @param azimuth_enable_pin Optional azimuth motor enable pin
   * @param shutter_open_drive_pin Optional shutter open control pin
   * @param shutter_close_drive_pin Optional shutter close control pin
   * @param shutter_open_pin Optional shutter open sensor pin
   * @param shutter_close_pin Optional shutter close sensor pin
   * @param home_sensor_pin Optional home sensor input pin
   */
  ArduinoDome(String devicename, int devicenumber, String description, 
         AsyncWebServer &server, bool has_shutter = true, bool has_altitude_control = false,
         int azimuth_step_pin = -1, int azimuth_dir_pin = -1, int azimuth_enable_pin = -1,
         int shutter_open_drive_pin = -1, int shutter_close_drive_pin = -1,int shutter_open_pin = -1, int shutter_close_pin = -1, int home_sensor_pin = -1)
    : AlpacaDeviceDome(devicename, devicenumber, description, server, true),
      canFindHome(home_sensor_pin >= 0),
      canPark(true),
      canSetAltitude(has_altitude_control),
      canSetAzimuth(true),
      canSetPark(true),
      canSetShutter(has_shutter),
      canSlave(false),
      canSyncAzimuth(true),
      hasShutterSensors(shutter_open_pin >= 0 && shutter_close_pin >= 0),
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
      shutterOpenDrivePin(shutter_open_drive_pin),
      shutterCloseDrivePin(shutter_close_drive_pin),
      shutterOpenSensorPin(shutter_open_pin),
      shutterCloseSensorPin(shutter_close_pin),
      homeSensorPin(home_sensor_pin) {
    loadSettingsFromEEPROM();
    applyPinConfiguration();
    
    LOG_DEBUG("ArduinoDome created - Shutter: " + String(has_shutter ? "Yes" : "No") + " Altitude: " + String(has_altitude_control ? "Yes" : "No"));
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
      if (shutterOpenDrivePin >= 0) digitalWrite(shutterOpenDrivePin, LOW);
      if (shutterCloseDrivePin >= 0) digitalWrite(shutterCloseDrivePin, LOW);
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
    
    if (shutterCloseDrivePin >= 0) {
      digitalWrite(shutterOpenDrivePin, LOW);
      digitalWrite(shutterCloseDrivePin, HIGH);
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
    
    if (shutterOpenDrivePin >= 0) {
      digitalWrite(shutterCloseDrivePin, LOW);
      digitalWrite(shutterOpenDrivePin, HIGH);
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
    saveSettingsToEEPROM();
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
    
    azimuth = normalizeAzimuth(azimuth);
    
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
    
    azimuth = normalizeAzimuth(azimuth);
    
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
    if (degreesPerSecond <= 0.0 || degreesPerSecond > 60.0) {
      LOG_WARN("Invalid azimuth speed requested: " + String(degreesPerSecond));
      return;
    }

    azimuthSpeed = degreesPerSecond;
    saveSettingsToEEPROM();
    LOG_DEBUG("Azimuth speed set to: " + String(azimuthSpeed) + " deg/sec");
  }
  
  /**
   * @brief Set altitude movement speed
   * @param degreesPerSecond Speed in degrees per second
   */
  void setAltitudeSpeed(double degreesPerSecond) {
    if (degreesPerSecond <= 0.0 || degreesPerSecond > 30.0) {
      LOG_WARN("Invalid altitude speed requested: " + String(degreesPerSecond));
      return;
    }

    altitudeSpeed = degreesPerSecond;
    saveSettingsToEEPROM();
    LOG_DEBUG("Altitude speed set to: " + String(altitudeSpeed) + " deg/sec");
  }
  
  /**
   * @brief Set shutter operation duration
   * @param durationMs Duration in milliseconds
   */
  void setShutterDuration(unsigned long durationMs) {
    if (durationMs < 1000 || durationMs > 300000) {
      LOG_WARN("Invalid shutter duration requested: " + String(durationMs));
      return;
    }

    shutterDuration = durationMs;
    saveSettingsToEEPROM();
    LOG_DEBUG("Shutter duration set to: " + String(shutterDuration) + " ms");
  }

  void setHomeAzimuth(double azimuth) {
    homeAzimuth = normalizeAzimuth(azimuth);
    updatePositionStatus();
    saveSettingsToEEPROM();
    LOG_DEBUG("Home azimuth set to: " + String(homeAzimuth));
  }

  void setParkPosition(double azimuth, double altitude) {
    if (altitude < 0.0 || altitude > 90.0) {
      LOG_WARN("Invalid park altitude requested: " + String(altitude));
      return;
    }

    parkAzimuth = normalizeAzimuth(azimuth);
    parkAltitude = altitude;
    updatePositionStatus();
    saveSettingsToEEPROM();
    LOG_DEBUG("Park position set to Az: " + String(parkAzimuth) + " Alt: " + String(parkAltitude));
  }

  void setCurrentPosition(double azimuth, double altitude) {
    if (altitude < 0.0 || altitude > 90.0) {
      LOG_WARN("Invalid current altitude requested: " + String(altitude));
      return;
    }

    currentAzimuth = normalizeAzimuth(azimuth);
    targetAzimuth = currentAzimuth;
    currentAltitude = altitude;
    targetAltitude = currentAltitude;
    isSlewing = false;
    updatePositionStatus();
    LOG_DEBUG("Current position set to Az: " + String(currentAzimuth) + " Alt: " + String(currentAltitude));
  }

  void setupHandler(AsyncWebServerRequest *request) override {
    LOG_INFO("Received dome setup request - Method: " + String(request->method() == HTTP_POST ? "POST" : "GET"));

    if (request->method() == HTTP_POST) {
      String message;

      if (request->hasParam("home_azimuth", true)) {
        double newHomeAzimuth = request->getParam("home_azimuth", true)->value().toDouble();
        setHomeAzimuth(newHomeAzimuth);
        message += "Home azimuth updated to: " + String(homeAzimuth, 2) + "°<br>";
      }

      if (request->hasParam("park_azimuth", true) && request->hasParam("park_altitude", true)) {
        double newParkAzimuth = request->getParam("park_azimuth", true)->value().toDouble();
        double newParkAltitude = request->getParam("park_altitude", true)->value().toDouble();
        if (newParkAltitude >= 0.0 && newParkAltitude <= 90.0) {
          setParkPosition(newParkAzimuth, newParkAltitude);
          message += "Park position updated to Az " + String(parkAzimuth, 2) + "° / Alt " + String(parkAltitude, 2) + "°<br>";
        } else {
          message += "Error: Park altitude must be 0..90°<br>";
        }
      }

      if (request->hasParam("azimuth_speed", true)) {
        double newAzimuthSpeed = request->getParam("azimuth_speed", true)->value().toDouble();
        if (newAzimuthSpeed > 0.0 && newAzimuthSpeed <= 60.0) {
          setAzimuthSpeed(newAzimuthSpeed);
          message += "Azimuth speed set to: " + String(azimuthSpeed, 2) + "°/s<br>";
        } else {
          message += "Error: Azimuth speed must be > 0 and <= 60°/s<br>";
        }
      }

      if (request->hasParam("altitude_speed", true)) {
        double newAltitudeSpeed = request->getParam("altitude_speed", true)->value().toDouble();
        if (newAltitudeSpeed > 0.0 && newAltitudeSpeed <= 30.0) {
          setAltitudeSpeed(newAltitudeSpeed);
          message += "Altitude speed set to: " + String(altitudeSpeed, 2) + "°/s<br>";
        } else {
          message += "Error: Altitude speed must be > 0 and <= 30°/s<br>";
        }
      }

      if (request->hasParam("shutter_duration", true)) {
        long newShutterDuration = request->getParam("shutter_duration", true)->value().toInt();
        if (newShutterDuration >= 1000 && newShutterDuration <= 300000) {
          setShutterDuration(static_cast<unsigned long>(newShutterDuration));
          message += "Shutter duration set to: " + String(shutterDuration) + " ms<br>";
        } else {
          message += "Error: Shutter duration must be 1000..300000 ms<br>";
        }
      }

      if (request->hasParam("current_azimuth", true) && request->hasParam("current_altitude", true)) {
        double newCurrentAzimuth = request->getParam("current_azimuth", true)->value().toDouble();
        double newCurrentAltitude = request->getParam("current_altitude", true)->value().toDouble();
        if (newCurrentAltitude >= 0.0 && newCurrentAltitude <= 90.0) {
          setCurrentPosition(newCurrentAzimuth, newCurrentAltitude);
          message += "Current position updated to Az " + String(currentAzimuth, 2) + "° / Alt " + String(currentAltitude, 2) + "°<br>";
        } else {
          message += "Error: Current altitude must be 0..90°<br>";
        }
      }

      if (request->hasParam("azimuth_step_pin", true) &&
          request->hasParam("azimuth_dir_pin", true) &&
          request->hasParam("azimuth_enable_pin", true) &&
          request->hasParam("shutter_open_drive_pin", true) &&
          request->hasParam("shutter_close_drive_pin", true) &&
          request->hasParam("shutter_open_sensor_pin", true) &&
          request->hasParam("shutter_close_sensor_pin", true) &&
          request->hasParam("home_sensor_pin", true)) {
        int newAzimuthStepPin = request->getParam("azimuth_step_pin", true)->value().toInt();
        int newAzimuthDirPin = request->getParam("azimuth_dir_pin", true)->value().toInt();
        int newAzimuthEnablePin = request->getParam("azimuth_enable_pin", true)->value().toInt();
        int newShutterOpenDrivePin = request->getParam("shutter_open_drive_pin", true)->value().toInt();
        int newShutterCloseDrivePin = request->getParam("shutter_close_drive_pin", true)->value().toInt();
        int newShutterOpenSensorPin = request->getParam("shutter_open_sensor_pin", true)->value().toInt();
        int newShutterCloseSensorPin = request->getParam("shutter_close_sensor_pin", true)->value().toInt();
        int newHomeSensorPin = request->getParam("home_sensor_pin", true)->value().toInt();

        if (setPinConfiguration(newAzimuthStepPin, newAzimuthDirPin, newAzimuthEnablePin,
                                newShutterOpenDrivePin, newShutterCloseDrivePin,
                                newShutterOpenSensorPin, newShutterCloseSensorPin,
                                newHomeSensorPin)) {
          message += "GPIO configuration updated and saved.<br>";
        } else {
          message += "Error: Invalid pin config. Use -1 (disabled) or GPIO 0..16, without duplicates.<br>";
        }
      }

      if (request->hasParam("wifi_ssid", true) && request->hasParam("wifi_password", true)) {
        String newSSID = request->getParam("wifi_ssid", true)->value();
        String newPassword = request->getParam("wifi_password", true)->value();

        if (newSSID.length() > 0) {
          if (wifiConfig.saveToEEPROM(newSSID, newPassword)) {
            message += "WiFi credentials saved for SSID: " + newSSID + "<br>";
            message += "<strong>Please restart the device to connect to the new network.</strong><br>";
            ESP.reset();
          } else {
            message += "Error: Failed to save WiFi credentials<br>";
          }
        } else {
          message += "Error: WiFi SSID cannot be empty<br>";
        }
      }

      String html = "<html><head><meta http-equiv='refresh' content='2;url=/setup/v1/dome/" +
                    String(GetDeviceNumber()) + "/setup'></head><body>";
      html += "<h1>Dome Setup</h1>";
      html += "<p>" + (message.length() > 0 ? message : String("No changes submitted")) + "</p>";
      html += "<p>Redirecting back to setup page...</p>";
      html += "</body></html>";
      request->send(200, "text/html", html);
      return;
    }

    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>Dome Setup</title>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; }";
    html += "h1 { color: #333; }";
    html += ".container { max-width: 700px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }";
    html += ".info-section { background-color: #e8f4f8; padding: 15px; border-radius: 5px; margin-bottom: 20px; }";
    html += ".form-section { background-color: #f9f9f9; padding: 15px; border-radius: 5px; margin-bottom: 15px; }";
    html += ".info-row { display: flex; justify-content: space-between; margin: 8px 0; }";
    html += ".info-label { font-weight: bold; color: #555; }";
    html += ".info-value { color: #0066cc; }";
    html += "h2 { color: #555; font-size: 1.2em; margin-top: 0; }";
    html += "label { display: block; margin: 10px 0 5px 0; font-weight: bold; }";
    html += "input[type='number'], input[type='text'], input[type='password'] { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }";
    html += "input[type='submit'] { background-color: #0066cc; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; margin-top: 10px; }";
    html += "input[type='submit']:hover { background-color: #0052a3; }";
    html += ".help-text { font-size: 0.9em; color: #666; margin-top: 5px; }";
    html += "</style></head><body><div class='container'>";

    html += "<h1>Dome Setup - " + GetDeviceName() + "</h1>";

    html += "<div class='info-section'><h2>Current Status</h2>";
    html += "<div class='info-row'><span class='info-label'>Azimuth:</span><span class='info-value'>" + String(currentAzimuth, 2) + "°</span></div>";
    html += "<div class='info-row'><span class='info-label'>Altitude:</span><span class='info-value'>" + String(currentAltitude, 2) + "°</span></div>";
    html += "<div class='info-row'><span class='info-label'>Slewing:</span><span class='info-value'>" + String(GetSlewing() ? "Yes" : "No") + "</span></div>";
    html += "<div class='info-row'><span class='info-label'>At Home:</span><span class='info-value'>" + String(atHome ? "Yes" : "No") + "</span></div>";
    html += "<div class='info-row'><span class='info-label'>At Park:</span><span class='info-value'>" + String(atPark ? "Yes" : "No") + "</span></div>";
    html += "<div class='info-row'><span class='info-label'>Shutter:</span><span class='info-value'>" + String(shutterStateToText(shutterStatus)) + "</span></div>";
    html += "<div class='info-row'><span class='info-label'>Shutter Control:</span><span class='info-value'>" + String(canSetShutter ? "Enabled" : "Disabled") + "</span></div>";
    html += "</div>";

    html += "<div class='info-section'><h2>WiFi Status</h2>";
    if (WiFi.getMode() == WIFI_AP) {
      html += "<div class='info-row'><span class='info-label'>Mode:</span><span class='info-value' style='color: #ff9900;'>Access Point (Fallback)</span></div>";
      html += "<div class='info-row'><span class='info-label'>AP SSID:</span><span class='info-value'>" + String(WiFi.softAPSSID()) + "</span></div>";
      html += "<div class='info-row'><span class='info-label'>AP IP:</span><span class='info-value'>" + WiFi.softAPIP().toString() + "</span></div>";
    } else {
      html += "<div class='info-row'><span class='info-label'>Mode:</span><span class='info-value' style='color: #00aa00;'>Station (Connected)</span></div>";
      html += "<div class='info-row'><span class='info-label'>SSID:</span><span class='info-value'>" + String(WiFi.SSID()) + "</span></div>";
      html += "<div class='info-row'><span class='info-label'>IP Address:</span><span class='info-value'>" + WiFi.localIP().toString() + "</span></div>";
      html += "<div class='info-row'><span class='info-label'>Signal:</span><span class='info-value'>" + String(WiFi.RSSI()) + " dBm</span></div>";
    }
    html += "<div class='info-row'><span class='info-label'>Hostname:</span><span class='info-value'>" + String(WiFi.hostname()) + "</span></div>";
    html += "</div>";

    html += "<div class='form-section'><form method='POST' action='/setup/v1/dome/" + String(GetDeviceNumber()) + "/setup'>";
    html += "<h2>Required Dome Settings</h2>";
    html += "<label for='home_azimuth'>Home Azimuth (°):</label>";
    html += "<input type='number' id='home_azimuth' name='home_azimuth' min='0' max='359.99' step='0.01' value='" + String(homeAzimuth, 2) + "' required>";
    html += "<label for='park_azimuth'>Park Azimuth (°):</label>";
    html += "<input type='number' id='park_azimuth' name='park_azimuth' min='0' max='359.99' step='0.01' value='" + String(parkAzimuth, 2) + "' required>";
    html += "<label for='park_altitude'>Park Altitude (°):</label>";
    html += "<input type='number' id='park_altitude' name='park_altitude' min='0' max='90' step='0.01' value='" + String(parkAltitude, 2) + "' required>";
    html += "<label for='azimuth_speed'>Azimuth Speed (°/s):</label>";
    html += "<input type='number' id='azimuth_speed' name='azimuth_speed' min='0.1' max='60' step='0.1' value='" + String(azimuthSpeed, 2) + "' required>";
    html += "<label for='altitude_speed'>Altitude Speed (°/s):</label>";
    html += "<input type='number' id='altitude_speed' name='altitude_speed' min='0.1' max='30' step='0.1' value='" + String(altitudeSpeed, 2) + "' required>";
    html += "<label for='shutter_duration'>Shutter Duration (ms):</label>";
    html += "<input type='number' id='shutter_duration' name='shutter_duration' min='1000' max='300000' step='100' value='" + String(shutterDuration) + "' required>";
    html += "<div class='help-text'>These values are stored in EEPROM and survive reboot.</div>";
    html += "<input type='submit' value='Save Required Dome Settings'>";
    html += "</form></div>";

    html += "<div class='form-section'><form method='POST' action='/setup/v1/dome/" + String(GetDeviceNumber()) + "/setup'>";
    html += "<h2>Set Current Position</h2>";
    html += "<label for='current_azimuth'>Current Azimuth (°):</label>";
    html += "<input type='number' id='current_azimuth' name='current_azimuth' min='0' max='359.99' step='0.01' value='" + String(currentAzimuth, 2) + "' required>";
    html += "<label for='current_altitude'>Current Altitude (°):</label>";
    html += "<input type='number' id='current_altitude' name='current_altitude' min='0' max='90' step='0.01' value='" + String(currentAltitude, 2) + "' required>";
    html += "<div class='help-text'>Use this after manual dome movement or calibration.</div>";
    html += "<input type='submit' value='Set Current Position'>";
    html += "</form></div>";

    html += "<div class='info-section'><h2>GPIO Configuration</h2>";
    html += "<div class='info-row'><span class='info-label'>Azimuth Step Pin:</span><span class='info-value'>" + String(azimuthStepPin) + "</span></div>";
    html += "<div class='info-row'><span class='info-label'>Azimuth Dir Pin:</span><span class='info-value'>" + String(azimuthDirPin) + "</span></div>";
    html += "<div class='info-row'><span class='info-label'>Azimuth Enable Pin:</span><span class='info-value'>" + String(azimuthEnablePin) + "</span></div>";
    html += "<div class='info-row'><span class='info-label'>Shutter Open Drive Pin:</span><span class='info-value'>" + String(shutterOpenDrivePin) + "</span></div>";
    html += "<div class='info-row'><span class='info-label'>Shutter Close Drive Pin:</span><span class='info-value'>" + String(shutterCloseDrivePin) + "</span></div>";
    html += "<div class='info-row'><span class='info-label'>Shutter Open Sensor Pin:</span><span class='info-value'>" + String(shutterOpenSensorPin) + "</span></div>";
    html += "<div class='info-row'><span class='info-label'>Shutter Close Sensor Pin:</span><span class='info-value'>" + String(shutterCloseSensorPin) + "</span></div>";
    html += "<div class='info-row'><span class='info-label'>Home Sensor Pin:</span><span class='info-value'>" + String(homeSensorPin) + "</span></div>";
    html += "</div>";

    html += "<div class='form-section'><form method='POST' action='/setup/v1/dome/" + String(GetDeviceNumber()) + "/setup'>";
    html += "<h2>GPIO Pin Configuration</h2>";
    html += "<label for='azimuth_step_pin'>Azimuth Step Pin (GPIO):</label>";
    html += "<input type='number' id='azimuth_step_pin' name='azimuth_step_pin' min='-1' max='16' step='1' value='" + String(azimuthStepPin) + "' required>";
    html += "<label for='azimuth_dir_pin'>Azimuth Direction Pin (GPIO):</label>";
    html += "<input type='number' id='azimuth_dir_pin' name='azimuth_dir_pin' min='-1' max='16' step='1' value='" + String(azimuthDirPin) + "' required>";
    html += "<label for='azimuth_enable_pin'>Azimuth Enable Pin (GPIO):</label>";
    html += "<input type='number' id='azimuth_enable_pin' name='azimuth_enable_pin' min='-1' max='16' step='1' value='" + String(azimuthEnablePin) + "' required>";
    html += "<label for='shutter_open_drive_pin'>Shutter Open Drive Pin (GPIO):</label>";
    html += "<input type='number' id='shutter_open_drive_pin' name='shutter_open_drive_pin' min='-1' max='16' step='1' value='" + String(shutterOpenDrivePin) + "' required>";
    html += "<label for='shutter_close_drive_pin'>Shutter Close Drive Pin (GPIO):</label>";
    html += "<input type='number' id='shutter_close_drive_pin' name='shutter_close_drive_pin' min='-1' max='16' step='1' value='" + String(shutterCloseDrivePin) + "' required>";
    html += "<label for='shutter_open_sensor_pin'>Shutter Open Sensor Pin (GPIO):</label>";
    html += "<input type='number' id='shutter_open_sensor_pin' name='shutter_open_sensor_pin' min='-1' max='16' step='1' value='" + String(shutterOpenSensorPin) + "' required>";
    html += "<label for='shutter_close_sensor_pin'>Shutter Close Sensor Pin (GPIO):</label>";
    html += "<input type='number' id='shutter_close_sensor_pin' name='shutter_close_sensor_pin' min='-1' max='16' step='1' value='" + String(shutterCloseSensorPin) + "' required>";
    html += "<label for='home_sensor_pin'>Home Sensor Pin (GPIO):</label>";
    html += "<input type='number' id='home_sensor_pin' name='home_sensor_pin' min='-1' max='16' step='1' value='" + String(homeSensorPin) + "' required>";
    html += "<div class='help-text'>Use -1 to disable a pin. Active pins must be unique.</div>";
    html += "<input type='submit' value='Save GPIO Pins'>";
    html += "</form></div>";

    html += "<div class='form-section'><form method='POST' action='/setup/v1/dome/" + String(GetDeviceNumber()) + "/setup'>";
    html += "<h2>WiFi Configuration</h2>";
    html += "<label for='wifi_ssid'>WiFi SSID:</label>";
    html += "<input type='text' id='wifi_ssid' name='wifi_ssid' maxlength='31' value='" + String(wifiConfig.getSSID()) + "' required>";
    html += "<label for='wifi_password'>WiFi Password:</label>";
    html += "<input type='password' id='wifi_password' name='wifi_password' maxlength='63' value='' placeholder='Enter new password or leave empty'>";
    html += "<div class='help-text' style='color: #ff6600;'><strong>Warning:</strong> Device will restart after saving WiFi settings.</div>";
    html += "<input type='submit' value='Save WiFi Settings'>";
    html += "</form></div>";

    html += "<p style='text-align: center; color: #888; font-size: 0.9em; margin-top: 30px;'>";
    html += "<a href='/management/v1/description' style='color: #0066cc; text-decoration: none;'>Back to Management API</a>";
    html += "</p></div></body></html>";

    request->send(200, "text/html", html);
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
