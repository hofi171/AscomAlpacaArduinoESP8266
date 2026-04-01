#ifndef D5A14B4C_BBEB_4B58_A6B0_77C2F5D439ED
#define D5A14B4C_BBEB_4B58_A6B0_77C2F5D439ED
#ifndef ARDUINO_FOCUSER_H
#define ARDUINO_FOCUSER_H

#include "alpaca_api/Alpaca_Device_Focuser.h"
#include "ArduinoStepper.h"
#include <OneWire.h>
#include <DallasTemperature.h>
#include <EEPROM.h>
#include "WiFi_Config.h"

// Forward declaration for accessing global WiFiConfig
extern WiFiConfig wifiConfig;


/**
 * @file ArduinoFocuser.h
 * @brief Example implementation of Focuser device
 * 
 * This example demonstrates how to create a concrete Focuser device
 * by extending the AlpacaDeviceFocuser class and implementing the
 * focuser control logic with stepper motor integration.
 */
class ArduinoFocuser : public AlpacaDeviceFocuser {
private:

  ArduinoStepper* stepper; // Stepper motor control object
  // Focuser configuration
  //OneWire oneWire = OneWire(4); // OneWire bus for temperature sensor
  //DallasTemperature sensors =DallasTemperature(&oneWire);
    OneWire oneWire; // OneWire bus for temperature sensor
  DallasTemperature sensors; // DallasTemperature object for reading temperature
  double TEMPOFFSET = 0.0; // Temperature offset for compensation (adjustable) 

  bool absolute;                    // Supports absolute positioning
  int maxStep;                      // Maximum step position
  int maxIncrement;                 // Maximum increment per move
  double stepSize;                  // Step size in microns
  
  // Current state
  //int currentPosition;              // Current position in steps
  //int targetPosition;               // Target position for movement
  //bool isMoving;                    // Is focuser currently moving
  
  // Temperature compensation
  bool tempCompAvailable;           // Temperature compensation available
  bool tempComp;                    // Temperature compensation enabled
  double temperature;               // Current temperature in Celsius
  double lastRawTemperature;        // Last raw sensor temperature
  bool temperatureSensorValid;      // Last sensor validity state
  int TEMP_PIN = 4;              // GPIO pin for temperature sensor (DS18B20)
  // Motor control pins (optional - for stepper motor control)
  //int stepPin;
  //int dirPin;
  //int enablePin;

  DeviceAddress Probe = { 0x28, 0xCC, 0xA7, 0xB3, 0x00, 0x00, 0x00, 0x87 };
  // Movement parameters
  unsigned long lastStepTime;       // Time of last step
  unsigned long stepDelayMicros;    // Delay between steps (microseconds)
  
  // EEPROM storage (ArduinoStepper uses addresses 0-7 for position and mode)
  static const int EEPROM_TEMPOFFSET_ADDR = 8;  // EEPROM address for temperature offset (8 bytes, double)
  static const int EEPROM_TEMP_PIN_ADDR = 154;  // EEPROM address for temperature sensor pin (1 byte)
  static const int EEPROM_SIZE = 512;            // Total EEPROM size to allocate
  
  /**
   * @brief Update focuser movement
   * Called periodically to advance motor one step at a time
   */
  void updateMovement() {
    if (stepper == nullptr || !stepper->isMoving() || stepper->getPosition() == stepper->getTarget()) {
      if (stepper != nullptr) {
        stepper->releaseDrive(); // Disable motor
      }
      return;
    }
    
    unsigned long currentTime = micros();
    if (currentTime - lastStepTime < stepDelayMicros) {
      return; // Not time for next step yet
    }
    
    lastStepTime = currentTime;
    
   
    
    // Check if movement is complete
    if (stepper->getPosition() == stepper->getTarget()) {
      stepper->releaseDrive(); // Disable motor
      LOG_DEBUG("Focuser movement complete - Position: " + String(stepper->getPosition()));
    }
  }
  
