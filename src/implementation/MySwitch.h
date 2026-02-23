#ifndef MY_SWITCH_H
#define MY_SWITCH_H

#include "alpaca_api/Alpaca_Device_Switch.h"
#include <string>

/**
 * @file MySwitch.h
 * @brief Example implementation of Switch device
 * 
 * This example demonstrates how to create a concrete Switch device
 * by extending the AlpacaDeviceSwitch class and implementing the
 * switch control logic for managing multiple switches.
 */

// Structure to define a single switch
struct SwitchData {
  std::string name;
  std::string description;
  bool canWrite;
  bool canAsync;
  double value;
  double minValue;
  double maxValue;
  double stepValue;
  int outputPin;             // GPIO pin for output (-1 if not used)
  bool isPWM;                // True if PWM output, false if digital
  bool stateChangeComplete;  // For async operations
};

class MySwitch : public AlpacaDeviceSwitch {
private:
  int maxSwitch;                    // Number of switches
  SwitchData* switches;             // Array of switch data
  
  /**
   * @brief Validate switch ID
   * @param id Switch ID to validate
   * @return true if valid
   */
  bool isValidSwitchId(int id) {
    return (id >= 0 && id < maxSwitch);
  }
  
  /**
   * @brief Apply physical output based on switch state
   * @param id Switch ID
   */
  void applyOutput(int id) {
    if (!isValidSwitchId(id)) return;
    
    SwitchData& sw = switches[id];
    if (sw.outputPin < 0) return;
    
    if (sw.isPWM) {
      // PWM output (analog value)
      int pwmValue = (int)map((long)(sw.value * 100), 
                               (long)(sw.minValue * 100), 
                               (long)(sw.maxValue * 100), 
                               0, 255);
      analogWrite(sw.outputPin, pwmValue);
      LOG_DEBUG("Switch " + String(id) + " PWM output: " + String(pwmValue));
    } else {
      // Digital output (boolean)
      bool state = (sw.value > 0.0);
      digitalWrite(sw.outputPin, state ? HIGH : LOW);
      LOG_DEBUG("Switch " + String(id) + " digital output: " + (state ? "HIGH" : "LOW"));
    }
  }

public:
  /**
   * @brief Constructor for MySwitch
   * @param devicename Name of the switch device
   * @param devicenumber Device number for Alpaca API
   * @param description Human-readable description
   * @param server Reference to AsyncWebServer
   * @param num_switches Number of switches to manage (default: 8)
   */
  MySwitch(String devicename, int devicenumber, String description, 
           AsyncWebServer &server, int num_switches = 8)
    : AlpacaDeviceSwitch(devicename, devicenumber, description, server),
      maxSwitch(num_switches) {
    
    // Allocate switch array
    switches = new SwitchData[maxSwitch];
    
    // Initialize switches with default values
    for (int i = 0; i < maxSwitch; i++) {
      switches[i].name = "Switch " + std::to_string(i);
      switches[i].description = "Switch device " + std::to_string(i);
      switches[i].canWrite = true;
      switches[i].canAsync = false;  // Set to true for ISwitchV3+ async support
      switches[i].value = 0.0;
      switches[i].minValue = 0.0;
      switches[i].maxValue = 1.0;
      switches[i].stepValue = 1.0;
      switches[i].outputPin = -1;
      switches[i].isPWM = false;
      switches[i].stateChangeComplete = true;
    }
    
    LOG_DEBUG("MySwitch created with " + String(maxSwitch) + " switches");
  }
  
  virtual ~MySwitch() {
    delete[] switches;
  }
  
  // ==================== Configuration Methods ====================
  
  /**
   * @brief Configure a specific switch
   * @param id Switch ID (0 to maxSwitch-1)
   * @param name Switch name
   * @param description Switch description
   * @param canWrite Can the switch be written to
   * @param minValue Minimum value
   * @param maxValue Maximum value
   * @param stepValue Step value
   * @param outputPin GPIO pin for output (-1 for none)
   * @param isPWM True for PWM/analog output, false for digital
   */
  void configureSwitch(int id, const std::string& name, const std::string& description,
                       bool canWrite, double minValue, double maxValue, double stepValue,
                       int outputPin = -1, bool isPWM = false) {
    if (!isValidSwitchId(id)) {
      LOG_DEBUG("ERROR: Invalid switch ID: " + String(id));
      return;
    }
    
    switches[id].name = name;
    switches[id].description = description;
    switches[id].canWrite = canWrite;
    switches[id].minValue = minValue;
    switches[id].maxValue = maxValue;
    switches[id].stepValue = stepValue;
    switches[id].outputPin = outputPin;
    switches[id].isPWM = isPWM;
    
    // Initialize output pin if specified
    if (outputPin >= 0) {
      pinMode(outputPin, OUTPUT);
      if (isPWM) {
        analogWrite(outputPin, 0);
      } else {
        digitalWrite(outputPin, LOW);
      }
    }
    
    LOG_DEBUG("Configured switch " + String(id) + ": " + name.c_str() + " on pin " + String(outputPin));
  }
  
