#ifndef MY_ROTATOR_H
#define MY_ROTATOR_H

#include "alpaca_api/Alpaca_Device_Rotator.h"

/**
 * @file MyRotator.h
 * @brief Example implementation of Rotator device
 * 
 * This example demonstrates how to create a concrete Rotator device
 * by extending the AlpacaDeviceRotator class and implementing the
 * rotator control logic for camera/instrument rotation.
 */

class MyRotator : public AlpacaDeviceRotator {
private:
  // Rotator configuration
  bool canReverse;                  // Supports reverse operation
  double stepSize;                  // Minimum step size in degrees
  
  // Current state
  double currentPosition;           // Current sky position (0-360 degrees)
  double mechanicalPosition;        // Current mechanical position (0-360 degrees)
  double targetPosition;            // Target position for movement
  bool isMoving;                    // Is rotator currently moving
  bool reverse;                     // Reverse state
  
  // Motor control pins (optional - for stepper motor control)
  int stepPin;
  int dirPin;
  int enablePin;
  
  // Movement parameters
  unsigned long lastStepTime;       // Time of last step
  unsigned long stepDelayMicros;    // Delay between steps (microseconds)
  double stepsPerDegree;            // Steps per degree for stepper motor
  
  /**
   * @brief Normalize angle to 0-360 range
   * @param angle Angle in degrees
   * @return Normalized angle (0-360)
   */
  double normalizeAngle(double angle) {
    while (angle < 0.0) angle += 360.0;
    while (angle >= 360.0) angle -= 360.0;
    return angle;
  }
  
  /**
   * @brief Calculate shortest rotation direction
   * @param current Current angle
   * @param target Target angle
   * @return Positive for clockwise, negative for counter-clockwise
   */
  double shortestRotation(double current, double target) {
    double diff = target - current;
    if (diff > 180.0) diff -= 360.0;
    if (diff < -180.0) diff += 360.0;
    return diff;
  }
  
  /**
   * @brief Update rotator movement
   * Called periodically to advance motor one step at a time
   */
  void updateMovement() {
    if (!isMoving) {
      if (enablePin >= 0) {
        digitalWrite(enablePin, HIGH); // Disable motor (active LOW)
      }
      return;
    }
    
    // Check if we've reached the target
    double diff = abs(targetPosition - currentPosition);
    if (diff < stepSize / 2.0) {
      currentPosition = targetPosition;
      mechanicalPosition = reverse ? normalizeAngle(360.0 - currentPosition) : currentPosition;
      isMoving = false;
      if (enablePin >= 0) {
        digitalWrite(enablePin, HIGH); // Disable motor (active LOW)
      }
      LOG_DEBUG("Rotator movement complete - Position: " + String(currentPosition));
      return;
    }
    
    unsigned long currentTime = micros();
    if (currentTime - lastStepTime < stepDelayMicros) {
      return; // Not time for next step yet
    }
    
    lastStepTime = currentTime;
    
    // Determine direction (shortest path)
    double rotation = shortestRotation(currentPosition, targetPosition);
    bool moveClockwise = (rotation > 0);
    
    // Set direction pin
    if (dirPin >= 0) {
      digitalWrite(dirPin, moveClockwise ? HIGH : LOW);
    }
    
    // Step the motor
    if (stepPin >= 0) {
      digitalWrite(stepPin, HIGH);
      delayMicroseconds(10);
      digitalWrite(stepPin, LOW);
    }
    
    // Update position
    double increment = stepSize;
    if (moveClockwise) {
      currentPosition += increment;
    } else {
      currentPosition -= increment;
    }
    currentPosition = normalizeAngle(currentPosition);
    
    // Update mechanical position based on reverse state
    mechanicalPosition = reverse ? normalizeAngle(360.0 - currentPosition) : currentPosition;
  }

public:
  /**
   * @brief Constructor for MyRotator
   * @param devicename Name of the rotator device
   * @param devicenumber Device number for Alpaca API
   * @param description Human-readable description
   * @param server Reference to AsyncWebServer
   * @param step_size_degrees Minimum step size in degrees (default: 0.1)
   * @param steps_per_degree Steps per degree for stepper motor (default: 100)
   * @param step_pin Optional stepper motor step pin
   * @param dir_pin Optional stepper motor direction pin
   * @param enable_pin Optional stepper motor enable pin
   */
  MyRotator(String devicename, int devicenumber, String description, 
            AsyncWebServer &server, double step_size_degrees = 0.1, 
            double steps_per_degree = 100.0,
            int step_pin = -1, int dir_pin = -1, int enable_pin = -1)
    : AlpacaDeviceRotator(devicename, devicenumber, description, server),
      canReverse(true),
      stepSize(step_size_degrees),
      currentPosition(0.0),
      mechanicalPosition(0.0),
      targetPosition(0.0),
      isMoving(false),
      reverse(false),
      stepPin(step_pin),
      dirPin(dir_pin),
      enablePin(enable_pin),
      lastStepTime(0),
      stepDelayMicros(2000),  // 2ms between steps
      stepsPerDegree(steps_per_degree) {
    
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
    
    LOG_DEBUG("MyRotator created - StepSize: " + String(stepSize) + " degrees");
  }
  