  /**
   * @brief Simulate temperature reading
   * In a real implementation, read from temperature sensor (e.g., DS18B20, DHT22)
   */
  void updateTemperature() {
    // Simulate temperature reading - replace with actual sensor code
    // Example for DS18B20: temperature = sensors.getTempCByIndex(0);
    // For now, just add a small random fluctuation
    static unsigned long lastTempUpdate = 0;
    unsigned long currentTime = millis();
    
    if (currentTime - lastTempUpdate > 2000) { // Update every 5 seconds
      lastTempUpdate = currentTime;

      sensors.requestTemperatures();
      double rawTemperature = sensors.getTempCByIndex(0);
      lastRawTemperature = rawTemperature;

      // DS18B20 invalid values: disconnected (-127), power-up default (85)
      if (rawTemperature == DEVICE_DISCONNECTED_C ||
          rawTemperature == 85.0 ||
          isnan(rawTemperature) ||
          isinf(rawTemperature) ||
          rawTemperature < -55.0 ||
          rawTemperature > 125.0) {
        temperatureSensorValid = false;
        LOG_WARN("Invalid temperature sensor value: " + String(rawTemperature));
        return;
      }

      temperatureSensorValid = true;
      temperature = rawTemperature + TEMPOFFSET;

     // LOG_DEBUG("1Wire Device connected: " + String(sensors.isConnected(0)));
      //LOG_DEBUG("1Wire Device count: " + String(sensors.getDeviceCount()));
      LOG_INFO("Temperature updated: " + String(temperature) + " °C");
    }
  }
  
  /**
   * @brief Load temperature offset from EEPROM
   */
  void loadTemperatureOffsetFromEEPROM() {
    // Read the double value from EEPROM
    double storedOffset = 0.0;
    EEPROM.get(EEPROM_TEMPOFFSET_ADDR, storedOffset);
    
    // Validate the read value (check if it's a reasonable number)
    // NaN or infinity would indicate uninitialized EEPROM
    if (isnan(storedOffset) || isinf(storedOffset) || storedOffset < -50.0 || storedOffset > 50.0) {
      LOG_WARN("Invalid temperature offset in EEPROM, using default 0.0");
      TEMPOFFSET = 0.0;
      saveTemperatureOffsetToEEPROM(); // Initialize EEPROM with default
    } else {
      TEMPOFFSET = storedOffset;
      LOG_INFO("Loaded temperature offset from EEPROM: " + String(TEMPOFFSET) + " °C");
    }
  }
  
  /**
   * @brief Save temperature offset to EEPROM
   */
  void saveTemperatureOffsetToEEPROM() {
    EEPROM.put(EEPROM_TEMPOFFSET_ADDR, TEMPOFFSET);
    EEPROM.commit(); // Commit changes to EEPROM (required for ESP8266)
    LOG_INFO("Saved temperature offset to EEPROM: " + String(TEMPOFFSET) + " °C");
  }

  void loadTemperaturePinFromEEPROM() {
    uint8_t storedPin = EEPROM.read(EEPROM_TEMP_PIN_ADDR);
    if (storedPin <= 16) {
      TEMP_PIN = storedPin;
      LOG_INFO("Loaded temperature sensor pin from EEPROM: GPIO " + String(TEMP_PIN));
    } else {
      LOG_WARN("Invalid temperature sensor pin in EEPROM, using default GPIO " + String(TEMP_PIN));
      saveTemperaturePinToEEPROM();
    }
  }

  void saveTemperaturePinToEEPROM() {
    EEPROM.write(EEPROM_TEMP_PIN_ADDR, static_cast<uint8_t>(TEMP_PIN));
    EEPROM.commit();
    LOG_INFO("Saved temperature sensor pin to EEPROM: GPIO " + String(TEMP_PIN));
  }

