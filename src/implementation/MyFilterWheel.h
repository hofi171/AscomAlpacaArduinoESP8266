#ifndef MY_FILTERWHEEL_H
#define MY_FILTERWHEEL_H

#include "alpaca_api/Alpaca_Device_FilterWheel.h"

/**
 * @file MyFilterWheel.h
 * @brief Example implementation of FilterWheel device
 * 
 * This example demonstrates how to create a concrete FilterWheel device
 * by extending the AlpacaDeviceFilterWheel class and implementing the
 * filter wheel control logic.
 */

class MyFilterWheel : public AlpacaDeviceFilterWheel {
private:
  // Filter wheel configuration
  int numFilters;
  int currentPosition;
  int targetPosition;
  bool isMoving;
  
  // Filter names (std::string from interface)
  std::vector<std::string> filterNames;
  
  // Focus offsets for each filter
  std::vector<int> focusOffsets;
  
  // Motor control pin (optional - for stepper motor control)
  int stepPin;
  int dirPin;
  int enablePin;
  
  /**
   * @brief Initialize filter names with defaults
   */
  void initializeFilterNames() {
    filterNames.clear();
    filterNames.push_back("Red");
    filterNames.push_back("Green");
    filterNames.push_back("Blue");
    filterNames.push_back("Luminance");
    filterNames.push_back("Ha");
    filterNames.push_back("OIII");
    filterNames.push_back("SII");
    filterNames.push_back("Clear");
  }
  
  /**
   * @brief Initialize focus offsets with defaults
   */
  void initializeFocusOffsets() {
    focusOffsets.clear();
    // Example offsets in microns (adjust for your actual filter wheel)
    focusOffsets.push_back(0);      // Red - baseline
    focusOffsets.push_back(-20);    // Green
    focusOffsets.push_back(-40);    // Blue
    focusOffsets.push_back(0);      // Luminance
    focusOffsets.push_back(10);     // Ha
    focusOffsets.push_back(-15);    // OIII
    focusOffsets.push_back(5);      // SII
    focusOffsets.push_back(0);      // Clear
  }
  
  /**
   * @brief Move filter wheel to target position
   * This is called periodically to update the movement
   */
  void updateMovement() {
    if (!isMoving || currentPosition == targetPosition) {
      isMoving = false;
      return;
    }
    
    // Example: Simple movement simulation
    // In real implementation, control stepper motor here
    if (currentPosition < targetPosition) {
      currentPosition++;
    } else if (currentPosition > targetPosition) {
      currentPosition--;
    }
    
    // Check if movement is complete
    if (currentPosition == targetPosition) {
      isMoving = false;
      LOG_DEBUG("FilterWheel movement complete - Position: " + String(currentPosition));
    }
  }

public:
  /**
   * @brief Constructor for MyFilterWheel
   * @param devicename Name of the filter wheel device
   * @param devicenumber Device number for Alpaca API
   * @param description Human-readable description
   * @param server Reference to AsyncWebServer
   * @param filters Number of filter positions (default: 8)
   * @param step_pin Optional stepper motor step pin
   * @param dir_pin Optional stepper motor direction pin
   * @param enable_pin Optional stepper motor enable pin
   */
  MyFilterWheel(String devicename, int devicenumber, String description, 
                AsyncWebServer &server, int filters = 8,
                int step_pin = -1, int dir_pin = -1, int enable_pin = -1)
    : AlpacaDeviceFilterWheel(devicename, devicenumber, description, server),
      numFilters(filters),
      currentPosition(0),
      targetPosition(0),
      isMoving(false),
      stepPin(step_pin),
      dirPin(dir_pin),
      enablePin(enable_pin) {
    
    // Initialize filter names and focus offsets
    initializeFilterNames();
    initializeFocusOffsets();
    
    // Ensure we have enough entries
    while (filterNames.size() < (size_t)numFilters) {
      filterNames.push_back("Filter " + std::to_string(filterNames.size()));
    }
    while (focusOffsets.size() < (size_t)numFilters) {
      focusOffsets.push_back(0);
    }
    
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
    
    LOG_DEBUG("MyFilterWheel created with " + String(numFilters) + " positions");
  }
  
  /**
   * @brief Get focus offsets for all filters
   * @return Vector of focus offsets in microns
   */
  std::vector<int> GetFocusOffsets() override {
    return focusOffsets;
  }
  
  /**
   * @brief Get names of all filters
   * @return Vector of filter names
   */
  std::vector<std::string> GetNames() override {
    return filterNames;
  }
  
  /**
   * @brief Get current filter wheel position
   * @return Current position (0 to numFilters-1)
   */
  int GetPosition() override {
    return currentPosition;
  }
  
  /**
   * @brief Set filter wheel to specified position
   * @param position Target position (0 to numFilters-1)
   */
  void SetPosition(int position) override {
    // Validate position
    if (position < 0 || position >= numFilters) {
      LOG_DEBUG("Invalid position requested: " + String(position));
      return;
    }
    
    LOG_DEBUG("Setting filter wheel position from " + String(currentPosition) + " to " + String(position));
    
    targetPosition = position;
    isMoving = (currentPosition != targetPosition);
    
    // In real implementation: Start motor movement here
    if (isMoving) {
      if (enablePin >= 0) {
        digitalWrite(enablePin, LOW); // Enable motor (active LOW)
      }
      
      // Set direction
      if (dirPin >= 0) {
        digitalWrite(dirPin, (targetPosition > currentPosition) ? HIGH : LOW);
      }
    }
  }
  
  /**
   * @brief Update filter wheel state
   * Call this periodically from loop()
   */
  void update() {
    updateMovement();
  }
  
  /**
   * @brief Check if filter wheel is currently moving
   * @return true if moving, false if stationary
   */
  bool IsMoving() const {
    return isMoving;
  }
  
  /**
   * @brief Get the target position
   * @return Target position during movement
   */
  int GetTargetPosition() const {
    return targetPosition;
  }
  
  /**
   * @brief Set a custom name for a filter
   * @param position Filter position (0 to numFilters-1)
   * @param name New name for the filter
   */
  void SetFilterName(int position, const std::string& name) {
    if (position >= 0 && position < numFilters) {
      filterNames[position] = name;
      LOG_DEBUG("Filter " + String(position) + " renamed to: " + name.c_str());
    }
  }
  
  /**
   * @brief Set a custom focus offset for a filter
   * @param position Filter position (0 to numFilters-1)
   * @param offset Focus offset in microns
   */
  void SetFocusOffset(int position, int offset) {
    if (position >= 0 && position < numFilters) {
      focusOffsets[position] = offset;
      LOG_DEBUG("Filter " + String(position) + " focus offset set to: " + String(offset));
    }
  }
};

#endif /* MY_FILTERWHEEL_H */