  // ==================== ISwitch Interface Implementation ====================
  
  /**
   * @brief Get the number of switch devices
   * @return Number of switches
   */
  int GetMaxSwitch() override {
    return maxSwitch;
  }
  
  /**
   * @brief Check if switch supports async operation
   * @param switchNumber Switch ID
   * @return true if async supported
   */
  bool GetCanAsync(int switchNumber) override {
    if (!isValidSwitchId(switchNumber)) return false;
    return switches[switchNumber].canAsync;
  }
  
  /**
   * @brief Check if switch can be written to
   * @param switchNumber Switch ID
   * @return true if writable
   */
  bool GetCanWrite(int switchNumber) override {
    if (!isValidSwitchId(switchNumber)) return false;
    return switches[switchNumber].canWrite;
  }
  
  /**
   * @brief Get switch state as boolean
   * @param switchNumber Switch ID
   * @return Switch state (true/false)
   */
  bool GetSwitch(int switchNumber) override {
    if (!isValidSwitchId(switchNumber)) return false;
    return (switches[switchNumber].value > 0.0);
  }
  
  /**
   * @brief Get switch description
   * @param switchNumber Switch ID
   * @return Description string
   */
  std::string GetSwitchDescription(int switchNumber) override {
    if (!isValidSwitchId(switchNumber)) return "Invalid switch";
    return switches[switchNumber].description;
  }
  
  /**
   * @brief Get switch name
   * @param switchNumber Switch ID
   * @return Name string
   */
  std::string GetSwitchName(int switchNumber) override {
    if (!isValidSwitchId(switchNumber)) return "Invalid";
    return switches[switchNumber].name;
  }
  
  /**
   * @brief Get switch value
   * @param switchNumber Switch ID
   * @return Value as double
   */
  double GetSwitchValue(int switchNumber) override {
    if (!isValidSwitchId(switchNumber)) return 0.0;
    return switches[switchNumber].value;
  }
  
  /**
   * @brief Get minimum switch value
   * @param switchNumber Switch ID
   * @return Minimum value
   */
  double GetMinSwitchValue(int switchNumber) override {
    if (!isValidSwitchId(switchNumber)) return 0.0;
    return switches[switchNumber].minValue;
  }
  
  /**
   * @brief Get maximum switch value
   * @param switchNumber Switch ID
   * @return Maximum value
   */
  double GetMaxSwitchValue(int switchNumber) override {
    if (!isValidSwitchId(switchNumber)) return 1.0;
    return switches[switchNumber].maxValue;
  }
  
  /**
   * @brief Check if async state change is complete
   * @param switchNumber Switch ID
   * @return true if complete
   */
  bool GetStateChangeComplete(int switchNumber) override {
    if (!isValidSwitchId(switchNumber)) return true;
    return switches[switchNumber].stateChangeComplete;
  }
  
  /**
   * @brief Get switch step size
   * @param switchNumber Switch ID
   * @return Step size
   */
  double GetSwitchStep(int switchNumber) override {
    if (!isValidSwitchId(switchNumber)) return 1.0;
    return switches[switchNumber].stepValue;
  }
  
  /**
   * @brief Set switch state asynchronously
   * @param switchNumber Switch ID
   * @param state Boolean state
   */
  void SetAsync(int switchNumber, bool state) override {
    if (!isValidSwitchId(switchNumber)) return;
    if (!switches[switchNumber].canWrite) {
      LOG_DEBUG("ERROR: Switch " + String(switchNumber) + " is not writable");
      return;
    }
    
    switches[switchNumber].stateChangeComplete = false;
    switches[switchNumber].value = state ? switches[switchNumber].maxValue : switches[switchNumber].minValue;
    applyOutput(switchNumber);
    
    // Simulate async completion (in real implementation, this might take time)
    switches[switchNumber].stateChangeComplete = true;
    
    LOG_DEBUG("Switch " + String(switchNumber) + " async set to: " + (state ? "true" : "false"));
  }
  
