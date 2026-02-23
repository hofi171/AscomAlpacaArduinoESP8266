#ifndef ALPACA_DEVICE_FOCUSER_H
#define ALPACA_DEVICE_FOCUSER_H

#include "Aplaca_Device.h"
#include "ascom_interfaces/IFocuser.h"
#include "DebugLog.h"
#include "Alpaca_Response_Builder.h"
#include "Alpaca_Errors.h"
#include "Alpaca_Driver_Settings.h"
#include "Alpaca_Request_Helper.h"
#include <ArduinoJson.h>

/**
 * @file Alpaca_Device_Focuser.h
 * @brief Alpaca API implementation for Focuser device
 * 
 * This class implements the ASCOM Alpaca API endpoints for Focuser devices.
 * It extends AplacaDevice and implements IFocuser interface.
 * 
 * Endpoints:
 * - GET /api/v1/focuser/{device_number}/absolute - Get absolute positioning capability
 * - GET /api/v1/focuser/{device_number}/ismoving - Check if focuser is moving
 * - GET /api/v1/focuser/{device_number}/maxincrement - Get maximum increment size
 * - GET /api/v1/focuser/{device_number}/maxstep - Get maximum step position
 * - GET /api/v1/focuser/{device_number}/position - Get current position
 * - GET /api/v1/focuser/{device_number}/stepsize - Get step size in microns
 * - GET /api/v1/focuser/{device_number}/tempcomp - Get temperature compensation state
 * - PUT /api/v1/focuser/{device_number}/tempcomp - Set temperature compensation state
 * - GET /api/v1/focuser/{device_number}/tempcompavailable - Get temperature compensation availability
 * - GET /api/v1/focuser/{device_number}/temperature - Get current temperature
 * - PUT /api/v1/focuser/{device_number}/halt - Halt focuser motion
 * - PUT /api/v1/focuser/{device_number}/move - Move focuser to position
 * 
 * Reference: https://ascom-standards.org/newdocs/focuser.html
 */
class AlpacaDeviceFocuser : public AplacaDevice, public IFocuser {
private:
  String Description;
  uint32_t serverTransID = 0;

public:
  /**
   * @brief Constructor for AlpacaDeviceFocuser
   * @param devicename Name of the device
   * @param devicenumber Device number (for multiple devices of same type)
   * @param description Human-readable description of the device
   * @param server Reference to the AsyncWebServer instance
   */
  AlpacaDeviceFocuser(String devicename, int devicenumber, String description,
                      AsyncWebServer &server)
      : AplacaDevice(devicename, "focuser", devicenumber, server) {
    Description = description;
    registerHandlers(server);
    LOG_DEBUG("AlpacaDeviceFocuser created:", devicename);
  }

  virtual ~AlpacaDeviceFocuser() {}

  // ==================== Device-specific endpoint registration ====================
  
  void registerHandlers(AsyncWebServer &server) override {
    LOG_DEBUG("Registering Focuser specific handlers");
    
    // GET /api/v1/focuser/{device_number}/absolute
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/absolute").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->absoluteHandler(request); });
    
    // GET /api/v1/focuser/{device_number}/ismoving
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/ismoving").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->isMovingHandler(request); });
    
    // GET /api/v1/focuser/{device_number}/maxincrement
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/maxincrement").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->maxIncrementHandler(request); });
    
    // GET /api/v1/focuser/{device_number}/maxstep
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/maxstep").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->maxStepHandler(request); });
    
    // GET /api/v1/focuser/{device_number}/position
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/position").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->positionHandler(request); });
    
    // GET /api/v1/focuser/{device_number}/stepsize
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/stepsize").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->stepSizeHandler(request); });
    
    // GET /api/v1/focuser/{device_number}/tempcomp
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/tempcomp").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->tempCompGetHandler(request); });
    
    // PUT /api/v1/focuser/{device_number}/tempcomp
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/tempcomp").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->tempCompPutHandler(request); });
    
    // GET /api/v1/focuser/{device_number}/tempcompavailable
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/tempcompavailable").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->tempCompAvailableHandler(request); });
    
    // GET /api/v1/focuser/{device_number}/temperature
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/temperature").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->temperatureHandler(request); });
    
    // PUT /api/v1/focuser/{device_number}/halt
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/halt").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->haltHandler(request); });
    
    // PUT /api/v1/focuser/{device_number}/move
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/move").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->moveHandler(request); });
    
    LOG_DEBUG("Focuser handlers registered!");
  }

  // ==================== Endpoint Handlers ====================
  
  /**
   * @brief Handler for GET /absolute endpoint
   * Returns whether focuser supports absolute positioning
   */
  void absoluteHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceFocuser::absoluteHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    // Extract ClientID and ClientTransactionID from query parameters
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

