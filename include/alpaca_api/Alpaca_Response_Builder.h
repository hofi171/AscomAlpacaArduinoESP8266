#ifndef Alpaca_Response_Builder_h
#define Alpaca_Response_Builder_h

#include <ArduinoJson.h>
#include "DebugLog.h"

void AlpacaResponseBuilder( JsonObject&, int clientTransID, int transID, int serverTransID, String methodName, int errNum , String errMsg );
void AlpacaResponseValueBuilder( JsonObject&, int clientTransID, int transID, int serverTransID, String value, int errNum , String errMsg );
//JSON error structures used in ASCOM REST calls
//https://ascom-standards.org/api/?urls.primaryName=Remote%20Management%20API
//Responses, as described below, are returned in JSON format and always include a common set of values 
//including the client's transaction number, the server's transaction number together with any error message 
//and error number. If the transaction completes successfully, 
//the ErrorMessage field will be an empty string and the ErrorNumber field will be zero.
//https://github.com/ASCOMInitiative/ASCOMRemote/blob/master/Documentation/ASCOM%20Alpaca%20API%20Reference.pdf
//HTTP Status Codes and ASCOM Error codes
void AlpacaResponseBuilder( JsonObject& root, int clientID, int clientTransID, int serverTransID, String methodName, AlpacaError errNum , String errMsg )
{
//ClientTransactionIDForm  integer($int32) Client's transaction ID.
//ServerTransactionID integer($int32) Server's transaction ID.
//Method  string Name of the calling method.
//ErrorNumber integer($int32) Error number from device.
//ErrorMessage  string Error message description from device.
//DriverException {...}
   
    //root["Value"] = 0;
    //root["ClientID"]= clientID;
    root["ClientTransactionID"]= clientTransID;
    root["ServerTransactionID"]= serverTransID;
    //root["Method"] = methodName;
    root["ErrorNumber"] = static_cast<int>(errNum);
    root["ErrorMessage"] = errMsg;
    LOG_DEBUG("AlpacaResponseBuilder - Method: " + methodName + " ErrorNumber: " + String(static_cast<int>(errNum)) + " ErrorMessage: " + errMsg);
    LOG_DEBUG("AlpacaResponseBuilder - ClientID: " + String(clientID) + " ClientTransactionID: " + String(clientTransID) + " ServerTransactionID: " + String(serverTransID));
}


void AlpacaResponseValueBuilder( JsonObject& root, int clientID, int clientTransID, int serverTransID, String value, AlpacaError errNum , String errMsg )
{
//ClientTransactionIDForm  integer($int32) Client's transaction ID.
//ServerTransactionID integer($int32) Server's transaction ID.
//Value  string Value returned by the device.
//ErrorNumber integer($int32) Error number from device.
//ErrorMessage  string Error message description from device.
//DriverException {...}
   
    root["ClientTransactionID"]= clientTransID;
    root["ServerTransactionID"]= serverTransID;
    root["ErrorNumber"] = static_cast<int>(errNum);
    root["ErrorMessage"] = errMsg;
    root["Value"] = value;
    LOG_DEBUG("AlpacaResponseValueBuilder-String Value - Value: " + value + " ErrorNumber: " + String(static_cast<int>(errNum)) + " ErrorMessage: " + errMsg);
    LOG_DEBUG("AlpacaResponseValueBuilder-String Value - ClientID: " + String(clientID) + " ClientTransactionID: " + String(clientTransID) + " ServerTransactionID: " + String(serverTransID));
}