  /**
   * @brief Set switch value asynchronously
   * @param switchNumber Switch ID
   * @param value Double value
   */
  void SetAsyncValue(int switchNumber, double value) override {
    if (!isValidSwitchId(switchNumber)) return;
    if (!switches[switchNumber].canWrite) {
      LOG_DEBUG("ERROR: Switch " + String(switchNumber) + " is not writable");
      return;
    }
    
    // Clamp value to valid range
    if (value < switches[switchNumber].minValue) value = switches[switchNumber].minValue;
    if (value > switches[switchNumber].maxValue) value = switches[switchNumber].maxValue;
    
    switches[switchNumber].stateChangeComplete = false;
    switches[switchNumber].value = value;
    applyOutput(switchNumber);
    
    // Simulate async completion
    switches[switchNumber].stateChangeComplete = true;
    
    LOG_DEBUG("Switch " + String(switchNumber) + " async value set to: " + String(value));
  }
  
  /**
   * @brief Set switch state
   * @param switchNumber Switch ID
   * @param state Boolean state
   */
  void SetSwitch(int switchNumber, bool state) override {
    if (!isValidSwitchId(switchNumber)) return;
    if (!switches[switchNumber].canWrite) {
      LOG_DEBUG("ERROR: Switch " + String(switchNumber) + " is not writable");
      return;
    }
    
    switches[switchNumber].value = state ? switches[switchNumber].maxValue : switches[switchNumber].minValue;
    applyOutput(switchNumber);
    
    LOG_DEBUG("Switch " + String(switchNumber) + " set to: " + (state ? "true" : "false"));
  }
  
  /**
   * @brief Set switch name
   * @param switchNumber Switch ID
   * @param name New name
   */
  void SetSwitchName(int switchNumber, const std::string& name) override {
    if (!isValidSwitchId(switchNumber)) return;
    switches[switchNumber].name = name;
    LOG_DEBUG("Switch " + String(switchNumber) + " name set to: " + name.c_str());
  }
  
  /**
   * @brief Set switch value
   * @param switchNumber Switch ID
   * @param value Double value
   */
  void SetSwitchValue(int switchNumber, double value) override {
    if (!isValidSwitchId(switchNumber)) return;
    if (!switches[switchNumber].canWrite) {
      LOG_DEBUG("ERROR: Switch " + String(switchNumber) + " is not writable");
      return;
    }
    
    // Clamp value to valid range
    if (value < switches[switchNumber].minValue) value = switches[switchNumber].minValue;
    if (value > switches[switchNumber].maxValue) value = switches[switchNumber].maxValue;
    
    switches[switchNumber].value = value;
    applyOutput(switchNumber);
    
    LOG_DEBUG("Switch " + String(switchNumber) + " value set to: " + String(value));
  }
  
  // ==================== Public Helper Methods ====================
  
  /**
   * @brief Get current value of a switch
   * @param switchNumber Switch ID
   * @return Current value
   */
  double getSwitchValue(int switchNumber) {
    if (!isValidSwitchId(switchNumber)) return 0.0;
    return switches[switchNumber].value;
  }
  
  /**
   * @brief Get current state of a switch as boolean
   * @param switchNumber Switch ID
   * @return Current state
   */
  bool getSwitchState(int switchNumber) {
    if (!isValidSwitchId(switchNumber)) return false;
    return (switches[switchNumber].value > 0.0);
  }
  
  /**
   * @brief Print status of all switches to serial
   */
  void printStatus() {
    Serial.println("\n=== Switch Status ===");
    for (int i = 0; i < maxSwitch; i++) {
      Serial.print("  [");
      Serial.print(i);
      Serial.print("] ");
      Serial.print(switches[i].name.c_str());
      Serial.print(": ");
      if (switches[i].isPWM) {
        Serial.print(switches[i].value, 2);
        Serial.print(" (");
        Serial.print(switches[i].minValue, 0);
        Serial.print("-");
        Serial.print(switches[i].maxValue, 0);
        Serial.print(")");
      } else {
        Serial.print(switches[i].value > 0.0 ? "ON" : "OFF");
      }
      if (switches[i].outputPin >= 0) {
        Serial.print(" [Pin ");
        Serial.print(switches[i].outputPin);
        Serial.print("]");
      }
      Serial.println();
    }
    Serial.println("===================");
  }
};

#endif // MY_SWITCH_H
