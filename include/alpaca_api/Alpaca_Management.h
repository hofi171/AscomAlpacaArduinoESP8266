#ifndef Alpaca_Management_h
#define Alpaca_Management_h

#ifndef MAX_NAME_LENGTH 
#define MAX_NAME_LENGTH 25
#endif 

#include "Alpaca_Errors.h"
#include "Alpaca_Driver_Settings.h"
#include "Alpaca_Response_Builder.h"
#include "Alpaca_Request_Helper.h"
#include "DebugLog.h"
#include "Aplaca_Device.h"
#include <vector>


class AlpacaManagement
{
    private:
std::vector<AplacaDevice*> RegisteredDevices; 

public:
void handleMgmtAPIversions(AsyncWebServerRequest *request)
{
    int clientID = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("handleMgmtAPIversions request - Method: " + String(request->method()) + " URL: " + request->url());
    
    if (!extractClientIDAndTransactionID(request, false, clientID, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientID, clientTransID, 0,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }
    
    String message;
 
    DynamicJsonBuffer jsonBuff(256);
    JsonObject& root = jsonBuff.createObject();
    AlpacaResponseBuilder( root, clientID, clientTransID, 1, "APIversions", AlpacaError::Success, "" );  
    //Expect to return an array of integer values.
	JsonArray& values  = root.createNestedArray("Value");     
    values.add( atoi( String(InterfaceVersion).c_str() ));  //Do this because InterfaceVersion is stored in PROGMEM
    root.printTo(message);
	LOG_DEBUG("ManagementVersions response : ", message.c_str() );    
    
    request->send(200, "application/json", message);
}

void handleMgmtConfiguredDevices(AsyncWebServerRequest *request)
{
    int clientID = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("handleMgmtConfiguredDevices request - Method: " + String(request->method()) + " URL: " + request->url());
    
    if (!extractClientIDAndTransactionID(request, false, clientID, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientID, clientTransID, 0,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }
    
    request->send(200, "application/json", BuildMgmtConfiguredDevices(clientID, clientTransID, 0) );
}

void handleMgmtDescription(AsyncWebServerRequest *request)
{
    int clientID = 0;
    int clientTransID = 0;
    
    LOG_DEBUG("handleMgmtDescription request - Method: " + String(request->method()) + " URL: " + request->url());
    
    if (!extractClientIDAndTransactionID(request, false, clientID, clientTransID)) {
      String message;
      DynamicJsonBuffer jsonBuff(256);
      JsonObject &root = jsonBuff.createObject();
      AlpacaResponseBuilder(root, clientID, clientTransID, 0,
                           "invalid_parameters", AlpacaError::InvalidValue, "Invalid ClientID or ClientTransactionID");
      root.printTo(message);
      request->send(400, "application/json", message);
      return;
    }
    
    request->send(200, "application/json", BuildMgmtDescription(clientID, clientTransID, 0) );
}

void registerDevice( String devicename, String devicetype, int devicenumber, AplacaDevice* device )
{
    LOG_DEBUG("registerDevice request - DeviceName: " + devicename + " DeviceType: " + devicetype + " DeviceNumber: " + String(devicenumber));
    RegisteredDevices.push_back( device );
}

void unregisterDevice( String devicename, String devicetype, int devicenumber )
{
    LOG_DEBUG("unregisterDevice request - DeviceName: " + devicename + " DeviceType: " + devicetype + " DeviceNumber: " + String(devicenumber));
    RegisteredDevices.erase( std::remove_if( RegisteredDevices.begin(), RegisteredDevices.end(), 
        [devicename, devicetype, devicenumber](const AplacaDevice* device) {
            return device->GetDeviceName() == devicename && device->GetDeviceType() == devicetype && device->GetDeviceNumber() == devicenumber;
        }), RegisteredDevices.end() );
}

String BuildMgmtDescription(int clientID, int clientTransID, uint32_t serverTransID)
{
    String message;
 
    DynamicJsonBuffer jsonBuff(256);
    JsonObject& root = jsonBuff.createObject();
    AlpacaResponseBuilder( root, clientID, clientTransID, ++serverTransID, "MgmtDescription", AlpacaError::Success, "" );  

	JsonObject& object = root.createNestedObject("Value"); 
    object["ServerName"]   = WiFi.hostname();
	object["Manufacturer"] = Manufacturer;
	object["ManufacturerVersion"] = instanceVersion;
	object["Location"] = Location; 
	
	root.printTo( message);
	LOG_DEBUG("ManagementDescription response : ", message.c_str() );

    return message;
}

String BuildMgmtVersions(int clientID, int clientTransID, uint32_t serverTransID)
{
    String message;
 
    DynamicJsonBuffer jsonBuff(256);
    JsonObject& root = jsonBuff.createObject();
    AlpacaResponseBuilder( root, clientID, clientTransID, ++serverTransID, "APIversions", AlpacaError::Success, "" );  
    //Expect to return an array of integer values.
	JsonArray& values  = root.createNestedArray("Value");     
    values.add( atoi( String(InterfaceVersion).c_str() ));  //Do this because InterfaceVersion is stored in PROGMEM
    root.printTo(message);
	LOG_DEBUG("ManagementVersions response : ", message.c_str() );    
    
    return message;
}

String BuildMgmtConfiguredDevices(int clientID, int clientTransID, uint32_t serverTransID)
{
	String message;
    //Dont care about client IDs for read-only data 

    // Increase buffer size: base(200) + per device(150) * max devices
    size_t bufferSize = 200 + (RegisteredDevices.size() * 150);
    DynamicJsonBuffer jsonBuff(bufferSize);
    JsonObject& root = jsonBuff.createObject();
    AlpacaResponseBuilder( root, clientID, clientTransID, ++serverTransID, "ConfiguredDevices", AlpacaError::Success, "" );  
    JsonArray& values  = root.createNestedArray("Value");
    

    for (const auto& registeredDevice : RegisteredDevices)
    {
        // Null pointer check
        if (registeredDevice == nullptr) {
            LOG_ERROR("Null device pointer in RegisteredDevices");
            continue;
        }
        
        // Create nested object within the SAME buffer
        JsonObject& device = values.createNestedObject();
        device["DeviceName"] = registeredDevice->GetDeviceName();
        device["DeviceType"] = registeredDevice->GetDeviceType();
        device["DeviceNumber"] = registeredDevice->GetDeviceNumber();
        device["UniqueID"] = registeredDevice->GetUniqueID();    
    }

    root.printTo(message);
	LOG_DEBUG( "ManagementConfiguredDevices response : ", message.c_str() );    
    
    return message;
}


public:
    AlpacaManagement() {}
    ~AlpacaManagement() {}

    void registerManagementHandlers(AsyncWebServer &server)
    {
        LOG_DEBUG("Registering management API handlers");
        server.on("/management/apiversions", HTTP_GET, [this](AsyncWebServerRequest *request){ this->handleMgmtAPIversions(request); });
        server.on("/management/v1/configureddevices", HTTP_GET, [this](AsyncWebServerRequest *request){ this->handleMgmtConfiguredDevices(request); });
        server.on("/management/v1/description", HTTP_GET, [this](AsyncWebServerRequest *request){ this->handleMgmtDescription(request); });    
    }



};

#endif /* Alpaca_Management_h */
