#ifndef Aplaca_Device_h
#define Aplaca_Device_h

#include "UUID.h"
#include "DebugLog.h"

class AplacaDevice
{
private:
    String DeviceName;
    String DeviceType;
    int DeviceNumber;
    String UniqueID;


    String GenerateUniqueID()
    {
        GUID guid;
        guid.generate();
        return guid.toCharArray();
    }

    void registerCommonDeviceHandlers(AsyncWebServer &server){  
        LOG_DEBUG("Registering common device handlers for DeviceName: " + DeviceName + " DeviceType: " + DeviceType + " DeviceNumber: " + String(DeviceNumber));
        LOG_DEBUG("Registering action handler at URL: " + (String("/api/v")+String(InterfaceVersion)+"/" + DeviceType + "/" + DeviceNumber + "/action"));
        server.on((String("/api/v")+String(InterfaceVersion)+"/" + DeviceType + "/" + DeviceNumber + "/action").c_str(), HTTP_PUT, [this](AsyncWebServerRequest *request){ this->actionHandler(request); });
        server.on((String("/api/v")+String(InterfaceVersion)+"/" + DeviceType + "/" + DeviceNumber + "/commandblind").c_str(), HTTP_PUT, [this](AsyncWebServerRequest *request){ this->commandblindHandler(request); });
        server.on((String("/api/v")+String(InterfaceVersion)+"/" + DeviceType + "/" + DeviceNumber + "/commandbool").c_str(), HTTP_PUT, [this](AsyncWebServerRequest *request){ this->commandboolHandler(request); });
        server.on((String("/api/v")+String(InterfaceVersion)+"/" + DeviceType + "/" + DeviceNumber + "/commandstring").c_str(), HTTP_PUT, [this](AsyncWebServerRequest *request){ this->commandstringHandler(request); });
        server.on((String("/api/v")+String(InterfaceVersion)+"/" + DeviceType + "/" + DeviceNumber + "/connect").c_str(), HTTP_PUT, [this](AsyncWebServerRequest *request){ this->connectHandler(request); });
        server.on((String("/api/v")+String(InterfaceVersion)+"/" + DeviceType + "/" + DeviceNumber + "/connected").c_str(), HTTP_GET, [this](AsyncWebServerRequest *request){ this->connectedHandler(request); });
        server.on((String("/api/v")+String(InterfaceVersion)+"/" + DeviceType + "/" + DeviceNumber + "/connected").c_str(), HTTP_PUT, [this](AsyncWebServerRequest *request){ this->connectedHandler(request); });
        server.on((String("/api/v")+String(InterfaceVersion)+"/" + DeviceType + "/" + DeviceNumber + "/connecting").c_str(), HTTP_GET, [this](AsyncWebServerRequest *request){ this->connectingHandler(request); });
        server.on((String("/api/v")+String(InterfaceVersion)+"/" + DeviceType + "/" + DeviceNumber + "/description").c_str(), HTTP_GET, [this](AsyncWebServerRequest *request){ this->descriptionHandler(request); });
        server.on((String("/api/v")+String(InterfaceVersion)+"/" + DeviceType + "/" + DeviceNumber + "/devicestate").c_str(), HTTP_GET, [this](AsyncWebServerRequest *request){ this->deviceStateHandler(request); });
        server.on((String("/api/v")+String(InterfaceVersion)+"/" + DeviceType + "/" + DeviceNumber + "/disconnect").c_str(), HTTP_PUT, [this](AsyncWebServerRequest *request){ this->disconnectHandler(request); });
        server.on((String("/api/v")+String(InterfaceVersion)+"/" + DeviceType + "/" + DeviceNumber + "/driverinfo").c_str(), HTTP_GET, [this](AsyncWebServerRequest *request){ this->driverInfoHandler(request); });
        server.on((String("/api/v")+String(InterfaceVersion)+"/" + DeviceType + "/" + DeviceNumber + "/driverversion").c_str(), HTTP_GET, [this](AsyncWebServerRequest *request){ this->driverVersionHandler(request); });
        server.on((String("/api/v")+String(InterfaceVersion)+"/" + DeviceType + "/" + DeviceNumber + "/interfaceversion").c_str(), HTTP_GET, [this](AsyncWebServerRequest *request){ this->interfaceVersionHandler(request); });
        server.on((String("/api/v")+String(InterfaceVersion)+"/" + DeviceType + "/" + DeviceNumber + "/name").c_str(), HTTP_GET, [this](AsyncWebServerRequest *request){ this->nameHandler(request); });
        server.on((String("/api/v")+String(InterfaceVersion)+"/" + DeviceType + "/" + DeviceNumber + "/supportedactions").c_str(), HTTP_GET, [this](AsyncWebServerRequest *request){ this->supportedActionsHandler(request); });
        LOG_DEBUG("registerCommonDeviceHandlers Done!");
    }

public:
    //Interface
    virtual void registerHandlers(AsyncWebServer &server)=0;

    virtual void actionHandler(AsyncWebServerRequest *request) = 0;
    virtual void commandblindHandler(AsyncWebServerRequest *request) = 0;
    virtual void commandboolHandler(AsyncWebServerRequest *request) = 0;
    virtual void commandstringHandler(AsyncWebServerRequest *request) = 0;
    virtual void connectHandler(AsyncWebServerRequest *request) = 0;
    virtual void connectedHandler(AsyncWebServerRequest *request) = 0;
    virtual void connectingHandler(AsyncWebServerRequest *request) = 0;
    virtual void descriptionHandler(AsyncWebServerRequest *request) = 0;
    virtual void deviceStateHandler(AsyncWebServerRequest *request) = 0;
    virtual void disconnectHandler(AsyncWebServerRequest *request) = 0;
    virtual void driverInfoHandler(AsyncWebServerRequest *request) = 0;
    virtual void driverVersionHandler(AsyncWebServerRequest *request) = 0;
    virtual void interfaceVersionHandler(AsyncWebServerRequest *request) = 0;
    virtual void nameHandler(AsyncWebServerRequest *request) = 0;
    virtual void supportedActionsHandler(AsyncWebServerRequest *request) = 0;

    //Constructor and getters
    AplacaDevice( String devicename, String devicetype, int devicenumber, AsyncWebServer &server ) {
        DeviceName = devicename;
        DeviceType = devicetype;
        DeviceNumber = devicenumber;
        UniqueID = GenerateUniqueID();
        registerCommonDeviceHandlers(server);
    }
    ~AplacaDevice() {}

    const String& GetDeviceName() const { return DeviceName; }
    const String& GetDeviceType() const { return DeviceType; }
    int GetDeviceNumber() const { return DeviceNumber; }
    const String& GetUniqueID() const { return UniqueID; }


    
};

#endif /* Aplaca_Device_h */
