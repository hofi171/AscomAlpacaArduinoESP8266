#ifndef ARDUINO_SAFETY_MONITOR_H
#define ARDUINO_SAFETY_MONITOR_H

#include <EEPROM.h>
#include <ESP8266WiFi.h>
#include "WiFi_Config.h"
#include "alpaca_api/Alpaca_Device_SafetyMonitor.h"

/**
 * @file ArduinoSafetyMonitor.h
 * @brief Example implementation of SafetyMonitor device
 * 
 * This example demonstrates how to create a concrete SafetyMonitor device
 * by extending the AlpacaDeviceSafetyMonitor class and implementing the
 * safety checking logic.
 */

class ArduinoSafetyMonitor : public AlpacaDeviceSafetyMonitor {
private:
  // EEPROM layout for SafetyMonitor settings
  // Address 278: valid marker (2 bytes, uint16_t)
  // Address 280: safetySensorPin (4 bytes, int)
  static const int SM_EEPROM_VALID_ADDR      = 278;
  static const int SM_EEPROM_SENSOR_PIN_ADDR = 280;
  static const uint16_t SM_VALID_MARKER      = 0x5AFE;

  // WiFi configuration manager
  WiFiConfig wifiConfig;

  // Example: Pin connected to a safety sensor
  int safetySensorPin;
  
  // Example: Safety state flags
  bool weatherSafe;
  bool powerSafe;
  bool hardwareSafe;
  
  /**
   * @brief Save safetySensorPin to EEPROM
   */
  void savePinToEEPROM() {
    EEPROM.put(SM_EEPROM_SENSOR_PIN_ADDR, safetySensorPin);
    uint16_t marker = SM_VALID_MARKER;
    EEPROM.put(SM_EEPROM_VALID_ADDR, marker);
    EEPROM.commit();
  }

  /**
   * @brief Load safetySensorPin from EEPROM (if valid marker present)
   */
  void loadPinFromEEPROM() {
    uint16_t marker;
    EEPROM.get(SM_EEPROM_VALID_ADDR, marker);
    if (marker == SM_VALID_MARKER) {
      int pin;
      EEPROM.get(SM_EEPROM_SENSOR_PIN_ADDR, pin);
      if (pin >= -1 && pin <= 16) {
        safetySensorPin = pin;
        LOG_DEBUG("ArduinoSafetyMonitor loaded sensor pin from EEPROM: " + String(safetySensorPin));
      }
    }
  }

  /**
   * @brief Check all safety sensors
   * @return true if all conditions are safe
   */
  bool checkSensors() {
    // Example: Read digital sensor from pin
    // In a real implementation, you would read actual sensors
    bool sensorState = (safetySensorPin >= 0) ? (digitalRead(safetySensorPin) == HIGH) : true;
    
    // Example: Check multiple safety conditions
    weatherSafe = checkWeatherConditions();
    powerSafe = checkPowerStatus();
    hardwareSafe = sensorState;
    
    // System is safe only if all conditions are met
    return weatherSafe && powerSafe && hardwareSafe;
  }
  
  /**
   * @brief Check weather conditions
   * Example: Check for rain, wind, clouds, etc.
   */
  bool checkWeatherConditions() {
    // Example implementation
    // In real use, you might read from weather sensors or API
    
    // For demonstration: always return true
    // Replace with actual weather checking logic
    return true;
  }
  