  void initializeTemperatureSensor() {
    LOG_INFO("Initializing temperature sensor on GPIO " + String(TEMP_PIN));
    oneWire = OneWire(TEMP_PIN);
    sensors = DallasTemperature(&oneWire);
    sensors.begin();
    sensors.setResolution(Probe, 10);
  }

public:
  /**
   * @brief Constructor for ArduinoFocuser
   * @param devicename Name of the focuser device
   * @param devicenumber Device number for Alpaca API
   * @param description Human-readable description
   * @param server Reference to AsyncWebServer
   * @param max_step Maximum step position (default: 10000)
   * @param step_size_microns Step size in microns (default: 5.0)
   * @param step_pin Optional stepper motor step pin
   * @param dir_pin Optional stepper motor direction pin
   * @param enable_pin Optional stepper motor enable pin
   */
  ArduinoFocuser(String devicename, int devicenumber, String description, 
            AsyncWebServer &server, int max_step = 10000, double step_size_microns = 5.0,
            int step_pin = -1, int dir_pin = -1, int enable_pin = -1)
    : AlpacaDeviceFocuser(devicename, devicenumber, description, server, true), // hasSetup = true
      absolute(true),
      maxStep(max_step),
      maxIncrement(1000),
      stepSize(step_size_microns),
      tempCompAvailable(true),
      tempComp(false),
      temperature(20.0),
      lastRawTemperature(DEVICE_DISCONNECTED_C),
      temperatureSensorValid(false),
      lastStepTime(0),
      stepDelayMicros(2000) { // 2ms between steps = 500 steps/sec
    
    // Initialize motor control (this also initializes EEPROM)
    LOG_INFO("Initializing ArduinoFocuser with maxStep: " + String(maxStep) + " stepSize: " + String(stepSize) + " microns");
    stepper = new ArduinoStepper();
    stepper->setStepMode(FullStep2Phase); // Set stepping mode (e.g., full step, half step)
    
    // Load temperature offset from EEPROM (now that EEPROM is initialized by ArduinoStepper)
    loadTemperatureOffsetFromEEPROM();

    // Load and initialize temperature sensor pin
    loadTemperaturePinFromEEPROM();
    initializeTemperatureSensor();

    LOG_DEBUG("ArduinoFocuser created - MaxStep: " + String(maxStep) + " StepSize: " + String(stepSize) + " microns");
  }
  
  // ==================== IFocuser Interface Implementation ====================
  
  /**
   * @brief Get absolute positioning capability
   * @return true if focuser supports absolute positioning
   */
  bool GetAbsolute() override {
    return true; // This focuser supports absolute positioning
  }
  
  /**
   * @brief Check if focuser is currently moving
   * @return true if focuser is moving
   */
  bool GetIsMoving() override {
    return stepper != nullptr && stepper->isMoving();
  }
  
  /**
   * @brief Get maximum increment size
   * @return Maximum increment allowed in a single move
   */
  int GetMaxIncrement() override {
    return maxIncrement;
  }
  
  /**
   * @brief Get maximum step position
   * @return Maximum position value
   */
  int GetMaxStep() override {
    return maxStep;
  }
  
  /**
   * @brief Get current position
   * @return Current focuser position in steps
   */
  int GetPosition() override {
    return stepper != nullptr ? stepper->getPosition() : 0;
  }
  
  /**
   * @brief Get step size in microns
   * @return Step size in microns
   */
  double GetStepSize() override {
    return stepSize;
  }
  
  /**
   * @brief Get temperature compensation state
   * @return true if temperature compensation is enabled
   */
  bool GetTempComp() override {
    return tempComp;
  }
  
  /**
   * @brief Get temperature compensation availability
   * @return true if temperature compensation is available
   */
  bool GetTempCompAvailable() override {
    return tempCompAvailable;
  }
  
  /**
   * @brief Get current temperature
   * @return Temperature in degrees Celsius
   */
  double GetTemperature() override {
    return temperature;
  }

  bool IsTemperatureSensorValid() const {
    return temperatureSensorValid;
  }

  double GetLastRawTemperature() const {
    return lastRawTemperature;
  }
  
  /**
   * @brief Set temperature compensation state
   * @param tempComp true to enable, false to disable
   */
  void SetTempComp(bool tempComp) override {
    this->tempComp = tempComp;
    LOG_DEBUG("Temperature compensation " + String(tempComp ? "enabled" : "disabled"));
    
    // In a real implementation, apply temperature compensation logic here
    // For example, adjust focus based on temperature changes
  }
  
  /**
   * @brief Immediately halt focuser motion
   */
  void Halt() override {
    LOG_DEBUG("Halt command received - stopping at position: " + String(GetPosition()));
    stepper->halt();
  }
  
