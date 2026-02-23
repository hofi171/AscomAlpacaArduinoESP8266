#ifndef ALPACA_REQUEST_HELPER_H
#define ALPACA_REQUEST_HELPER_H

#include <ESPAsyncWebServer.h>
#include "Alpaca_Errors.h"
#include "Alpaca_Response_Builder.h"

/**
 * @file Alpaca_Request_Helper.h
 * @brief Helper functions for extracting ClientID and ClientTransactionID from requests
 *
 * This file provides utility functions to extract and validate ClientID and ClientTransactionID
 * from ASCOM Alpaca API requests. These parameters are required by the Alpaca protocol.
 *
 * Functions:
 * - extractClientID: Extract ClientID from GET (query params) or PUT (form body) requests
 * - extractClientTransactionID: Extract ClientTransactionID from GET or PUT requests
 */

/**
 * @brief Extract ClientID from request parameters
 * @param request The AsyncWebServerRequest object
 * @param fromBody If true, extract from form body (PUT); if false, from query params (GET)
 * @param clientID Reference to store the extracted client ID
 * @return true if valid (>= 0), false if invalid (< 0)
 *
 * ClientID is optional per the Alpaca spec. If not present, defaults to 0.
 * Returns false if the parsed value is negative.
 */
inline bool extractClientID(AsyncWebServerRequest *request, bool fromBody, int &clientID)
{
  clientID = 0;
  if (request->hasParam("ClientID", fromBody))
  {
    String value = request->getParam("ClientID", fromBody)->value();
    if (value.length() == 0)
    {
      return false;
    }
    for (size_t i = 0; i < value.length(); ++i)
    {
      if (!isDigit(value[i]))
      {
        return false;
      }
    }
    clientID = atoi(value.c_str());
    if (clientID < 0)
    {
      return false;
    }
  }
  else if (request->hasParam("clientid", fromBody))
  {
    String value = request->getParam("clientid", fromBody)->value();
    if (value.length() == 0)
    {
      return false;
    }
    for (size_t i = 0; i < value.length(); ++i)
    {
      if (!isDigit(value[i]))
      {
        return false;
      }
    }
    clientID = atoi(value.c_str());
    if (clientID < 0)
    {
      return false;
    }
  }
  return true;
}

/**
 * @brief Extract ClientTransactionID from request parameters
 * @param request The AsyncWebServerRequest object
 * @param fromBody If true, extract from form body (PUT); if false, from query params (GET)
 * @param clientTransactionID Reference to store the extracted client transaction ID
 * @return true if valid (>= 0), false if invalid (< 0)
 *
 * ClientTransactionID is optional per the Alpaca spec. If not present, defaults to 0.
 * Returns false if the parsed value is negative.
 */
inline bool extractClientTransactionID(AsyncWebServerRequest *request, bool fromBody, int &clientTransactionID)
{
  clientTransactionID = 0;
  if (request->hasParam("ClientTransactionID", fromBody))
  {
    String value = request->getParam("ClientTransactionID", fromBody)->value();
    if (value.length() == 0)
    {
      return false;
    }
    for (size_t i = 0; i < value.length(); ++i)
    {
      if (!isDigit(value[i]))
      {
        return false;
      }
    }
    clientTransactionID = atoi(value.c_str());
    if (clientTransactionID < 0)
    {
      return false;
    }
  }
  else if (request->hasParam("clienttransactionid", fromBody))
  {
    if (!fromBody)
    {
      String value = request->getParam("clienttransactionid", fromBody)->value();
      if (value.length() == 0)
      {
        return false;
      }
      for (size_t i = 0; i < value.length(); ++i)
      {
        if (!isDigit(value[i]))
        {
          return false;
        }
      }
      clientTransactionID = atoi(value.c_str());
      if (clientTransactionID < 0)
      {
        return false;
      }
    }
    else
    {
      clientTransactionID = 0;
      return true;
    }
  }

  return true;
}

/**
 * @brief Extract both ClientID and ClientTransactionID from request parameters
 * @param request The AsyncWebServerRequest object
 * @param fromBody If true, extract from form body (PUT); if false, from query params (GET)
 * @param clientID Reference to store the extracted client ID
 * @param clientTransactionID Reference to store the extracted client transaction ID
 * @return true if both IDs are valid (>= 0), false if either is invalid (< 0)
 *
 * Convenience function to extract both parameters in a single call.
 * Returns false if either ClientID or ClientTransactionID is negative.
 */
inline bool extractClientIDAndTransactionID(AsyncWebServerRequest *request, bool fromBody,
                                            int &clientID, int &clientTransactionID)
{
  bool clientIDValid = extractClientID(request, fromBody, clientID);
  bool transIDValid = extractClientTransactionID(request, fromBody, clientTransactionID);
  return clientIDValid && transIDValid;
}

/**
 * @brief Check if a string is numeric (digits only)
 * @param value The string to validate
 * @return true if value is non-empty and all digits
 */