  /**
   * @brief Check power status
   * Example: Check UPS, voltage levels, etc.
   */
  bool checkPowerStatus() {
    // Example implementation
    // In real use, you might check UPS status, voltage levels
    
    // For demonstration: always return true
    // Replace with actual power checking logic
    return true;
  }

public:
  /**
   * @brief Constructor for ArduinoSafetyMonitor
   * @param devicename Name of the safety monitor device
   * @param devicenumber Device number for Alpaca API
   * @param description Human-readable description
   * @param server Reference to AsyncWebServer
   * @param sensorPin GPIO pin number for safety sensor (-1 to disable)
   */
  ArduinoSafetyMonitor(String devicename, int devicenumber, String description, 
                       AsyncWebServer &server, int sensorPin = -1)
    : AlpacaDeviceSafetyMonitor(devicename, devicenumber, description, server, true),
      safetySensorPin(sensorPin),
      weatherSafe(true),
      powerSafe(true),
      hardwareSafe(true) {
    
    // Load WiFi config from EEPROM
    wifiConfig.loadFromEEPROM();

    // Load pin from EEPROM (overrides constructor argument if valid data exists)
    loadPinFromEEPROM();

    // Initialize the sensor pin if provided
    if (safetySensorPin >= 0) {
      pinMode(safetySensorPin, INPUT);
      LOG_DEBUG("ArduinoSafetyMonitor initialized with sensor pin: " + String(safetySensorPin));
    }
    
    LOG_DEBUG("ArduinoSafetyMonitor created: " + devicename);
  }
  
  /**
   * @brief Override GetIsSafe to implement actual safety checking
   * @return true if all safety conditions are met, false otherwise
   */
  bool GetIsSafe() override {
    bool isSafe = checkSensors();
    
    LOG_DEBUG("Safety check - Weather: " + String(weatherSafe) +
              " Power: " + String(powerSafe) +
              " Hardware: " + String(hardwareSafe) +
              " Overall: " + String(isSafe));
    
    return isSafe;
  }
  
  // Optional: Add custom methods for detailed safety status
  bool IsWeatherSafe()  const { return weatherSafe; }
  bool IsPowerSafe()    const { return powerSafe; }
  bool IsHardwareSafe() const { return hardwareSafe; }

  /**
   * @brief Setup page — displays current safety state and allows pin configuration
   */
  void setupHandler(AsyncWebServerRequest *request) override {
    // Handle POST
    if (request->method() == HTTP_POST) {
      String message;

      if (request->hasParam("sensor_pin", true)) {
        int newPin = request->getParam("sensor_pin", true)->value().toInt();
        if (newPin >= -1 && newPin <= 16) {
          if (safetySensorPin >= 0) {
            pinMode(safetySensorPin, INPUT);
          }
          safetySensorPin = newPin;
          if (safetySensorPin >= 0) {
            pinMode(safetySensorPin, INPUT);
          }
          savePinToEEPROM();
          message += "Sensor pin saved: " + String(safetySensorPin) + "<br>";
          LOG_DEBUG("ArduinoSafetyMonitor sensor pin updated to: " + String(safetySensorPin));
        } else {
          message += "Error: Invalid pin value (use -1 to 16).<br>";
        }
      }

      if (request->hasParam("wifi_ssid", true) && request->hasParam("wifi_password", true)) {
        String newSSID = request->getParam("wifi_ssid", true)->value();
        String newPassword = request->getParam("wifi_password", true)->value();
        if (newSSID.length() > 0) {
          if (wifiConfig.saveToEEPROM(newSSID, newPassword)) {
            message += "WiFi credentials saved for SSID: " + newSSID + "<br>";
            message += "<strong>Please restart the device to connect to the new network.</strong><br>";
            String html = "<html><head><meta http-equiv='refresh' content='3;url=/setup/v1/safetymonitor/" +
                          String(GetDeviceNumber()) + "/setup'></head><body>";
            html += "<h1>Safety Monitor Setup</h1><p>" + message + "</p><p>Restarting...</p></body></html>";
            request->send(200, "text/html", html);
            delay(500);
            ESP.reset();
            return;
          } else {
            message += "Error: Failed to save WiFi credentials.<br>";
          }
        } else {
          message += "Error: WiFi SSID cannot be empty.<br>";
        }
      }

      String html = "<html><head><meta http-equiv='refresh' content='2;url=/setup/v1/safetymonitor/" +
                    String(GetDeviceNumber()) + "/setup'></head><body>";
      html += "<h1>Safety Monitor Setup</h1>";
      html += "<p>" + (message.length() > 0 ? message : String("No changes submitted")) + "</p>";
      html += "<p>Redirecting back to setup page...</p></body></html>";
      request->send(200, "text/html", html);
      return;
    }

    checkSensors(); // refresh state before rendering

    String html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>Safety Monitor Setup</title>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; }";
    html += "h1 { color: #333; }";
    html += ".container { max-width: 600px; margin: 0 auto; background-color: white; padding: 20px; border-radius: 8px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }";
    html += ".info-section { background-color: #e8f4f8; padding: 15px; border-radius: 5px; margin-bottom: 20px; }";
    html += ".form-section { background-color: #f9f9f9; padding: 15px; border-radius: 5px; margin-bottom: 15px; }";
    html += ".info-row { display: flex; justify-content: space-between; margin: 8px 0; }";
    html += ".info-label { font-weight: bold; color: #555; }";
    html += ".info-value { color: #0066cc; }";
    html += ".safe    { color: #00aa00; }";
    html += ".unsafe  { color: #cc0000; }";
    html += "h2 { color: #555; font-size: 1.2em; margin-top: 0; }";
    html += "label { display: block; margin: 10px 0 5px 0; font-weight: bold; }";
    html += "input[type='number'], input[type='text'], input[type='password'] { width: 100%; padding: 8px; border: 1px solid #ddd; border-radius: 4px; box-sizing: border-box; }";
    html += "input[type='submit'] { background-color: #0066cc; color: white; padding: 10px 20px; border: none; border-radius: 4px; cursor: pointer; margin-top: 10px; }";
    html += "input[type='submit']:hover { background-color: #0052a3; }";
    html += ".help-text { font-size: 0.9em; color: #666; margin-top: 5px; }";
    html += "</style></head><body><div class='container'>";

