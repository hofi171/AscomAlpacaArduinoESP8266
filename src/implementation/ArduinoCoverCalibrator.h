#ifndef ARDUINO_COVERCALIBRATOR_H
#define ARDUINO_COVERCALIBRATOR_H

#include "alpaca_api/Alpaca_Device_CoverCalibrator.h"
#ifdef ESP32
  #include <ESP32Servo.h>
#else
  #include <Servo.h>
#endif

/**
 * @file ArduinoCoverCalibrator.h
 * @brief Example implementation of CoverCalibrator device with relay-based light control
 * 
 * This implementation uses a digital output to control a relay for the calibrator light.
 * The relay provides only one brightness level (on/off), with no PWM brightness adjustment.
 * It extends the AlpacaDeviceCoverCalibrator class and implements cover and 
 * flat panel calibrator control logic.
 */

class ArduinoCoverCalibrator : public AlpacaDeviceCoverCalibrator {
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
  int relayPin;                       // Digital pin for relay control (on/off only)
  int coverOpenPin;                   // Pin to open cover (servo or motor)
  int coverClosePin;                  // Pin to close cover
  int coverOpenSensorPin;             // Sensor for open position (optional)
  int coverCloseSensorPin;            // Sensor for closed position (optional)
  
  // Relay configuration
  bool relayInverted;                 // Set to true if relay logic is inverted (active low)
  
  // Servo configuration (for cover control)
  bool useServo;                      // Use servo motor for cover control
  Servo coverServo;                   // Servo object for cover control
  int servoOpenAngle;                 // Servo angle for open position (default: 0)
  int servoClosedAngle;               // Servo angle for closed position (default: 180)
  int currentServoAngle;              // Current servo position
  int targetServoAngle;               // Target servo position during movement
  bool coverDirection;                // true = opening, false = closing
  
  // EEPROM storage for CoverCalibrator settings
  static const int CC_EEPROM_SETTINGS_VALID_ADDR = 320;
  static const int CC_EEPROM_RELAY_PIN_ADDR = 322;
  static const int CC_EEPROM_COVER_OPEN_PIN_ADDR = 326;
  static const int CC_EEPROM_COVER_CLOSE_PIN_ADDR = 330;
  static const int CC_EEPROM_COVER_OPEN_SENSOR_PIN_ADDR = 334;
  static const int CC_EEPROM_COVER_CLOSE_SENSOR_PIN_ADDR = 338;
  static const int CC_EEPROM_SERVO_OPEN_ANGLE_ADDR = 342;
  static const int CC_EEPROM_SERVO_CLOSED_ANGLE_ADDR = 346;
  static const int CC_EEPROM_COVER_DURATION_ADDR = 350;
  static const int CC_EEPROM_CONFIG_ADDR = 354;  // Packed: relay_inverted (bit 0), use_servo (bit 1)
  static const uint16_t CC_EEPROM_VALID_MARKER = 0xCC41;  // "CC" + "A"lpha

  void saveSettingsToEEPROM() {
    EEPROM.put(CC_EEPROM_RELAY_PIN_ADDR, relayPin);
    EEPROM.put(CC_EEPROM_COVER_OPEN_PIN_ADDR, coverOpenPin);
    EEPROM.put(CC_EEPROM_COVER_CLOSE_PIN_ADDR, coverClosePin);
    EEPROM.put(CC_EEPROM_COVER_OPEN_SENSOR_PIN_ADDR, coverOpenSensorPin);
    EEPROM.put(CC_EEPROM_COVER_CLOSE_SENSOR_PIN_ADDR, coverCloseSensorPin);
    EEPROM.put(CC_EEPROM_SERVO_OPEN_ANGLE_ADDR, servoOpenAngle);
    EEPROM.put(CC_EEPROM_SERVO_CLOSED_ANGLE_ADDR, servoClosedAngle);
    EEPROM.put(CC_EEPROM_COVER_DURATION_ADDR, (int)coverDuration);
    
    uint8_t configByte = 0;
    if (relayInverted) configByte |= 0x01;
    if (useServo) configByte |= 0x02;
    EEPROM.put(CC_EEPROM_CONFIG_ADDR, configByte);
    
    uint16_t marker = CC_EEPROM_VALID_MARKER;
    EEPROM.put(CC_EEPROM_SETTINGS_VALID_ADDR, marker);
    EEPROM.commit();
    LOG_DEBUG("CoverCalibrator settings saved to EEPROM");
  }