void AlpacaResponseValueBuilder( JsonObject& root, int clientID, int clientTransID, int serverTransID, int value, AlpacaError errNum , String errMsg )
{
//ClientTransactionIDForm  integer($int32) Client's transaction ID.
//ServerTransactionID integer($int32) Server's transaction ID.
//Value  integer($int32) Value returned by the device.
//ErrorNumber integer($int32) Error number from device.
//ErrorMessage  string Error message description from device.
//DriverException {...}
   
    root["ClientTransactionID"]= clientTransID;
    root["ServerTransactionID"]= serverTransID;
    root["ErrorNumber"] = static_cast<int>(errNum);
    root["ErrorMessage"] = errMsg;
    root["Value"] = value;
    LOG_DEBUG("AlpacaResponseValueBuilder-Integer Value - Value: " + String(value) + " ErrorNumber: " + String(static_cast<int>(errNum)) + " ErrorMessage: " + errMsg);
    LOG_DEBUG("AlpacaResponseValueBuilder-Integer Value - ClientID: " + String(clientID) + " ClientTransactionID: " + String(clientTransID) + " ServerTransactionID: " + String(serverTransID));
}

void AlpacaResponseValueBuilder( JsonObject& root, int clientID, int clientTransID, int serverTransID, double value, AlpacaError errNum , String errMsg )
{
//ClientTransactionIDForm  integer($int32) Client's transaction ID.
//ServerTransactionID integer($int32) Server's transaction ID.
//Value  double Value returned by the device.
//ErrorNumber integer($int32) Error number from device.
//ErrorMessage  string Error message description from device.
//DriverException {...}
   
    root["ClientTransactionID"]= clientTransID;
    root["ServerTransactionID"]= serverTransID;
    root["ErrorNumber"] = static_cast<int>(errNum);
    root["ErrorMessage"] = errMsg;
    root["Value"] = value;
    LOG_DEBUG("AlpacaResponseValueBuilder-Double Value - Value: " + String(value) + " ErrorNumber: " + String(static_cast<int>(errNum)) + " ErrorMessage: " + errMsg);
    LOG_DEBUG("AlpacaResponseValueBuilder-Double Value - ClientID: " + String(clientID) + " ClientTransactionID: " + String(clientTransID) + " ServerTransactionID: " + String(serverTransID));
}

void AlpacaResponseValueBuilder( JsonObject& root, int clientID, int clientTransID, int serverTransID, AlpacaError errNum , String errMsg )
{
//ClientTransactionIDForm  integer($int32) Client's transaction ID.
//ServerTransactionID integer($int32) Server's transaction ID.
//Value  integer($int32) Value returned by the device.
//ErrorNumber integer($int32) Error number from device.
//ErrorMessage  string Error message description from device.
//DriverException {...}
   
    root["ClientTransactionID"]= clientTransID;
    root["ServerTransactionID"]= serverTransID;
    root["ErrorNumber"] = static_cast<int>(errNum);
    root["ErrorMessage"] = errMsg;
    LOG_DEBUG("AlpacaResponseValueBuilder - ErrorNumber: " + String(static_cast<int>(errNum)) + " ErrorMessage: " + errMsg);
    LOG_DEBUG("AlpacaResponseValueBuilder - ClientID: " + String(clientID) + " ClientTransactionID: " + String(clientTransID) + " ServerTransactionID: " + String(serverTransID));
}

void AlpacaResponseValueBuilder( JsonObject& root, int clientID, int clientTransID, int serverTransID, bool value, AlpacaError errNum , String errMsg )
{
//ClientTransactionIDForm  integer($int32) Client's transaction ID.
//ServerTransactionID integer($int32) Server's transaction ID.
//Value  boolean Value returned by the device.
//ErrorNumber integer($int32) Error number from device.
//ErrorMessage  string Error message description from device.
//DriverException {...}
   
    root["ClientTransactionID"]= clientTransID;
    root["ServerTransactionID"]= serverTransID;
    root["ErrorNumber"] = static_cast<int>(errNum);
    root["ErrorMessage"] = errMsg;
    root["Value"] = value;
    LOG_DEBUG("AlpacaResponseValueBuilder-Boolean Value - Value: " + String(value ? "true" : "false") + " ErrorNumber: " + String(static_cast<int>(errNum)) + " ErrorMessage: " + errMsg);
    LOG_DEBUG("AlpacaResponseValueBuilder-Boolean Value - ClientID: " + String(clientID) + " ClientTransactionID: " + String(clientTransID) + " ServerTransactionID: " + String(serverTransID));
}

#endif /* Alpaca_Response_Builder_h */