    html += "<h1>Safety Monitor - " + GetDeviceName() + "</h1>";

    auto safeStr = [](bool v) -> String { return v ? "<span class='safe'>Safe</span>" : "<span class='unsafe'>UNSAFE</span>"; };
    bool overall = weatherSafe && powerSafe && hardwareSafe;

    // Status section
    html += "<div class='info-section'><h2>Current Status</h2>";
    html += "<div class='info-row'><span class='info-label'>Overall:</span><span>" + safeStr(overall) + "</span></div>";
    html += "<div class='info-row'><span class='info-label'>Weather:</span><span>" + safeStr(weatherSafe) + "</span></div>";
    html += "<div class='info-row'><span class='info-label'>Power:</span><span>" + safeStr(powerSafe) + "</span></div>";
    html += "<div class='info-row'><span class='info-label'>Hardware (pin " + String(safetySensorPin) + "):</span><span>" + safeStr(hardwareSafe) + "</span></div>";
    html += "</div>";

    // WiFi status section
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

    // Sensor pin configuration form
    html += "<div class='form-section'><form method='POST' action='/setup/v1/safetymonitor/" + String(GetDeviceNumber()) + "/setup'>";
    html += "<h2>Sensor Pin Configuration</h2>";
    html += "<label for='sensor_pin'>Safety Sensor Pin (-1 = disabled):</label>";
    html += "<input type='number' id='sensor_pin' name='sensor_pin' min='-1' max='16' value='" + String(safetySensorPin) + "' required>";
    html += "<div class='help-text'>GPIO pin connected to the safety sensor (HIGH = safe). Use -1 to disable hardware sensor monitoring.</div>";
    html += "<input type='submit' value='Save Sensor Pin'>";
    html += "</form></div>";

    // WiFi configuration form
    html += "<div class='form-section'><form method='POST' action='/setup/v1/safetymonitor/" + String(GetDeviceNumber()) + "/setup'>";
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
};

#endif /* ARDUINO_SAFETY_MONITOR_H */
