# ASCOM Alpaca Discovery Implementation

This document describes the ASCOM Alpaca Discovery protocol implementation for the Arduino server.

## Overview

The ASCOM Alpaca Discovery protocol allows clients to automatically discover Alpaca devices on the network without prior knowledge of their IP addresses. The discovery protocol uses UDP broadcasts on port 32227.

## Protocol Specification

According to the ASCOM Alpaca API specification:

### Discovery Request
- **Protocol**: UDP broadcast
- **Port**: 32227
- **Message**: ASCII string `"alpacadiscovery1"`

### Discovery Response
- **Protocol**: UDP unicast (sent back to requester)
- **Format**: JSON
- **Content**:
```json
{
  "AlpacaPort": 80
}
```

## Implementation

### AlpacaDiscovery Class

The `Alpaca_Discovery.h` header file provides the `AlpacaDiscovery` class that implements the discovery protocol.

#### Constructor
```cpp
AlpacaDiscovery(uint16_t port = 80)
```
- **port**: The HTTP port where the Alpaca API is running (default: 80)

#### Methods

##### begin()
```cpp
bool begin()
```
Starts the UDP listener on port 32227.
- **Returns**: `true` if successful, `false` otherwise

##### handleDiscovery()
```cpp
void handleDiscovery()
```
Processes incoming UDP packets. Must be called repeatedly in the main loop.
- Checks for incoming UDP packets
- Validates that the packet contains "alpacadiscovery1"
- Sends a JSON response with the AlpacaPort

## Usage Example

```cpp
#include <ESP8266WiFi.h>
#include "Alpaca_Discovery.h"
#include "Alpaca_Management.h"

AsyncWebServer server(80);
AlpacaManagement* management = new AlpacaManagement();
AlpacaDiscovery* discovery = new AlpacaDiscovery(80);

void setup() {
  // Initialize WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
  
  // Start HTTP server
  management->registerManagementHandlers(server);
  server.begin();
  
  // Start Discovery service
  discovery->begin();
  Serial.println("Alpaca Discovery started on UDP port 32227");
}

void loop() {
  // Handle discovery requests
  discovery->handleDiscovery();
}
```

## Testing Discovery

You can test the discovery protocol using various methods:

### Method 1: Using netcat (nc)
```bash
# Send discovery request
echo -n "alpacadiscovery1" | nc -u -w1 <broadcast-address> 32227

# Example for 192.168.1.0/24 network
echo -n "alpacadiscovery1" | nc -u -w1 192.168.1.255 32227
```

### Method 2: Using Python
```python
import socket
import json

# Create UDP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
sock.settimeout(2)

# Send discovery request
message = b"alpacadiscovery1"
sock.sendto(message, ('<broadcast>', 32227))

# Receive responses
try:
    while True:
        data, addr = sock.recvfrom(1024)
        response = json.loads(data.decode())
        print(f"Found Alpaca device at {addr[0]}")
        print(f"  Alpaca Port: {response['AlpacaPort']}")
except socket.timeout:
    print("Discovery complete")
finally:
    sock.close()
```

### Method 3: Using ASCOM Alpaca Clients
Most ASCOM Alpaca client applications (such as the ASCOM Remote Client) automatically send discovery broadcasts and will detect your device.

## Network Configuration

### Firewall Settings
Ensure that UDP port 32227 is open for incoming traffic on your device's network.

### Broadcast Address
The discovery protocol uses UDP broadcasts. Ensure your network allows broadcast packets and that your WiFi configuration supports broadcast traffic.

### Multiple Network Interfaces
If your device has multiple network interfaces, the discovery service will respond on the interface where it receives the request.

## Compatibility

This implementation follows the ASCOM Alpaca API v1 specification and is compatible with:
- ASCOM Remote Client
- N.I.N.A. (Nighttime Imaging 'N' Astronomy)
- Sequence Generator Pro
- Other ASCOM Alpaca compatible clients

## Troubleshooting

### Discovery not working
1. **Check WiFi connection**: Ensure the device is connected to WiFi
2. **Verify port**: Confirm UDP port 32227 is not blocked by firewall
3. **Check broadcast address**: Ensure you're sending to the correct broadcast address
4. **Serial debugging**: Enable debug logging to see incoming packets:
   ```cpp
   #define DEBUGLOG_DEFAULT_LOG_LEVEL_TRACE
   ```

### No response received
1. Check that `handleDiscovery()` is being called in the loop
2. Verify `begin()` returned true during setup
3. Check Serial output for error messages
4. Test with a direct UDP packet to the device's IP address instead of broadcast

## Technical Details

### Performance Considerations
- The `handleDiscovery()` method is non-blocking and returns immediately if no packet is available
- Minimal CPU overhead when no discovery requests are being processed
- Response time is typically under 10ms

### Memory Usage
- Static memory: ~100 bytes for WiFiUDP object
- Dynamic memory per request: ~128 bytes for JSON buffer
- Stack usage: minimal (~50 bytes)

### Thread Safety
- The implementation is single-threaded and safe for use in the Arduino loop
- No thread synchronization required

## References

- [ASCOM Alpaca API Reference](https://ascom-standards.org/AlpacaDeveloper/ASCOMAlpacaAPIReference.html)
- [ASCOM Initiative](https://ascom-standards.org/)
- [ESP8266 Arduino Core WiFiUdp Documentation](https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/udp-class.html)
