#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <Arduino.h>
#include <EEPROM.h>
#include "DebugLog.h"

/**
 * @brief WiFi Configuration Manager
 * 
 * Manages WiFi credentials in EEPROM with load/save functionality
 * EEPROM Memory Layout:
 * - Bytes 52-83: WiFi SSID (32 bytes)
 * - Bytes 84-146: WiFi Password (63 bytes)
 * - Byte 147:148: Validation marker (0xAA55)
 */
class WiFiConfig {
private:
    static const int EEPROM_WIFI_SSID_ADDR = 52;        // Start address for SSID
    static const int EEPROM_WIFI_SSID_LENGTH = 32;      // Max SSID length
    static const int EEPROM_WIFI_PASSWORD_ADDR = 84;    // Start address for password
    static const int EEPROM_WIFI_PASSWORD_LENGTH = 63;  // Max password length
    static const int EEPROM_WIFI_VALID_ADDR = 147;      // Validation marker address
    static const uint16_t EEPROM_WIFI_VALID_MARKER = 0xAA55; // Magic number to indicate valid config
    
    String ssid;
    String password;
    bool hasValidConfig;
    
public:
    WiFiConfig() : hasValidConfig(false) {}
    
    /**
     * @brief Load WiFi credentials from EEPROM
     * @return true if valid credentials were loaded, false otherwise
     */
    bool loadFromEEPROM() {
        // Check validation marker
        uint16_t marker = (EEPROM.read(EEPROM_WIFI_VALID_ADDR) << 8) | EEPROM.read(EEPROM_WIFI_VALID_ADDR + 1);
        
        if (marker != EEPROM_WIFI_VALID_MARKER) {
            LOG_WARN("No valid WiFi configuration found in EEPROM");
            hasValidConfig = false;
            return false;
        }
        
        // Read SSID
        char ssidBuffer[EEPROM_WIFI_SSID_LENGTH + 1];
        for (int i = 0; i < EEPROM_WIFI_SSID_LENGTH; i++) {
            ssidBuffer[i] = EEPROM.read(EEPROM_WIFI_SSID_ADDR + i);
        }
        ssidBuffer[EEPROM_WIFI_SSID_LENGTH] = '\0';
        ssid = String(ssidBuffer);
        ssid.trim(); // Remove any padding
        
        // Read Password
        char passwordBuffer[EEPROM_WIFI_PASSWORD_LENGTH + 1];
        for (int i = 0; i < EEPROM_WIFI_PASSWORD_LENGTH; i++) {
            passwordBuffer[i] = EEPROM.read(EEPROM_WIFI_PASSWORD_ADDR + i);
        }
        passwordBuffer[EEPROM_WIFI_PASSWORD_LENGTH] = '\0';
        password = String(passwordBuffer);
        password.trim(); // Remove any padding
        
        // Validate that SSID is not empty
        if (ssid.length() == 0) {
            LOG_WARN("SSID is empty in EEPROM");
            hasValidConfig = false;
            return false;
        }
        
        LOG_INFO("Loaded WiFi configuration from EEPROM - SSID: " + ssid);
        hasValidConfig = true;
        return true;
    }
    
    /**
     * @brief Save WiFi credentials to EEPROM
     * @param newSSID WiFi network name
     * @param newPassword WiFi password
     * @return true if saved successfully
     */
    bool saveToEEPROM(const String& newSSID, const String& newPassword) {
        if (newSSID.length() == 0) {
            LOG_ERROR("Cannot save empty SSID");
            return false;
        }
        
        if (newSSID.length() > EEPROM_WIFI_SSID_LENGTH) {
            LOG_ERROR("SSID too long (max 32 characters)");
            return false;
        }
        
        if (newPassword.length() > EEPROM_WIFI_PASSWORD_LENGTH) {
            LOG_ERROR("Password too long (max 63 characters)");
            return false;
        }
        
        // Write SSID
        int i;
        int ssidLength = static_cast<int>(newSSID.length());
        for (i = 0; i < ssidLength && i < EEPROM_WIFI_SSID_LENGTH; i++) {
            EEPROM.write(EEPROM_WIFI_SSID_ADDR + i, newSSID.charAt(i));
        }
        // Pad with zeros
        for (; i < EEPROM_WIFI_SSID_LENGTH; i++) {
            EEPROM.write(EEPROM_WIFI_SSID_ADDR + i, 0);
        }
        
        // Write Password
        int passwordLength = static_cast<int>(newPassword.length());
        for (i = 0; i < passwordLength && i < EEPROM_WIFI_PASSWORD_LENGTH; i++) {
            EEPROM.write(EEPROM_WIFI_PASSWORD_ADDR + i, newPassword.charAt(i));
        }
        // Pad with zeros
        for (; i < EEPROM_WIFI_PASSWORD_LENGTH; i++) {
            EEPROM.write(EEPROM_WIFI_PASSWORD_ADDR + i, 0);
        }
        
        // Write validation marker
        EEPROM.write(EEPROM_WIFI_VALID_ADDR, (EEPROM_WIFI_VALID_MARKER >> 8) & 0xFF);
        EEPROM.write(EEPROM_WIFI_VALID_ADDR + 1, EEPROM_WIFI_VALID_MARKER & 0xFF);
        
        EEPROM.commit();
        
        // Update internal state
        ssid = newSSID;
        password = newPassword;
        hasValidConfig = true;
        
        LOG_INFO("Saved WiFi configuration to EEPROM - SSID: " + ssid);
        return true;
    }
    
    /**
     * @brief Get the stored SSID
     * @return SSID string
     */
    String getSSID() const {
        return ssid;
    }
    
    /**
     * @brief Get the stored password
     * @return Password string
     */
    String getPassword() const {
        return password;
    }
    
    /**
     * @brief Check if valid configuration exists
     * @return true if valid config is loaded
     */
    bool hasConfig() const {
        return hasValidConfig;
    }
    
    /**
     * @brief Clear WiFi configuration from EEPROM
     */
    void clearConfig() {
        // Clear validation marker
        EEPROM.write(EEPROM_WIFI_VALID_ADDR, 0);
        EEPROM.write(EEPROM_WIFI_VALID_ADDR + 1, 0);
        EEPROM.commit();
        
        ssid = "";
        password = "";
        hasValidConfig = false;
        
        LOG_INFO("Cleared WiFi configuration from EEPROM");
    }
};

#endif // WIFI_CONFIG_H
