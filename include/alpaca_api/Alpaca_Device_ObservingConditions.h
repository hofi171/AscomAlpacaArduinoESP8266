#ifndef ALPACA_DEVICE_OBSERVINGCONDITIONS_H
#define ALPACA_DEVICE_OBSERVINGCONDITIONS_H

#include "Aplaca_Device.h"
#include "ascom_interfaces/IObservingConditions.h"
#include "DebugLog.h"
#include "Alpaca_Response_Builder.h"
#include "Alpaca_Errors.h"
#include "Alpaca_Driver_Settings.h"
#include "Alpaca_Request_Helper.h"
#include <ArduinoJson.h>

/**
 * @file Alpaca_Device_ObservingConditions.h
 * @brief Alpaca API implementation for ObservingConditions device
 * 
 * This class implements the ASCOM Alpaca API endpoints for ObservingConditions devices.
 * It extends AplacaDevice and implements IObservingConditions interface.
 * 
 * Endpoints:
 * - GET /api/v1/observingconditions/{device_number}/averageperiod - Get averaging period
 * - PUT /api/v1/observingconditions/{device_number}/averageperiod - Set averaging period
 * - GET /api/v1/observingconditions/{device_number}/cloudcover - Get cloud cover percentage
 * - GET /api/v1/observingconditions/{device_number}/dewpoint - Get dew point
 * - GET /api/v1/observingconditions/{device_number}/humidity - Get humidity
 * - GET /api/v1/observingconditions/{device_number}/pressure - Get pressure
 * - GET /api/v1/observingconditions/{device_number}/rainrate - Get rain rate
 * - GET /api/v1/observingconditions/{device_number}/skybrightness - Get sky brightness
 * - GET /api/v1/observingconditions/{device_number}/skyquality - Get sky quality
 * - GET /api/v1/observingconditions/{device_number}/skytemperature - Get sky temperature
 * - GET /api/v1/observingconditions/{device_number}/starfwhm - Get star FWHM
 * - GET /api/v1/observingconditions/{device_number}/temperature - Get temperature
 * - GET /api/v1/observingconditions/{device_number}/winddirection - Get wind direction
 * - GET /api/v1/observingconditions/{device_number}/windgust - Get wind gust
 * - GET /api/v1/observingconditions/{device_number}/windspeed - Get wind speed
 * - PUT /api/v1/observingconditions/{device_number}/refresh - Refresh sensor readings
 * - GET /api/v1/observingconditions/{device_number}/sensordescription - Get sensor description
 * - GET /api/v1/observingconditions/{device_number}/timesincelastupdate - Get time since last update
 * 
 * Reference: https://ascom-standards.org/newdocs/observingconditions.html
 */
class AlpacaDeviceObservingConditions : public AplacaDevice, public IObservingConditions {
private:
  String Description;
  uint32_t serverTransID = 0;

public:
  /**
   * @brief Constructor for AlpacaDeviceObservingConditions
   * @param devicename Name of the device
   * @param devicenumber Device number (for multiple devices of same type)
   * @param description Human-readable description of the device
   * @param server Reference to the AsyncWebServer instance
   */
  AlpacaDeviceObservingConditions(String devicename, int devicenumber, String description,
                   AsyncWebServer &server)
      : AplacaDevice(devicename, "observingconditions", devicenumber, server) {
    Description = description;
    registerHandlers(server);
    LOG_DEBUG("AlpacaDeviceObservingConditions created:", devicename);
  }

  virtual ~AlpacaDeviceObservingConditions() {}

  // ==================== Device-specific endpoint registration ====================
  