  /**
   * @brief Move focuser to new position
   * @param position Target position (absolute position)
   */
  void Move(int position) override {
    // Validate position
    if (position < 0 || position > maxStep) {
      LOG_ERROR("Invalid position requested: " + String(position) + " (valid range: 0 - " + String(maxStep) + ")");
      return;
    }
    
    // Check if move is within max increment
    int moveDistance = abs(position - stepper->getPosition());
    if (moveDistance > maxIncrement) {
      LOG_WARN("Move distance " + String(moveDistance) + " exceeds max increment " + String(maxIncrement));
      LOG_WARN("Target " + String(position) + " current " + String(stepper->getPosition())); 

    }
    
    LOG_INFO("Moving focuser from " + String(stepper->getPosition()) + " to " + String(position));
    
    stepper->setTarget(position);
    
  }
  
  // ==================== Additional Methods ====================
  
  /**
   * @brief Update focuser state
   * Call this periodically from loop() to handle movement and temperature updates
   */
  void update() {
    stepper->Update();
    updateMovement();
    updateTemperature();
    
    // If temperature compensation is enabled, adjust focus based on temperature
    if (tempComp && tempCompAvailable) {
      // Example: Compensate for temperature changes
      // In a real implementation, calculate and apply compensation
      // based on temperature delta and thermal coefficient
      static double lastCompTemp = temperature;
      static unsigned long lastCompTime = 0;
      unsigned long currentTime = millis();
      
      if (currentTime - lastCompTime > 30000) { // Check every 30 seconds
        double tempDelta = temperature - lastCompTemp;
        if (abs(tempDelta) > 0.5) { // Significant temperature change
          // Example: 5 steps per degree C
          int compensation = (int)(tempDelta * 5.0);
          if (!stepper->isMoving() && abs(compensation) > 0) {
            int newPosition = stepper->getPosition() + compensation;
            if (newPosition >= 0 && newPosition <= maxStep) {
              LOG_DEBUG("Temperature compensation: moving " + String(compensation) + " steps");
              Move(newPosition);
            }
          }
          lastCompTemp = temperature;
        }
        lastCompTime = currentTime;
      }
    }
  }
  
  /**
   * @brief Get the target position
   * @return Target position during movement
   */
  int GetTargetPosition() const {
    return stepper->getTarget();
  }
  
  /**
   * @brief Set motor speed (steps per second)
   * @param stepsPerSecond Speed in steps per second
   */
  void SetSpeed(int stepsPerSecond) {
    if (stepsPerSecond > 0 && stepsPerSecond <= 5000) {
      stepDelayMicros = 1000000 / stepsPerSecond;
      LOG_INFO("Motor speed set to " + String(stepsPerSecond) + " steps/sec");
    }
  }
  
  /**
   * @brief Set maximum increment size
   * @param increment Maximum increment in steps
   */
  void SetMaxIncrement(int increment) {
    maxIncrement = increment;
    LOG_DEBUG("MaxIncrement set to: " + String(maxIncrement));
  }
  
  /**
   * @brief Home the focuser to position 0
   * In a real implementation, this would move to a home switch
   */
  void Home() {
    LOG_DEBUG("Homing focuser to position 0");
    Move(0);
  }
  
  /**
   * @brief Set current position (useful for calibration)
   * @param position New current position value
   */
  void SetCurrentPosition(int position) {
    if (position >= 0 && position <= maxStep) {
      stepper->setActualPosition(position);
      LOG_DEBUG("Current position set to: " + String(stepper->getPosition()));
    }
  }
  
  /**
   * @brief Manual temperature override (for testing)
   * @param temp Temperature in Celsius
   */
  void SetTemperature(double temp) {
    temperature = temp;
  }
  
  /**
   * @brief Set temperature offset for sensor calibration
   * @param offset Temperature offset in Celsius
   */
  void SetTemperatureOffset(double offset) {
    TEMPOFFSET = offset;
    saveTemperatureOffsetToEEPROM(); // Persist to EEPROM
    LOG_INFO("Temperature offset set to: " + String(TEMPOFFSET) + " °C (saved to EEPROM)");
  }
  
  /**
   * @brief Get current temperature offset
   * @return Temperature offset in Celsius
   */
  double GetTemperatureOffset() const {
    return TEMPOFFSET;
  }

  int GetTemperaturePin() const {
    return TEMP_PIN;
  }

  bool IsStepperDrivePin(int pin) const {
    return pin == stepper->getPin1() ||
           pin == stepper->getPin2() ||
           pin == stepper->getPin3() ||
           pin == stepper->getPin4();
  }

