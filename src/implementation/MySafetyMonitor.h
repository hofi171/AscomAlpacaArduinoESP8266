#ifndef MY_SAFETY_MONITOR_H
#define MY_SAFETY_MONITOR_H

#include "alpaca_api/Alpaca_Device_SafetyMonitor.h"

/**
 * @file MySafetyMonitor.h
 * @brief Example implementation of SafetyMonitor device
 * 
 * This example demonstrates how to create a concrete SafetyMonitor device
 * by extending the AlpacaDeviceSafetyMonitor class and implementing the
 * safety checking logic.
 */

class MySafetyMonitor : public AlpacaDeviceSafetyMonitor {
private:
  // Example: Pin connected to a safety sensor
  int safetySensorPin;
  
  // Example: Safety state flags
  bool weatherSafe;
  bool powerSafe;
  bool hardwareSafe;
  
  /**
   * @brief Check all safety sensors
   * @return true if all conditions are safe
   */
  bool checkSensors() {
    // Example: Read digital sensor from pin
    // In a real implementation, you would read actual sensors
    bool sensorState = digitalRead(safetySensorPin);
    
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
   * @brief Constructor for MySafetyMonitor
   * @param devicename Name of the safety monitor device
   * @param devicenumber Device number for Alpaca API
   * @param description Human-readable description
   * @param server Reference to AsyncWebServer
   * @param sensorPin Arduino pin number for safety sensor (optional)
   */
  MySafetyMonitor(String devicename, int devicenumber, String description, 
                  AsyncWebServer &server, int sensorPin = -1)
    : AlpacaDeviceSafetyMonitor(devicename, devicenumber, description, server),
      safetySensorPin(sensorPin),
      weatherSafe(true),
      powerSafe(true),
      hardwareSafe(true) {
    
    // Initialize the sensor pin if provided
    if (safetySensorPin >= 0) {
      pinMode(safetySensorPin, INPUT);
      LOG_DEBUG("MySafetyMonitor initialized with sensor pin: " + String(safetySensorPin));
    }
    
    LOG_DEBUG("MySafetyMonitor created: " + devicename);
  }
  
  /**
   * @brief Override GetIsSafe to implement actual safety checking
   * @return true if all safety conditions are met, false otherwise
   */
  bool GetIsSafe() override {
    bool isSafe = checkSensors();
    
    LOG_DEBUG("Safety check - Weather: " + String(weatherSafe) + " Power: " + String(powerSafe) + " Hardware: " + String(hardwareSafe) + " Overall: " + String(isSafe));
    
    return isSafe;
  }
  
  // Optional: Add custom methods for detailed safety status
  bool IsWeatherSafe() const { return weatherSafe; }
  bool IsPowerSafe() const { return powerSafe; }
  bool IsHardwareSafe() const { return hardwareSafe; }
};

#endif /* MY_SAFETY_MONITOR_H */
