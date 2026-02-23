#ifndef MY_FOCUSER_H
#define MY_FOCUSER_H

#include "alpaca_api/Alpaca_Device_Focuser.h"

/**
 * @file MyFocuser.h
 * @brief Example implementation of Focuser device
 * 
 * This example demonstrates how to create a concrete Focuser device
 * by extending the AlpacaDeviceFocuser class and implementing the
 * focuser control logic with stepper motor integration.
 */

class MyFocuser : public AlpacaDeviceFocuser {
private:
  // Focuser configuration
  bool absolute;                    // Supports absolute positioning
  int maxStep;                      // Maximum step position
  int maxIncrement;                 // Maximum increment per move
  double stepSize;                  // Step size in microns
  
  // Current state
  int currentPosition;              // Current position in steps
  int targetPosition;               // Target position for movement
  bool isMoving;                    // Is focuser currently moving
  
  // Temperature compensation
  bool tempCompAvailable;           // Temperature compensation available
  bool tempComp;                    // Temperature compensation enabled
  double temperature;               // Current temperature in Celsius
  
  // Motor control pins (optional - for stepper motor control)
  int stepPin;
  int dirPin;
  int enablePin;
  
  // Movement parameters
  unsigned long lastStepTime;       // Time of last step
  unsigned long stepDelayMicros;    // Delay between steps (microseconds)
  
  /**
   * @brief Update focuser movement
   * Called periodically to advance motor one step at a time
   */
  void updateMovement() {
    if (!isMoving || currentPosition == targetPosition) {
      isMoving = false;
      if (enablePin >= 0) {
        digitalWrite(enablePin, HIGH); // Disable motor (active LOW)
      }
      return;
    }
    
    unsigned long currentTime = micros();
    if (currentTime - lastStepTime < stepDelayMicros) {
      return; // Not time for next step yet
    }
    
    lastStepTime = currentTime;
    
    // Determine direction
    bool moveForward = (targetPosition > currentPosition);
    
    // Set direction pin
    if (dirPin >= 0) {
      digitalWrite(dirPin, moveForward ? HIGH : LOW);
    }
    
    // Step the motor
    if (stepPin >= 0) {
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(stepPin, LOW);
    }
    
    // Update position
    if (moveForward) {
      currentPosition++;
    } else {
      currentPosition--;
    }
    
    // Check if movement is complete
    if (currentPosition == targetPosition) {
      isMoving = false;
      if (enablePin >= 0) {
        digitalWrite(enablePin, HIGH); // Disable motor (active LOW)
      }
      LOG_DEBUG("Focuser movement complete - Position: " + String(currentPosition));
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
    
    if (currentTime - lastTempUpdate > 5000) { // Update every 5 seconds
      lastTempUpdate = currentTime;
      // Simulate temperature variation around 20°C
      temperature = 20.0 + (random(-100, 100) / 100.0);
    }
  }

public:
  /**
   * @brief Constructor for MyFocuser
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
  MyFocuser(String devicename, int devicenumber, String description, 
            AsyncWebServer &server, int max_step = 10000, double step_size_microns = 5.0,
            int step_pin = -1, int dir_pin = -1, int enable_pin = -1)
    : AlpacaDeviceFocuser(devicename, devicenumber, description, server),
      absolute(true),
      maxStep(max_step),
      maxIncrement(1000),
      stepSize(step_size_microns),
      currentPosition(0),
      targetPosition(0),
      isMoving(false),
      tempCompAvailable(true),
      tempComp(false),
      temperature(20.0),
      stepPin(step_pin),
      dirPin(dir_pin),
      enablePin(enable_pin),
      lastStepTime(0),
      stepDelayMicros(2000) { // 2ms between steps = 500 steps/sec
    
    // Initialize motor control pins if provided
    if (stepPin >= 0) {
      pinMode(stepPin, OUTPUT);
      digitalWrite(stepPin, LOW);
    }
    if (dirPin >= 0) {
      pinMode(dirPin, OUTPUT);
      digitalWrite(dirPin, LOW);
    }
    if (enablePin >= 0) {
      pinMode(enablePin, OUTPUT);
      digitalWrite(enablePin, HIGH); // Disabled (active LOW)
    }
    
    LOG_DEBUG("MyFocuser created - MaxStep: " + String(maxStep) + " StepSize: " + String(stepSize) + " microns");
  }
  
  // ==================== IFocuser Interface Implementation ====================
  
  /**
   * @brief Get absolute positioning capability
   * @return true if focuser supports absolute positioning
   */
  bool GetAbsolute() override {
    return absolute;
  }
  
  /**
   * @brief Check if focuser is currently moving
   * @return true if focuser is moving
   */
  bool GetIsMoving() override {
    return isMoving;
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
    return currentPosition;
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
    LOG_DEBUG("Halt command received - stopping at position: " + String(currentPosition));
    targetPosition = currentPosition;
    isMoving = false;
    
    if (enablePin >= 0) {
      digitalWrite(enablePin, HIGH); // Disable motor (active LOW)
    }
  }
  
  /**
   * @brief Move focuser to new position
   * @param position Target position (absolute position)
   */
  void Move(int position) override {
    // Validate position
    if (position < 0 || position > maxStep) {
      LOG_DEBUG("Invalid position requested: " + String(position) + " (valid range: 0 - " + String(maxStep) + ")");
      return;
    }
    
    // Check if move is within max increment
    int moveDistance = abs(position - currentPosition);
    if (moveDistance > maxIncrement) {
      LOG_DEBUG("Move distance " + String(moveDistance) + " exceeds max increment " + String(maxIncrement));
      // You could either reject the move or break it into smaller moves
      // For this example, we'll allow it but log a warning
    }
    
    LOG_DEBUG("Moving focuser from " + String(currentPosition) + " to " + String(position));
    
    targetPosition = position;
    isMoving = (currentPosition != targetPosition);
    
    if (isMoving) {
      // Enable motor
      if (enablePin >= 0) {
        //digitalWrite(enablePin, LOW); // Enable motor (active LOW)
      }
      
      // Set initial direction
      if (dirPin >= 0) {
        //digitalWrite(dirPin, (targetPosition > currentPosition) ? HIGH : LOW);
      }     

      lastStepTime = micros();
    }
  }
  
  // ==================== Additional Methods ====================
  
  /**
   * @brief Update focuser state
   * Call this periodically from loop() to handle movement and temperature updates
   */
  void update() {
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
          if (!isMoving && abs(compensation) > 0) {
            int newPosition = currentPosition + compensation;
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
    return targetPosition;
  }
  
  /**
   * @brief Set motor speed (steps per second)
   * @param stepsPerSecond Speed in steps per second
   */
  void SetSpeed(int stepsPerSecond) {
    if (stepsPerSecond > 0 && stepsPerSecond <= 5000) {
      stepDelayMicros = 1000000 / stepsPerSecond;
      LOG_DEBUG("Motor speed set to " + String(stepsPerSecond) + " steps/sec");
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
      currentPosition = position;
      targetPosition = position;
      LOG_DEBUG("Current position set to: " + String(currentPosition));
    }
  }
  
  /**
   * @brief Manual temperature override (for testing)
   * @param temp Temperature in Celsius
   */
  void SetTemperature(double temp) {
    temperature = temp;
  }
};

#endif /* MY_FOCUSER_H */
