#ifndef ALPACA_DEVICE_SAFETYMONITOR_H
#define ALPACA_DEVICE_SAFETYMONITOR_H

#include "Aplaca_Device.h"
#include "ascom_interfaces/ISafetyMonitor.h"
#include "DebugLog.h"
#include "Alpaca_Response_Builder.h"
#include "Alpaca_Errors.h"
#include "Alpaca_Driver_Settings.h"
#include "Alpaca_Request_Helper.h"
#include <ArduinoJson.h>

/**
 * @file Alpaca_Device_SafetyMonitor.h
 * @brief Alpaca API implementation for SafetyMonitor device
 *
 * This class implements the ASCOM Alpaca API endpoints for SafetyMonitor devices.
 * It extends AplacaDevice and implements ISafetyMonitor interface.
 *
 * Endpoints:
 * - GET /api/v1/safetymonitor/{device_number}/issafe - Check if conditions are safe
 *
 * Reference: https://ascom-standards.org/newdocs/safetymonitor.html
 */
class AlpacaDeviceSafetyMonitor : public AplacaDevice, public ISafetyMonitor
{
private:
  String Description;
  uint32_t serverTransID = 0;

  // Helper methods for method-specific required parameters

  bool tryGetActionParam(AsyncWebServerRequest *request, bool fromBody, String &action)
  {
    if (!request->hasParam("Action", fromBody))
    {
      return false;
    }
    action = request->getParam("Action", fromBody)->value();
    // Action must not be empty
    if (action.length() == 0)
    {
      return false;
    }
    return true;
  }

  bool tryGetParametersParam(AsyncWebServerRequest *request, bool fromBody, String &parameters)
  {
    if (!request->hasParam("Parameters", fromBody))
    {
      return false;
    }
    parameters = request->getParam("Parameters", fromBody)->value();
    return true;
  }

  void sendMissingParamError(AsyncWebServerRequest *request, int clientID, int clientTransID,
                             const String &methodName, const String &paramName)
  {
    LOG_DEBUG("sendMissingParamError - Url: " + request->url() + ", Missing or invalid required parameter: " + paramName + " for method: " + methodName + " ClientID: " + String(clientID) + " ClientTransactionID: " + String(clientTransID));
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientID, clientTransID, ++serverTransID, methodName,
                          AlpacaError::InvalidValue, "Missing or invalid required parameter: " + paramName);
    root.printTo(message);
    request->send(400, "application/json", message);
  }

  bool ensureActionParams(AsyncWebServerRequest *request, bool fromBody, int clientID,
                          int clientTransID, const String &methodName,
                          String &action, String &parameters)
  {
    if (!tryGetActionParam(request, fromBody, action))
    {
      sendMissingParamError(request, clientID, clientTransID, methodName, "Action");
      return false;
    }
    if (!tryGetParametersParam(request, fromBody, parameters))
    {
      sendMissingParamError(request, clientID, clientTransID, methodName, "Parameters");
      return false;
    }
    return true;
  }

  bool tryGetCommandParam(AsyncWebServerRequest *request, bool fromBody, String &command)
  {
    if (!request->hasParam("Command", fromBody))
    {
      return false;
    }
    command = request->getParam("Command", fromBody)->value();
    // Command must not be empty
    if (command.length() == 0)
    {
      return false;
    }
    return true;
  }

  bool tryGetRawParam(AsyncWebServerRequest *request, bool fromBody, bool &raw)
  {
    if (!request->hasParam("Raw", fromBody))
    {
      return false;
    }
    String rawStr = request->getParam("Raw", fromBody)->value();
    rawStr.toLowerCase();
    // Validate boolean value
    if (rawStr == "true" || rawStr == "1")
    {
      raw = true;
      return true;
    }
    else if (rawStr == "false" || rawStr == "0")
    {
      raw = false;
      return true;
    }
    // Invalid boolean value
    return false;
  }

  bool ensureCommandParams(AsyncWebServerRequest *request, bool fromBody, int clientID,
                           int clientTransID, const String &methodName,
                           String &command, bool &raw)
  {
    if (!tryGetCommandParam(request, fromBody, command))
    {
      sendMissingParamError(request, clientID, clientTransID, methodName, "Command");
      return false;
    }
    if (!tryGetRawParam(request, fromBody, raw))
    {
      sendMissingParamError(request, clientID, clientTransID, methodName, "Raw");
      return false;
    }
    return true;
  }

  bool tryGetConnectedParam(AsyncWebServerRequest *request, bool fromBody, bool &connected)
  {
    if (!request->hasParam("Connected", fromBody))
    {
      return false;
    }
    String connectedStr = request->getParam("Connected", fromBody)->value();
    connectedStr.toLowerCase();
    // Validate boolean value
    if (connectedStr == "true" || connectedStr == "1")
    {
      connected = true;
      return true;
    }
    else if (connectedStr == "false" || connectedStr == "0")
    {
      connected = false;
      return true;
    }
    // Invalid boolean value
    return false;
  }

  bool ensureConnectedParam(AsyncWebServerRequest *request, bool fromBody, int clientID,
                            int clientTransID, const String &methodName, bool &connected)
  {
    if (!tryGetConnectedParam(request, fromBody, connected))
    {
      sendMissingParamError(request, clientID, clientTransID, methodName, "Connected");
      return false;
    }
    return true;
  }

  bool ensureClientIDs(AsyncWebServerRequest *request, bool fromBody, int &clientIDInt, int &clientTransID)
  {
    if (!extractClientIDAndTransactionID(request, fromBody, clientIDInt, clientTransID))
    {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                           "invalid_parameters", AlpacaError::InvalidValue, 
                           "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return false;
    }
    return true;
  }

