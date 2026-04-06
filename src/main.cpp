// You can also set default log level by defining macro (default: INFO)
#define DEBUGLOG_DEFAULT_LOG_LEVEL_INFO
#include "DebugLog.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h> //https://arduinojson.org/v5/api/

// #include <ESP8266WiFi.h>
#include <WiFiClient.h>
// #include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>
#include <EEPROM.h>

#include "DebugLog.h"

#include "alpaca_api/Alpaca_Errors.h"
#include "alpaca_api/Alpaca_Management.h"
#include "alpaca_api/Alpaca_Discovery.h"
#include "WiFi_Config.h"
// #include "Alpaca_Device_Focuser.h"
#include "implementation/ArduinoDome.h"
#include "implementation/ArduinoSafetyMonitor.h"

#define HOSTNAME "Arduino-Alpaca-Dome"

const char *ssid;
const char *password;
const char *hostname = HOSTNAME;

AsyncWebServer server(80); // default HTTP port for Alpaca API is 80

AlpacaManagement *management = new AlpacaManagement();
AlpacaDiscovery *discovery = nullptr; // Will be initialized after WiFi connection
WiFiConfig wifiConfig; // WiFi configuration manager

ArduinoDome *dome = nullptr;
ArduinoSafetyMonitor *safetyMonitor = nullptr;


const int ledPin = 2; // GPIO2 is the built-in LED on most ESP8266 boards

void setup()
{
  LOG_INFO("Start Setup");
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);
  EEPROM.begin(512);
  
  // Load WiFi credentials from EEPROM (if available)
  String wifiSSID;
  String wifiPassword;
  
  if (wifiConfig.loadFromEEPROM() && wifiConfig.hasConfig()) {
    wifiSSID = wifiConfig.getSSID();
    wifiPassword = wifiConfig.getPassword();
    LOG_INFO("Using WiFi credentials from EEPROM");
  
  
  WiFi.mode(WIFI_STA);
  WiFi.hostname(hostname); // Set the device hostname
  WiFi.begin(wifiSSID.c_str(), wifiPassword.c_str());

  // Wait for connection with timeout
  LOG_INFO("Wait for connection");
  int timeout_counter = 0;
  const int timeout_limit = 20; // 10 seconds timeout (20 * 500ms)
  
  while (WiFi.status() != WL_CONNECTED && timeout_counter < timeout_limit)
  {
    delay(500);
    LOG_INFO(".");
    timeout_counter++;
  }
}
  else {
    LOG_WARN("No valid WiFi credentials found in EEPROM. Please connect to the device's Access Point and configure WiFi settings.");
    wifiSSID = ""; // Empty SSID will trigger AP mode
  }
  // Check if connection was successful
  if (WiFi.status() == WL_CONNECTED) {
    LOG_INFO("");
    LOG_INFO("Connected to %s", wifiSSID.c_str());
    LOG_INFO("IP address: %s", WiFi.localIP().toString().c_str());
    LOG_INFO("Hostname: %s.local", hostname);
  } else {
    // Connection failed - start AP mode
    LOG_WARN("Failed to connect to WiFi network: %s", wifiSSID.c_str());
    LOG_INFO("Starting Access Point mode...");
    
    WiFi.mode(WIFI_AP);
    
    // Create AP name from hostname
    String apSSID = String(hostname);
    
    // Start AP without password
    bool apStarted = WiFi.softAP(apSSID.c_str());
    
    if (apStarted) {
      IPAddress apIP = WiFi.softAPIP();
      LOG_INFO("Access Point started successfully");
      LOG_INFO("AP SSID: %s", apSSID.c_str());
      LOG_INFO("AP IP address: %s", apIP.toString().c_str());
      LOG_INFO("Connect to '%s' WiFi network and access at http://%s", apSSID.c_str(), apIP.toString().c_str());
    } else {
      LOG_ERROR("Failed to start Access Point");
    }
  }

  try
  {

    LOG_INFO("Start management->registerManagementHandlers(server);");
    management->registerManagementHandlers(server);
    LOG_INFO("Done management->registerManagementHandlers(server);");
   
    LOG_INFO("Start dome = new ArduinoDome(...);");
    // pin 5= shutter open drive, pin 4= shutter close drive, pin 3= shutter open sensor, pin 2= shutter close sensor
    dome = new ArduinoDome(HOSTNAME, 0, "Arduino Alpaca Dome based on ESP8266", server, true, false, -1, -1, -1, 5, 4, 3, 2, -1); 
    LOG_INFO("Done dome = new ArduinoDome(...);");


    LOG_INFO("Start management->registerDevice(...) dome;");
    management->registerDevice(server, dome->GetDeviceName(), dome->GetDeviceType(), dome->GetDeviceNumber(), dome);
    LOG_INFO("Done management->registerDevice(...) dome;");

    LOG_INFO("Start safetyMonitor = new ArduinoSafetyMonitor(...);");
    safetyMonitor = new ArduinoSafetyMonitor(HOSTNAME, 0, "Arduino Alpaca Safety Monitor based on ESP8266", server);
    LOG_INFO("Done safetyMonitor = new ArduinoSafetyMonitor(...);");

    LOG_INFO("Start management->registerDevice(...) safetyMonitor;");
    management->registerDevice(server, safetyMonitor->GetDeviceName(), safetyMonitor->GetDeviceType(), safetyMonitor->GetDeviceNumber(), safetyMonitor);
    LOG_INFO("Done management->registerDevice(...) safetyMonitor;");
  }
  catch (const std::exception &e)
  {
    LOG_ERROR("Error in management setup: %s", e.what());
  }

  try
  {
    LOG_INFO("Start server.begin();");
    server.begin();
    LOG_INFO("Done server.begin();");

    LOG_INFO("HTTP server started");
  }
  catch (const std::exception &e)
  {
    LOG_ERROR("Error starting server: %s", e.what());
  }

  // Start Alpaca Discovery - must be after WiFi is fully connected
  try
  {
    LOG_INFO("Start discovery->begin();");
    delay(100); // Small delay to ensure WiFi is stable
    discovery = new AlpacaDiscovery(80);
    if (discovery->begin())
    {
      LOG_INFO("Alpaca Discovery initialized successfully");
    }
    else
    {
      LOG_ERROR("Failed to initialize Alpaca Discovery");
    }
  }
  catch (const std::exception &e)
  {
    LOG_ERROR("Error in discovery: %s", e.what());
  }
}

void loop(void)
{
 dome->update(); // Update dome state (handles movement and shutter control)
 
  // Handle Alpaca Discovery requests
  if (discovery != nullptr)
  {
    discovery->handleDiscovery();
  }
  //delay(2); // Small delay to prevent watchdog timer issues
}
