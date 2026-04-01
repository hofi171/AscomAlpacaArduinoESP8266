#ifndef Aplaca_Device_h
#define Aplaca_Device_h

#include "UUID.h"
#include "DebugLog.h"
#include <EEPROM.h>

class AplacaDevice
{
private:
    String DeviceName;
    String DeviceType;
    int DeviceNumber;
    String UniqueID;
    bool HasSetup = false;
    
    // EEPROM storage for UniqueID
    // ArduinoStepper uses bytes 0-7, ArduinoFocuser uses bytes 8-15
    static const int EEPROM_UNIQUEID_ADDR = 16;  // Start address for UniqueID storage
    static const int EEPROM_UNIQUEID_LENGTH = 36; // Standard UUID length (xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx)


    String GenerateUniqueID()
    {
        GUID guid;
        guid.generate();
        return guid.toCharArray();
    }
    
    /**
     * @brief Load UniqueID from EEPROM
     * @return true if valid ID was loaded, false otherwise
     */
    bool LoadUniqueIDFromEEPROM()
    {
        char buffer[EEPROM_UNIQUEID_LENGTH + 1];
        
        // Read the UniqueID from EEPROM
        for (int i = 0; i < EEPROM_UNIQUEID_LENGTH; i++) {
            buffer[i] = EEPROM.read(EEPROM_UNIQUEID_ADDR + i);
        }
        buffer[EEPROM_UNIQUEID_LENGTH] = '\0'; // Null terminate
        
        String loadedID = String(buffer);
        
        // Validate the loaded ID (should be 36 characters and contain hyphens at positions 8, 13, 18, 23)
        if (loadedID.length() == 36 && 
            loadedID.charAt(8) == '-' && 
            loadedID.charAt(13) == '-' && 
            loadedID.charAt(18) == '-' && 
            loadedID.charAt(23) == '-') {
            
            // Additional validation: check if it contains valid hex characters
            bool valid = true;
            for (int i = 0; i < 36; i++) {
                char c = loadedID.charAt(i);
                if (i == 8 || i == 13 || i == 18 || i == 23) {
                    if (c != '-') {
                        valid = false;
                        break;
                    }
                } else {
                    if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F'))) {
                        valid = false;
                        break;
                    }
                }
            }
            
            if (valid) {
                UniqueID = loadedID;
                LOG_INFO("Loaded UniqueID from EEPROM: " + UniqueID);
                return true;
            }
        }
        
        LOG_WARN("Invalid UniqueID in EEPROM, will generate new one");
        return false;
    }
    
    /**
     * @brief Save UniqueID to EEPROM
     */
    void SaveUniqueIDToEEPROM()
    {
        // Write the UniqueID to EEPROM
        int idLength = (int)UniqueID.length();
        for (int i = 0; i < EEPROM_UNIQUEID_LENGTH && i < idLength; i++) {
            EEPROM.write(EEPROM_UNIQUEID_ADDR + i, UniqueID.charAt(i));
        }
        
        // Pad with zeros if UniqueID is shorter than expected
        for (int i = idLength; i < EEPROM_UNIQUEID_LENGTH; i++) {
            EEPROM.write(EEPROM_UNIQUEID_ADDR + i, 0);
        }
        
        EEPROM.commit(); // Commit changes to EEPROM (required for ESP8266)
        LOG_INFO("Saved UniqueID to EEPROM: " + UniqueID);
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
    virtual bool hasSetupHandler() {return HasSetup;  };
    virtual void setupHandler(AsyncWebServerRequest *request) = 0;

    //Constructor and getters
    AplacaDevice( String devicename, String devicetype, int devicenumber, AsyncWebServer &server, bool hasSetup=false) {
        DeviceName = devicename;
        DeviceType = devicetype;
        DeviceNumber = devicenumber;
        HasSetup = hasSetup;
        
        // Try to load UniqueID from EEPROM, generate new one if invalid
        if (!LoadUniqueIDFromEEPROM()) {
            UniqueID = GenerateUniqueID();
            SaveUniqueIDToEEPROM();
            LOG_INFO("Generated new UniqueID: " + UniqueID);
        }
        
        registerCommonDeviceHandlers(server);
    }
    ~AplacaDevice() {}

    const String& GetDeviceName() const { return DeviceName; }
    const String& GetDeviceType() const { return DeviceType; }
    int GetDeviceNumber() const { return DeviceNumber; }
    const String& GetUniqueID() const { return UniqueID; }


    
};

#endif /* Aplaca_Device_h */
