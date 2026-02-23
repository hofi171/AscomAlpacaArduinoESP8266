#ifndef Alpaca_Discovery_h
#define Alpaca_Discovery_h

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <ArduinoJson.h>
#include "DebugLog.h"

#define ALPACA_DISCOVERY_PORT 32227
#define ALPACA_DISCOVERY_REQUEST "alpacadiscovery1"

class AlpacaDiscovery
{
private:
    WiFiUDP udp;
    uint16_t alpacaPort;
    
public:
    AlpacaDiscovery(uint16_t port = 80) : alpacaPort(port) {
        LOG_INFO("AlpacaDiscovery created with Alpaca port " + String(alpacaPort));
    }
    
    ~AlpacaDiscovery() {
        udp.stop();
        LOG_INFO("AlpacaDiscovery stopped");
    }
    
    bool begin() {
        LOG_INFO("try start Alpaca listening on UDP port " + String(ALPACA_DISCOVERY_PORT));
        if (udp.begin(ALPACA_DISCOVERY_PORT)) {
            LOG_INFO("Alpaca Discovery listening on UDP port " + String(ALPACA_DISCOVERY_PORT));
            return true;
        } else {
            LOG_ERROR("Failed to start Alpaca Discovery on UDP port " + String(ALPACA_DISCOVERY_PORT));
            return false;
        }
    }
    
    void handleDiscovery() {
        int packetSize = udp.parsePacket();
        if (packetSize) {
            LOG_DEBUG("Received UDP packet of size " + String(packetSize) + " from " + udp.remoteIP().toString() + ":" + String(udp.remotePort()));
            
            // Read the incoming packet
            char incomingPacket[255];
            int len = udp.read(incomingPacket, 255);
            if (len > 0) {
                incomingPacket[len] = 0;
            }
            
            LOG_DEBUG("UDP packet contents: " + String(incomingPacket));
            
            // Check if this is an Alpaca discovery request
            if (strcmp(incomingPacket, ALPACA_DISCOVERY_REQUEST) == 0) {
                LOG_INFO("Valid Alpaca discovery request received");
                sendDiscoveryResponse();
            } else {
                LOG_DEBUG("Ignoring non-Alpaca discovery packet");
            }
        }
    }
    
private:
    void sendDiscoveryResponse() {
        // Build JSON response according to ASCOM Alpaca specification
        DynamicJsonBuffer jsonBuffer(128);
        JsonObject& root = jsonBuffer.createObject();
        root["AlpacaPort"] = alpacaPort;
        
        String response;
        root.printTo(response);
        
        LOG_DEBUG("Sending discovery response: " + response);
        
        // Send response back to the requester
        udp.beginPacket(udp.remoteIP(), udp.remotePort());
        udp.write((const uint8_t*)response.c_str(), response.length());
        udp.endPacket();
        
        LOG_INFO("Discovery response sent to " + udp.remoteIP().toString() + ":" + String(udp.remotePort()));
    }
};

#endif /* Alpaca_Discovery_h */
