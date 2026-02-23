#ifndef ALPACA_DEVICE_COVERCALIBRATOR_H
#define ALPACA_DEVICE_COVERCALIBRATOR_H

#include "Aplaca_Device.h"
#include "ascom_interfaces/ICoverCalibrator.h"
#include "DebugLog.h"
#include "Alpaca_Response_Builder.h"
#include "Alpaca_Errors.h"
#include "Alpaca_Driver_Settings.h"
#include "Alpaca_Request_Helper.h"
#include <ArduinoJson.h>

/**
 * @file Alpaca_Device_CoverCalibrator.h
 * @brief Alpaca API implementation for CoverCalibrator device
 * 
 * This class implements the ASCOM Alpaca API endpoints for CoverCalibrator devices.
 * It extends AplacaDevice and implements ICoverCalibrator interface.
 * 
 * Endpoints:
 * - GET /api/v1/covercalibrator/{device_number}/brightness - Get current brightness
 * - GET /api/v1/covercalibrator/{device_number}/calibratorchanging - Check if calibrator is changing
 * - GET /api/v1/covercalibrator/{device_number}/calibratorstate - Get calibrator state
 * - GET /api/v1/covercalibrator/{device_number}/covermoving - Check if cover is moving
 * - GET /api/v1/covercalibrator/{device_number}/coverstate - Get cover state
 * - GET /api/v1/covercalibrator/{device_number}/maxbrightness - Get maximum brightness
 * - PUT /api/v1/covercalibrator/{device_number}/calibratoroff - Turn calibrator off
 * - PUT /api/v1/covercalibrator/{device_number}/calibratoron - Turn calibrator on
 * - PUT /api/v1/covercalibrator/{device_number}/closecover - Close the cover
 * - PUT /api/v1/covercalibrator/{device_number}/haltcover - Halt cover movement
 * - PUT /api/v1/covercalibrator/{device_number}/opencover - Open the cover
 * 
 * Reference: https://ascom-standards.org/newdocs/covercalibrator.html
 */
class AlpacaDeviceCoverCalibrator : public AplacaDevice, public ICoverCalibrator {
private:
  String Description;
  uint32_t serverTransID = 0;

public:
  /**
   * @brief Constructor for AlpacaDeviceCoverCalibrator
   * @param devicename Name of the device
   * @param devicenumber Device number (for multiple devices of same type)
   * @param description Human-readable description of the device
   * @param server Reference to the AsyncWebServer instance
   */
  AlpacaDeviceCoverCalibrator(String devicename, int devicenumber, String description,
                              AsyncWebServer &server)
      : AplacaDevice(devicename, "covercalibrator", devicenumber, server) {
    Description = description;
    registerHandlers(server);
    LOG_DEBUG("AlpacaDeviceCoverCalibrator created:", devicename);
  }

  virtual ~AlpacaDeviceCoverCalibrator() {}

  // ==================== Device-specific endpoint registration ====================
  
  void registerHandlers(AsyncWebServer &server) override {
    LOG_DEBUG("Registering CoverCalibrator specific handlers");
    
    // GET /api/v1/covercalibrator/{device_number}/brightness
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/brightness").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->brightnessHandler(request); });
    
    // GET /api/v1/covercalibrator/{device_number}/calibratorchanging
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/calibratorchanging").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->calibratorChangingHandler(request); });
    
    // GET /api/v1/covercalibrator/{device_number}/calibratorstate
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/calibratorstate").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->calibratorStateHandler(request); });
    
    // GET /api/v1/covercalibrator/{device_number}/covermoving
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/covermoving").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->coverMovingHandler(request); });
    
    // GET /api/v1/covercalibrator/{device_number}/coverstate
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/coverstate").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->coverStateHandler(request); });
    
    // GET /api/v1/covercalibrator/{device_number}/maxbrightness
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/maxbrightness").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->maxBrightnessHandler(request); });
    
    // PUT /api/v1/covercalibrator/{device_number}/calibratoroff
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/calibratoroff").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->calibratorOffHandler(request); });
    
    // PUT /api/v1/covercalibrator/{device_number}/calibratoron
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/calibratoron").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->calibratorOnHandler(request); });
    
    // PUT /api/v1/covercalibrator/{device_number}/closecover
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/closecover").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->closeCoverHandler(request); });
    
    // PUT /api/v1/covercalibrator/{device_number}/haltcover
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/haltcover").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->haltCoverHandler(request); });
    
    // PUT /api/v1/covercalibrator/{device_number}/opencover
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/opencover").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->openCoverHandler(request); });
    
    LOG_DEBUG("CoverCalibrator handlers registered!");
  }

  // ==================== Endpoint Handlers ====================
  
  void brightnessHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceCoverCalibrator::brightnessHandler - Method:", request->method());

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

    int brightness = GetBrightness();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  brightness, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("brightnessHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void calibratorChangingHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceCoverCalibrator::calibratorChangingHandler - Method:", request->method());

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

    bool calibratorChanging = GetCalibratorChanging();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              calibratorChanging, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("calibratorChangingHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void calibratorStateHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceCoverCalibrator::calibratorStateHandler - Method:", request->method());

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

    CalibratorStatus calibratorState = GetCalibratorState();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              String((int)calibratorState), AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("calibratorStateHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void coverMovingHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceCoverCalibrator::coverMovingHandler - Method:", request->method());

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

    bool coverMoving = GetCoverMoving();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              coverMoving, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("coverMovingHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void coverStateHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceCoverCalibrator::coverStateHandler - Method:", request->method());

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

    CoverStatus coverState = GetCoverState();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              String((int)coverState), AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("coverStateHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void maxBrightnessHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceCoverCalibrator::maxBrightnessHandler - Method:", request->method());

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

    int maxBrightness = GetMaxBrightness();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              String(maxBrightness), AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("maxBrightnessHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void calibratorOffHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceCoverCalibrator::calibratorOffHandler - Method:", request->method());

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

    LOG_DEBUG("Turning calibrator off");

    CalibratorOff();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "calibratoroff", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("calibratorOffHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void calibratorOnHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceCoverCalibrator::calibratorOnHandler - Method:", request->method());

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

    int brightness = 0;
    if (!tryGetIntParam(request, "Brightness", true, brightness)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "calibratoron", "Brightness");
      return;
    }
    
    LOG_DEBUG("Turning calibrator on with brightness:", brightness);

    CalibratorOn(brightness);

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "calibratoron", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("calibratorOnHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void closeCoverHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceCoverCalibrator::closeCoverHandler - Method:", request->method());

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

    LOG_DEBUG("Closing cover");

    CloseCover();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "closecover", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("closeCoverHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void haltCoverHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceCoverCalibrator::haltCoverHandler - Method:", request->method());

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

    LOG_DEBUG("Halting cover movement");

    HaltCover();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "haltcover", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("haltCoverHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void openCoverHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceCoverCalibrator::openCoverHandler - Method:", request->method());

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

    LOG_DEBUG("Opening cover");

    OpenCover();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "opencover", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("openCoverHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  // ==================== Common Device Handlers ====================

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
                              "ASCOM Alpaca CoverCalibrator Driver", AlpacaError::Success, "");
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
                              DRIVER_VERSION, AlpacaError::Success, "");
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
                              1, AlpacaError::Success, "");
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

#endif // ALPACA_DEVICE_COVERCALIBRATOR_H