  void registerHandlers(AsyncWebServer &server) override {
    LOG_DEBUG("Registering ObservingConditions specific handlers");
    
    // GET /api/v1/observingconditions/{device_number}/averageperiod
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/averageperiod").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->averagePeriodGetHandler(request); });
    
    // PUT /api/v1/observingconditions/{device_number}/averageperiod
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/averageperiod").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->averagePeriodPutHandler(request); });
    
    // GET /api/v1/observingconditions/{device_number}/cloudcover
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/cloudcover").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->cloudCoverHandler(request); });
    
    // GET /api/v1/observingconditions/{device_number}/dewpoint
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/dewpoint").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->dewPointHandler(request); });
    
    // GET /api/v1/observingconditions/{device_number}/humidity
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/humidity").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->humidityHandler(request); });
    
    // GET /api/v1/observingconditions/{device_number}/pressure
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/pressure").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->pressureHandler(request); });
    
    // GET /api/v1/observingconditions/{device_number}/rainrate
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/rainrate").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->rainRateHandler(request); });
    
    // GET /api/v1/observingconditions/{device_number}/skybrightness
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/skybrightness").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->skyBrightnessHandler(request); });
    
    // GET /api/v1/observingconditions/{device_number}/skyquality
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/skyquality").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->skyQualityHandler(request); });
    
    // GET /api/v1/observingconditions/{device_number}/skytemperature
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/skytemperature").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->skyTemperatureHandler(request); });
    
    // GET /api/v1/observingconditions/{device_number}/starfwhm
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/starfwhm").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->starFWHMHandler(request); });
    
    // GET /api/v1/observingconditions/{device_number}/temperature
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/temperature").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->temperatureHandler(request); });
    
    // GET /api/v1/observingconditions/{device_number}/winddirection
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/winddirection").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->windDirectionHandler(request); });
    
    // GET /api/v1/observingconditions/{device_number}/windgust
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/windgust").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->windGustHandler(request); });
    
    // GET /api/v1/observingconditions/{device_number}/windspeed
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/windspeed").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->windSpeedHandler(request); });
    
    // PUT /api/v1/observingconditions/{device_number}/refresh
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/refresh").c_str(), 
              HTTP_PUT, 
              [this](AsyncWebServerRequest *request){ this->refreshHandler(request); });
    
    // GET /api/v1/observingconditions/{device_number}/sensordescription
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/sensordescription").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->sensorDescriptionHandler(request); });
    
    // GET /api/v1/observingconditions/{device_number}/timesincelastupdate
    server.on((String("/api/v") + String(InterfaceVersion) + "/" + 
               GetDeviceType() + "/" + GetDeviceNumber() + "/timesincelastupdate").c_str(), 
              HTTP_GET, 
              [this](AsyncWebServerRequest *request){ this->timeSinceLastUpdateHandler(request); });
    