  bool SetTemperaturePin(int pin) {
    if (pin < 0 || pin > 16) {
      LOG_WARN("Invalid temperature sensor pin requested: GPIO " + String(pin));
      return false;
    }

    if (IsStepperDrivePin(pin)) {
      LOG_WARN("Temperature sensor pin conflicts with stepper drive pin: GPIO " + String(pin));
      return false;
    }

    if (pin == TEMP_PIN) {
      return true;
    }

    TEMP_PIN = pin;
    saveTemperaturePinToEEPROM();
    initializeTemperatureSensor();
    LOG_INFO("Temperature sensor pin changed to GPIO " + String(TEMP_PIN));
    return true;
  }
  
  /**
   * @brief Override setup handler to provide web configuration interface
   */
  void setupHandler(AsyncWebServerRequest *request) override {
    // Check if this is a POST request (form submission)

    LOG_INFO("Received setup request - Method: " + String(request->method() == HTTP_POST ? "POST" : "GET"));

    if (request->method() == HTTP_POST) {
      // Process form data
      LOG_INFO("Processing focuser setup form submission");

      String message = "";
      
      // Check for position update
      if (request->hasParam("position", true)) {
        String posStr = request->getParam("position", true)->value();
        int newPos = posStr.toInt();
        if (newPos >= 0 && newPos <= maxStep) {
          SetCurrentPosition(newPos);
          message += "Position set to: " + String(newPos) + "<br>";
        } else {
          message += "Error: Position must be between 0 and " + String(maxStep) + "<br>";
        }
      }
      
      // Check for temperature offset update
      if (request->hasParam("tempoffset", true)) {
        String offsetStr = request->getParam("tempoffset", true)->value();
        double newOffset = offsetStr.toDouble();
        SetTemperatureOffset(newOffset);
        message += "Temperature offset set to: " + String(newOffset) + " °C<br>";
      }

      if (request->hasParam("temp_pin", true)) {
        int newTempPin = request->getParam("temp_pin", true)->value().toInt();
        if (SetTemperaturePin(newTempPin)) {
          message += "Temperature sensor pin set to GPIO: " + String(newTempPin) + "<br>";
        } else {
          message += "Error: Temperature sensor pin must be GPIO 0..16 and must not match any stepper drive pin<br>";
        }
      }
      
      // Check for WiFi configuration update
      if (request->hasParam("wifi_ssid", true) && request->hasParam("wifi_password", true)) {
        String newSSID = request->getParam("wifi_ssid", true)->value();
        String newPassword = request->getParam("wifi_password", true)->value();
        
        if (newSSID.length() > 0) {
          if (wifiConfig.saveToEEPROM(newSSID, newPassword)) {
            message += "WiFi credentials saved! SSID: " + newSSID + "<br>";
            message += "<strong>Please restart the device to connect to the new network.</strong><br>";
            ESP.reset();
          } else {
            message += "Error: Failed to save WiFi credentials<br>";
          }
        } else {
          message += "Error: SSID cannot be empty<br>";
        }
      }
      
      // Check for stepper configuration update
      if (request->hasParam("stepper_mode", true)) {
        int newMode = request->getParam("stepper_mode", true)->value().toInt();
        if (newMode >= FullStep && newMode <= FullStep2Phase) {
          stepper->setStepMode((eSTEPMODE)newMode);
          message += "Stepper mode updated to: " + String(newMode) + "<br>";
        } else {
          message += "Error: Invalid stepper mode<br>";
        }
      }
      
      // Check for stepper pins update
      if (request->hasParam("pin1", true) && request->hasParam("pin2", true) && 
          request->hasParam("pin3", true) && request->hasParam("pin4", true)) {
        int pin1 = request->getParam("pin1", true)->value().toInt();
        int pin2 = request->getParam("pin2", true)->value().toInt();
        int pin3 = request->getParam("pin3", true)->value().toInt();
        int pin4 = request->getParam("pin4", true)->value().toInt();
        
        // Validate pin numbers (ESP8266 GPIO 0-16)
        if (pin1 >= 0 && pin1 <= 16 && pin2 >= 0 && pin2 <= 16 && 
            pin3 >= 0 && pin3 <= 16 && pin4 >= 0 && pin4 <= 16) {
          // Check for duplicate pins
          if (pin1 != pin2 && pin1 != pin3 && pin1 != pin4 && 
              pin2 != pin3 && pin2 != pin4 && pin3 != pin4) {
            if (pin1 == TEMP_PIN || pin2 == TEMP_PIN || pin3 == TEMP_PIN || pin4 == TEMP_PIN) {
              message += "Error: Stepper drive pins must not include the configured temperature sensor pin (GPIO " + String(TEMP_PIN) + ")<br>";
            } else {
              stepper->setPins(pin1, pin2, pin3, pin4);
              message += "Stepper pins updated!<br>";
              message += "<strong>Warning: Please verify motor connections before moving!</strong><br>";
            }
          } else {
            message += "Error: Duplicate pin numbers not allowed<br>";
          }
        } else {
          message += "Error: Pin numbers must be between 0 and 16<br>";
        }
      }
      
      // Redirect back to GET after POST
      String html = "<html><head><meta http-equiv='refresh' content='2;url=/setup/v1/focuser/" + 
                    String(GetDeviceNumber()) + "/setup'></head><body>";
      html += "<h1>Focuser Setup</h1>";
      html += "<p>" + message + "</p>";
      html += "<p>Redirecting back to setup page...</p>";
      html += "</body></html>";
      request->send(200, "text/html", html);
      return;
    }
    
    LOG_INFO("Serving focuser setup page");
    // GET request - show the form
    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>Focuser Setup</title>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; }";
    html += "h1 { color: #333; }";
    html += ".container { max-width: 600px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }";
    html += ".info-section { background-color: #e8f4f8; padding: 15px; border-radius: 5px; margin-bottom: 20px; }";
    html += ".info-row { display: flex; justify-content: space-between; margin: 8px 0; }";
    html += ".info-label { font-weight: bold; color: #555; }";
    html += ".info-value { color: #0066cc; }";
    html += ".form-section { background-color: #f9f9f9; padding: 15px; border-radius: 5px; margin-bottom: 15px; }";
    html += "h2 { color: #555; font-size: 1.2em; margin-top: 0; }";
    html += "label { display: block; margin: 10px 0 5px 0; font-weight: bold; }";
    html += "input[type='number'], input[type='text'], input[type='password'], select { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }";
    html += "input[type='submit'] { background-color: #0066cc; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; margin-top: 10px; }";
    html += "input[type='submit']:hover { background-color: #0052a3; }";
    html += ".help-text { font-size: 0.9em; color: #666; margin-top: 5px; }";
    html += "</style>";
    html += "</head><body>";
    html += "<div class='container'>";
    html += "<h1>Focuser Setup - " + GetDeviceName() + "</h1>";
    
