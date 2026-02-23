#ifndef MY_COVERCALIBRATOR_H
#define MY_COVERCALIBRATOR_H

#include "alpaca_api/Alpaca_Device_CoverCalibrator.h"

/**
 * @file MyCoverCalibrator.h
 * @brief Example implementation of CoverCalibrator device
 * 
 * This example demonstrates how to create a concrete CoverCalibrator device
 * by extending the AlpacaDeviceCoverCalibrator class and implementing the
 * cover (dust cap) and flat panel calibrator control logic.
 */

class MyCoverCalibrator : public AlpacaDeviceCoverCalibrator {
private:
  // Calibrator configuration
  int maxBrightness;
  int currentBrightness;
  int targetBrightness;
  CalibratorStatus calibratorState;
  bool calibratorChanging;
  
  // Cover configuration
  CoverStatus coverState;
  bool coverMoving;
  unsigned long coverStartTime;
  unsigned long coverDuration;        // Time to open/close cover (ms)
  
  // Brightness adjustment
  unsigned long brightnessStartTime;
  unsigned long brightnessDuration;   // Time to adjust brightness (ms)
  
  // Control pins
  int calibratorPin;                  // PWM pin for calibrator light
  int coverOpenPin;                   // Pin to open cover (servo or motor)
  int coverClosePin;                  // Pin to close cover
  int coverOpenSensorPin;             // Sensor for open position (optional)
  int coverCloseSensorPin;            // Sensor for closed position (optional)
  
  // Servo control (if using servo for cover)
  #ifdef ESP32
    int servoChannel;                 // PWM channel for ESP32
  #endif
  
  /**
   * @brief Update cover state machine
   */
  void updateCover() {
    if (!coverMoving) return;
    
    unsigned long currentTime = millis();
    
    // Check sensors if available
    if (coverOpenSensorPin >= 0 && coverState == COVER_MOVING) {
      if (digitalRead(coverOpenSensorPin) == LOW) {
        coverState = COVER_OPEN;
        coverMoving = false;
        LOG_DEBUG("Cover is now OPEN (sensor triggered)");
        return;
      }
    }
    
    if (coverCloseSensorPin >= 0 && coverState == COVER_MOVING) {
      if (digitalRead(coverCloseSensorPin) == LOW) {
        coverState = COVER_CLOSED;
        coverMoving = false;
        LOG_DEBUG("Cover is now CLOSED (sensor triggered)");
        return;
      }
    }
    
    // Time-based completion if no sensors
    if (currentTime - coverStartTime >= coverDuration) {
      coverMoving = false;
      
      // Determine final state based on direction
      if (coverState == COVER_MOVING) {
        // Need to track what we were doing - simplified here
        // In real implementation, track opening vs closing separately
        LOG_DEBUG("Cover movement complete");
      }
      
      // Stop cover motor
      if (coverOpenPin >= 0) digitalWrite(coverOpenPin, LOW);
      if (coverClosePin >= 0) digitalWrite(coverClosePin, LOW);
    }
  }
  
  /**
   * @brief Update calibrator brightness state machine
   */
  void updateCalibrator() {
    if (!calibratorChanging) return;
    
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - brightnessStartTime;
    
    if (elapsed >= brightnessDuration) {
      // Brightness adjustment complete
      currentBrightness = targetBrightness;
      calibratorChanging = false;
      
      if (currentBrightness == 0) {
        calibratorState = CALIBRATOR_OFF;
        LOG_DEBUG("Calibrator is now OFF");
      } else {
        calibratorState = CALIBRATOR_READY;
        LOG_DEBUG("Calibrator is now READY at brightness:", currentBrightness);
      }
    } else {
      // Gradually adjust brightness
      float progress = (float)elapsed / brightnessDuration;
      int startBrightness = (targetBrightness > currentBrightness) ? 
                            currentBrightness : targetBrightness;
      int endBrightness = (targetBrightness > currentBrightness) ? 
                          targetBrightness : currentBrightness;
      currentBrightness = startBrightness + (int)((endBrightness - startBrightness) * progress);
      
      calibratorState = (targetBrightness == 0) ? CALIBRATOR_NOT_READY : CALIBRATOR_NOT_READY;
      
      // Update PWM output
      updateCalibratorOutput();
    }
    
    // Always update PWM during adjustment
    if (calibratorChanging) {
      updateCalibratorOutput();
    }
  }
  
