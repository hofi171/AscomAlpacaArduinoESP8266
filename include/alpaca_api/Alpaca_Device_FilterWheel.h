#ifndef ALPACA_DEVICE_FILTERWHEEL_H
#define ALPACA_DEVICE_FILTERWHEEL_H

#include "Aplaca_Device.h"
#include "ascom_interfaces/IFilterWheel.h"
#include "DebugLog.h"
#include "Alpaca_Response_Builder.h"
#include "Alpaca_Errors.h"
#include "Alpaca_Driver_Settings.h"
#include "Alpaca_Request_Helper.h"
#include <ArduinoJson.h>

/**
 * @file Alpaca_Device_FilterWheel.h
 * @brief Alpaca API implementation for FilterWheel device
 * 
 * This class implements the ASCOM Alpaca API endpoints for FilterWheel devices.
 * It extends AplacaDevice and implements IFilterWheel interface.
 * 
 * Endpoints:
 * - GET /api/v1/filterwheel/{device_number}/focusoffsets - Get filter focus offsets
 * - GET /api/v1/filterwheel/{device_number}/names - Get filter names
 * - GET /api/v1/filterwheel/{device_number}/position - Get current filter position
 * - PUT /api/v1/filterwheel/{device_number}/position - Set filter position
 * 
 * Reference: https://ascom-standards.org/newdocs/filterwheel.html
 */
class AlpacaDeviceFilterWheel : public AplacaDevice, public IFilterWheel {
private:
  String Description;
  uint32_t serverTransID = 0;

public:
  /**
   * @brief Constructor for AlpacaDeviceFilterWheel
   * @param devicename Name of the device
   * @param devicenumber Device number (for multiple devices of same type)
   * @param description Human-readable description of the device
   * @param server Reference to the AsyncWebServer instance
   */
  AlpacaDeviceFilterWheel(String devicename, int devicenumber, String description,
                          AsyncWebServer &server)
      : AplacaDevice(devicename, "filterwheel", devicenumber, server) {
    Description = description;
    registerHandlers(server);
    LOG_DEBUG("AlpacaDeviceFilterWheel created:", devicename);
  }

  virtual ~AlpacaDeviceFilterWheel() {}

  // ==================== Device-specific endpoint registration ====================
  
  void registerHandlers(AsyncWebServer &server) override {
    LOG_DEBUG("Registering FilterWheel specific handlers");
    
    // GET /api/v1/filterwheel/{device_number}/focusoffsets
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/focusoffsets").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->focusOffsetsHandler(request); });
    
    // GET /api/v1/filterwheel/{device_number}/names
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/names").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->namesHandler(request); });
    
    // GET /api/v1/filterwheel/{device_number}/position
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/position").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->positionGetHandler(request); });
    
    // PUT /api/v1/filterwheel/{device_number}/position
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/position").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->positionPutHandler(request); });
    
    LOG_DEBUG("FilterWheel handlers registered!");
  }

  // ==================== Endpoint Handlers ====================
  
  /**
   * @brief Handler for GET /focusoffsets endpoint
   * Returns focus offsets for each filter position
   */
  void focusOffsetsHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceFilterWheel::focusOffsetsHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    // Extract ClientID and ClientTransactionID
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

    // Get focus offsets from device implementation
    std::vector<int> focusOffsets = GetFocusOffsets();

    // Build JSON response
    String message;
    DynamicJsonBuffer jsonBuff(512);
    JsonObject &root = jsonBuff.createObject();
    
    root["ClientTransactionID"] = clientTransID;
    root["ServerTransactionID"] = ++serverTransID;
    root["ErrorNumber"] = static_cast<int>(AlpacaError::Success);
    root["ErrorMessage"] = "";
    
    JsonArray &values = root.createNestedArray("Value");
    for (size_t i = 0; i < focusOffsets.size(); i++) {
      values.add(focusOffsets[i]);
    }
    
    root.printTo(message);
    LOG_DEBUG("focusOffsetsHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for GET /names endpoint
   * Returns names of filters in each position
   */
  void namesHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceFilterWheel::namesHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    // Extract ClientID and ClientTransactionID
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

    // Get filter names from device implementation
    std::vector<std::string> names = GetNames();

    // Build JSON response
    String message;
    DynamicJsonBuffer jsonBuff(512);
    JsonObject &root = jsonBuff.createObject();
    
    root["ClientTransactionID"] = clientTransID;
    root["ServerTransactionID"] = ++serverTransID;
    root["ErrorNumber"] = static_cast<int>(AlpacaError::Success);
    root["ErrorMessage"] = "";
    
    JsonArray &values = root.createNestedArray("Value");
    for (size_t i = 0; i < names.size(); i++) {
      values.add(names[i].c_str());
    }
    
    root.printTo(message);
    LOG_DEBUG("namesHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for GET /position endpoint
   * Returns current filter wheel position
   */
  void positionGetHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceFilterWheel::positionGetHandler - Method:", request->method());

    if (request->method() != HTTP_GET) {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    // Extract ClientID and ClientTransactionID
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

    // Get current position from device implementation
    int position = GetPosition();

    // Build JSON response
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  position, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("positionGetHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for PUT /position endpoint
   * Sets filter wheel to specified position
   */
  void positionPutHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceFilterWheel::positionPutHandler - Method:", request->method());

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

    int position = 0;
    if (!tryGetIntParam(request, "Position", true, position)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "position", "Position");
      return;
    }
    
    LOG_DEBUG("Setting position to:", position);

    // Set position via device implementation
    SetPosition(position);

    // Build JSON response
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "position", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("positionPutHandler response:", message.c_str());

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
                              "ASCOM Alpaca FilterWheel Driver", AlpacaError::Success, "");
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

#endif // ALPACA_DEVICE_FILTERWHEEL_H
