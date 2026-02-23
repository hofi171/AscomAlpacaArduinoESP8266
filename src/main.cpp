// You can also set default log level by defining macro (default: INFO)
#define DEBUGLOG_DEFAULT_LOG_LEVEL_TRACE
#include "DebugLog.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h> //https://arduinojson.org/v5/api/

// #include <ESP8266WiFi.h>
#include <WiFiClient.h>
// #include <ESPAsyncWebServer.h>
#include <ESP8266mDNS.h>

#include "DebugLog.h"

#include "alpaca_api/Alpaca_Errors.h"
#include "alpaca_api/Alpaca_Management.h"
#include "alpaca_api/Alpaca_Discovery.h"
// #include "Alpaca_Device_Focuser.h"
#include "implementation/MySafetyMonitor.h"
#include "implementation/MyFocuser.h"
#include "implementation/MyDome.h"
#include "implementation/MySwitch.h"
#include "implementation/MyFilterWheel.h"
#include "implementation/MyRotator.h"

#ifndef STASSID
#define STASSID "yourSSID"
#define STAPSK "yourPassword"
#endif

const char *ssid = STASSID;
const char *password = STAPSK;

AsyncWebServer server(80); // default HTTP port for Alpaca API is 80

AlpacaManagement *management = new AlpacaManagement();
AlpacaDiscovery *discovery = nullptr; // Will be initialized after WiFi connection

MySafetyMonitor *safetyMonitor = nullptr;
MyFocuser *focuser = nullptr;
MyDome *dome = nullptr;
MySwitch *switchDevice = nullptr; 
MyFilterWheel *filterWheel = nullptr;
MyRotator *rotator = nullptr;

const int ledPin = 2; // GPIO2 is the built-in LED on most ESP8266 boards

void setup()
{
  LOG_INFO("Start Setup");
  pinMode(ledPin, OUTPUT);
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  // Wait for connection
  LOG_INFO("Wait for connection");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    LOG_INFO(".");
  }
  LOG_INFO("");
  LOG_INFO("Connected to %s", ssid);
  LOG_INFO("IP address: %s", WiFi.localIP().toString().c_str());

  try
  {

    LOG_INFO("Start management->registerManagementHandlers(server);");
    management->registerManagementHandlers(server);
    LOG_INFO("Done management->registerManagementHandlers(server);");

    // focuser = new AlpacaDeviceFocuser("My Focuser", 0,"This is a test focuser device", server);

    LOG_INFO("Start safetyMonitor = new MySafetyMonitor(...);");
    safetyMonitor = new MySafetyMonitor("My Safety Monitor", 0, "This is a test safety monitor device", server, 5);
    LOG_INFO("Done safetyMonitor = new MySafetyMonitor(...);");

    LOG_INFO("Start focuser = new MyFocuser(...);");
    focuser = new MyFocuser("My Focuser", 0, "This is a test focuser device", server, 200, 10);
    LOG_INFO("Done focuser = new MyFocuser(...);");

    LOG_INFO("Start dome = new MyDome(...);");
    dome = new MyDome("My Dome", 0, "This is a test dome device", server, true, true);
    LOG_INFO("Done dome = new MyDome(...);");

    LOG_INFO("Start switchDevice = new MySwitch(...);");
    switchDevice = new MySwitch("My Switch", 0, "This is a test switch device", server, 4);
    LOG_INFO("Done switchDevice = new MySwitch(...);");

    LOG_INFO("Start management->registerDevice(...);");
    management->registerDevice(safetyMonitor->GetDeviceName(), safetyMonitor->GetDeviceType(), safetyMonitor->GetDeviceNumber(), safetyMonitor);
    management->registerDevice(focuser->GetDeviceName(), focuser->GetDeviceType(), focuser->GetDeviceNumber(), focuser);
    management->registerDevice(dome->GetDeviceName(), dome->GetDeviceType(), dome->GetDeviceNumber(), dome);
    management->registerDevice(switchDevice->GetDeviceName(), switchDevice->GetDeviceType(), switchDevice->GetDeviceNumber(), switchDevice);
    LOG_INFO("Done management->registerDevice(...);");
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
 focuser->update(); // Update focuser state (handles movement and temperature compensation)
 dome->update(); // Update dome state (handles movement and shutter control)
 
  // Handle Alpaca Discovery requests
  if (discovery != nullptr)
  {
    discovery->handleDiscovery();
  }
  delay(10); // Small delay to prevent watchdog timer issues
}