  /**
   * @brief Update PWM output for calibrator
   */
  void updateCalibratorOutput() {
    if (calibratorPin < 0) return;
    
    // Map brightness (0-maxBrightness) to PWM (0-255 for Arduino, 0-1023 for ESP32)
    #ifdef ESP32
      int pwmValue = map(currentBrightness, 0, maxBrightness, 0, 1023);
      ledcWrite(servoChannel, pwmValue);
    #else
      int pwmValue = map(currentBrightness, 0, maxBrightness, 0, 255);
      analogWrite(calibratorPin, pwmValue);
    #endif
  }

public:
  /**
   * @brief Constructor for MyCoverCalibrator
   * @param devicename Name of the device
   * @param devicenumber Device number for Alpaca API
   * @param description Human-readable description
   * @param server Reference to AsyncWebServer
   * @param max_brightness Maximum brightness value (default: 255)
   * @param calibrator_pin PWM pin for calibrator light
   * @param cover_open_pin Pin to control cover opening
   * @param cover_close_pin Pin to control cover closing
   * @param cover_open_sensor_pin Optional sensor for open position
   * @param cover_close_sensor_pin Optional sensor for closed position
   */
  MyCoverCalibrator(String devicename, int devicenumber, String description, 
                    AsyncWebServer &server, int max_brightness = 255,
                    int calibrator_pin = -1, int cover_open_pin = -1, int cover_close_pin = -1,
                    int cover_open_sensor_pin = -1, int cover_close_sensor_pin = -1)
    : AlpacaDeviceCoverCalibrator(devicename, devicenumber, description, server),
      maxBrightness(max_brightness),
      currentBrightness(0),
      targetBrightness(0),
      calibratorState(CALIBRATOR_OFF),
      calibratorChanging(false),
      coverState(COVER_CLOSED),
      coverMoving(false),
      coverStartTime(0),
      coverDuration(5000),          // 5 seconds to open/close
      brightnessStartTime(0),
      brightnessDuration(2000),     // 2 seconds to adjust brightness
      calibratorPin(calibrator_pin),
      coverOpenPin(cover_open_pin),
      coverClosePin(cover_close_pin),
      coverOpenSensorPin(cover_open_sensor_pin),
      coverCloseSensorPin(cover_close_sensor_pin)
  #ifdef ESP32
      , servoChannel(0)
  #endif
  {
    // Initialize calibrator pin
    if (calibratorPin >= 0) {
      #ifdef ESP32
        // Use LEDC for PWM on ESP32
        ledcSetup(servoChannel, 5000, 10); // 5kHz, 10-bit resolution
        ledcAttachPin(calibratorPin, servoChannel);
        ledcWrite(servoChannel, 0);
      #else
        pinMode(calibratorPin, OUTPUT);
        analogWrite(calibratorPin, 0);
      #endif
    }
    
    // Initialize cover control pins
    if (coverOpenPin >= 0) {
      pinMode(coverOpenPin, OUTPUT);
      digitalWrite(coverOpenPin, LOW);
    }
    if (coverClosePin >= 0) {
      pinMode(coverClosePin, OUTPUT);
      digitalWrite(coverClosePin, LOW);
    }
    
    // Initialize sensor pins
    if (coverOpenSensorPin >= 0) {
      pinMode(coverOpenSensorPin, INPUT_PULLUP);
    }
    if (coverCloseSensorPin >= 0) {
      pinMode(coverCloseSensorPin, INPUT_PULLUP);
    }
    
    LOG_DEBUG("MyCoverCalibrator created - MaxBrightness:", maxBrightness);
  }
  
  // ==================== ICoverCalibrator Interface Implementation ====================
  
  int GetBrightness() override {
    return currentBrightness;
  }
  
  bool GetCalibratorChanging() override {
    return calibratorChanging;
  }
  
