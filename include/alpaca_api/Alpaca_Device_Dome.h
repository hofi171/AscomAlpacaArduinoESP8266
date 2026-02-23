#ifndef ALPACA_DEVICE_DOME_H
#define ALPACA_DEVICE_DOME_H

#include "Aplaca_Device.h"
#include "ascom_interfaces/IDome.h"
#include "DebugLog.h"
#include "Alpaca_Response_Builder.h"
#include "Alpaca_Errors.h"
#include "Alpaca_Driver_Settings.h"
#include "Alpaca_Request_Helper.h"
#include <ArduinoJson.h>

/**
 * @file Alpaca_Device_Dome.h
 * @brief Alpaca API implementation for Dome device
 * 
 * This class implements the ASCOM Alpaca API endpoints for Dome devices.
 * It extends AplacaDevice and implements IDome interface.
 * 
 * Endpoints:
 * - GET /api/v1/dome/{device_number}/altitude - Get dome altitude
 * - GET /api/v1/dome/{device_number}/athome - Check if at home position
 * - GET /api/v1/dome/{device_number}/atpark - Check if at park position
 * - GET /api/v1/dome/{device_number}/azimuth - Get dome azimuth
 * - GET /api/v1/dome/{device_number}/canfindome - Check if can find home
 * - GET /api/v1/dome/{device_number}/canpark - Check if can park
 * - GET /api/v1/dome/{device_number}/cansetaltitude - Check if can set altitude
 * - GET /api/v1/dome/{device_number}/cansetazimuth - Check if can set azimuth
 * - GET /api/v1/dome/{device_number}/cansetpark - Check if can set park
 * - GET /api/v1/dome/{device_number}/cansetshutter - Check if can control shutter
 * - GET /api/v1/dome/{device_number}/canslave - Check if can slave to telescope
 * - GET /api/v1/dome/{device_number}/cansyncazimuth - Check if can sync azimuth
 * - GET /api/v1/dome/{device_number}/shutterstatus - Get shutter status
 * - GET /api/v1/dome/{device_number}/slaved - Get slaved state
 * - PUT /api/v1/dome/{device_number}/slaved - Set slaved state
 * - GET /api/v1/dome/{device_number}/slewing - Check if dome is slewing
 * - PUT /api/v1/dome/{device_number}/abortslew - Abort current slew
 * - PUT /api/v1/dome/{device_number}/closeshutter - Close the shutter
 * - PUT /api/v1/dome/{device_number}/findhome - Find home position
 * - PUT /api/v1/dome/{device_number}/openshutter - Open the shutter
 * - PUT /api/v1/dome/{device_number}/park - Park the dome
 * - PUT /api/v1/dome/{device_number}/setpark - Set current position as park
 * - PUT /api/v1/dome/{device_number}/slewtoaltitude - Slew to altitude
 * - PUT /api/v1/dome/{device_number}/slewtoazimuth - Slew to azimuth
 * - PUT /api/v1/dome/{device_number}/synctoazimuth - Sync to azimuth
 * 
 * Reference: https://ascom-standards.org/newdocs/dome.html
 */
class AlpacaDeviceDome : public AplacaDevice, public IDome {
private:
  String Description;
  uint32_t serverTransID = 0;

public:
  /**
   * @brief Constructor for AlpacaDeviceDome
   * @param devicename Name of the device
   * @param devicenumber Device number (for multiple devices of same type)
   * @param description Human-readable description of the device
   * @param server Reference to the AsyncWebServer instance
   */
  AlpacaDeviceDome(String devicename, int devicenumber, String description,
                   AsyncWebServer &server)
      : AplacaDevice(devicename, "dome", devicenumber, server) {
    Description = description;
    registerHandlers(server);
    LOG_DEBUG("AlpacaDeviceDome created:", devicename);
  }

  virtual ~AlpacaDeviceDome() {}

  // ==================== Device-specific endpoint registration ====================
  