    // Current status section
    html += "<div class='info-section'>";
    html += "<h2>Current Status</h2>";
    html += "<div class='info-row'><span class='info-label'>Position:</span><span class='info-value'>" + String(GetPosition()) + " steps</span></div>";
    html += "<div class='info-row'><span class='info-label'>Temperature:</span><span class='info-value'>" + String(GetTemperature(), 2) + " °C</span></div>";
    if (IsTemperatureSensorValid()) {
      html += "<div class='info-row'><span class='info-label'>Temperature Sensor:</span><span class='info-value' style='color: #00aa00;'>Valid</span></div>";
      html += "<div class='info-row'><span class='info-label'>Raw Sensor Value:</span><span class='info-value'>" + String(GetLastRawTemperature(), 2) + " °C</span></div>";
    } else {
      html += "<div class='info-row'><span class='info-label'>Temperature Sensor:</span><span class='info-value' style='color: #cc0000;'>Invalid</span></div>";
      html += "<div class='info-row'><span class='info-label'>Raw Sensor Value:</span><span class='info-value' style='color: #cc0000;'>" + String(GetLastRawTemperature(), 2) + " °C</span></div>";
    }
    html += "<div class='info-row'><span class='info-label'>Temperature Offset:</span><span class='info-value'>" + String(GetTemperatureOffset(), 2) + " °C</span></div>";
    html += "<div class='info-row'><span class='info-label'>Temperature Sensor Pin:</span><span class='info-value'>GPIO " + String(GetTemperaturePin()) + "</span></div>";
    html += "<div class='info-row'><span class='info-label'>Max Position:</span><span class='info-value'>" + String(maxStep) + " steps</span></div>";
    html += "<div class='info-row'><span class='info-label'>Step Size:</span><span class='info-value'>" + String(stepSize, 2) + " microns</span></div>";
    html += "<div class='info-row'><span class='info-label'>Moving:</span><span class='info-value'>" + String(GetIsMoving() ? "Yes" : "No") + "</span></div>";
    html += "</div>";
    