  CalibratorStatus GetCalibratorState() override {
    return calibratorState;
  }
  
  bool GetCoverMoving() override {
    return coverMoving;
  }
  
  CoverStatus GetCoverState() override {
    return coverState;
  }
  
  int GetMaxBrightness() override {
    return maxBrightness;
  }
  
  void CalibratorOff() override {
    LOG_DEBUG("Turning calibrator off");
    targetBrightness = 0;
    calibratorChanging = true;
    calibratorState = CALIBRATOR_NOT_READY;
    brightnessStartTime = millis();
  }
  
  void CalibratorOn(int brightness) override {
    if (brightness < 0 || brightness > maxBrightness) {
      LOG_DEBUG("Invalid brightness: " + String(brightness) + " (valid range: 0- " + String(maxBrightness) + ")");
      return;
    }
    
    LOG_DEBUG("Turning calibrator on at brightness:", brightness);
    targetBrightness = brightness;
    calibratorChanging = true;
    calibratorState = CALIBRATOR_NOT_READY;
    brightnessStartTime = millis();
  }
  
  void CloseCover() override {
    if (coverState == COVER_CLOSED) {
      LOG_DEBUG("Cover already closed");
      return;
    }
    
    LOG_DEBUG("Closing cover");
    coverState = COVER_MOVING;
    coverMoving = true;
    coverStartTime = millis();
    
    // Control cover motor/servo
    if (coverOpenPin >= 0) digitalWrite(coverOpenPin, LOW);
    if (coverClosePin >= 0) digitalWrite(coverClosePin, HIGH);
    
    // Alternative: Use servo for cover control
    // In this case, coverOpenPin would be servo pin
    // and you'd write servo angle for closed position
  }
  
  void HaltCover() override {
    LOG_DEBUG("Halting cover movement");
    coverMoving = false;
    
    // Stop cover motor
    if (coverOpenPin >= 0) digitalWrite(coverOpenPin, LOW);
    if (coverClosePin >= 0) digitalWrite(coverClosePin, LOW);
    
    // Set state based on current position
    // In a real implementation with position feedback, determine actual state
    coverState = COVER_UNKNOWN;
  }
  
  void OpenCover() override {
    if (coverState == COVER_OPEN) {
      LOG_DEBUG("Cover already open");
      return;
    }
    
    LOG_DEBUG("Opening cover");
    coverState = COVER_MOVING;
    coverMoving = true;
    coverStartTime = millis();
    
    // Control cover motor/servo
    if (coverClosePin >= 0) digitalWrite(coverClosePin, LOW);
    if (coverOpenPin >= 0) digitalWrite(coverOpenPin, HIGH);
    
    // Alternative: Use servo for cover control
    // In this case, coverOpenPin would be servo pin
    // and you'd write servo angle for open position
  }
  
  // ==================== Additional Methods ====================
  
  /**
   * @brief Update device state
   * Call this periodically from loop() to handle cover and calibrator state machines
   */
  void update() {
    updateCover();
    updateCalibrator();
  }
  
  /**
   * @brief Set cover movement duration
   * @param durationMs Duration in milliseconds
   */
  void setCoverDuration(unsigned long durationMs) {
    coverDuration = durationMs;
    LOG_DEBUG("Cover duration set to: " + String(coverDuration) + " ms");
  }
  
  /**
   * @brief Set brightness adjustment duration
   * @param durationMs Duration in milliseconds
   */
  void setBrightnessDuration(unsigned long durationMs) {
    brightnessDuration = durationMs;
    LOG_DEBUG("Brightness duration set to: " + String(brightnessDuration) + " ms");
  }
  
  /**
   * @brief Get target brightness
   * @return Target brightness value
   */
  int getTargetBrightness() const {
    return targetBrightness;
  }
  
  /**
   * @brief Manually set cover state (for testing or sensor override)
   * @param state New cover state
   */
  void setCoverState(CoverStatus state) {
    coverState = state;
    coverMoving = false;
    LOG_DEBUG("Cover state manually set to:", (int)state);
  }
};

#endif // MY_COVERCALIBRATOR_H
