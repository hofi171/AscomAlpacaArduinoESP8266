# ASCOM Alpaca Device Implementations

This directory contains example implementations and Arduino sketches for various ASCOM Alpaca device types.

## Overview

The ASCOM Alpaca Arduino Server library provides a framework for creating ASCOM-compliant astronomical devices using ESP8266 or ESP32 microcontrollers. This folder contains:

1. **Example Device Implementations** (`My*.h` files) - Concrete implementations showing how to extend the Alpaca device classes
2. **Arduino Sketches** (`*_example.ino` files) - Complete, runnable Arduino examples demonstrating device usage

## Quick Start

### 1. Hardware Requirements

- **ESP8266** (e.g., NodeMCU, D1 Mini, Wemos D1) or **ESP32** development board
- USB cable for programming
- Optional: Sensors or hardware specific to your device type
- Stable WiFi network

### 2. Software Requirements

- Arduino IDE (1.8.x or newer) or PlatformIO
- Required libraries (automatically installed via platformio.ini):
  - ESPAsyncWebServer
  - AsyncTCP (ESP32) or ESPAsyncTCP (ESP8266)
  - ArduinoJson (v5.13.4)
  - DebugLog
  - UUID

### 3. Using the Examples

#### Option A: Arduino IDE

1. Open the desired `*_example.ino` file in Arduino IDE
2. Update WiFi credentials in the sketch:
   ```cpp
   #define WIFI_SSID "YourWiFiSSID"
   #define WIFI_PASSWORD "YourWiFiPassword"
   ```
3. Select your board type (ESP8266 or ESP32)
4. Upload to your board
5. Open Serial Monitor (115200 baud) to see connection status and IP address

#### Option B: PlatformIO

1. Use the provided `platformio.ini` configuration
2. Build and upload:
   ```bash
   platformio run --target upload
   platformio device monitor
   ```

### 4. Connecting with ASCOM

Once your device is running:

1. Note the IP address shown in Serial Monitor
2. Use any ASCOM Alpaca-compatible client software
3. Add a new Alpaca device with the IP address
4. The device will be discovered automatically

Example API endpoint:
```
http://<IP_ADDRESS>/api/v1/<device_type>/0/<property>?ClientID=1&ClientTransactionID=1
```

## Available Device Implementations

### SafetyMonitor

**Files:**
- `MySafetyMonitor.h` - Example SafetyMonitor implementation
- `safety_monitor_example.ino` - Complete Arduino sketch

**Description:**
A safety monitor checks environmental or system conditions to determine if it's safe to operate astronomical equipment. This example demonstrates:
- Basic safety checking logic
- Integration with digital sensors
- Multiple safety condition monitoring
- Periodic status reporting

**Hardware Connection:**
- Optional digital sensor on pin D5
- Built-in LED indicates status

**API Endpoints:**
- `GET /api/v1/safetymonitor/0/issafe` - Check if conditions are safe

### FilterWheel

**Files:**
- `MyFilterWheel.h` - Example FilterWheel implementation
- `filterwheel_example.ino` - Complete Arduino sketch

**Description:**
Controls a motorized filter wheel for astrophotography. Manages filter positions, names, and focus offsets.

**API Endpoints:**
- `GET /api/v1/filterwheel/0/position` - Get current filter position
- `PUT /api/v1/filterwheel/0/position` - Set filter position
- `GET /api/v1/filterwheel/0/names` - Get filter names
- `GET /api/v1/filterwheel/0/focusoffsets` - Get focus offsets

### Focuser

**Files:**
- `MyFocuser.h` - Example Focuser implementation  
- `focuser_example.ino` - Complete Arduino sketch

**Description:**
Controls a motorized telescope focuser with absolute positioning and temperature compensation.

**API Endpoints:**
- `GET /api/v1/focuser/0/position` - Get current position
- `PUT /api/v1/focuser/0/move` - Move to position
- `GET /api/v1/focuser/0/ismoving` - Check if moving
- `GET /api/v1/focuser/0/temperature` - Get temperature
- `GET/PUT /api/v1/focuser/0/tempcomp` - Temperature compensation

### Rotator

**Files:**
- `MyRotator.h` - Example Rotator implementation
- `rotator_example.ino` - Complete Arduino sketch

**Description:**
Controls a camera or instrument rotator with precise angle positioning.

### Switch

**Files:**
- `MySwitch.h` - Example Switch implementation
- `switch_example.ino` - Complete Arduino sketch

**Description:**
Manages multiple on/off switches or analog outputs for observatory equipment control.

### Dome

**Files:**
- `MyDome.h` - Example Dome implementation
- `dome_example.ino` - Complete Arduino sketch

