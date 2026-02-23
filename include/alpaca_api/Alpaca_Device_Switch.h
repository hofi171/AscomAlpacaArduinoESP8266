#ifndef ALPACA_DEVICE_SWITCH_H
#define ALPACA_DEVICE_SWITCH_H

#include "Aplaca_Device.h"
#include "ascom_interfaces/ISwitch.h"
#include "DebugLog.h"
#include "Alpaca_Response_Builder.h"
#include "Alpaca_Errors.h"
#include "Alpaca_Driver_Settings.h"
#include "Alpaca_Request_Helper.h"
#include <ArduinoJson.h>

/**
 * @file Alpaca_Device_Switch.h
 * @brief Alpaca API implementation for Switch device
 * 
 * This class implements the ASCOM Alpaca API endpoints for Switch devices.
 * It extends AplacaDevice and implements ISwitch interface.
 * 
 * Endpoints (note {id} parameter for per-switch operations):
 * - GET /api/v1/switch/{device_number}/maxswitch - Get number of switches
 * - GET /api/v1/switch/{device_number}/canasync - Check async capability (ISwitchV3+)
 * - GET /api/v1/switch/{device_number}/canwrite - Check if switch is writable
 * - GET /api/v1/switch/{device_number}/getswitch - Get switch boolean state
 * - GET /api/v1/switch/{device_number}/getswitchdescription - Get switch description
 * - GET /api/v1/switch/{device_number}/getswitchname - Get switch name
 * - GET /api/v1/switch/{device_number}/getswitchvalue - Get switch value
 * - GET /api/v1/switch/{device_number}/minswitchvalue - Get minimum value
 * - GET /api/v1/switch/{device_number}/maxswitchvalue - Get maximum value
 * - GET /api/v1/switch/{device_number}/statechangecomplete - Check async state change complete (ISwitchV3+)
 * - GET /api/v1/switch/{device_number}/switchstep - Get switch step size
 * - PUT /api/v1/switch/{device_number}/setasync - Set switch state asynchronously (ISwitchV3+)
 * - PUT /api/v1/switch/{device_number}/setasyncvalue - Set switch value asynchronously (ISwitchV3+)
 * - PUT /api/v1/switch/{device_number}/setswitch - Set switch boolean state
 * - PUT /api/v1/switch/{device_number}/setswitchname - Set switch name
 * - PUT /api/v1/switch/{device_number}/setswitchvalue - Set switch value
 * 
 * Reference: https://ascom-standards.org/newdocs/switch.html
 */
class AlpacaDeviceSwitch : public AplacaDevice, public ISwitch {
private:
  String Description;
  uint32_t serverTransID = 0;

public:
  /**
   * @brief Constructor for AlpacaDeviceSwitch
   * @param devicename Name of the device
   * @param devicenumber Device number (for multiple devices of same type)
   * @param description Human-readable description of the device
   * @param server Reference to the AsyncWebServer instance
   */
  AlpacaDeviceSwitch(String devicename, int devicenumber, String description,
                     AsyncWebServer &server)
      : AplacaDevice(devicename, "switch", devicenumber, server) {
    Description = description;
    registerHandlers(server);
    LOG_DEBUG("AlpacaDeviceSwitch created:", devicename);
  }

  virtual ~AlpacaDeviceSwitch() {}

  // ==================== Device-specific endpoint registration ====================
  
  void registerHandlers(AsyncWebServer &server) override {
    LOG_DEBUG("Registering Switch specific handlers");
    
    // GET /api/v1/switch/{device_number}/maxswitch
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/maxswitch").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->maxSwitchHandler(request); });
    
    // GET /api/v1/switch/{device_number}/canasync
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/canasync").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->canAsyncHandler(request); });
    
    // GET /api/v1/switch/{device_number}/canwrite
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/canwrite").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->canWriteHandler(request); });
    
    // GET /api/v1/switch/{device_number}/getswitch
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/getswitch").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->getSwitchHandler(request); });
    
    // GET /api/v1/switch/{device_number}/getswitchdescription
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/getswitchdescription").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->getSwitchDescriptionHandler(request); });
    
    // GET /api/v1/switch/{device_number}/getswitchname
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/getswitchname").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->getSwitchNameHandler(request); });
    
    // GET /api/v1/switch/{device_number}/getswitchvalue
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/getswitchvalue").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->getSwitchValueHandler(request); });
    
    // GET /api/v1/switch/{device_number}/minswitchvalue
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/minswitchvalue").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->minSwitchValueHandler(request); });
    
    // GET /api/v1/switch/{device_number}/maxswitchvalue
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/maxswitchvalue").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->maxSwitchValueHandler(request); });
    
    // GET /api/v1/switch/{device_number}/statechangecomplete
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/statechangecomplete").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->stateChangeCompleteHandler(request); });
    
    // GET /api/v1/switch/{device_number}/switchstep
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/switchstep").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->switchStepHandler(request); });
    
    // PUT /api/v1/switch/{device_number}/setasync
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/setasync").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->setAsyncHandler(request); });
    
    // PUT /api/v1/switch/{device_number}/setasyncvalue
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/setasyncvalue").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->setAsyncValueHandler(request); });
    
    // PUT /api/v1/switch/{device_number}/setswitch
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/setswitch").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->setSwitchHandler(request); });
    
    // PUT /api/v1/switch/{device_number}/setswitchname
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/setswitchname").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->setSwitchNameHandler(request); });
    
    // PUT /api/v1/switch/{device_number}/setswitchvalue
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/setswitchvalue").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->setSwitchValueHandler(request); });
    
    LOG_DEBUG("Switch handlers registered!");
  }