inline bool isNumericValue(const String &value)
{
  if (value.length() == 0)
  {
    return false;
  }
  for (size_t i = 0; i < value.length(); ++i)
  {
    if (!isDigit(value[i]))
    {
      return false;
    }
  }
  return true;
}

inline bool isIntegerValue(const String &value, bool allowSign)
{
  if (value.length() == 0)
  {
    return false;
  }
  size_t start = 0;
  if (allowSign && (value[0] == '-' || value[0] == '+'))
  {
    start = 1;
  }
  if (start >= value.length())
  {
    return false;
  }
  for (size_t i = start; i < value.length(); ++i)
  {
    if (!isDigit(value[i]))
    {
      return false;
    }
  }
  return true;
}

inline bool isDecimalValue(const String &value, bool allowSign)
{
  if (value.length() == 0)
  {
    return false;
  }
  size_t start = 0;
  if (allowSign && (value[0] == '-' || value[0] == '+'))
  {
    start = 1;
  }
  if (start >= value.length())
  {
    return false;
  }
  bool sawDigit = false;
  bool sawDot = false;
  for (size_t i = start; i < value.length(); ++i)
  {
    char c = value[i];
    if (isDigit(c))
    {
      sawDigit = true;
      continue;
    }
    if (c == '.' && !sawDot)
    {
      sawDot = true;
      continue;
    }
    return false;
  }
  return sawDigit;
}

inline bool tryGetStringParam(AsyncWebServerRequest *request, const String &name, bool fromBody, String &value)
{
  if (!request->hasParam(name, fromBody))
  {
    return false;
  }
  value = request->getParam(name, fromBody)->value();
  return value.length() > 0;
}

inline bool tryGetOptionalStringParam(AsyncWebServerRequest *request, const String &name, bool fromBody, String &value)
{
  if (!request->hasParam(name, fromBody))
  {
    return false;
  }
  value = request->getParam(name, fromBody)->value();
  return true;
}

inline bool tryGetIntParam(AsyncWebServerRequest *request, const String &name, bool fromBody, int &value, bool allowSign = false)
{
  String raw;
  if (!tryGetStringParam(request, name, fromBody, raw))
  {
    return false;
  }
  if (!isIntegerValue(raw, allowSign))
  {
    return false;
  }
  value = raw.toInt();
  return true;
}

inline bool tryGetIntParamAlt(AsyncWebServerRequest *request, const String &name, const String &altName,
                              bool fromBody, int &value, bool allowSign = false)
{
  if (tryGetIntParam(request, name, fromBody, value, allowSign))
  {
    return true;
  }
  return tryGetIntParam(request, altName, fromBody, value, allowSign);
}

inline bool tryGetDoubleParam(AsyncWebServerRequest *request, const String &name, bool fromBody, double &value, bool allowSign = true)
{
  String raw;
  if (!tryGetStringParam(request, name, fromBody, raw))
  {
    return false;
  }
  if (!isDecimalValue(raw, allowSign))
  {
    return false;
  }
  value = atof(raw.c_str());
  return true;
}

inline bool tryGetBoolParam(AsyncWebServerRequest *request, const String &name, bool fromBody, bool &value)
{
  String raw;
  if (!tryGetStringParam(request, name, fromBody, raw))
  {
    return false;
  }
  raw.toLowerCase();
  if (raw == "true" || raw == "1")
  {
    value = true;
    return true;
  }
  if (raw == "false" || raw == "0")
  {
    value = false;
    return true;
  }
  return false;
}

inline void sendInvalidParamResponse(AsyncWebServerRequest *request, int clientID, int clientTransID,
                                     uint32_t &serverTransID, const String &methodName, const String &paramName)
{
  String message;
  DynamicJsonBuffer jsonBuff(256);
  JsonObject &root = jsonBuff.createObject();
  AlpacaResponseBuilder(root, clientID, clientTransID, ++serverTransID,
                        methodName, AlpacaError::InvalidValue,
                        "Missing or invalid required parameter: " + paramName);
  root.printTo(message);
  request->send(400, "application/json", message);
}

/**
 * @brief Find the first invalid ClientID/ClientTransactionID value
 * @param request The AsyncWebServerRequest object
 * @param fromBody If true, extract from form body (PUT); if false, from query params (GET)
 * @return The invalid value or "unknown" if none found
 */
inline String findInvalidClientIdValue(AsyncWebServerRequest *request, bool fromBody)
{
  if (request->hasParam("ClientID", fromBody))
  {
    String value = request->getParam("ClientID", fromBody)->value();
    if (!isNumericValue(value))
    {
      return value;
    }
  }
  if (request->hasParam("ClientTransactionID", fromBody))
  {
    String value = request->getParam("ClientTransactionID", fromBody)->value();
    if (!isNumericValue(value))
    {
      return value;
    }
  }
  if (request->hasParam("clienttransactionid", fromBody))
  {
    String value = request->getParam("clienttransactionid", fromBody)->value();
    if (!isNumericValue(value))
    {
      return value;
    }
  }
  return "unknown";
}

#endif /* ALPACA_REQUEST_HELPER_H */