**Description:**
Controls an observatory dome with shutter, rotation, and parking capabilities.

### CoverCalibrator

**Files:**
- `MyCoverCalibrator.h` - Example CoverCalibrator implementation
- `covercalibrator_example.ino` - Complete Arduino sketch

**Description:**
Controls a telescope cover and flat field calibration light source.

### ObservingConditions

**Files:**
- `MyObservingConditions.h` - Example ObservingConditions implementation
- `observingconditions_example.ino` - Complete Arduino sketch

**Description:**
Monitors weather and environmental conditions at an observatory site.

### Camera

**Files:**
- `MyCamera.h` - Example Camera implementation
- `camera_example.ino` - Complete Arduino sketch

**Description:**
Controls a CCD or CMOS astronomical camera with imaging capabilities.

### Telescope

**Files:**
- `MyTelescope.h` - Example Telescope implementation
- `telescope_example.ino` - Complete Arduino sketch

**Description:**
Controls a telescope mount with slewing, tracking, and goto capabilities.

## Creating Custom Implementations

To create your own device implementation:

1. **Extend the Alpaca Device Class:**
   ```cpp
   #include "alpaca_api/Alpaca_Device_YourDevice.h"
   
   class MyYourDevice : public AlpacaDeviceYourDevice {
   public:
     MyYourDevice(String name, int number, String desc, AsyncWebServer &server)
       : AlpacaDeviceYourDevice(name, number, desc, server) {
       // Your initialization
     }
     
     // Implement required interface methods
     YourType GetYourProperty() override {
       // Your implementation
       return value;
     }
   };
   ```

2. **Create Arduino Sketch:**
   ```cpp
   #include "implementation/MyYourDevice.h"
   
   AsyncWebServer server(80);
   MyYourDevice* device = nullptr;
   
   void setup() {
     // WiFi setup
     // Device creation
     device = new MyYourDevice("Device Name", 0, "Description", server);
     device->registerHandlers(server);
     server.begin();
   }
   
   void loop() {
     // Your periodic tasks
   }
   ```

3. **Test Your Device:**
   - Upload and monitor serial output
   - Test endpoints with curl or browser
   - Connect with ASCOM client software

## Testing with curl

You can test endpoints using curl:

```bash
# Get device description
curl "http://<IP>/api/v1/<device>/0/description?ClientID=1&ClientTransactionID=1"

# Get a property (example: SafetyMonitor)
curl "http://<IP>/api/v1/safetymonitor/0/issafe?ClientID=1&ClientTransactionID=1"

# Set a property (example: Focuser position)
curl -X PUT "http://<IP>/api/v1/focuser/0/move" \
  -d "Position=5000&ClientID=1&ClientTransactionID=1"

# Get management info
curl "http://<IP>/management/apiversions"
curl "http://<IP>/management/v1/configureddevices"
```

## Troubleshooting

### WiFi Connection Issues
- Verify SSID and password are correct
- Check WiFi signal strength (should be > -70 dBm)
- Ensure 2.4GHz WiFi is enabled (ESP8266/ESP32 don't support 5GHz)
- Try moving closer to the router

### Device Not Discovered
- Verify device IP address in Serial Monitor
- Check firewall settings on client computer
- Ensure client and device are on same network/VLAN
- Try accessing endpoints directly with curl

### Compilation Errors
- Install all required libraries
- Check board selection matches your hardware
- Update ESP8266/ESP32 board definitions
- Verify Arduino IDE or PlatformIO version

### Runtime Errors
- Enable DEBUG logging in code
- Check Serial Monitor output
- Verify sufficient memory (heap) available
- Check for stack overflow if device crashes

## Debug Logging

Enable verbose logging by adding at the top of your sketch:

```cpp
#define DEBUGLOG_DEFAULT_LOG_LEVEL_TRACE
#include "DebugLog.h"
```

This will output detailed information about:
- HTTP requests and responses
- Device method calls
- Error conditions
- Network status

## ASCOM Alpaca Resources

- **ASCOM Standards:** https://ascom-standards.org/
- **Alpaca API Documentation:** https://ascom-standards.org/api/
- **ASCOM Initiative:** https://ascom-standards.org/About/Index.htm
- **Developer Forum:** https://ascomtalk.groups.io/

## License

This example code is provided as-is for educational and development purposes.

## Contributing

Contributions are welcome! If you create new device implementations or improvements:
1. Test thoroughly with actual ASCOM client software
2. Follow the existing code style and patterns
3. Include comprehensive documentation
4. Add example Arduino sketches

## Support

For issues, questions, or suggestions:
- Check existing examples and documentation
- Review ASCOM Alpaca API specification
- Open an issue on the project repository