    // WiFi status section
    html += "<div class='info-section'>";
    html += "<h2>WiFi Status</h2>";
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
    
    // Stepper configuration status section
    html += "<div class='info-section'>";
    html += "<h2>Stepper Configuration</h2>";
    html += "<div class='info-row'><span class='info-label'>Mode:</span><span class='info-value'>";
    switch(stepper->getStepMode()) {
      case FullStep: html += "Full Step (1)"; break;
      case HalfStep: html += "Half Step (1/2)"; break;
      case QuaterStep: html += "Quarter Step (1/4)"; break;
      case EighthStep: html += "Eighth Step (1/8)"; break;
      case SixteenthStep: html += "Sixteenth Step (1/16)"; break;
      case FullStep2Phase: html += "Full Step 2-Phase"; break;
      default: html += "Unknown"; break;
    }
    html += "</span></div>";
    html += "<div class='info-row'><span class='info-label'>Pin 1:</span><span class='info-value'>GPIO " + String(stepper->getPin1()) + "</span></div>";
    html += "<div class='info-row'><span class='info-label'>Pin 2:</span><span class='info-value'>GPIO " + String(stepper->getPin2()) + "</span></div>";
    html += "<div class='info-row'><span class='info-label'>Pin 3:</span><span class='info-value'>GPIO " + String(stepper->getPin3()) + "</span></div>";
    html += "<div class='info-row'><span class='info-label'>Pin 4:</span><span class='info-value'>GPIO " + String(stepper->getPin4()) + "</span></div>";
    html += "</div>";
    
    // Set Position form
    html += "<div class='form-section'>";
    html += "<form method='POST' action='/setup/v1/focuser/" + String(GetDeviceNumber()) + "/setup'>";
    html += "<h2>Set Current Position</h2>";
    html += "<label for='position'>New Position (steps):</label>";
    html += "<input type='number' id='position' name='position' min='0' max='" + String(maxStep) + "' value='" + String(GetPosition()) + "' required>";
    html += "<div class='help-text'>Set the current position value (useful after manual adjustment or homing)</div>";
    html += "<input type='submit' value='Set Position'>";
    html += "</form>";
    html += "</div>";
    
    // Set Temperature Offset form
    html += "<div class='form-section'>";
    html += "<form method='POST' action='/setup/v1/focuser/" + String(GetDeviceNumber()) + "/setup'>";
    html += "<h2>Calibrate Temperature Sensor</h2>";
    html += "<label for='tempoffset'>Temperature Offset (°C):</label>";
    html += "<input type='number' id='tempoffset' name='tempoffset' step='0.1' value='" + String(GetTemperatureOffset(), 2) + "' required>";
    html += "<div class='help-text'>Adjust this offset to calibrate the temperature sensor reading</div>";
    html += "<input type='submit' value='Set Temperature Offset'>";
    html += "</form>";
    html += "</div>";

    // Temperature Sensor Pin form
    html += "<div class='form-section'>";
    html += "<form method='POST' action='/setup/v1/focuser/" + String(GetDeviceNumber()) + "/setup'>";
    html += "<h2>Temperature Sensor Pin</h2>";
    html += "<label for='temp_pin'>GPIO Pin (DS18B20 Data):</label>";
    html += "<input type='number' id='temp_pin' name='temp_pin' min='0' max='16' value='" + String(GetTemperaturePin()) + "' required>";
    html += "<div class='help-text'>Set the GPIO pin used by the DS18B20 data line. Sensor bus will be reinitialized immediately.</div>";
    html += "<input type='submit' value='Set Temperature Pin'>";
    html += "</form>";
    html += "</div>";
    
