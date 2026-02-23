# ASCOM Alpaca C++ Device Interfaces

This folder contains C++ interface definitions for ASCOM Alpaca device types based on the official ASCOM standards documentation.

## Overview

These interfaces provide pure virtual base classes that define the API contract for ASCOM-compliant device drivers. By implementing these interfaces, you can create Arduino-compatible drivers that conform to the ASCOM Alpaca protocol.

## Available Interfaces

### ICamera.h
Defines the interface for camera devices.
- **Reference**: https://ascom-standards.org/newdocs/camera.html
- **Methods**: 56 methods (42 properties, 14 read/write properties, 5 actions)
- **Features**:
  - Image capture and exposure control
  - Cooler and temperature management
  - Binning and subframe support
  - Gain and offset control
  - Pulse guiding support

### ICoverCalibrator.h
Defines the interface for cover calibrator devices.
- **Reference**: https://ascom-standards.org/newdocs/covercalibrator.html
- **Methods**: 11 methods (6 properties, 5 actions)
- **Features**:
  - Cover open/close control
  - Calibration light control
  - Brightness adjustment
  - State monitoring

### IDome.h
Defines the interface for dome/observatory control devices.
- **Reference**: https://ascom-standards.org/newdocs/dome.html
- **Methods**: 25 methods (15 properties, 1 read/write property, 9 actions)
- **Features**:
  - Dome positioning (altitude and azimuth control)
  - Shutter control (open/close)
  - Home and park position management
  - Telescope slaving support
  - Movement synchronization

### IFilterWheel.h
Defines the interface for filter wheel devices.
- **Reference**: https://ascom-standards.org/newdocs/filterwheel.html
- **Methods**: 4 methods (3 properties, 1 read/write property)
- **Features**:
  - Filter position control
  - Filter naming
  - Focus offset management

### IFocuser.h
Defines the interface for telescope focuser devices.
- **Reference**: https://ascom-standards.org/newdocs/focuser.html
- **Methods**: 12 methods (9 properties, 1 read/write property, 2 actions)
- **Features**:
  - Absolute and relative positioning
  - Temperature compensation
  - Position querying
  - Motion control (move, halt)
  - Step size configuration

### IObservingConditions.h
Defines the interface for observing conditions sensors.
- **Reference**: https://ascom-standards.org/newdocs/observingconditions.html
- **Methods**: 18 methods (15 properties, 1 read/write property, 3 query methods)
- **Features**:
  - Weather monitoring (temperature, humidity, pressure)
  - Cloud cover and sky conditions
  - Wind speed and direction
  - Seeing (star FWHM) measurement
  - Sensor management

### IRotator.h
Defines the interface for camera rotator devices.
- **Reference**: https://ascom-standards.org/newdocs/rotator.html
- **Methods**: 13 methods (8 properties, 1 read/write property, 5 actions)
- **Features**:
  - Absolute and relative rotation
  - Mechanical position tracking
  - Reverse state control
  - Position synchronization

### ISafetyMonitor.h
Defines the interface for safety monitor devices.
- **Reference**: https://ascom-standards.org/newdocs/safetymonitor.html
- **Methods**: 1 method (1 property)
- **Features**:
  - Safety state monitoring

### ISwitch.h
Defines the interface for multi-purpose switch devices.
- **Reference**: https://ascom-standards.org/newdocs/switch.html
- **Methods**: 16 methods (11 properties, 5 actions)
- **Features**:
  - Multiple switch management
  - Boolean and analog value support
  - Asynchronous operations (ISwitchV3+)
  - Switch naming and description
  - Value range configuration

### ITelescope.h
Defines the interface for telescope mount devices.
- **Reference**: https://ascom-standards.org/newdocs/telescope.html
- **Methods**: 65 methods (36 properties, 17 capabilities, 9 read/write properties, 16 actions)
- **Features**:
  - Equatorial and Alt/Az positioning
  - Slewing (sync and async)
  - Tracking rate control
  - Pulse guiding
  - Park and home positioning
  - Side-of-pier management
  - Axis movement control

## Supporting Files

### AscomTypes.h
Common type definitions and structures used across interfaces:
- **GuideDirection**: Enumeration for pulse guide directions
- **Rate**: Structure for axis rate ranges
- **StateValue**: Structure for device state values

References:
- https://ascom-standards.org/newdocs/rate.html
- https://ascom-standards.org/newdocs/statevalue.html

### AscomExceptions.h
ASCOM standard exception definitions and error codes.
- **AscomException**: Base exception class
- **AscomErrorCode**: Standard ASCOM error codes
- Specific exception classes for common errors

Reference: https://ascom-standards.org/newdocs/exceptions.html

## Usage

To implement a device driver, create a class that inherits from the appropriate interface:

```cpp
#include "interfaces/ICamera.h"

class MyCameraDriver : public ICamera {
public:
    // Implement all pure virtual methods
    CameraState GetCameraState() override {
        // Your implementation here
    }
    
    void StartExposure(double duration, bool light) override {
        // Your implementation here
    }
    
    // ... implement remaining methods
};
```

## Interface Design

All interfaces follow these conventions:

1. **Pure Virtual Classes**: All methods are pure virtual (`= 0`), requiring implementation by derived classes
2. **Naming Convention**: 
   - Properties use `Get` prefix for read operations (e.g., `GetPosition()`)
   - Properties use `Set` prefix for write operations (e.g., `SetPosition()`)
   - Actions use verb names (e.g., `Move()`, `Halt()`)
3. **No State**: Interfaces don't contain any member variables
4. **Virtual Destructor**: Each interface has a virtual destructor for proper cleanup
5. **Common Types**: Shared enumerations and structures are defined in `AscomTypes.h`

## ASCOM Alpaca Compatibility

These interfaces are designed to match the ASCOM Alpaca Device API v1 specification. They represent the device-specific methods and properties defined in the API, excluding the common methods that all ASCOM devices share (e.g., `Connected`, `Description`, `Action`, etc.).

For complete ASCOM Alpaca compliance, drivers should also implement:
- Common ASCOM device methods (Connection, Info, etc.)
- Error handling with ASCOM error codes (see `AscomExceptions.h`)
- Transaction ID management
- HTTP REST endpoints (for Alpaca protocol)

## Notes

- These interfaces use standard C++ types (`bool`, `int`, `double`, `std::string`, `std::vector`)
- Enumerations are defined for device states and modes
- All angle measurements are in degrees
- Temperature measurements are in degrees Celsius
- Position/step values are device-dependent integers
- Time durations are in seconds unless otherwise specified

## Version

These interfaces are based on ASCOM Platform 7 / Alpaca API v1 specifications as of February 2026.