  // ==================== IRotator Interface Implementation ====================
  
  /**
   * @brief Get reverse capability
   * @return true if rotator supports reverse
   */
  bool GetCanReverse() override {
    return canReverse;
  }
  
  /**
   * @brief Check if rotator is currently moving
   * @return true if rotator is moving
   */
  bool GetIsMoving() override {
    return isMoving;
  }
  
  /**
   * @brief Get mechanical position
   * @return Current mechanical position in degrees (0-360)
   */
  double GetMechanicalPosition() override {
    return mechanicalPosition;
  }
  
  /**
   * @brief Get current sky position
   * @return Current position in degrees (0-360)
   */
  double GetPosition() override {
    return currentPosition;
  }
  
  /**
   * @brief Get reverse state
   * @return true if rotator is reversed
   */
  bool GetReverse() override {
    return reverse;
  }
  
  /**
   * @brief Set reverse state
   * @param reverse true to reverse rotator
   */
  void SetReverse(bool reverse) override {
    this->reverse = reverse;
    LOG_DEBUG("Rotator reverse set to: " + String(reverse ? "true" : "false"));
    
    // Update mechanical position based on new reverse state
    mechanicalPosition = reverse ? normalizeAngle(360.0 - currentPosition) : currentPosition;
  }
  
  /**
   * @brief Get minimum step size
   * @return Step size in degrees
   */
  double GetStepSize() override {
    return stepSize;
  }
  
  /**
   * @brief Get target position
   * @return Target position in degrees
   */
  double GetTargetPosition() override {
    return targetPosition;
  }
  
  /**
   * @brief Immediately halt rotator motion
   */
  void Halt() override {
    LOG_DEBUG("Halting rotator - Current position: " + String(currentPosition));
    isMoving = false;
    targetPosition = currentPosition;
    
    if (enablePin >= 0) {
      digitalWrite(enablePin, HIGH); // Disable motor (active LOW)
    }
  }
  
  /**
   * @brief Move rotator relative to current position
   * @param position Relative position in degrees
   */
  void Move(double position) override {
    double newTarget = normalizeAngle(currentPosition + position);
    LOG_DEBUG("Moving rotator relative by: " + String(position) + " degrees to position: " + String(newTarget));
    
    targetPosition = newTarget;
    isMoving = true;
    
    if (enablePin >= 0) {
      digitalWrite(enablePin, LOW); // Enable motor (active LOW)
    }
  }
  
  /**
   * @brief Move rotator to absolute position
   * @param position Absolute position in degrees (0-360)
   */
  void MoveAbsolute(double position) override {
    targetPosition = normalizeAngle(position);
    LOG_DEBUG("Moving rotator to absolute position: " + String(targetPosition) + " degrees");
    
    isMoving = true;
    
    if (enablePin >= 0) {
      digitalWrite(enablePin, LOW); // Enable motor (active LOW)
    }
  }
  
  /**
   * @brief Move rotator to mechanical position
   * @param position Mechanical position in degrees (0-360)
   */
  void MoveMechanical(double position) override {
    // Convert mechanical position to sky position based on reverse state
    double skyPosition = reverse ? normalizeAngle(360.0 - position) : position;
    targetPosition = skyPosition;
    
    LOG_DEBUG("Moving rotator to mechanical position: " + String(position) + " degrees (sky: " + String(targetPosition) + ")");
    
    isMoving = true;
    
    if (enablePin >= 0) {
      digitalWrite(enablePin, LOW); // Enable motor (active LOW)
    }
  }
  
  /**
   * @brief Sync rotator to position without moving
   * @param position Position to sync to in degrees
   */
  void Sync(double position) override {
    currentPosition = normalizeAngle(position);
    targetPosition = currentPosition;
    mechanicalPosition = reverse ? normalizeAngle(360.0 - currentPosition) : currentPosition;
    isMoving = false;
    
    LOG_DEBUG("Synced rotator to position: " + String(currentPosition) + " degrees");
  }
  
  // ==================== Public Methods ====================
  
  /**
   * @brief Update rotator state - call this from loop()
   * Handles movement updates
   */
  void update() {
    updateMovement();
  }
  
  /**
   * @brief Get current position in degrees
   * @return Current position (0-360)
   */
  double getCurrentPosition() {
    return currentPosition;
  }
  
  /**
   * @brief Get current mechanical position in degrees
   * @return Current mechanical position (0-360)
   */
  double getCurrentMechanicalPosition() {
    return mechanicalPosition;
  }
  
  /**
   * @brief Check if rotator is moving
   * @return true if moving
   */
  bool isRotatorMoving() {
    return isMoving;
  }
};

#endif // MY_ROTATOR_H