    // WiFi Configuration form
    html += "<div class='form-section'>";
    html += "<form method='POST' action='/setup/v1/focuser/" + String(GetDeviceNumber()) + "/setup'>";
    html += "<h2>WiFi Configuration</h2>";
    html += "<label for='wifi_ssid'>WiFi SSID:</label>";
    html += "<input type='text' id='wifi_ssid' name='wifi_ssid' maxlength='31' value='" + String(wifiConfig.getSSID()) + "' required>";
    html += "<label for='wifi_password'>WiFi Password:</label>";
    html += "<input type='password' id='wifi_password' name='wifi_password' maxlength='63' value='' placeholder='Enter new password or leave empty'>";
    html += "<div class='help-text' style='color: #ff6600;'><strong>Warning:</strong> Device will restart after saving WiFi settings to connect to the new network.</div>";
    html += "<input type='submit' value='Save WiFi Settings'>";
    html += "</form>";
    html += "</div>";
    
    // Stepper Mode Configuration form
    html += "<div class='form-section'>";
    html += "<form method='POST' action='/setup/v1/focuser/" + String(GetDeviceNumber()) + "/setup'>";
    html += "<h2>Stepper Mode Configuration</h2>";
    html += "<label for='stepper_mode'>Stepping Mode:</label>";
    html += "<select id='stepper_mode' name='stepper_mode' style='width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box;'>";
    html += "<option value='0'" + String(stepper->getStepMode() == FullStep ? " selected" : "") + ">Full Step (1)</option>";
    html += "<option value='1'" + String(stepper->getStepMode() == HalfStep ? " selected" : "") + ">Half Step (1/2)</option>";
    html += "<option value='2'" + String(stepper->getStepMode() == QuaterStep ? " selected" : "") + ">Quarter Step (1/4)</option>";
    html += "<option value='3'" + String(stepper->getStepMode() == EighthStep ? " selected" : "") + ">Eighth Step (1/8)</option>";
    html += "<option value='4'" + String(stepper->getStepMode() == SixteenthStep ? " selected" : "") + ">Sixteenth Step (1/16)</option>";
    html += "<option value='5'" + String(stepper->getStepMode() == FullStep2Phase ? " selected" : "") + ">Full Step 2-Phase</option>";
    html += "</select>";
    html += "<div class='help-text'>Select the stepping mode for your motor driver. Higher resolution = smoother but slower movement.</div>";
    html += "<input type='submit' value='Set Stepper Mode'>";
    html += "</form>";
    html += "</div>";
    
    // Stepper Pins Configuration form
    html += "<div class='form-section'>";
    html += "<form method='POST' action='/setup/v1/focuser/" + String(GetDeviceNumber()) + "/setup'>";
    html += "<h2>Stepper Pin Configuration</h2>";
    html += "<label for='pin1'>Pin 1 (GPIO):</label>";
    html += "<input type='number' id='pin1' name='pin1' min='0' max='16' value='" + String(stepper->getPin1()) + "' required>";
    html += "<label for='pin2'>Pin 2 (GPIO):</label>";
    html += "<input type='number' id='pin2' name='pin2' min='0' max='16' value='" + String(stepper->getPin2()) + "' required>";
    html += "<label for='pin3'>Pin 3 (GPIO):</label>";
    html += "<input type='number' id='pin3' name='pin3' min='0' max='16' value='" + String(stepper->getPin3()) + "' required>";
    html += "<label for='pin4'>Pin 4 (GPIO):</label>";
    html += "<input type='number' id='pin4' name='pin4' min='0' max='16' value='" + String(stepper->getPin4()) + "' required>";
    html += "<div class='help-text' style='color: #ff6600;'><strong>Warning:</strong> Changing pins will immediately reconfigure GPIO. Verify motor connections!</div>";
    html += "<input type='submit' value='Set Stepper Pins'>";
    html += "</form>";
    html += "</div>";
    
    html += "<p style='text-align: center; color: #888; font-size: 0.9em; margin-top: 30px;'>";
    html += "<a href='/management/v1/description' style='color: #0066cc; text-decoration: none;'>Back to Management API</a>";
    html += "</p>";
    html += "</div>";
    html += "</body></html>";
    
    request->send(200, "text/html", html);
  }
};

#endif /* ARDUINO_FOCUSER_H */


#endif /* D5A14B4C_BBEB_4B58_A6B0_77C2F5D439ED */