    // Get absolute capability from device implementation
    bool absolute = GetAbsolute();

    // Build JSON response
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              absolute, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("absoluteHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for GET /ismoving endpoint
   * Returns whether focuser is currently moving
   */
  void isMovingHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceFocuser::isMovingHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    // Extract ClientID and ClientTransactionID from query parameters
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

    // Get moving state from device implementation
    bool isMoving = GetIsMoving();

    // Build JSON response
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              isMoving, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("isMovingHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for GET /maxincrement endpoint
   * Returns maximum increment size
   */
  void maxIncrementHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceFocuser::maxIncrementHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    // Extract ClientID and ClientTransactionID from query parameters
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

    // Get max increment from device implementation
    int maxIncrement = GetMaxIncrement();

    // Build JSON response
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              maxIncrement, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("maxIncrementHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for GET /maxstep endpoint
   * Returns maximum step position
   */
  void maxStepHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceFocuser::maxStepHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    // Extract ClientID and ClientTransactionID from query parameters
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

    // Get max step from device implementation
    int maxStep = GetMaxStep();

    // Build JSON response
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              maxStep, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("maxStepHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for GET /position endpoint
   * Returns current focuser position
   */
  void positionHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceFocuser::positionHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    // Extract ClientID and ClientTransactionID from query parameters
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

    // Get position from device implementation
    int position = GetPosition();

    // Build JSON response
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              position, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("positionHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for GET /stepsize endpoint
   * Returns step size in microns
   */
  void stepSizeHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceFocuser::stepSizeHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    // Extract ClientID and ClientTransactionID from query parameters
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

    // Get step size from device implementation
    double stepSize = GetStepSize();

    // Build JSON response
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              stepSize, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("stepSizeHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for GET /tempcomp endpoint
   * Returns temperature compensation state
   */
  void tempCompGetHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceFocuser::tempCompGetHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    // Extract ClientID and ClientTransactionID from query parameters
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

    // Get temperature compensation state from device implementation
    bool tempComp = GetTempComp();

    // Build JSON response
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              tempComp, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("tempCompGetHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for PUT /tempcomp endpoint
   * Sets temperature compensation state
   */
  void tempCompPutHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceFocuser::tempCompPutHandler - Method:", request->method());

    if (request->method() != HTTP_PUT) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    // Extract ClientID and ClientTransactionID from form data (PUT request)
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

    bool tempComp = false;
    if (!tryGetBoolParam(request, "TempComp", true, tempComp)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "tempcomp", "TempComp");
      return;
    }
    
    LOG_DEBUG("Setting TempComp to: " + String(tempComp ? "true" : "false"));

    // Set temperature compensation via device implementation
    SetTempComp(tempComp);

    // Build JSON response
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "tempcomp", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("tempCompPutHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for GET /tempcompavailable endpoint
   * Returns whether temperature compensation is available
   */
  void tempCompAvailableHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceFocuser::tempCompAvailableHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    // Extract ClientID and ClientTransactionID from query parameters
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

    // Get temperature compensation availability from device implementation
    bool tempCompAvailable = GetTempCompAvailable();

    // Build JSON response
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              tempCompAvailable, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("tempCompAvailableHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for GET /temperature endpoint
   * Returns current temperature
   */
  void temperatureHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceFocuser::temperatureHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    // Extract ClientID and ClientTransactionID from query parameters
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

    // Get temperature from device implementation
    double temperature = GetTemperature();

    // Build JSON response
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              temperature, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("temperatureHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for PUT /halt endpoint
   * Immediately halts focuser motion
   */
  void haltHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceFocuser::haltHandler - Method:", request->method());

    if (request->method() != HTTP_PUT) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    // Extract ClientID and ClientTransactionID from form data (PUT request)
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

    LOG_DEBUG("Halting focuser motion");

    // Halt focuser via device implementation
    Halt();

    // Build JSON response
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "halt", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("haltHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for PUT /move endpoint
   * Moves focuser to specified position
   */
  void moveHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceFocuser::moveHandler - Method:", request->method());

    if (request->method() != HTTP_PUT) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    // Extract ClientID and ClientTransactionID from form data (PUT request)
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

    int position = 0;
    if (!tryGetIntParam(request, "Position", true, position)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "move", "Position");
      return;
    }
    
    LOG_DEBUG("Moving focuser to position:", position);

    // Move focuser via device implementation
    Move(position);

    // Build JSON response
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "move", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("moveHandler response:", message.c_str());

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
                              "ASCOM Alpaca Focuser Driver", AlpacaError::Success, "");
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

#endif // ALPACA_DEVICE_FOCUSER_H