  void registerHandlers(AsyncWebServer &server) override {
    LOG_DEBUG("Registering Dome specific handlers");
    
    // GET /api/v1/dome/{device_number}/altitude
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/altitude").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->altitudeHandler(request); });
    
    // GET /api/v1/dome/{device_number}/athome
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/athome").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->atHomeHandler(request); });
    
    // GET /api/v1/dome/{device_number}/atpark
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/atpark").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->atParkHandler(request); });
    
    // GET /api/v1/dome/{device_number}/azimuth
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/azimuth").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->azimuthHandler(request); });
    
    // GET /api/v1/dome/{device_number}/canfindome
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/canfindhome").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->canFindHomeHandler(request); });
    
    // GET /api/v1/dome/{device_number}/canpark
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/canpark").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->canParkHandler(request); });
    
    // GET /api/v1/dome/{device_number}/cansetaltitude
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/cansetaltitude").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->canSetAltitudeHandler(request); });
    
    // GET /api/v1/dome/{device_number}/cansetazimuth
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/cansetazimuth").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->canSetAzimuthHandler(request); });
    
    // GET /api/v1/dome/{device_number}/cansetpark
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/cansetpark").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->canSetParkHandler(request); });
    
    // GET /api/v1/dome/{device_number}/cansetshutter
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/cansetshutter").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->canSetShutterHandler(request); });
    
    // GET /api/v1/dome/{device_number}/canslave
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/canslave").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->canSlaveHandler(request); });
    
    // GET /api/v1/dome/{device_number}/cansyncazimuth
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/cansyncazimuth").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->canSyncAzimuthHandler(request); });
    
    // GET /api/v1/dome/{device_number}/shutterstatus
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/shutterstatus").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->shutterStatusHandler(request); });
    
    // GET /api/v1/dome/{device_number}/slaved
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/slaved").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->slavedGetHandler(request); });
    
    // PUT /api/v1/dome/{device_number}/slaved
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/slaved").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->slavedPutHandler(request); });
    
    // GET /api/v1/dome/{device_number}/slewing
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/slewing").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->slewingHandler(request); });
    
    // PUT /api/v1/dome/{device_number}/abortslew
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/abortslew").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->abortSlewHandler(request); });
    
    // PUT /api/v1/dome/{device_number}/closeshutter
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/closeshutter").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->closeShutterHandler(request); });
    
    // PUT /api/v1/dome/{device_number}/findhome
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/findhome").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->findHomeHandler(request); });
    
    // PUT /api/v1/dome/{device_number}/openshutter
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/openshutter").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->openShutterHandler(request); });
    
    // PUT /api/v1/dome/{device_number}/park
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/park").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->parkHandler(request); });
    
    // PUT /api/v1/dome/{device_number}/setpark
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/setpark").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->setParkHandler(request); });
    
    // PUT /api/v1/dome/{device_number}/slewtoaltitude
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/slewtoaltitude").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->slewToAltitudeHandler(request); });
    
    // PUT /api/v1/dome/{device_number}/slewtoazimuth
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/slewtoazimuth").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->slewToAzimuthHandler(request); });
    
    // PUT /api/v1/dome/{device_number}/synctoazimuth
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/synctoazimuth").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->syncToAzimuthHandler(request); });
    
    LOG_DEBUG("Dome handlers registered!");
  }

  // ==================== Endpoint Handlers ====================
  