  // ==================== Endpoint Handlers ====================
  
  /**
   * @brief Handler for GET /maxswitch endpoint
   * Returns the number of switch devices managed by this driver
   */
  void maxSwitchHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceSwitch::maxSwitchHandler - Method:", request->method());

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

    int maxSwitch = GetMaxSwitch();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  maxSwitch, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("maxSwitchHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for GET /canasync endpoint
   * Returns whether specified switch can operate asynchronously
   */
  void canAsyncHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceSwitch::canAsyncHandler - Method:", request->method());

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

    int id = 0;
    if (!tryGetIntParamAlt(request, "Id", "iD", false, id)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "canasync", "Id");
      return;
    }
    bool canAsync = GetCanAsync(id);

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              canAsync, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("canAsyncHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for GET /canwrite endpoint
   * Returns whether specified switch can be written to
   */
  void canWriteHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceSwitch::canWriteHandler - Method:", request->method());

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

    int id = 0;
    if (!tryGetIntParamAlt(request, "Id", "iD", false, id)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "canwrite", "Id");
      return;
    }
    bool canWrite = GetCanWrite(id);

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              canWrite, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("canWriteHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for GET /getswitch endpoint
   * Returns the state of the specified switch as a boolean
   */
  void getSwitchHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceSwitch::getSwitchHandler - Method:", request->method());

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

    int id = 0;
    if (!tryGetIntParamAlt(request, "Id", "iD", false, id)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "getswitch", "Id");
      return;
    }
    bool state = GetSwitch(id);

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              state, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("getSwitchHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for GET /getswitchdescription endpoint
   * Returns the description of the specified switch
   */
  void getSwitchDescriptionHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceSwitch::getSwitchDescriptionHandler - Method:", request->method());

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

    int id = 0;
    if (!tryGetIntParamAlt(request, "Id", "iD", false, id)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "getswitchdescription", "Id");
      return;
    }
    std::string description = GetSwitchDescription(id);

    String message;
    DynamicJsonBuffer jsonBuff(512);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              String(description.c_str()), AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("getSwitchDescriptionHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for GET /getswitchname endpoint
   * Returns the name of the specified switch
   */
  void getSwitchNameHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceSwitch::getSwitchNameHandler - Method:", request->method());

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

    int id = 0;
    if (!tryGetIntParamAlt(request, "Id", "iD", false, id)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "getswitchname", "Id");
      return;
    }
    std::string name = GetSwitchName(id);