    LOG_DEBUG("ObservingConditions handlers registered!");
  }

  // ==================== Endpoint Handlers ====================
  
  void averagePeriodGetHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceObservingConditions::averagePeriodGetHandler - Method:", request->method());

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

    double averagePeriod = GetAveragePeriod();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  averagePeriod, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("averagePeriodGetHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void averagePeriodPutHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceObservingConditions::averagePeriodPutHandler - Method:", request->method());

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

    double period = 0.0;
    if (!tryGetDoubleParam(request, "AveragePeriod", true, period)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "averageperiod", "AveragePeriod");
      return;
    }
    
    LOG_DEBUG("Setting AveragePeriod to:", period);

    SetAveragePeriod(period);

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "averageperiod", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("averagePeriodPutHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void cloudCoverHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceObservingConditions::cloudCoverHandler - Method:", request->method());

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

    double cloudCover = GetCloudCover();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  cloudCover, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("cloudCoverHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void dewPointHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceObservingConditions::dewPointHandler - Method:", request->method());

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

    double dewPoint = GetDewPoint();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  dewPoint, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("dewPointHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void humidityHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceObservingConditions::humidityHandler - Method:", request->method());

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

    double humidity = GetHumidity();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  humidity, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("humidityHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void pressureHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceObservingConditions::pressureHandler - Method:", request->method());

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

    double pressure = GetPressure();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  pressure, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("pressureHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void rainRateHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceObservingConditions::rainRateHandler - Method:", request->method());

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

    double rainRate = GetRainRate();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  rainRate, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("rainRateHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void skyBrightnessHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceObservingConditions::skyBrightnessHandler - Method:", request->method());

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

    double skyBrightness = GetSkyBrightness();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  skyBrightness, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("skyBrightnessHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void skyQualityHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceObservingConditions::skyQualityHandler - Method:", request->method());

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

    double skyQuality = GetSkyQuality();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  skyQuality, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("skyQualityHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void skyTemperatureHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceObservingConditions::skyTemperatureHandler - Method:", request->method());

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

    double skyTemperature = GetSkyTemperature();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  skyTemperature, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("skyTemperatureHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void starFWHMHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceObservingConditions::starFWHMHandler - Method:", request->method());

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

    double starFWHM = GetStarFWHM();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  starFWHM, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("starFWHMHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void temperatureHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceObservingConditions::temperatureHandler - Method:", request->method());

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

    double temperature = GetTemperature();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  temperature, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("temperatureHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void windDirectionHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceObservingConditions::windDirectionHandler - Method:", request->method());

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

    double windDirection = GetWindDirection();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  windDirection, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("windDirectionHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void windGustHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceObservingConditions::windGustHandler - Method:", request->method());

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

    double windGust = GetWindGust();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  windGust, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("windGustHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void windSpeedHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceObservingConditions::windSpeedHandler - Method:", request->method());

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

    double windSpeed = GetWindSpeed();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  windSpeed, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("windSpeedHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void refreshHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceObservingConditions::refreshHandler - Method:", request->method());

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

    LOG_DEBUG("Refreshing sensor readings");

    Refresh();

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    AlpacaResponseBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                         "refresh", AlpacaError::Success, "");
    root.printTo(message);
    LOG_DEBUG("refreshHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void sensorDescriptionHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceObservingConditions::sensorDescriptionHandler - Method:", request->method());

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

    String sensorName;
    if (!tryGetStringParam(request, "SensorName", false, sensorName)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "sensordescription", "SensorName");
      return;
    }
    std::string sensorNameStd = sensorName.c_str();
    std::string description = GetSensorDescription(sensorNameStd);

    String message;
    DynamicJsonBuffer jsonBuff(512);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                              String(description.c_str()), AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("sensorDescriptionHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  void timeSinceLastUpdateHandler(AsyncWebServerRequest *request) {
    int clientIDInt = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("AlpacaDeviceObservingConditions::timeSinceLastUpdateHandler - Method:", request->method());

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

    String sensorName;
    if (!tryGetStringParam(request, "SensorName", false, sensorName)) {
      sendInvalidParamResponse(request, clientIDInt, clientTransID, serverTransID, "timesincelastupdate", "SensorName");
      return;
    }
    std::string sensorNameStd = sensorName.c_str();
    double timeSince = GetTimeSinceLastUpdate(sensorNameStd);

    String message;
    DynamicJsonBuffer jsonBuff(256);
    JsonObject &root = jsonBuff.createObject();
    
    AlpacaResponseValueBuilder(root, clientIDInt, clientTransID, ++serverTransID,
                  timeSince, AlpacaError::Success, "");
    
    root.printTo(message);
    LOG_DEBUG("timeSinceLastUpdateHandler response:", message.c_str());

    request->send(200, "application/json", message);
  }

  // ==================== Common Device Methods ====================
  
  String GetDeviceDescription() override {
    return Description;
  }

  String GetDeviceDriverInfo() override {
    return "ASCOM Alpaca ObservingConditions Driver";
  }

  String GetDeviceDriverVersion() override {
    return "1.0";
  }

  String GetDeviceInterfaceVersion() override {
    return "1";
  }
};

#endif // ALPACA_DEVICE_OBSERVINGCONDITIONS_H
