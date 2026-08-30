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
  int shutterPowerOnPin;       // -1 = disabled; if set, direction pin controls open/close
  int shutterDirectionPin;     // direction pin used when shutterPowerOnPin >= 0
  bool shutterOpenDriveLowActive;   // true = LOW is the active/driving level
  bool shutterCloseDriveLowActive;
  bool shutterPowerOnLowActive;     // true = LOW enables the motor
  bool shutterDirectionLowActive;   // true = LOW means open direction
  bool shutterOpenSensorLowActive;  // true = LOW means shutter is open
  bool shutterCloseSensorLowActive; // true = LOW means shutter is closed
  bool homeSensorLowActive;         // true = LOW means at home position

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
  static const int EEPROM_DOME_SHUTTER_POWER_ON_PIN_ADDR = 278;
  static const int EEPROM_DOME_SHUTTER_DIR_PIN_ADDR = 282;
  static const int EEPROM_DOME_SHUTTER_POLARITY_ADDR = 286; // packed nibble: bits 0-3 = openDrive/closeDrive/powerOn/direction low-active
  static const uint16_t EEPROM_DOME_VALID_MARKER = 0xD06F;

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
    if (pin == -1) return true;
    if (pin < 0 || pin > 16) return false;
    if (pin >= 6 && pin <= 11) return false; // GPIO 6-11 are connected to the SPI flash chip
    return true;
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
                                int newHomeSensorPin,
                                int newShutterPowerOnPin,
                                int newShutterDirectionPin) {
    if (!isValidPinOrDisabled(newAzimuthStepPin) ||
        !isValidPinOrDisabled(newAzimuthDirPin) ||
        !isValidPinOrDisabled(newAzimuthEnablePin) ||
        !isValidPinOrDisabled(newShutterOpenDrivePin) ||
        !isValidPinOrDisabled(newShutterCloseDrivePin) ||
        !isValidPinOrDisabled(newShutterOpenSensorPin) ||
        !isValidPinOrDisabled(newShutterCloseSensorPin) ||
        !isValidPinOrDisabled(newHomeSensorPin) ||
        !isValidPinOrDisabled(newShutterPowerOnPin) ||
        !isValidPinOrDisabled(newShutterDirectionPin)) {
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
      newHomeSensorPin,
      newShutterPowerOnPin,
      newShutterDirectionPin
    };

    return !hasDuplicateActivePins(pins, 10);
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
      digitalWrite(shutterOpenDrivePin, shutterOpenDriveLowActive ? HIGH : LOW); // inactive
    }
    if (shutterCloseDrivePin >= 0) {
      pinMode(shutterCloseDrivePin, OUTPUT);
      digitalWrite(shutterCloseDrivePin, shutterCloseDriveLowActive ? HIGH : LOW); // inactive
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

    if (shutterPowerOnPin >= 0) {
      pinMode(shutterPowerOnPin, OUTPUT);
      digitalWrite(shutterPowerOnPin, shutterPowerOnLowActive ? HIGH : LOW); // off
    }
    if (shutterPowerOnPin >= 0 && shutterDirectionPin >= 0) {
      pinMode(shutterDirectionPin, OUTPUT);
      digitalWrite(shutterDirectionPin, LOW);
    }

    hasShutterSensors = (shutterOpenSensorPin >= 0 && shutterCloseSensorPin >= 0);
    canFindHome = (homeSensorPin >= 0);
    canSetShutter = shutterPowerOnPin >= 0
      ? (shutterDirectionPin >= 0)
      : (shutterOpenDrivePin >= 0 && shutterCloseDrivePin >= 0);
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
    EEPROM.put(EEPROM_DOME_SHUTTER_POWER_ON_PIN_ADDR, shutterPowerOnPin);
    EEPROM.put(EEPROM_DOME_SHUTTER_DIR_PIN_ADDR, shutterDirectionPin);
    EEPROM.write(EEPROM_DOME_SHUTTER_POLARITY_ADDR,
      (shutterOpenDriveLowActive ? 0x01 : 0) | (shutterCloseDriveLowActive ? 0x02 : 0) |
      (shutterPowerOnLowActive     ? 0x04 : 0) | (shutterDirectionLowActive   ? 0x08 : 0) |
      (shutterOpenSensorLowActive  ? 0x10 : 0) | (shutterCloseSensorLowActive ? 0x20 : 0) |
      (homeSensorLowActive         ? 0x40 : 0));
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
    int loadedShutterPowerOnPin = shutterPowerOnPin;
    int loadedShutterDirectionPin = shutterDirectionPin;

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
    EEPROM.get(EEPROM_DOME_SHUTTER_POWER_ON_PIN_ADDR, loadedShutterPowerOnPin);
    EEPROM.get(EEPROM_DOME_SHUTTER_DIR_PIN_ADDR, loadedShutterDirectionPin);

    if (!validateLoadedSettings(loadedHomeAzimuth, loadedParkAzimuth, loadedParkAltitude,
                  loadedAzimuthSpeed, loadedAltitudeSpeed, loadedShutterDuration) ||
      !validatePinConfiguration(loadedAzimuthStepPin, loadedAzimuthDirPin, loadedAzimuthEnablePin,
                    loadedShutterOpenDrivePin, loadedShutterCloseDrivePin,
                    loadedShutterOpenSensorPin, loadedShutterCloseSensorPin,
                    loadedHomeSensorPin, loadedShutterPowerOnPin,
                    loadedShutterDirectionPin)) {
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
    shutterPowerOnPin = loadedShutterPowerOnPin;
    shutterDirectionPin = loadedShutterDirectionPin;
    uint8_t polarity = EEPROM.read(EEPROM_DOME_SHUTTER_POLARITY_ADDR);
    shutterOpenDriveLowActive  = (polarity & 0x01) != 0;
    shutterCloseDriveLowActive = (polarity & 0x02) != 0;
    shutterPowerOnLowActive    = (polarity & 0x04) != 0;
    shutterDirectionLowActive  = (polarity & 0x08) != 0;
    shutterOpenSensorLowActive  = (polarity & 0x10) != 0;
    shutterCloseSensorLowActive = (polarity & 0x20) != 0;
    homeSensorLowActive         = (polarity & 0x40) != 0;
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
                           int newHomeSensorPin,
                           int newShutterPowerOnPin,
                           int newShutterDirectionPin) {
    if (!validatePinConfiguration(newAzimuthStepPin, newAzimuthDirPin, newAzimuthEnablePin,
                                  newShutterOpenDrivePin, newShutterCloseDrivePin,
                                  newShutterOpenSensorPin, newShutterCloseSensorPin,
                                  newHomeSensorPin, newShutterPowerOnPin,
                                  newShutterDirectionPin)) {
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
    shutterPowerOnPin = newShutterPowerOnPin;
    shutterDirectionPin = newShutterDirectionPin;
    // polarity flags are unchanged; update separately via setPolarity()

    applyPinConfiguration();
    saveSettingsToEEPROM();
    LOG_INFO("Dome GPIO configuration updated");
    return true;
  }

  void setPolarity(bool newOpenDriveLowActive, bool newCloseDriveLowActive,
                   bool newPowerOnLowActive, bool newDirectionLowActive,
                   bool newOpenSensorLowActive, bool newCloseSensorLowActive,
                   bool newHomeSensorLowActive) {
    shutterOpenDriveLowActive  = newOpenDriveLowActive;
    shutterCloseDriveLowActive = newCloseDriveLowActive;
    shutterPowerOnLowActive    = newPowerOnLowActive;
    shutterDirectionLowActive  = newDirectionLowActive;
    shutterOpenSensorLowActive  = newOpenSensorLowActive;
    shutterCloseSensorLowActive = newCloseSensorLowActive;
    homeSensorLowActive         = newHomeSensorLowActive;
    applyPinConfiguration();
    saveSettingsToEEPROM();
    LOG_INFO("Shutter output polarity updated");
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
      LOG_INFO("Dome slew complete - Az: " + String(currentAzimuth) + " Alt: " + String(currentAltitude));
      
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

        switch (shutterStatus)
        {
        case SHUTTER_OPEN:
          LOG_INFO("Shutter is OPEN, checking open sensor");
          if (digitalRead(shutterOpenSensorPin) != (shutterOpenSensorLowActive ? LOW : HIGH)) {
            shutterStatus = SHUTTER_ERROR;
            LOG_ERROR("Shutter OPEN sensor not active, error");
          }
          break;
        case SHUTTER_OPENING:
          LOG_INFO("Shutter is OPENING, checking open sensor");
          if (digitalRead(shutterOpenSensorPin) == (shutterOpenSensorLowActive ? LOW : HIGH)) {
            shutterStatus = SHUTTER_OPEN;
            LOG_INFO("Shutter is now OPEN (sensor)");
            if (shutterPowerOnPin >= 0) {
              LOG_INFO("Turning off shutter power");
              digitalWrite(shutterPowerOnPin, shutterPowerOnLowActive ? HIGH : LOW);
            } else if (shutterOpenDrivePin >= 0) {
              digitalWrite(shutterOpenDrivePin, shutterOpenDriveLowActive ? HIGH : LOW);
            }
          }
          break;
        case SHUTTER_CLOSING:
        LOG_INFO("Shutter is CLOSING, checking close sensor");
          if (digitalRead(shutterCloseSensorPin) == (shutterCloseSensorLowActive ? LOW : HIGH)) {
            shutterStatus = SHUTTER_CLOSED;
            LOG_INFO("Shutter is now CLOSED (sensor)");
            if (shutterPowerOnPin >= 0) {
              LOG_INFO("Turning off shutter power");
              digitalWrite(shutterPowerOnPin, shutterPowerOnLowActive ? HIGH : LOW);
            } else if (shutterCloseDrivePin >= 0) {
              digitalWrite(shutterCloseDrivePin, shutterCloseDriveLowActive ? HIGH : LOW);
            }
          }
          case SHUTTER_CLOSED:
            LOG_INFO("Shutter is CLOSED, checking close sensor");
            if (digitalRead(shutterCloseSensorPin) != (shutterCloseSensorLowActive ? LOW : HIGH)) {
              shutterStatus = SHUTTER_ERROR;
              LOG_ERROR("Shutter CLOSE sensor not active, error");
            }
            break;
          case SHUTTER_ERROR:
            LOG_ERROR("Shutter is in ERROR state, manual intervention required");
            break;
          default:
            LOG_ERROR("Unknown shutter state - set to ERROR");
            shutterStatus = SHUTTER_ERROR;
            break;
        }
      } else { // Fallback to time-based shutter control if no sensors
      if (currentTime - shutterStartTime >= shutterDuration) {
        // Shutter operation complete
        if (shutterStatus == SHUTTER_OPENING) {
          shutterStatus = SHUTTER_OPEN;
          LOG_INFO("Shutter is now OPEN");
        } else {
          shutterStatus = SHUTTER_CLOSED;
          LOG_INFO("Shutter is now CLOSED");
        }
        
        // Turn off motor
        if (shutterPowerOnPin >= 0) {
          digitalWrite(shutterPowerOnPin, shutterPowerOnLowActive ? HIGH : LOW);
        } else {
          if (shutterOpenDrivePin >= 0) digitalWrite(shutterOpenDrivePin, shutterOpenDriveLowActive ? HIGH : LOW);
          if (shutterCloseDrivePin >= 0) digitalWrite(shutterCloseDrivePin, shutterCloseDriveLowActive ? HIGH : LOW);
        }
      }
    }

    // Keep the reported state synchronized with stationary end sensors.
    if (hasShutterSensors && shutterStatus != SHUTTER_OPENING && shutterStatus != SHUTTER_CLOSING) {
      bool openSensorActive = digitalRead(shutterOpenSensorPin) == (shutterOpenSensorLowActive ? LOW : HIGH);
      bool closeSensorActive = digitalRead(shutterCloseSensorPin) == (shutterCloseSensorLowActive ? LOW : HIGH);

      if (openSensorActive && !closeSensorActive) {
        shutterStatus = SHUTTER_OPEN;
      } else if (closeSensorActive && !openSensorActive) {
        shutterStatus = SHUTTER_CLOSED;
      } else if (openSensorActive && closeSensorActive) {
        shutterStatus = SHUTTER_ERROR;
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
      if (digitalRead(homeSensorPin) == (homeSensorLowActive ? LOW : HIGH)) {
        atHome = true;
        currentAzimuth = homeAzimuth;
        LOG_INFO("Home sensor triggered");
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
   * @param shutter_power_on_pin Optional motor power enable pin for shutter (direction mode)
   * @param shutter_direction_pin Optional direction pin used when shutter_power_on_pin is set
   */
  ArduinoDome(String devicename, int devicenumber, String description, 
         AsyncWebServer &server, bool has_shutter = true, bool has_altitude_control = false,
         int azimuth_step_pin = -1, int azimuth_dir_pin = -1, int azimuth_enable_pin = -1,
         int shutter_open_drive_pin = -1, int shutter_close_drive_pin = -1,int shutter_open_pin = -1, int shutter_close_pin = -1, int home_sensor_pin = -1,
         int shutter_power_on_pin = -1, int shutter_direction_pin = -1,
         bool open_drive_low_active = false, bool close_drive_low_active = false,
         bool power_on_low_active = false, bool direction_low_active = false,
         bool open_sensor_low_active = true, bool close_sensor_low_active = true,
         bool home_sensor_low_active = true)
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
      homeSensorPin(home_sensor_pin),
      shutterPowerOnPin(shutter_power_on_pin),
      shutterDirectionPin(shutter_direction_pin),
      shutterOpenDriveLowActive(open_drive_low_active),
      shutterCloseDriveLowActive(close_drive_low_active),
      shutterPowerOnLowActive(power_on_low_active),
      shutterDirectionLowActive(direction_low_active),
      shutterOpenSensorLowActive(open_sensor_low_active),
      shutterCloseSensorLowActive(close_sensor_low_active),
      homeSensorLowActive(home_sensor_low_active) {
    loadSettingsFromEEPROM();
    applyPinConfiguration();
    
    LOG_INFO("ArduinoDome created - Shutter: " + String(has_shutter ? "Yes" : "No") + " Altitude: " + String(has_altitude_control ? "Yes" : "No"));
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
    LOG_INFO("Dome slaved to telescope: " + String(slaved ? "Yes" : "No"));
    
    // In a real implementation with slaving:
    // - Subscribe to telescope position updates
    // - Automatically slew dome to keep aperture aligned with telescope
    // - Calculate dome azimuth from telescope RA/Dec and site coordinates
  }
  
  void AbortSlew() override {
    LOG_INFO("Abort slew command received");
    targetAzimuth = currentAzimuth;
    targetAltitude = currentAltitude;
    isSlewing = false;
    
    if (azimuthEnablePin >= 0) {
      digitalWrite(azimuthEnablePin, HIGH); // Disable motor
    }
    
    // Also abort shutter movement
    if (shutterStatus == SHUTTER_OPENING || shutterStatus == SHUTTER_CLOSING) {
      shutterStatus = SHUTTER_ERROR;
      if (shutterPowerOnPin >= 0) digitalWrite(shutterPowerOnPin, shutterPowerOnLowActive ? HIGH : LOW);
      if (shutterDirectionPin >= 0) digitalWrite(shutterDirectionPin, LOW);
      if (shutterOpenDrivePin >= 0) digitalWrite(shutterOpenDrivePin, shutterOpenDriveLowActive ? HIGH : LOW);
      if (shutterCloseDrivePin >= 0) digitalWrite(shutterCloseDrivePin, shutterCloseDriveLowActive ? HIGH : LOW);
      LOG_INFO("Shutter movement aborted - state set to ERROR");
    }
  }
  
  void CloseShutter() override {
    if (!canSetShutter) {
      LOG_INFO("Shutter control not available");
      return;
    }
    
    if (shutterStatus == SHUTTER_CLOSED) {
      LOG_INFO("Shutter already closed");
      return;
    }
    
    LOG_INFO("Closing shutter");
    shutterStatus = SHUTTER_CLOSING;
    shutterStartTime = millis();
    
    if (shutterPowerOnPin >= 0) {
      if (shutterDirectionPin >= 0) digitalWrite(shutterDirectionPin, shutterDirectionLowActive ? HIGH : LOW); // close direction
      digitalWrite(shutterPowerOnPin, shutterPowerOnLowActive ? LOW : HIGH); // enable
    } else if (shutterCloseDrivePin >= 0) {
      digitalWrite(shutterOpenDrivePin, shutterOpenDriveLowActive ? HIGH : LOW); // inactive
      digitalWrite(shutterCloseDrivePin, shutterCloseDriveLowActive ? LOW : HIGH); // active
    }
  }
  
  void FindHome() override {
    if (!canFindHome) {
      LOG_INFO("Find home not available");
      return;
    }
    
    LOG_INFO("Finding home position");
    
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
      LOG_INFO("Shutter control not available");
      return;
    }
    
    if (shutterStatus == SHUTTER_OPEN) {
      LOG_INFO("Shutter already open");
      return;
    }
    
    LOG_INFO("Opening shutter");
    shutterStatus = SHUTTER_OPENING;
    shutterStartTime = millis();
    
    if (shutterPowerOnPin >= 0) {
      if (shutterDirectionPin >= 0) digitalWrite(shutterDirectionPin, shutterDirectionLowActive ? LOW : HIGH); // open direction
      digitalWrite(shutterPowerOnPin, shutterPowerOnLowActive ? LOW : HIGH); // enable
    } else if (shutterOpenDrivePin >= 0) {
      digitalWrite(shutterCloseDrivePin, shutterCloseDriveLowActive ? HIGH : LOW); // inactive
      digitalWrite(shutterOpenDrivePin, shutterOpenDriveLowActive ? LOW : HIGH); // active
    }
  }
  
  void Park() override {
    if (!canPark) {
      LOG_INFO("Park not available");
      return;
    }
    
    LOG_INFO("Parking dome at Az: " + String(parkAzimuth) + " Alt: " + String(parkAltitude));
    
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
      LOG_INFO("Set park not available");
      return;
    }
    
    parkAzimuth = currentAzimuth;
    parkAltitude = currentAltitude;
    saveSettingsToEEPROM();
    LOG_INFO("Park position set to Az: " + String(parkAzimuth) + " Alt: " + String(parkAltitude));
  }
  
  void SlewToAltitude(double altitude) override {
    if (!canSetAltitude) {
      LOG_INFO("Altitude control not available");
      return;
    }
    
    if (altitude < 0.0 || altitude > 90.0) {
      LOG_INFO("Invalid altitude: " + String(altitude) + " (valid range: 0-90)");
      return;
    }
    
    LOG_INFO("Slewing to altitude: " + String(altitude));
    targetAltitude = altitude;
    isSlewing = true;
    lastUpdateTime = millis();
  }
  
  void SlewToAzimuth(double azimuth) override {
    if (!canSetAzimuth) {
      LOG_INFO("Azimuth control not available");
      return;
    }
    
    azimuth = normalizeAzimuth(azimuth);
    
    LOG_INFO("Slewing to azimuth: " + String(azimuth));
    targetAzimuth = azimuth;
    isSlewing = true;
    lastUpdateTime = millis();
    
    if (azimuthEnablePin >= 0) {
      digitalWrite(azimuthEnablePin, LOW); // Enable motor
    }
  }
  
  void SyncToAzimuth(double azimuth) override {
    if (!canSyncAzimuth) {
      LOG_INFO("Sync azimuth not available");
      return;
    }
    
    azimuth = normalizeAzimuth(azimuth);
    
    LOG_INFO("Syncing to azimuth: " + String(azimuth) + " (was: " + String(currentAzimuth) + ")");
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
    LOG_INFO("Azimuth speed set to: " + String(azimuthSpeed) + " deg/sec");
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
    LOG_INFO("Altitude speed set to: " + String(altitudeSpeed) + " deg/sec");
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
    LOG_INFO("Shutter duration set to: " + String(shutterDuration) + " ms");
  }

  void setHomeAzimuth(double azimuth) {
    homeAzimuth = normalizeAzimuth(azimuth);
    updatePositionStatus();
    saveSettingsToEEPROM();
    LOG_INFO("Home azimuth set to: " + String(homeAzimuth));
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
    LOG_INFO("Park position set to Az: " + String(parkAzimuth) + " Alt: " + String(parkAltitude));
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
    LOG_INFO("Current position set to Az: " + String(currentAzimuth) + " Alt: " + String(currentAltitude));
  }

  void setupHandler(AsyncWebServerRequest *request) override {
    LOG_INFO("Received dome setup request - Method: " + String(request->method() == HTTP_POST ? "POST" : "GET"));

    String baseUrl = "/setup/v1/dome/" + String(GetDeviceNumber()) + "/setup";

    if (request->method() == HTTP_POST) {
      String message;

      // Carry ?page=... from the form action URL back into the redirect
      String redirectSuffix = "";
      if (request->hasParam("page")) {
        String p = request->getParam("page")->value();
        if (p == "settings" || p == "gpio" || p == "polarity") redirectSuffix = "?page=" + p;
      }

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
          request->hasParam("home_sensor_pin", true) &&
          request->hasParam("shutter_power_on_pin", true) &&
          request->hasParam("shutter_direction_pin", true)) {
        int newAzimuthStepPin = request->getParam("azimuth_step_pin", true)->value().toInt();
        int newAzimuthDirPin = request->getParam("azimuth_dir_pin", true)->value().toInt();
        int newAzimuthEnablePin = request->getParam("azimuth_enable_pin", true)->value().toInt();
        int newShutterOpenDrivePin = request->getParam("shutter_open_drive_pin", true)->value().toInt();
        int newShutterCloseDrivePin = request->getParam("shutter_close_drive_pin", true)->value().toInt();
        int newShutterOpenSensorPin = request->getParam("shutter_open_sensor_pin", true)->value().toInt();
        int newShutterCloseSensorPin = request->getParam("shutter_close_sensor_pin", true)->value().toInt();
        int newHomeSensorPin = request->getParam("home_sensor_pin", true)->value().toInt();
        int newShutterPowerOnPin = request->getParam("shutter_power_on_pin", true)->value().toInt();
        int newShutterDirectionPin = request->getParam("shutter_direction_pin", true)->value().toInt();

        if (setPinConfiguration(newAzimuthStepPin, newAzimuthDirPin, newAzimuthEnablePin,
                                newShutterOpenDrivePin, newShutterCloseDrivePin,
                                newShutterOpenSensorPin, newShutterCloseSensorPin,
                                newHomeSensorPin, newShutterPowerOnPin,
                                newShutterDirectionPin)) {
          message += "GPIO configuration updated and saved.<br>";
        } else {
          message += "Error: Invalid pin config. Use -1 (disabled) or GPIO 0..16, without duplicates.<br>";
        }
      }

      if (request->hasParam("polarity_update", true)) {
        bool newOpenDriveLowActive  = request->hasParam("open_drive_low_active", true);
        bool newCloseDriveLowActive = request->hasParam("close_drive_low_active", true);
        bool newPowerOnLowActive    = request->hasParam("power_on_low_active", true);
        bool newDirectionLowActive  = request->hasParam("direction_low_active", true);
        bool newOpenSensorLowActive  = request->hasParam("open_sensor_low_active", true);
        bool newCloseSensorLowActive = request->hasParam("close_sensor_low_active", true);
        bool newHomeSensorLowActive  = request->hasParam("home_sensor_low_active", true);
        setPolarity(newOpenDriveLowActive, newCloseDriveLowActive, newPowerOnLowActive, newDirectionLowActive,
                    newOpenSensorLowActive, newCloseSensorLowActive, newHomeSensorLowActive);
        message += "Output polarity updated and saved.<br>";
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

      String html = "<html><head><meta http-equiv='refresh' content='2;url=" + baseUrl + redirectSuffix + "'></head><body>";
      html += "<h1>Dome Setup</h1>";
      html += "<p>" + (message.length() > 0 ? message : String("No changes submitted")) + "</p>";
      html += "<p>Redirecting back to setup page...</p>";
      html += "</body></html>";
      request->send(200, "text/html", html);
      return;
    }

    // --- GET ---
    String page = "";
    if (request->hasParam("page")) page = request->getParam("page")->value();

    // Pre-allocate enough for the largest page so cbuf::resizeAdd() is never called.
    // Multiple resizeAdd() cycles between requests fragment the heap and cause abort() on new char[].
    AsyncResponseStream *response = request->beginResponseStream("text/html", 7000);

    // Shared head / CSS (streamed directly — never held in one large String)
    response->print("<!DOCTYPE html><html><head>");
    response->print("<meta charset='UTF-8'>");
    response->print("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    response->print("<style>");
    response->print("body{font-family:Arial,sans-serif;margin:20px;background-color:#f0f0f0;}");
    response->print("h1{color:#333;}");
    response->print(".container{max-width:700px;margin:0 auto;background-color:white;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,0.1);}");
    response->print(".info-section{background-color:#e8f4f8;padding:15px;border-radius:5px;margin-bottom:20px;}");
    response->print(".form-section{background-color:#f9f9f9;padding:15px;border-radius:5px;margin-bottom:15px;}");
    response->print(".info-row{display:flex;justify-content:space-between;margin:8px 0;}");
    response->print(".info-label{font-weight:bold;color:#555;}");
    response->print(".info-value{color:#0066cc;}");
    response->print("h2{color:#555;font-size:1.2em;margin-top:0;}");
    response->print("label{display:block;margin:10px 0 5px 0;font-weight:bold;}");
    response->print("input[type='number'],input[type='text'],input[type='password']{width:100%;padding:8px;border:1px solid #ddd;border-radius:4px;box-sizing:border-box;}");
    response->print("input[type='submit']{background-color:#0066cc;color:white;padding:10px 20px;border:none;border-radius:4px;cursor:pointer;margin-top:10px;}");
    response->print("input[type='submit']:hover{background-color:#0052a3;}");
    response->print(".help-text{font-size:0.9em;color:#666;margin-top:5px;}");
    response->print(".nav{margin-bottom:20px;}");
    response->print(".nav a{display:inline-block;margin-right:8px;padding:6px 14px;background-color:#0066cc;color:white;text-decoration:none;border-radius:4px;font-size:0.9em;}");
    response->print(".nav a.active{background-color:#003d7a;}");
    response->print(".nav a:hover{background-color:#0052a3;}");
    response->print("</style>");

    // Page title
    if      (page == "settings") response->print("<title>Dome Settings - ");
    else if (page == "gpio")     response->print("<title>GPIO Pins - ");
    else if (page == "polarity") response->print("<title>Output Polarity - ");
    else if (page == "sensors")  response->print("<title>Sensor Status - ");
    else                         response->print("<title>Dome Setup - ");
    response->print(GetDeviceName());
    response->print("</title></head><body><div class='container'>");

    // Heading
    response->print("<h1>Dome Setup - ");
    response->print(GetDeviceName());
    response->print("</h1>");

    // Nav bar
    response->print("<div class='nav'>");
    response->print("<a href='"); response->print(baseUrl); response->print("'");
    if (page == "") response->print(" class='active'");
    response->print(">Status &amp; WiFi</a>");
    response->print("<a href='"); response->print(baseUrl); response->print("?page=settings'");
    if (page == "settings") response->print(" class='active'");
    response->print(">Dome Settings</a>");
    response->print("<a href='"); response->print(baseUrl); response->print("?page=gpio'");
    if (page == "gpio") response->print(" class='active'");
    response->print(">GPIO Pins</a>");
    response->print("<a href='"); response->print(baseUrl); response->print("?page=polarity'");
    if (page == "polarity") response->print(" class='active'");
    response->print(">Output Polarity</a>");
    response->print("<a href='"); response->print(baseUrl); response->print("?page=sensors'");
    if (page == "sensors") response->print(" class='active'");
    response->print(">Sensor Status</a>");
    response->print("</div>");

    if (page == "settings") {
      response->print("<div class='form-section'><form method='POST' action='");
      response->print(baseUrl); response->print("?page=settings'>");
      response->print("<h2>Required Dome Settings</h2>");
      response->print("<label for='home_azimuth'>Home Azimuth (&deg;):</label>");
      response->print("<input type='number' id='home_azimuth' name='home_azimuth' min='0' max='359.99' step='0.01' value='"); response->print(String(homeAzimuth, 2)); response->print("' required>");
      response->print("<label for='park_azimuth'>Park Azimuth (&deg;):</label>");
      response->print("<input type='number' id='park_azimuth' name='park_azimuth' min='0' max='359.99' step='0.01' value='"); response->print(String(parkAzimuth, 2)); response->print("' required>");
      response->print("<label for='park_altitude'>Park Altitude (&deg;):</label>");
      response->print("<input type='number' id='park_altitude' name='park_altitude' min='0' max='90' step='0.01' value='"); response->print(String(parkAltitude, 2)); response->print("' required>");
      response->print("<label for='azimuth_speed'>Azimuth Speed (&deg;/s):</label>");
      response->print("<input type='number' id='azimuth_speed' name='azimuth_speed' min='0.1' max='60' step='0.1' value='"); response->print(String(azimuthSpeed, 2)); response->print("' required>");
      response->print("<label for='altitude_speed'>Altitude Speed (&deg;/s):</label>");
      response->print("<input type='number' id='altitude_speed' name='altitude_speed' min='0.1' max='30' step='0.1' value='"); response->print(String(altitudeSpeed, 2)); response->print("' required>");
      response->print("<label for='shutter_duration'>Shutter Duration (ms):</label>");
      response->print("<input type='number' id='shutter_duration' name='shutter_duration' min='1000' max='300000' step='100' value='"); response->print(shutterDuration); response->print("' required>");
      response->print("<div class='help-text'>Stored in EEPROM and survive reboot.</div>");
      response->print("<input type='submit' value='Save Dome Settings'>");
      response->print("</form></div>");

    } else if (page == "gpio") {
      response->print("<div class='info-section'><h2>Current GPIO Configuration</h2>");
      response->print("<div class='info-row'><span class='info-label'>Azimuth Step Pin:</span><span class='info-value'>"); response->print(azimuthStepPin); response->print("</span></div>");
      response->print("<div class='info-row'><span class='info-label'>Azimuth Dir Pin:</span><span class='info-value'>"); response->print(azimuthDirPin); response->print("</span></div>");
      response->print("<div class='info-row'><span class='info-label'>Azimuth Enable Pin:</span><span class='info-value'>"); response->print(azimuthEnablePin); response->print("</span></div>");
      response->print("<div class='info-row'><span class='info-label'>Shutter Open Drive Pin:</span><span class='info-value'>"); response->print(shutterOpenDrivePin); response->print("</span></div>");
      response->print("<div class='info-row'><span class='info-label'>Shutter Close Drive Pin:</span><span class='info-value'>"); response->print(shutterCloseDrivePin); response->print("</span></div>");
      response->print("<div class='info-row'><span class='info-label'>Shutter Open Sensor Pin:</span><span class='info-value'>"); response->print(shutterOpenSensorPin); response->print("</span></div>");
      response->print("<div class='info-row'><span class='info-label'>Shutter Close Sensor Pin:</span><span class='info-value'>"); response->print(shutterCloseSensorPin); response->print("</span></div>");
      response->print("<div class='info-row'><span class='info-label'>Home Sensor Pin:</span><span class='info-value'>"); response->print(homeSensorPin); response->print("</span></div>");
      response->print("<div class='info-row'><span class='info-label'>Shutter Power-On Pin:</span><span class='info-value'>"); response->print(shutterPowerOnPin); response->print("</span></div>");
      response->print("<div class='info-row'><span class='info-label'>Shutter Direction Pin:</span><span class='info-value'>"); response->print(shutterDirectionPin); response->print("</span></div>");
      response->print("<div class='info-row'><span class='info-label'>Open Drive Polarity:</span><span class='info-value'>"); response->print(shutterOpenDriveLowActive ? "LOW active" : "HIGH active"); response->print("</span></div>");
      response->print("<div class='info-row'><span class='info-label'>Close Drive Polarity:</span><span class='info-value'>"); response->print(shutterCloseDriveLowActive ? "LOW active" : "HIGH active"); response->print("</span></div>");
      response->print("<div class='info-row'><span class='info-label'>Power-On Polarity:</span><span class='info-value'>"); response->print(shutterPowerOnLowActive ? "LOW active" : "HIGH active"); response->print("</span></div>");
      response->print("<div class='info-row'><span class='info-label'>Direction Polarity:</span><span class='info-value'>"); response->print(shutterDirectionLowActive ? "LOW = open" : "HIGH = open"); response->print("</span></div>");
      response->print("</div>");

      response->print("<div class='form-section'><form method='POST' action='");
      response->print(baseUrl); response->print("?page=gpio'>");
      response->print(F("<h2>GPIO Pin Configuration</h2>"));

      // Azimuth pins
      response->print(F("<label for='azimuth_step_pin'>Azimuth Step Pin (GPIO):</label>"));
      response->print(F("<input type='number' id='azimuth_step_pin' name='azimuth_step_pin' min='-1' max='16' step='1' value='")); response->print(azimuthStepPin); response->print(F("' required>"));
      response->print(F("<label for='azimuth_dir_pin'>Azimuth Direction Pin (GPIO):</label>"));
      response->print(F("<input type='number' id='azimuth_dir_pin' name='azimuth_dir_pin' min='-1' max='16' step='1' value='")); response->print(azimuthDirPin); response->print(F("' required>"));
      response->print(F("<label for='azimuth_enable_pin'>Azimuth Enable Pin (GPIO):</label>"));
      response->print(F("<input type='number' id='azimuth_enable_pin' name='azimuth_enable_pin' min='-1' max='16' step='1' value='")); response->print(azimuthEnablePin); response->print(F("' required>"));
      // Shutter drive pins
      response->print(F("<label for='shutter_open_drive_pin'>Shutter Open Drive Pin (GPIO):</label>"));
      response->print(F("<input type='number' id='shutter_open_drive_pin' name='shutter_open_drive_pin' min='-1' max='16' step='1' value='")); response->print(shutterOpenDrivePin); response->print(F("' required>"));
      response->print(F("<label for='shutter_close_drive_pin'>Shutter Close Drive Pin (GPIO):</label>"));
      response->print(F("<input type='number' id='shutter_close_drive_pin' name='shutter_close_drive_pin' min='-1' max='16' step='1' value='")); response->print(shutterCloseDrivePin); response->print(F("' required>"));
      // Sensor pins
      response->print(F("<label for='shutter_open_sensor_pin'>Shutter Open Sensor Pin (GPIO):</label>"));
      response->print(F("<input type='number' id='shutter_open_sensor_pin' name='shutter_open_sensor_pin' min='-1' max='16' step='1' value='")); response->print(shutterOpenSensorPin); response->print(F("' required>"));
      response->print(F("<label for='shutter_close_sensor_pin'>Shutter Close Sensor Pin (GPIO):</label>"));
      response->print(F("<input type='number' id='shutter_close_sensor_pin' name='shutter_close_sensor_pin' min='-1' max='16' step='1' value='")); response->print(shutterCloseSensorPin); response->print(F("' required>"));
      response->print(F("<label for='home_sensor_pin'>Home Sensor Pin (GPIO):</label>"));
      response->print(F("<input type='number' id='home_sensor_pin' name='home_sensor_pin' min='-1' max='16' step='1' value='")); response->print(homeSensorPin); response->print(F("' required>"));
      // Power / direction pins
      response->print(F("<label for='shutter_power_on_pin'>Shutter Power-On Pin (GPIO):</label>"));
      response->print(F("<input type='number' id='shutter_power_on_pin' name='shutter_power_on_pin' min='-1' max='16' step='1' value='")); response->print(shutterPowerOnPin); response->print(F("' required>"));
      response->print(F("<label for='shutter_direction_pin'>Shutter Direction Pin (GPIO):</label>"));
      response->print(F("<input type='number' id='shutter_direction_pin' name='shutter_direction_pin' min='-1' max='16' step='1' value='")); response->print(shutterDirectionPin); response->print(F("' required>"));

      response->print(F("<div class='help-text'>Valid GPIOs: 0-5, 12-16 (pins 6-11 are reserved for SPI flash). Use -1 to disable. Active pins must be unique.<br>When <em>Shutter Power-On Pin</em> is set, the Direction pin controls open/close direction. Open/Close Drive pins are only used when Power-On Pin is -1.<br>Configure output polarity on the <a href='"));
      response->print(baseUrl);
      response->print(F("?page=polarity'>Output Polarity</a> page.</div>"));

      response->print(F("<input type='submit' value='Save GPIO Pins'>"));
      response->print(F("</form></div>"));

    } else if (page == "polarity") {
      response->print(F("<div class='info-section'><h2>Current Polarity Settings</h2>"));
      response->print(F("<div class='info-row'><span class='info-label'>Open Drive:</span><span class='info-value'>")); response->print(shutterOpenDriveLowActive ? "LOW active" : "HIGH active"); response->print(F("</span></div>"));
      response->print(F("<div class='info-row'><span class='info-label'>Close Drive:</span><span class='info-value'>")); response->print(shutterCloseDriveLowActive ? "LOW active" : "HIGH active"); response->print(F("</span></div>"));
      response->print(F("<div class='info-row'><span class='info-label'>Power-On:</span><span class='info-value'>")); response->print(shutterPowerOnLowActive ? "LOW active" : "HIGH active"); response->print(F("</span></div>"));
      response->print(F("<div class='info-row'><span class='info-label'>Direction:</span><span class='info-value'>")); response->print(shutterDirectionLowActive ? "LOW = open" : "HIGH = open"); response->print(F("</span></div>"));
      response->print(F("<div class='info-row'><span class='info-label'>Open Sensor:</span><span class='info-value'>")); response->print(shutterOpenSensorLowActive ? "LOW = open" : "HIGH = open"); response->print(F("</span></div>"));
      response->print(F("<div class='info-row'><span class='info-label'>Close Sensor:</span><span class='info-value'>")); response->print(shutterCloseSensorLowActive ? "LOW = closed" : "HIGH = closed"); response->print(F("</span></div>"));
      response->print(F("<div class='info-row'><span class='info-label'>Home Sensor:</span><span class='info-value'>")); response->print(homeSensorLowActive ? "LOW = home" : "HIGH = home"); response->print(F("</span></div>"));
      response->print(F("</div>"));

      response->print(F("<div class='form-section'><form method='POST' action='"));
      response->print(baseUrl); response->print(F("?page=polarity'>"));
      response->print(F("<h2>Output Polarity</h2>"));
      response->print(F("<div class='help-text' style='margin-bottom:8px;'>Check if the signal is <strong>LOW&nbsp;=&nbsp;active</strong> (active-low / inverted logic)</div>"));

      response->print(F("<b>Output pins</b><br>"));
      response->print(F("<label style='font-weight:normal;display:block;margin:4px 0;'><input type='checkbox' name='open_drive_low_active' value='1'"));
      if (shutterOpenDriveLowActive) response->print(F(" checked"));
      response->print(F("> Shutter Open Drive (LOW active)</label>"));
      response->print(F("<label style='font-weight:normal;display:block;margin:4px 0;'><input type='checkbox' name='close_drive_low_active' value='1'"));
      if (shutterCloseDriveLowActive) response->print(F(" checked"));
      response->print(F("> Shutter Close Drive (LOW active)</label>"));
      response->print(F("<label style='font-weight:normal;display:block;margin:4px 0;'><input type='checkbox' name='power_on_low_active' value='1'"));
      if (shutterPowerOnLowActive) response->print(F(" checked"));
      response->print(F("> Shutter Power-On (LOW enables motor)</label>"));
      response->print(F("<label style='font-weight:normal;display:block;margin:4px 0;'><input type='checkbox' name='direction_low_active' value='1'"));
      if (shutterDirectionLowActive) response->print(F(" checked"));
      response->print(F("> Shutter Direction (LOW = open)</label>"));

      response->print(F("<br><b>Input sensors</b><br>"));
      response->print(F("<label style='font-weight:normal;display:block;margin:4px 0;'><input type='checkbox' name='open_sensor_low_active' value='1'"));
      if (shutterOpenSensorLowActive) response->print(F(" checked"));
      response->print(F("> Shutter Open Sensor (LOW = open)</label>"));
      response->print(F("<label style='font-weight:normal;display:block;margin:4px 0;'><input type='checkbox' name='close_sensor_low_active' value='1'"));
      if (shutterCloseSensorLowActive) response->print(F(" checked"));
      response->print(F("> Shutter Close Sensor (LOW = closed)</label>"));
      response->print(F("<label style='font-weight:normal;display:block;margin:4px 0;'><input type='checkbox' name='home_sensor_low_active' value='1'"));
      if (homeSensorLowActive) response->print(F(" checked"));
      response->print(F("> Home Sensor (LOW = at home)</label>"));

      response->print(F("<input type='hidden' name='polarity_update' value='1'>"));
      response->print(F("<input type='submit' value='Save Polarity'>"));
      response->print(F("</form></div>"));

    } else if (page == "sensors") {
      response->print(F("<div class='info-section'><h2>Live Sensor Values</h2>"));
      response->print(F("<div class='info-row'><span class='info-label'>Current Shutter Status:</span><span class='info-value'>"));
      response->print(shutterStateToText(shutterStatus));
      response->print(F("</span></div>"));

      bool openSensorConfigured = shutterOpenSensorPin >= 0;
      bool closeSensorConfigured = shutterCloseSensorPin >= 0;
      bool openSensorActive = false;
      bool closeSensorActive = false;

      response->print(F("<div class='info-row'><span class='info-label'>Shutter Open Sensor:</span><span class='info-value'>"));
      if (openSensorConfigured) {
        int rawValue = digitalRead(shutterOpenSensorPin);
        openSensorActive = rawValue == (shutterOpenSensorLowActive ? LOW : HIGH);
        response->print(rawValue == LOW ? "LOW" : "HIGH");
        response->print(" (" );
        response->print(openSensorActive ? "OPEN" : "inactive");
        response->print(")");
      } else {
        response->print("Disabled");
      }
      response->print(F("</span></div>"));

      response->print(F("<div class='info-row'><span class='info-label'>Shutter Close Sensor:</span><span class='info-value'>"));
      if (closeSensorConfigured) {
        int rawValue = digitalRead(shutterCloseSensorPin);
        closeSensorActive = rawValue == (shutterCloseSensorLowActive ? LOW : HIGH);
        response->print(rawValue == LOW ? "LOW" : "HIGH");
        response->print(" (" );
        response->print(closeSensorActive ? "CLOSED" : "inactive");
        response->print(")");
      } else {
        response->print("Disabled");
      }
      response->print(F("</span></div>"));

      response->print(F("<div class='info-row'><span class='info-label'>Home Sensor:</span><span class='info-value'>"));
      if (homeSensorPin >= 0) {
        int rawValue = digitalRead(homeSensorPin);
        response->print(rawValue == LOW ? "LOW" : "HIGH");
        response->print(" (" );
        response->print(rawValue == (homeSensorLowActive ? LOW : HIGH) ? "HOME" : "inactive");
        response->print(")");
      } else {
        response->print("Disabled");
      }
      response->print(F("</span></div></div>"));

      response->print(F("<div class='info-section'><h2>Sensor Diagnostics</h2>"));
      if (openSensorConfigured != closeSensorConfigured) {
        response->print(F("<div class='info-row'><span class='info-label'>Sensor Error:</span><span class='info-value' style='color:#cc0000;'>Only one shutter end sensor is configured</span></div>"));
      } else if (openSensorActive && closeSensorActive) {
        response->print(F("<div class='info-row'><span class='info-label'>Sensor Error:</span><span class='info-value' style='color:#cc0000;'>Open and close sensors are active at the same time</span></div>"));
      } else if (!openSensorConfigured && !closeSensorConfigured) {
        response->print(F("<div class='info-row'><span class='info-label'>Sensor Error:</span><span class='info-value' style='color:#cc0000;'>No shutter end sensors are configured</span></div>"));
      } else {
        response->print(F("<div class='info-row'><span class='info-label'>Sensor Error:</span><span class='info-value' style='color:#008800;'>None</span></div>"));
      }
      response->print(F("</div>"));
      response->print(F("<div class='help-text'>Values are read when this page is loaded. Sensor polarity is configured on the Output Polarity page.</div>"));

    } else {
      // Main page: status, set current position, WiFi
      response->print("<div class='info-section'><h2>Current Status</h2>");
      response->print("<div class='info-row'><span class='info-label'>Azimuth:</span><span class='info-value'>"); response->print(String(currentAzimuth, 2)); response->print("&deg;</span></div>");
      response->print("<div class='info-row'><span class='info-label'>Altitude:</span><span class='info-value'>"); response->print(String(currentAltitude, 2)); response->print("&deg;</span></div>");
      response->print("<div class='info-row'><span class='info-label'>Slewing:</span><span class='info-value'>"); response->print(GetSlewing() ? "Yes" : "No"); response->print("</span></div>");
      response->print("<div class='info-row'><span class='info-label'>At Home:</span><span class='info-value'>"); response->print(atHome ? "Yes" : "No"); response->print("</span></div>");
      response->print("<div class='info-row'><span class='info-label'>At Park:</span><span class='info-value'>"); response->print(atPark ? "Yes" : "No"); response->print("</span></div>");
      response->print("<div class='info-row'><span class='info-label'>Shutter:</span><span class='info-value'>"); response->print(shutterStateToText(shutterStatus)); response->print("</span></div>");
      response->print("<div class='info-row'><span class='info-label'>Shutter Control:</span><span class='info-value'>"); response->print(canSetShutter ? "Enabled" : "Disabled"); response->print("</span></div>");
      response->print("</div>");

      response->print("<div class='info-section'><h2>WiFi Status</h2>");
      if (WiFi.getMode() == WIFI_AP) {
        response->print("<div class='info-row'><span class='info-label'>Mode:</span><span class='info-value' style='color:#ff9900;'>Access Point (Fallback)</span></div>");
        response->print("<div class='info-row'><span class='info-label'>AP SSID:</span><span class='info-value'>"); response->print(WiFi.softAPSSID()); response->print("</span></div>");
        response->print("<div class='info-row'><span class='info-label'>AP IP:</span><span class='info-value'>"); response->print(WiFi.softAPIP().toString()); response->print("</span></div>");
      } else {
        response->print("<div class='info-row'><span class='info-label'>Mode:</span><span class='info-value' style='color:#00aa00;'>Station (Connected)</span></div>");
        response->print("<div class='info-row'><span class='info-label'>SSID:</span><span class='info-value'>"); response->print(WiFi.SSID()); response->print("</span></div>");
        response->print("<div class='info-row'><span class='info-label'>IP Address:</span><span class='info-value'>"); response->print(WiFi.localIP().toString()); response->print("</span></div>");
        response->print("<div class='info-row'><span class='info-label'>Signal:</span><span class='info-value'>"); response->print(WiFi.RSSI()); response->print(" dBm</span></div>");
      }
      response->print("<div class='info-row'><span class='info-label'>Hostname:</span><span class='info-value'>"); response->print(WiFi.hostname()); response->print("</span></div>");
      response->print("</div>");

      response->print("<div class='form-section'><form method='POST' action='"); response->print(baseUrl); response->print("'>");
      response->print("<h2>Set Current Position</h2>");
      response->print("<label for='current_azimuth'>Current Azimuth (&deg;):</label>");
      response->print("<input type='number' id='current_azimuth' name='current_azimuth' min='0' max='359.99' step='0.01' value='"); response->print(String(currentAzimuth, 2)); response->print("' required>");
      response->print("<label for='current_altitude'>Current Altitude (&deg;):</label>");
      response->print("<input type='number' id='current_altitude' name='current_altitude' min='0' max='90' step='0.01' value='"); response->print(String(currentAltitude, 2)); response->print("' required>");
      response->print("<div class='help-text'>Use this after manual dome movement or calibration.</div>");
      response->print("<input type='submit' value='Set Current Position'>");
      response->print("</form></div>");

      response->print("<div class='form-section'><form method='POST' action='"); response->print(baseUrl); response->print("'>");
      response->print("<h2>WiFi Configuration</h2>");
      response->print("<label for='wifi_ssid'>WiFi SSID:</label>");
      response->print("<input type='text' id='wifi_ssid' name='wifi_ssid' maxlength='31' value='"); response->print(wifiConfig.getSSID()); response->print("' required>");
      response->print("<label for='wifi_password'>WiFi Password:</label>");
      response->print("<input type='password' id='wifi_password' name='wifi_password' maxlength='63' value='' placeholder='Enter new password or leave empty'>");
      response->print("<div class='help-text' style='color:#ff6600;'><strong>Warning:</strong> Device will restart after saving WiFi settings.</div>");
      response->print("<input type='submit' value='Save WiFi Settings'>");
      response->print("</form></div>");
    }

    response->print("<p style='text-align:center;color:#888;font-size:0.9em;margin-top:30px;'>");
    response->print("<a href='/management/v1/description' style='color:#0066cc;text-decoration:none;'>Back to Management API</a>");
    response->print("</p></div></body></html>");

    request->send(response);
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