public:
  /**
   * @brief Constructor for AlpacaDeviceSafetyMonitor
   * @param devicename Name of the device
   * @param devicenumber Device number (for multiple devices of same type)
   * @param description Human-readable description of the device
   * @param server Reference to the AsyncWebServer instance
   */
  AlpacaDeviceSafetyMonitor(String devicename, int devicenumber, String description,
                            AsyncWebServer &server)
      : AplacaDevice(devicename, "safetymonitor", devicenumber, server)
  {
    Description = description;
    registerHandlers(server);
    LOG_DEBUG("AlpacaDeviceSafetyMonitor created: " + devicename);
  }

  virtual ~AlpacaDeviceSafetyMonitor() {}

  // ==================== Device-specific endpoint registration ====================

  void registerHandlers(AsyncWebServer &server) override
  {
    LOG_DEBUG("Registering SafetyMonitor specific handlers");

    // GET /api/v1/safetymonitor/{device_number}/issafe
    server.on((String("/api/v") + String(InterfaceVersion) + "/" +
               GetDeviceType() + "/" + GetDeviceNumber() + "/issafe")
                  .c_str(),
              HTTP_GET,
              [this](AsyncWebServerRequest *request)
              { this->isSafeHandler(request); });

    LOG_DEBUG("SafetyMonitor handlers registered!");
  }

  // ==================== Endpoint Handlers ====================

  /**
   * @brief Handler for /issafe endpoint
   * Returns whether monitored state is safe for use
   */
  void isSafeHandler(AsyncWebServerRequest *request)
  {
    int clientIDInt = 0;
    int clientTransID = 0;
    LOG_DEBUG("############################################");
    LOG_DEBUG("\r\nAlpacaDeviceSafetyMonitor::isSafeHandler - Method: " + String(request->method()) + " URL: " + request->url());

    if (request->method() != HTTP_GET)
    {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    size_t count = request->params();
    LOG_DEBUG("Request has " + String(count) + " params");
    for (size_t i = 0; i < count; i++)
    {
      const AsyncWebParameter *p = request->getParam(i);
      LOG_DEBUG("PARAM[" + String(i) + "]: " + p->name() + " = " + p->value());
    }

    if (!ensureClientIDs(request, false, clientIDInt, clientTransID))
    {
      return;
    }

    // Get the safety state from the device implementation
    bool isSafe = GetIsSafe();

    // Build JSON response
    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();

    // Use AlpacaResponseValueBuilder for value responses
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                               isSafe, AlpacaError::Success, "");

    root.printTo(message);
    LOG_DEBUG("isSafeHandler response: " + message);

    request->send(200, "application/json", message);
  }

  // ==================== Common Device Handlers ====================

  void actionHandler(AsyncWebServerRequest *request) override
  {
    int clientIDInt = 0;
    int clientTransID = 0;

    LOG_DEBUG("############################################");
    LOG_DEBUG("\r\nAlpacaDeviceSafetyMonitor::actionHandler - Method: " + String(request->method()));

    if (request->method() != HTTP_PUT)
    {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!ensureClientIDs(request, request->method() == HTTP_PUT, clientIDInt, clientTransID))
    {
      return;
    }

    String action;
    String parameters;
    if (!ensureActionParams(request, request->method() == HTTP_PUT, clientIDInt, clientTransID, "action", action, parameters))
    {
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

  void commandblindHandler(AsyncWebServerRequest *request) override
  {
    int clientIDInt = 0;
    int clientTransID = 0;

    LOG_DEBUG("############################################");
    LOG_DEBUG("\r\nAlpacaDeviceSafetyMonitor::commandblindHandler - Method: " + String(request->method()) + " URL: " + request->url());

    if (request->method() != HTTP_PUT)
    {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!ensureClientIDs(request, request->method() == HTTP_PUT, clientIDInt, clientTransID))
    {
      return;
    }

    String command;
    bool raw;
    if (!ensureCommandParams(request, request->method() == HTTP_PUT, clientIDInt, clientTransID, "commandblind", command, raw))
    {
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

  void commandboolHandler(AsyncWebServerRequest *request) override
  {
    int clientIDInt = 0;
    int clientTransID = 0;

    LOG_DEBUG("############################################");
    LOG_DEBUG("\r\nAlpacaDeviceSafetyMonitor::commandboolHandler - Method: " + String(request->method()) + " URL: " + request->url());

    if (request->method() != HTTP_PUT)
    {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!ensureClientIDs(request, request->method() == HTTP_PUT, clientIDInt, clientTransID))
    {
      return;
    }

    String command;
    bool raw;
    if (!ensureCommandParams(request, request->method() == HTTP_PUT, clientIDInt, clientTransID, "commandbool", command, raw))
    {
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

  void commandstringHandler(AsyncWebServerRequest *request) override
  {
    int clientIDInt = 0;
    int clientTransID = 0;

    LOG_DEBUG("############################################");
    LOG_DEBUG("\r\nAlpacaDeviceSafetyMonitor::commandstringHandler - Method: " + String(request->method()) + " URL: " + request->url());

    if (request->method() != HTTP_PUT)
    {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!ensureClientIDs(request, request->method() == HTTP_PUT, clientIDInt, clientTransID))
    {
      return;
    }

    String command;
    bool raw;
    if (!ensureCommandParams(request, request->method() == HTTP_PUT, clientIDInt, clientTransID, "commandstring", command, raw))
    {
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

  void connectHandler(AsyncWebServerRequest *request) override
  {
    int clientIDInt = 0;
    int clientTransID = 0;

    LOG_DEBUG("############################################");
    LOG_DEBUG("\r\nAlpacaDeviceSafetyMonitor::connectHandler - Method: " + String(request->method()) + " URL: " + request->url());

    if (request->method() != HTTP_PUT)
    {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!ensureClientIDs(request, request->method() == HTTP_PUT, clientIDInt, clientTransID))
    {
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

  void connectedHandler(AsyncWebServerRequest *request) override
  {
    int clientIDInt = 0;
    int clientTransID = 0;

    LOG_DEBUG("############################################");
    LOG_DEBUG("\r\nAlpacaDeviceSafetyMonitor::connectedHandler - Method: " + String(request->method()) + " URL: " + request->url());

    bool isGet = (request->method() == HTTP_GET);
    bool isPut = (request->method() == HTTP_PUT);

    if (!isGet && !isPut)
    {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!ensureClientIDs(request, !isGet, clientIDInt, clientTransID))
    {
      return;
    }

    if (isPut)
    {
      bool connected;
      if (!ensureConnectedParam(request, request->method() == HTTP_PUT, clientIDInt, clientTransID, "connected", connected))
      {
        return;
      }
      // TODO: Actually set the connected state based on the 'connected' parameter
    }

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                               true, AlpacaError::Success, "");
    root.printTo(message);
    request->send(200, "application/json", message);
  }

  void connectingHandler(AsyncWebServerRequest *request) override
  {
    int clientIDInt = 0;
    int clientTransID = 0;

    LOG_DEBUG("############################################");
    LOG_DEBUG("\r\nAlpacaDeviceSafetyMonitor::connectingHandler - Method: " + String(request->method()) + " URL: " + request->url());

    if (request->method() != HTTP_GET)
    {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!ensureClientIDs(request, false, clientIDInt, clientTransID))
    {
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

  void descriptionHandler(AsyncWebServerRequest *request) override
  {
    int clientIDInt = 0;
    int clientTransID = 0;
    LOG_DEBUG("############################################");
    LOG_DEBUG("\r\nAlpacaDeviceSafetyMonitor::descriptionHandler - Method: " + String(request->method()) + " URL: " + request->url());

    if (request->method() != HTTP_GET)
    {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!ensureClientIDs(request, false, clientIDInt, clientTransID))
    {
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

  void deviceStateHandler(AsyncWebServerRequest *request) override
  {
    int clientIDInt = 0;
    int clientTransID = 0;

    LOG_DEBUG("############################################");
    LOG_DEBUG("\r\nAlpacaDeviceSafetyMonitor::deviceStateHandler - Method: " + String(request->method()) + " URL: " + request->url());

    if (request->method() != HTTP_GET)
    {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!ensureClientIDs(request, false, clientIDInt, clientTransID))
    {
      return;
    }

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();

    JsonArray &value = root.createNestedArray("Value");
    JsonObject &item1 = value.createNestedObject();
    item1["Name"] = "IsSafe";
    item1["Value"] = GetIsSafe();
    JsonObject &item2 = value.createNestedObject();
    item2["Name"] = "Timestamp";
    item2["Value"] = String(millis());

    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID, AlpacaError::Success, "");

    // isSafe ? "true" : "false"
    root.printTo(message);
    request->send(200, "application/json", message);
  }

  void disconnectHandler(AsyncWebServerRequest *request) override
  {
    int clientIDInt = 0;
    int clientTransID = 0;

    LOG_DEBUG("############################################");
    LOG_DEBUG("\r\n AlpacaDeviceSafetyMonitor::disconnectHandler - Method: " + String(request->method()) + " URL: " + request->url());

    if (request->method() != HTTP_PUT)
    {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!ensureClientIDs(request, request->method() == HTTP_PUT, clientIDInt, clientTransID))
    {
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

  void driverInfoHandler(AsyncWebServerRequest *request) override
  {
    int clientIDInt = 0;
    int clientTransID = 0;

    LOG_DEBUG("############################################");
    LOG_DEBUG("\r\nAlpacaDeviceSafetyMonitor::driverInfoHandler - Method: " + String(request->method()) + " URL: " + request->url());

    if (request->method() != HTTP_GET)
    {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!ensureClientIDs(request, false, clientIDInt, clientTransID))
    {
      return;
    }

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                               "ASCOM Alpaca SafetyMonitor Driver", AlpacaError::Success, "");
    root.printTo(message);
    request->send(200, "application/json", message);
  }

  void driverVersionHandler(AsyncWebServerRequest *request) override
  {
    int clientIDInt = 0;
    int clientTransID = 0;

    LOG_DEBUG("############################################");
    LOG_DEBUG("\r\nAlpacaDeviceSafetyMonitor::driverVersionHandler - Method: " + String(request->method()) + " URL: " + request->url());

    if (request->method() != HTTP_GET)
    {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!ensureClientIDs(request, false, clientIDInt, clientTransID))
    {
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

  void interfaceVersionHandler(AsyncWebServerRequest *request) override
  {
    int clientIDInt = 0;
    int clientTransID = 0;

    LOG_DEBUG("############################################");
    LOG_DEBUG("\r\nAlpacaDeviceSafetyMonitor::interfaceVersionHandler - Method: " + String(request->method()) + " URL: " + request->url());

    if (request->method() != HTTP_GET)
    {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!ensureClientIDs(request, false, clientIDInt, clientTransID))
    {
      return;
    }

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                               3, AlpacaError::Success, "");
    root.printTo(message);
    request->send(200, "application/json", message);
  }

  void nameHandler(AsyncWebServerRequest *request) override
  {
    int clientIDInt = 0;
    int clientTransID = 0;

    LOG_DEBUG("############################################");
    LOG_DEBUG("\r\nAlpacaDeviceSafetyMonitor::nameHandler - Method: " + String(request->method()) + " URL: " + request->url());

    if (request->method() != HTTP_GET)
    {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!ensureClientIDs(request, false, clientIDInt, clientTransID))
    {
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

  void supportedActionsHandler(AsyncWebServerRequest *request) override
  {
    int clientIDInt = 0;
    int clientTransID = 0;

    LOG_DEBUG("############################################");
    LOG_DEBUG("\r\nAlpacaDeviceSafetyMonitor::supportedActionsHandler - Method: " + String(request->method()) + " URL: " + request->url());

    if (request->method() != HTTP_GET)
    {
      request->send(405, "application/json", "{\"ErrorMessage\": \"Method Not Allowed\"}");
      return;
    }

    if (!ensureClientIDs(request, false, clientIDInt, clientTransID))
    {
      return;
    }

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    root["ClientTransactionID"] = clientTransID;
    root["ServerTransactionID"] = ++serverTransID;
    root["ErrorNumber"] = static_cast<int>(AlpacaError::Success);
    root["ErrorMessage"] = "";
    root.createNestedArray("Value");
    // No custom actions supported - return empty array
    root.printTo(message);
    request->send(200, "application/json", message);
  }
};

#endif // ALPACA_DEVICE_SAFETYMONITOR_H