    String message;
    DynamicJsonBuffer jsonBuff(512);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              String(name.c_str()), AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("getSwitchNameHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for GET /getswitchvalue endpoint
   * Returns the value of the specified switch as a double
   */
  void getSwitchValueHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceSwitch::getSwitchValueHandler - Method:", request->method());

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

    int id = 0;
    if (!tryGetIntParamAlt(request, "Id", "iD", false, id)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "getswitchvalue", "Id");
      return;
    }
    double value = GetSwitchValue(id);

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  value, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("getSwitchValueHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for GET /minswitchvalue endpoint
   * Returns the minimum value of the specified switch
   */
  void minSwitchValueHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceSwitch::minSwitchValueHandler - Method:", request->method());

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

    int id = 0;
    if (!tryGetIntParamAlt(request, "Id", "iD", false, id)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "minswitchvalue", "Id");
      return;
    }
    double minValue = GetMinSwitchValue(id);

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  minValue, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("minSwitchValueHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for GET /maxswitchvalue endpoint
   * Returns the maximum value of the specified switch
   */
  void maxSwitchValueHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceSwitch::maxSwitchValueHandler - Method:", request->method());

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

    int id = 0;
    if (!tryGetIntParamAlt(request, "Id", "iD", false, id)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "maxswitchvalue", "Id");
      return;
    }
    double maxValue = GetMaxSwitchValue(id);

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  maxValue, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("maxSwitchValueHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for GET /statechangecomplete endpoint
   * Returns whether asynchronous state change is complete (ISwitchV3+)
   */
  void stateChangeCompleteHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceSwitch::stateChangeCompleteHandler - Method:", request->method());

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

    int id = 0;
    if (!tryGetIntParamAlt(request, "Id", "iD", false, id)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "statechangecomplete", "Id");
      return;
    }
    bool stateChangeComplete = GetStateChangeComplete(id);

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              stateChangeComplete, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("stateChangeCompleteHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for GET /switchstep endpoint
   * Returns the step size for the specified switch
   */
  void switchStepHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceSwitch::switchStepHandler - Method:", request->method());

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

    int id = 0;
    if (!tryGetIntParamAlt(request, "Id", "iD", false, id)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "switchstep", "Id");
      return;
    }
    double step = GetSwitchStep(id);

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  step, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("switchStepHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for PUT /setasync endpoint
   * Sets the switch state asynchronously (ISwitchV3+)
   */
  void setAsyncHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceSwitch::setAsyncHandler - Method:", request->method());

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

    int id = 0;
    if (!tryGetIntParamAlt(request, "Id", "iD", true, id)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "setasync", "Id");
      return;
    }

    bool state = false;
    if (!tryGetBoolParam(request, "State", true, state)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "setasync", "State");
      return;
    }
    
    LOG_DEBUG("Setting switch " + String(id) + " async to: " + (state ? "true" : "false"));

    SetAsync(id, state);

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "setasync", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("setAsyncHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for PUT /setasyncvalue endpoint
   * Sets the switch value asynchronously (ISwitchV3+)
   */
  void setAsyncValueHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceSwitch::setAsyncValueHandler - Method:", request->method());

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

    int id = 0;
    if (!tryGetIntParamAlt(request, "Id", "iD", true, id)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "setasyncvalue", "Id");
      return;
    }

    double value = 0.0;
    if (!tryGetDoubleParam(request, "Value", true, value)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "setasyncvalue", "Value");
      return;
    }
    
    LOG_DEBUG("Setting switch " + String(id) + " async value to: " + String(value));

    SetAsyncValue(id, value);

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "setasyncvalue", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("setAsyncValueHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for PUT /setswitch endpoint
   * Sets the switch state
   */
  void setSwitchHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceSwitch::setSwitchHandler - Method:", request->method());

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

    int id = 0;
    if (!tryGetIntParam(request, "Id", true, id)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "setswitch", "Id");
      return;
    }

    bool state = false;
    if (!tryGetBoolParam(request, "State", true, state)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "setswitch", "State");
      return;
    }
    
    LOG_DEBUG("Setting switch " + String(id) + " to: " + (state ? "true" : "false"));

    SetSwitch(id, state);

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "setswitch", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("setSwitchHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for PUT /setswitchname endpoint
   * Sets the name of the specified switch
   */
  void setSwitchNameHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceSwitch::setSwitchNameHandler - Method:", request->method());

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

    int id = 0;
    if (!tryGetIntParam(request, "Id", true, id)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "setswitchname", "Id");
      return;
    }

    String name;
    if (!tryGetStringParam(request, "Name", true, name)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "setswitchname", "Name");
      return;
    }
    
    LOG_DEBUG("Setting switch " + String(id) + " name to: " + name);

    SetSwitchName(id, name.c_str());

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "setswitchname", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("setSwitchNameHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  /**
   * @brief Handler for PUT /setswitchvalue endpoint
   * Sets the value of the specified switch
   */
  void setSwitchValueHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceSwitch::setSwitchValueHandler - Method:", request->method());

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

    int id = 0;
    if (!tryGetIntParam(request, "Id", true, id)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "setswitchvalue", "Id");
      return;
    }

    double value = 0.0;
    if (!tryGetDoubleParam(request, "Value", true, value)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "setswitchvalue", "Value");
      return;
    }
    
    LOG_DEBUG("Setting switch " + String(id) + " value to: " + String(value));

    SetSwitchValue(id, value);

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "setswitchvalue", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("setSwitchValueHandler response:", message.c_str());

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
                              "ASCOM Alpaca Switch Driver", AlpacaError::Success, "");
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

#endif // ALPACA_DEVICE_SWITCH_H