  void altitudeHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::altitudeHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    double altitude = GetAltitude();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  altitude, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("altitudeHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void atHomeHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::atHomeHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    bool atHome = GetAtHome();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              atHome, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("atHomeHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void atParkHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::atParkHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    bool atPark = GetAtPark();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              atPark, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("atParkHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void azimuthHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::azimuthHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    double azimuth = GetAzimuth();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  azimuth, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("azimuthHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void canFindHomeHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::canFindHomeHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    bool canFindHome = GetCanFindHome();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              canFindHome, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("canFindHomeHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void canParkHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::canParkHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    bool canPark = GetCanPark();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              canPark, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("canParkHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void canSetAltitudeHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::canSetAltitudeHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    bool canSetAltitude = GetCanSetAltitude();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              canSetAltitude, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("canSetAltitudeHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void canSetAzimuthHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::canSetAzimuthHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    bool canSetAzimuth = GetCanSetAzimuth();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              canSetAzimuth, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("canSetAzimuthHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void canSetParkHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::canSetParkHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    bool canSetPark = GetCanSetPark();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              canSetPark, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("canSetParkHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void canSetShutterHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::canSetShutterHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    bool canSetShutter = GetCanSetShutter();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              canSetShutter, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("canSetShutterHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void canSlaveHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::canSlaveHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    bool canSlave = GetCanSlave();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              canSlave, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("canSlaveHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void canSyncAzimuthHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::canSyncAzimuthHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    bool canSyncAzimuth = GetCanSyncAzimuth();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              canSyncAzimuth, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("canSyncAzimuthHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void shutterStatusHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::shutterStatusHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    ShutterState shutterStatus = GetShutterStatus();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              shutterStatus, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("shutterStatusHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void slavedGetHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::slavedGetHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    bool slaved = GetSlaved();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              slaved, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("slavedGetHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void slavedPutHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::slavedPutHandler - Method:", request->method());

    if (request->method() != HTTP_PUT) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, true, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    bool slaved = false;
    if (!tryGetBoolParam(request, "Slaved", true, slaved)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "slaved", "Slaved");
      return;
    }
    
    LOG_DEBUG("Setting Slaved to: " + String(slaved ? "true" : "false"));

    SetSlaved(slaved);

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "slaved", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("slavedPutHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void slewingHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::slewingHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    bool slewing = GetSlewing();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  slewing, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("slewingHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void abortSlewHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::abortSlewHandler - Method:", request->method());

    if (request->method() != HTTP_PUT) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, true, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    LOG_DEBUG("Aborting dome slew");

    AbortSlew();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "abortslew", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("abortSlewHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void closeShutterHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::closeShutterHandler - Method:", request->method());

    if (request->method() != HTTP_PUT) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, true, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    LOG_DEBUG("Closing dome shutter");

    CloseShutter();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "closeshutter", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("closeShutterHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void findHomeHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::findHomeHandler - Method:", request->method());

    if (request->method() != HTTP_PUT) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, true, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    LOG_DEBUG("Finding dome home position");

    FindHome();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "findhome", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("findHomeHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void openShutterHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::openShutterHandler - Method:", request->method());

    if (request->method() != HTTP_PUT) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, true, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    LOG_DEBUG("Opening dome shutter");

    OpenShutter();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "openshutter", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("openShutterHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void parkHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::parkHandler - Method:", request->method());

    if (request->method() != HTTP_PUT) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, true, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    LOG_DEBUG("Parking dome");

    Park();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "park", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("parkHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void setParkHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::setParkHandler - Method:", request->method());

    if (request->method() != HTTP_PUT) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, true, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    LOG_DEBUG("Setting current position as park");

    SetPark();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "setpark", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("setParkHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void slewToAltitudeHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::slewToAltitudeHandler - Method:", request->method());

    if (request->method() != HTTP_PUT) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, true, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    double altitude = 0.0;
    if (!tryGetDoubleParam(request, "Altitude", true, altitude)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "slewtoaltitude", "Altitude");
      return;
    }
    
    LOG_DEBUG("Slewing dome to altitude:", altitude);

    SlewToAltitude(altitude);

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "slewtoaltitude", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("slewToAltitudeHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void slewToAzimuthHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::slewToAzimuthHandler - Method:", request->method());

    if (request->method() != HTTP_PUT) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, true, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    double azimuth = 0.0;
    if (!tryGetDoubleParam(request, "Azimuth", true, azimuth)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "slewtoazimuth", "Azimuth");
      return;
    }
    
    LOG_DEBUG("Slewing dome to azimuth:", azimuth);

    SlewToAzimuth(azimuth);

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "slewtoazimuth", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("slewToAzimuthHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void syncToAzimuthHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceDome::syncToAzimuthHandler - Method:", request->method());

    if (request->method() != HTTP_PUT) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, true, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    double azimuth = 0.0;
    if (!tryGetDoubleParam(request, "Azimuth", true, azimuth)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "synctoazimuth", "Azimuth");
      return;
    }
    
    LOG_DEBUG("Syncing dome to azimuth:", azimuth);

    SyncToAzimuth(azimuth);

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "synctoazimuth", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("syncToAzimuthHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  // ==================== Common Device Handlers ====================

  void actionHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;

    if (request->method() != HTTP_PUT) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, true, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    String action;
    if (!tryGetStringParam(request, "Action", true, action)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "action", "Action");
      return;
    }

    String parameters;
    if (!tryGetOptionalStringParam(request, "Parameters", true, parameters)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "action", "Parameters");
      return;
    }

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "action", AlpacaError::ActionNotImplemented,
                         "Action not implemented");
    root.printTo(message);
    request->send(400, "application/json", message);
  }

  void commandblindHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;

    if (!extractClientIDAndTransactionID(request, true, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    String command;
    bool raw = false;
    if (!tryGetStringParam(request, "Command", true, command)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "commandblind", "Command");
      return;
    }
    if (!tryGetBoolParam(request, "Raw", true, raw)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "commandblind", "Raw");
      return;
    }

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "commandblind", AlpacaError::NotImplemented,
                         "CommandBlind not implemented");
    root.printTo(message);
    request->send(400, "application/json", message);
  }

  void commandboolHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;

    if (!extractClientIDAndTransactionID(request, true, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    String command;
    bool raw = false;
    if (!tryGetStringParam(request, "Command", true, command)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "commandbool", "Command");
      return;
    }
    if (!tryGetBoolParam(request, "Raw", true, raw)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "commandbool", "Raw");
      return;
    }

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "commandbool", AlpacaError::NotImplemented,
                         "CommandBool not implemented");
    root.printTo(message);
    request->send(400, "application/json", message);
  }

  void commandstringHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;

    if (!extractClientIDAndTransactionID(request, true, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    String command;
    bool raw = false;
    if (!tryGetStringParam(request, "Command", true, command)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "commandstring", "Command");
      return;
    }
    if (!tryGetBoolParam(request, "Raw", true, raw)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "commandstring", "Raw");
      return;
    }

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "commandstring", AlpacaError::NotImplemented,
                         "CommandString not implemented");
    root.printTo(message);
    request->send(400, "application/json", message);
  }

  void connectHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;

    if (!extractClientIDAndTransactionID(request, true, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "connect", AlpacaError::Success, "");
    root.printTo(message);
    request->send(200, "application/json", message);
  }

  void connectedHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;
    bool isGet = (request->method() == HTTP_GET);
    bool isPut = (request->method() == HTTP_PUT);

    if (!isGet && !isPut) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!extractClientIDAndTransactionID(request, !isGet, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    if (isPut) {
      bool connected = false;
      if (!tryGetBoolParam(request, "Connected", true, connected)) {
        sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "connected", "Connected");
        return;
      }
    }

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              true, AlpacaError::Success, "");
    root.printTo(message);
    request->send(200, "application/json", message);
  }

  void connectingHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;

    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              false, AlpacaError::Success, "");
    root.printTo(message);
    request->send(200, "application/json", message);
  }

  void deviceStateHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;

    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "devicestate", AlpacaError::NotImplemented,
                         "DeviceState not implemented");
    root.printTo(message);
    request->send(400, "application/json", message);
  }

  void disconnectHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;

    if (!extractClientIDAndTransactionID(request, true, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "disconnect", AlpacaError::Success, "");
    root.printTo(message);
    request->send(200, "application/json", message);
  }

  void driverInfoHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              "ASCOM Alpaca Dome Driver", AlpacaError::Success, "");
    root.printTo(message);
    request->send(200, "application/json", message);
  }

  void descriptionHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              Description, AlpacaError::Success, "");
    root.printTo(message);
    request->send(200, "application/json", message);
  }

  void driverVersionHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              String(instanceVersion), AlpacaError::Success, "");
    root.printTo(message);
    request->send(200, "application/json", message);
  }

  void interfaceVersionHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              2, AlpacaError::Success, "");
    root.printTo(message);
    request->send(200, "application/json", message);
  }

  void nameHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              GetDeviceName(), AlpacaError::Success, "");
    root.printTo(message);
    request->send(200, "application/json", message);
  }

  void supportedActionsHandler(AsyncWebServerRequest *request) override {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    if (!extractClientIDAndTransactionID(request, false, clientIDInt, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    root["ClientTransactionID"] = clientTransID;
    root["ServerTransactionID"] = ++serverTransID;
    root["ErrorNumber"] = static_cast<int>(AlpacaError::Success);
    root["ErrorMessage"] = "";
    JsonArray &values = root.createNestedArray("Value");
    root.printTo(message);
    request->send(200, "application/json", message);
  }
};

#endif // ALPACA_DEVICE_DOME_H