  void loadSettingsFromEEPROM() {
    uint16_t marker;
    EEPROM.get(CC_EEPROM_SETTINGS_VALID_ADDR, marker);
    
    if (marker == CC_EEPROM_VALID_MARKER) {
      EEPROM.get(CC_EEPROM_RELAY_PIN_ADDR, relayPin);
      EEPROM.get(CC_EEPROM_COVER_OPEN_PIN_ADDR, coverOpenPin);
      EEPROM.get(CC_EEPROM_COVER_CLOSE_PIN_ADDR, coverClosePin);
      EEPROM.get(CC_EEPROM_COVER_OPEN_SENSOR_PIN_ADDR, coverOpenSensorPin);
      EEPROM.get(CC_EEPROM_COVER_CLOSE_SENSOR_PIN_ADDR, coverCloseSensorPin);
      EEPROM.get(CC_EEPROM_SERVO_OPEN_ANGLE_ADDR, servoOpenAngle);
      EEPROM.get(CC_EEPROM_SERVO_CLOSED_ANGLE_ADDR, servoClosedAngle);
      
      int savedDuration;
      EEPROM.get(CC_EEPROM_COVER_DURATION_ADDR, savedDuration);
      coverDuration = (unsigned long)savedDuration;
      
      uint8_t configByte = 0;
      EEPROM.get(CC_EEPROM_CONFIG_ADDR, configByte);
      relayInverted = (configByte & 0x01) != 0;
      useServo = (configByte & 0x02) != 0;
      
      LOG_DEBUG("CoverCalibrator settings loaded from EEPROM");
    } else {
      LOG_WARN("No valid CoverCalibrator settings in EEPROM, using defaults");
    }
  }
  
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
    
    // Servo-based cover control
    if (useServo && coverServo.attached()) {
      unsigned long elapsed = currentTime - coverStartTime;
      float progress = min(1.0f, (float)elapsed / coverDuration);
      
      // Interpolate servo angle
      int startAngle = coverDirection ? servoClosedAngle : servoOpenAngle;
      int endAngle = coverDirection ? servoOpenAngle : servoClosedAngle;
      currentServoAngle = startAngle + (int)((endAngle - startAngle) * progress);
      coverServo.write(currentServoAngle);
      
      // Check if movement is complete
      if (progress >= 1.0f) {
        coverMoving = false;
        coverState = coverDirection ? COVER_OPEN : COVER_CLOSED;
        LOG_DEBUG("Servo cover movement complete. State:", coverState == COVER_OPEN ? "OPEN" : "CLOSED");
      }
      return;
    }
    
    // Time-based completion (for motor control or simulated movement)
    if (currentTime - coverStartTime >= coverDuration) {
      coverMoving = false;
      
      // Determine final state based on direction
      // coverDirection: true = opening, false = closing
      coverState = coverDirection ? COVER_OPEN : COVER_CLOSED;
      LOG_DEBUG("Cover movement complete. State:", coverState == COVER_OPEN ? "OPEN" : "CLOSED");
      
      // Stop cover motor (if pins are configured)
      if (coverOpenPin >= 0) digitalWrite(coverOpenPin, LOW);
      if (coverClosePin >= 0) digitalWrite(coverClosePin, LOW);
    }
  }
  
  /**
   * @brief Update calibrator brightness state machine
   * For relay control, brightness is binary: 0 = off, > 0 = on
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
        LOG_DEBUG("Calibrator is now READY (Relay ON)");
      }
    } else {
      // For relay, we just wait for duration then turn on/off
      // No gradual adjustment like PWM
      calibratorState = CALIBRATOR_NOT_READY;
    }
    
    // Always update relay output during adjustment
    if (calibratorChanging) {
      updateCalibratorOutput();
    }
  }
  
  /**
   * @brief Update relay output for calibrator
   * Relay is binary: HIGH for on, LOW for off (or inverted based on relayInverted)
   */
  void updateCalibratorOutput() {
    if (relayPin < 0) return;
    
    // Determine relay state: on if targetBrightness > 0, off if 0
    bool relayOn = (targetBrightness > 0);
    
    // Handle inverted logic if relay is active-low
    int relayState = relayOn ? HIGH : LOW;
    if (relayInverted) {
      relayState = relayOn ? LOW : HIGH;
    }
    
    digitalWrite(relayPin, relayState);
    LOG_DEBUG("Relay output set to:", relayState == HIGH ? "HIGH (ON)" : "LOW (OFF)");
  }

public:
  /**
   * @brief Constructor for ArduinoCoverCalibrator
   * @param devicename Name of the device
   * @param devicenumber Device number for Alpaca API
   * @param description Human-readable description
   * @param server Reference to AsyncWebServer
   * @param relay_pin Digital pin for relay control (binary on/off)
   * @param cover_open_pin Pin to control cover opening (or servo pin if using servo)
   * @param cover_close_pin Pin to control cover closing (not used with servo)
   * @param cover_open_sensor_pin Optional sensor for open position
   * @param cover_close_sensor_pin Optional sensor for closed position
   * @param relay_inverted Set to true if relay logic is inverted (active low)
   * @param use_servo Set to true to use servo motor for cover control
   */
  ArduinoCoverCalibrator(String devicename, int devicenumber, String description, 
                         AsyncWebServer &server, int relay_pin = -1,
                         int cover_open_pin = -1, int cover_close_pin = -1,
                         int cover_open_sensor_pin = -1, int cover_close_sensor_pin = -1,
                         bool relay_inverted = false, bool use_servo = false)
    : AlpacaDeviceCoverCalibrator(devicename, devicenumber, description, server),
      maxBrightness(1),                // Binary brightness: 0 or 1
      currentBrightness(0),
      targetBrightness(0),
      calibratorState(CALIBRATOR_OFF),
      calibratorChanging(false),
      coverState(COVER_CLOSED),
      coverMoving(false),
      coverStartTime(0),
      coverDuration(5000),             // 5 seconds to open/close
      brightnessStartTime(0),
      brightnessDuration(500),         // 0.5 seconds to turn relay on/off
      relayPin(relay_pin),
      coverOpenPin(cover_open_pin),
      coverClosePin(cover_close_pin),
      coverOpenSensorPin(cover_open_sensor_pin),
      coverCloseSensorPin(cover_close_sensor_pin),
      relayInverted(relay_inverted),
      useServo(use_servo),
      servoOpenAngle(0),
      servoClosedAngle(180),
      currentServoAngle(180),
      targetServoAngle(180),
      coverDirection(false)
  {
    // Load settings from EEPROM before initializing pins
    loadSettingsFromEEPROM();
    
    // Initialize relay pin
    if (relayPin >= 0) {
      pinMode(relayPin, OUTPUT);
      // Set initial state to off
      int initialState = relayInverted ? HIGH : LOW;
      digitalWrite(relayPin, initialState);
      LOG_DEBUG("Relay pin initialized - Pin:", relayPin, ", Inverted:", relayInverted ? "true" : "false");
    }
    
    // Initialize servo for cover control
    if (useServo && coverOpenPin >= 0) {
      coverServo.attach(coverOpenPin);
      coverServo.write(servoClosedAngle);  // Initialize to closed position
      currentServoAngle = servoClosedAngle;
      LOG_DEBUG("Servo initialized on pin:", coverOpenPin, " - Open angle:", servoOpenAngle, " Closed angle:", servoClosedAngle);
    } else if (!useServo) {
      // Initialize motor control pins (only if not using servo)
      if (coverOpenPin >= 0) {
        pinMode(coverOpenPin, OUTPUT);
        digitalWrite(coverOpenPin, LOW);
      }
      if (coverClosePin >= 0) {
        pinMode(coverClosePin, OUTPUT);
        digitalWrite(coverClosePin, LOW);
      }
    }
    
    // Initialize sensor pins
    if (coverOpenSensorPin >= 0) {
      pinMode(coverOpenSensorPin, INPUT_PULLUP);
    }
    if (coverCloseSensorPin >= 0) {
      pinMode(coverCloseSensorPin, INPUT_PULLUP);
    }
    
    LOG_DEBUG("ArduinoCoverCalibrator created - Relay Control (On/Off only), Servo:", useServo ? "enabled" : "disabled");
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
    LOG_DEBUG("Turning calibrator relay off");
    targetBrightness = 0;
    calibratorChanging = true;
    calibratorState = CALIBRATOR_NOT_READY;
    brightnessStartTime = millis();
    updateCalibratorOutput();
  }
  
  void CalibratorOn(int brightness) override {
    // For relay control, any brightness > 0 turns relay on
    // Clamp to maxBrightness (which is 1 for relay)
    if (brightness < 0 || brightness > maxBrightness) {
      LOG_DEBUG("Invalid brightness: " + String(brightness) + " (valid range: 0-" + String(maxBrightness) + ")");
      return;
    }
    
    LOG_DEBUG("Turning calibrator relay on (brightness value:", brightness, ")");
    targetBrightness = (brightness > 0) ? 1 : 0;  // Normalize to 0 or 1
    calibratorChanging = true;
    calibratorState = CALIBRATOR_NOT_READY;
    brightnessStartTime = millis();
    updateCalibratorOutput();
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
    coverDirection = false;  // false = closing
    
    if (useServo) {
      // Servo will be controlled by updateCover() via interpolation
      LOG_DEBUG("Moving servo to closed angle:", servoClosedAngle);
    } else {
      // Control cover motor
      if (coverOpenPin >= 0) digitalWrite(coverOpenPin, LOW);
      if (coverClosePin >= 0) digitalWrite(coverClosePin, HIGH);
    }
  }
  
  void HaltCover() override {
    LOG_DEBUG("Halting cover movement");
    coverMoving = false;
    
    if (!useServo) {
      // Stop cover motor
      if (coverOpenPin >= 0) digitalWrite(coverOpenPin, LOW);
      if (coverClosePin >= 0) digitalWrite(coverClosePin, LOW);
    }
    // Servo will maintain its current position
    
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
    coverDirection = true;  // true = opening
    
    if (useServo) {
      // Servo will be controlled by updateCover() via interpolation
      LOG_DEBUG("Moving servo to open angle:", servoOpenAngle);
    } else {
      // Control cover motor
      if (coverClosePin >= 0) digitalWrite(coverClosePin, LOW);
      if (coverOpenPin >= 0) digitalWrite(coverOpenPin, HIGH);
    }
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
   * @brief Set relay activation duration
   * @param durationMs Duration in milliseconds for relay to turn on/off
   */
  void setRelayActivationDuration(unsigned long durationMs) {
    brightnessDuration = durationMs;
    LOG_DEBUG("Relay activation duration set to: " + String(brightnessDuration) + " ms");
  }
  
  /**
   * @brief Get target brightness (0 or 1 for relay)
   * @return Target brightness value
   */
  int getTargetBrightness() const {
    return targetBrightness;
  }
  
  /**
   * @brief Get relay state (true = on, false = off)
   * @return Current relay state
   */
  bool getRelayState() const {
    if (relayPin < 0) return false;
    
    int pinState = digitalRead(relayPin);
    bool relayOn = (pinState == HIGH);
    
    // Account for inverted logic
    if (relayInverted) {
      relayOn = (pinState == LOW);
    }
    
    return relayOn;
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
  
  /**
   * @brief Set relay inversion (active low)
   * @param inverted Set to true if relay is active low (logic inverted)
   */
  void setRelayInverted(bool inverted) {
    relayInverted = inverted;
    LOG_DEBUG("Relay inverted logic set to:", inverted ? "true" : "false");
    updateCalibratorOutput();
  }
  
  /**
   * @brief Enable or disable servo motor control
   * @param enable Set to true to use servo for cover control
   */
  void setServoEnabled(bool enable) {
    if (enable && !useServo && coverOpenPin >= 0) {
      // Enable servo
      if (!coverServo.attached()) {
        coverServo.attach(coverOpenPin);
        coverServo.write(servoClosedAngle);
        LOG_DEBUG("Servo enabled on pin:", coverOpenPin);
      }
      useServo = true;
    } else if (!enable && useServo) {
      // Disable servo
      if (coverServo.attached()) {
        coverServo.detach();
        LOG_DEBUG("Servo disabled");
      }
      useServo = false;
    }
  }
  
  /**
   * @brief Set servo angles for open and closed positions
   * @param openAngle Servo angle when cover is open (degrees)
   * @param closedAngle Servo angle when cover is closed (degrees)
   */
  void setServoAngles(int openAngle, int closedAngle) {
    servoOpenAngle = openAngle;
    servoClosedAngle = closedAngle;
    LOG_DEBUG("Servo angles set - Open:", openAngle, " Closed:", closedAngle);
  }
  
  /**
   * @brief Get current servo angle
   * @return Current servo position in degrees
   */
  int getCurrentServoAngle() const {
    return currentServoAngle;
  }
  
  /**
   * @brief Get servo open angle
   * @return Servo angle for open position
   */
  int getServoOpenAngle() const {
    return servoOpenAngle;
  }
  
  /**
   * @brief Get servo closed angle
   * @return Servo angle for closed position
   */
  int getServoClosedAngle() const {
    return servoClosedAngle;
  }
  
  // ==================== Virtual Function Implementations (Required by AplacaDevice) ====================
  
  void actionHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;
    if (!extractClientIDAndTransactionID(request, true, clientIDInt, clientTransID)) {
      request->send(400, "application/json", "{\"ErrorMessage\": \"Invalid ClientID or ClientTransactionID\"}");
      return;
    }
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, 0, "action", AlpacaError::NotImplemented, "Action not implemented");
    root.printTo(message);
    request->send(400, "application/json", message);
  }
  
  void commandblindHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;
    if (!extractClientIDAndTransactionID(request, true, clientIDInt, clientTransID)) {
      request->send(400, "application/json", "{\"ErrorMessage\": \"Invalid ClientID or ClientTransactionID\"}");
      return;
    }
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, 0, "commandblind", AlpacaError::NotImplemented, "Command not implemented");
    root.printTo(message);
    request->send(400, "application/json", message);
  }
  
  void commandboolHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;
    if (!extractClientIDAndTransactionID(request, true, clientIDInt, clientTransID)) {
      request->send(400, "application/json", "{\"ErrorMessage\": \"Invalid ClientID or ClientTransactionID\"}");
      return;
    }
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, 0, "commandbool", AlpacaError::NotImplemented, "Command not implemented");
    root.printTo(message);
    request->send(400, "application/json", message);
  }
  
  void commandstringHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;
    if (!extractClientIDAndTransactionID(request, true, clientIDInt, clientTransID)) {
      request->send(400, "application/json", "{\"ErrorMessage\": \"Invalid ClientID or ClientTransactionID\"}");
      return;
    }
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, 0, "commandstring", AlpacaError::NotImplemented, "Command not implemented");
    root.printTo(message);
    request->send(400, "application/json", message);
  }
  
  void connectHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;
    if (!extractClientIDAndTransactionID(request, true, clientIDInt, clientTransID)) {
      request->send(400, "application/json", "{\"ErrorMessage\": \"Invalid ClientID or ClientTransactionID\"}");
      return;
    }
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, 0, "connect", AlpacaError::Success, "");
    root.printTo(message);
    request->send(200, "application/json", message);
  }
  
  void connectedHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;
    if (!extractClientIDAndTransactionID(request, request->method() == HTTP_PUT, clientIDInt, clientTransID)) {
      request->send(400, "application/json", "{\"ErrorMessage\": \"Invalid ClientID or ClientTransactionID\"}");
      return;
    }
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, 0, true, AlpacaError::Success, "");
    root.printTo(message);
    request->send(200, "application/json", message);
  }
  
  void connectingHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;
    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      request->send(400, "application/json", "{\"ErrorMessage\": \"Invalid ClientID or ClientTransactionID\"}");
      return;
    }
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, 0, false, AlpacaError::Success, "");
    root.printTo(message);
    request->send(200, "application/json", message);
  }
  
  void deviceStateHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;
    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      request->send(400, "application/json", "{\"ErrorMessage\": \"Invalid ClientID or ClientTransactionID\"}");
      return;
    }
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    root["ClientTransactionID"] = clientTransID;
    root["ServerTransactionID"] = 0;
    root["ErrorNumber"] = static_cast<int>(AlpacaError::Success);
    root["ErrorMessage"] = "";
    JsonArray &values = root.createNestedArray("Value");
    root.printTo(message);
    request->send(200, "application/json", message);
  }
  
  void disconnectHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;
    if (!extractClientIDAndTransactionID(request, true, clientIDInt, clientTransID)) {
      request->send(400, "application/json", "{\"ErrorMessage\": \"Invalid ClientID or ClientTransactionID\"}");
      return;
    }
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, 0, "disconnect", AlpacaError::Success, "");
    root.printTo(message);
    request->send(200, "application/json", message);
  }
  
  void driverVersionHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;
    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      request->send(400, "application/json", "{\"ErrorMessage\": \"Invalid ClientID or ClientTransactionID\"}");
      return;
    }
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, 0, "1.0.0", AlpacaError::Success, "");
    root.printTo(message);
    request->send(200, "application/json", message);
  }
  
  void setupHandler(AsyncWebServerRequest *request) override {
    LOG_INFO("Received cover calibrator setup request - Method: " + String(request->method() == HTTP_POST ? "POST" : "GET"));

    String baseUrl = "/setup/v1/covercalibrator/" + String(GetDeviceNumber()) + "/setup";

    if (request->method() == HTTP_POST) {
      String message;
      String redirectPage = "";

      // Preserve page parameter for redirect
      if (request->hasParam("page")) {
        redirectPage = "?page=" + request->getParam("page")->value();
      }

      // Handle GPIO configuration
      if (request->hasParam("relay_pin", true)) {
        int newRelayPin = request->getParam("relay_pin", true)->value().toInt();
        int newCoverOpenPin = request->getParam("cover_open_pin", true)->value().toInt();
        int newCoverClosePin = request->getParam("cover_close_pin", true)->value().toInt();
        int newCoverOpenSensorPin = request->getParam("cover_open_sensor_pin", true)->value().toInt();
        int newCoverCloseSensorPin = request->getParam("cover_close_sensor_pin", true)->value().toInt();
        
        relayPin = newRelayPin;
        coverOpenPin = newCoverOpenPin;
        coverClosePin = newCoverClosePin;
        coverOpenSensorPin = newCoverOpenSensorPin;
        coverCloseSensorPin = newCoverCloseSensorPin;
        message = "GPIO configuration updated.";
      }

      if (request->hasParam("cover_duration", true)) {
        long newCoverDuration = request->getParam("cover_duration", true)->value().toInt();
        if (newCoverDuration >= 1000 && newCoverDuration <= 300000) {
          coverDuration = (unsigned long)newCoverDuration;
          message = "Duration updated.";
        }
      }

      if (request->hasParam("relay_inverted_update", true)) {
        relayInverted = request->hasParam("relay_inverted", true);
        message = "Relay polarity updated.";
      }

      if (request->hasParam("servo_update", true)) {
        useServo = request->hasParam("use_servo", true);
        if (request->hasParam("servo_open_angle", true)) {
          servoOpenAngle = request->getParam("servo_open_angle", true)->value().toInt();
        }
        if (request->hasParam("servo_closed_angle", true)) {
          servoClosedAngle = request->getParam("servo_closed_angle", true)->value().toInt();
        }
        message = "Servo configuration updated.";
      }

      saveSettingsToEEPROM();

      String html = "<html><head><meta http-equiv='refresh' content='2;url=" + baseUrl + redirectPage + "'></head><body>";
      html += "<h1>Setup Updated</h1><p>" + message + "</p></body></html>";
      request->send(200, "text/html", html);
      return;
    }

    // --- GET: Render appropriate page (minimal HTML, synchronous) ---
    String page = "";
    if (request->hasParam("page")) page = request->getParam("page")->value();

    String html = F("<!DOCTYPE html><html><head><title>Cover Calibrator</title></head><body>");
    html += F("<h1>Cover Calibrator Setup</h1>");
    html += F("<p><a href='") + baseUrl + F("'>Status</a> | ");
    html += F("<a href='") + baseUrl + F("?page=gpio'>GPIO</a> | ");
    html += F("<a href='") + baseUrl + F("?page=duration'>Duration</a> | ");
    html += F("<a href='") + baseUrl + F("?page=servo'>Servo</a></p>");

    if (page == "gpio") {
      html += F("<h2>GPIO Configuration</h2>");
      html += F("<p>Relay Pin: ") + String(relayPin) + F("</p>");
      html += F("<p>Open Pin: ") + String(coverOpenPin) + F("</p>");
      html += F("<p>Close Pin: ") + String(coverClosePin) + F("</p>");
      html += F("<p>Open Sensor: ") + String(coverOpenSensorPin) + F("</p>");
      html += F("<p>Close Sensor: ") + String(coverCloseSensorPin) + F("</p>");
      html += F("<form method='POST' action='") + baseUrl + F("'>");
      html += F("<label>Relay Pin: <input type='number' name='relay_pin' min='-1' max='16' value='") + String(relayPin) + F("'></label><br>");
      html += F("<label>Open Pin: <input type='number' name='cover_open_pin' min='-1' max='16' value='") + String(coverOpenPin) + F("'></label><br>");
      html += F("<label>Close Pin: <input type='number' name='cover_close_pin' min='-1' max='16' value='") + String(coverClosePin) + F("'></label><br>");
      html += F("<label>Open Sensor: <input type='number' name='cover_open_sensor_pin' min='-1' max='16' value='") + String(coverOpenSensorPin) + F("'></label><br>");
      html += F("<label>Close Sensor: <input type='number' name='cover_close_sensor_pin' min='-1' max='16' value='") + String(coverCloseSensorPin) + F("'></label><br>");
      html += F("<input type='hidden' name='page' value='gpio'>");
      html += F("<button type='submit'>Save</button></form>");

    } else if (page == "duration") {
      html += F("<h2>Duration Settings</h2>");
      html += F("<p>Duration: ") + String(coverDuration) + F(" ms</p>");
      html += F("<p>Relay Inverted: ") + String(relayInverted ? "Yes" : "No") + F("</p>");
      html += F("<form method='POST' action='") + baseUrl + F("'>");
      html += F("<label>Duration (ms): <input type='number' name='cover_duration' min='1000' max='300000' value='") + String(coverDuration) + F("'></label><br>");
      html += F("<label><input type='checkbox' name='relay_inverted' value='1'");
      html += relayInverted ? F(" checked") : F("");
      html += F("> Relay Inverted</label><br>");
      html += F("<input type='hidden' name='relay_inverted_update' value='1'>");
      html += F("<input type='hidden' name='page' value='duration'>");
      html += F("<button type='submit'>Save</button></form>");

    } else if (page == "servo") {
      html += F("<h2>Servo Configuration</h2>");
      html += F("<p>Use Servo: ") + String(useServo ? "Yes" : "No") + F("</p>");
      if (useServo) {
        html += F("<p>Open Angle: ") + String(servoOpenAngle) + F("</p>");
        html += F("<p>Closed Angle: ") + String(servoClosedAngle) + F("</p>");
      }
      html += F("<form method='POST' action='") + baseUrl + F("'>");
      html += F("<label><input type='checkbox' name='use_servo' value='1'");
      html += useServo ? F(" checked") : F("");
      html += F("> Enable Servo</label><br>");
      html += F("<label>Open Angle: <input type='number' name='servo_open_angle' min='0' max='180' value='") + String(servoOpenAngle) + F("'></label><br>");
      html += F("<label>Closed Angle: <input type='number' name='servo_closed_angle' min='0' max='180' value='") + String(servoClosedAngle) + F("'></label><br>");
      html += F("<input type='hidden' name='servo_update' value='1'>");
      html += F("<input type='hidden' name='page' value='servo'>");
      html += F("<button type='submit'>Save</button></form>");

    } else {
      html += F("<h2>Status</h2>");
      html += F("<p>Cover State: ");
      switch (coverState) {
        case COVER_OPEN: html += F("Open"); break;
        case COVER_CLOSED: html += F("Closed"); break;
        case COVER_MOVING: html += F("Moving"); break;
        case COVER_UNKNOWN: html += F("Unknown"); break;
        default: html += F("Error"); break;
      }
      html += F("</p>");
      html += F("<p>Relay Pin: ") + String(relayPin) + F("</p>");
      html += F("<p>Open Pin: ") + String(coverOpenPin) + F("</p>");
      html += F("<p>Close Pin: ") + String(coverClosePin) + F("</p>");
      html += F("<p>Duration: ") + String(coverDuration) + F(" ms</p>");
      html += F("<p>Relay Inverted: ") + String(relayInverted ? "Yes" : "No") + F("</p>");
      html += F("<p>Servo Enabled: ") + String(useServo ? "Yes" : "No") + F("</p>");
    }

    html += F("</body></html>");
    request->send(200, F("text/html"), html);
  }
};

#endif // ARDUINO_COVERCALIBRATOR_H
