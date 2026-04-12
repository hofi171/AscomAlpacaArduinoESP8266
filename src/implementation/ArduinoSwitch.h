#ifndef ARDUINO_SWITCH_H
#define ARDUINO_SWITCH_H

#include "alpaca_api/Alpaca_Device_Switch.h"
#include "DewHeater.h"
#include "WiFi_Config.h"
#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <string>

// Forward declaration for accessing global WiFiConfig
extern WiFiConfig wifiConfig;

/**
 * @file ArduinoSwitch.h
 * @brief Example implementation of Switch device
 * 
 * This example demonstrates how to create a concrete Switch device
 * by extending the AlpacaDeviceSwitch class and implementing the
 * switch control logic for managing multiple switches.
 */

// Structure to define a single switch
enum class SwitchType : uint8_t {
  Default = 0,
  DewHeater = 1,
  DHT22Temp = 2,
  DHT22Humidity = 3,
  DS18B20Temp = 4,
  DHT22DewPoint = 5,
  DewHeaterOutputPct = 6
};

enum class DewHeaterMode : uint8_t {
  Automatic = 0,
  Manual = 1
};

enum class DewHeaterTempSensorType : uint8_t {
  DS18B20 = 0,
  NTCThermistor = 1
};

struct SwitchData {
  std::string name;
  std::string description;
  SwitchType type;
  DewHeaterMode dewHeaterMode;
  bool canWrite;
  bool canAsync;
  bool enabled;
  double value;
  double minValue;
  double maxValue;
  double stepValue;
  double dewTargetOffsetC;
  double dewTargetManualC;
  double heaterTempOffsetC;
  double dhtTempOffsetC;
  DewHeaterTempSensorType heaterTempSensorType;
  double ntcSeriesResistorOhm;
  double ntcNominalResistanceOhm;
  double ntcNominalTemperatureC;
  double ntcBeta;
  int outputPin;             // GPIO pin for output (-1 if not used)
  int tempInputPin;          // Secondary GPIO input pin for dew heater temperature (-1 if not used)
  int heaterTempPin;         // DS18B20 heater temperature pin (-1 if not used)
  bool isPWM;                // True if PWM output, false if digital
  bool stateChangeComplete;  // For async operations
};

// Packed struct stored in EEPROM — size handled by sizeof(SwitchEEPROMData)
// EEPROM layout: [149-150] magic marker | [151..] 8 x SwitchEEPROMData
struct SwitchEEPROMData {
  char    name[16];         // Switch name (null-terminated, 15 usable chars)
  char    description[21];  // Description  (null-terminated, 20 usable chars)
  uint8_t outputPin;        // GPIO pin; 0xFF means -1 (not connected)
  uint8_t tempInputPin;     // Temperature input GPIO pin for dew heater; 0xFF means -1
  uint8_t heaterTempPin;    // DS18B20 heater temperature GPIO pin; 0xFF means -1
  uint8_t flags;            // bit0=canWrite, bit1=canAsync, bit2=isPWM, bit3-5=switchType, bit6=dewHeaterMode, bit7=enabled
  float   value;            // Current / last value
  float   minValue;
  float   maxValue;
  float   stepValue;
  float   dewTargetOffsetC;
  float   dewTargetManualC;
  float   heaterTempOffsetC;
  float   dhtTempOffsetC;
  uint8_t heaterTempSensorType; // 0=DS18B20, 1=NTC thermistor
  float   ntcSeriesResistorOhm;
  float   ntcNominalResistanceOhm;
  float   ntcNominalTemperatureC;
  float   ntcBeta;
};

class ArduinoSwitch : public AlpacaDeviceSwitch {
private:
  int maxSwitch;                    // Number of switches
  SwitchData* switches;             // Array of switch data
  DewHeater** dewHeaters;           // Optional helper per switch for sensor-backed modes
  uint32_t pendingRecreateMask;     // Bitmask of switches requiring DewHeater recreation
  bool pendingSaveAllEeprom;        // Save request deferred from HTTP handler to loop context
  bool ledDisabled;                 // True when onboard LED should be disabled (GPIO2 HIGH)

  // EEPROM address map (bytes 0-148 are used by AplacaDevice / WiFiConfig)
  static const int      EEPROM_SW_MAGIC_ADDR = 149;
  static const int      EEPROM_SW_DATA_ADDR  = 151;
  static const uint16_t EEPROM_SW_MAGIC_VAL  = 0xA5CD;
  static const int      EEPROM_SW_MAX        = 11;    // max switches stored
  static const int      EEPROM_SW_ENTRY_SIZE = sizeof(SwitchEEPROMData);

  bool usesDewHeaterHelper(SwitchType type) const {
    return type == SwitchType::DewHeater ||
           type == SwitchType::DHT22Temp ||
           type == SwitchType::DHT22Humidity ||
           type == SwitchType::DS18B20Temp ||
           type == SwitchType::DHT22DewPoint ||
           type == SwitchType::DewHeaterOutputPct;
  }

  int getDhtPinForSwitch(int id) const {
    if (!isValidSwitchId(id)) return -1;
    SwitchType type = switches[id].type;
    if (type == SwitchType::DewHeater ||
        type == SwitchType::DHT22Temp ||
        type == SwitchType::DHT22Humidity ||
        type == SwitchType::DHT22DewPoint) {
      return switches[id].tempInputPin;
    }
    return -1;
  }

  int getDs18b20PinForSwitch(int id) const {
    if (!isValidSwitchId(id)) return -1;
    if (switches[id].heaterTempPin >= 0) return switches[id].heaterTempPin;
    if (switches[id].type == SwitchType::DS18B20Temp) return switches[id].tempInputPin;
    return -1;
  }

  int getAmbientSourceSwitchIndex() const {
    if (isValidSwitchId(2) && switches[2].tempInputPin >= 0) return 2;
    if (isValidSwitchId(4) && switches[4].type == SwitchType::DewHeater && switches[4].tempInputPin >= 0) return 4;
    if (isValidSwitchId(5) && switches[5].type == SwitchType::DewHeater && switches[5].tempInputPin >= 0) return 5;
    return -1;
  }

  void recreateDewHeater(int id) {
    if (!isValidSwitchId(id)) return;

    // DHT22 roles are tied to switch 2 sensor reader.
    // Switch 3/8 (humidity/dew point), 6/7 (heater temp) and 9/10 (heater output %)
    // consume values from other source switches.
    if (id == 3 || id == 6 || id == 7 || id == 8 || id == 9 || id == 10) {
      if (dewHeaters[id] != nullptr) {
        delete dewHeaters[id];
        dewHeaters[id] = nullptr;
      }
      return;
    }

    if (dewHeaters[id] != nullptr) {
      delete dewHeaters[id];
      dewHeaters[id] = nullptr;
    }

    if (!usesDewHeaterHelper(switches[id].type)) {
      return;
    }

    int heaterPin = (switches[id].type == SwitchType::DewHeater) ? switches[id].outputPin : -1;
    int dhtPin = getDhtPinForSwitch(id);
    bool useNtc = (switches[id].heaterTempSensorType == DewHeaterTempSensorType::NTCThermistor);
    uint8_t heaterTempSensorType = useNtc ? 1 : 0;
    // For NTC, the ADC input is always A0 on ESP8266; for DS18B20 use the configured GPIO pin
    int heaterTempPin = useNtc ? A0 : getDs18b20PinForSwitch(id);
    LOG_DEBUG("recreateDewHeater id=" + String(id) +
              " heaterPin=" + String(heaterPin) +
              " dhtPin=" + String(dhtPin) +
              " heaterTempPin=" + String(heaterTempPin) +
              " sensorType=" + String(heaterTempSensorType));

    dewHeaters[id] = new DewHeater(heaterPin, dhtPin, 2000UL, heaterTempPin, heaterTempSensorType);
    dewHeaters[id]->setNtcParameters((float)switches[id].ntcSeriesResistorOhm,
                     (float)switches[id].ntcNominalResistanceOhm,
                     (float)switches[id].ntcNominalTemperatureC,
                     (float)switches[id].ntcBeta);
    dewHeaters[id]->begin();
    dewHeaters[id]->setHeaterTemperatureOffsetC((float)switches[id].heaterTempOffsetC);

    if (switches[id].type == SwitchType::DewHeater) {
      dewHeaters[id]->enablePidControl(true);
      if (switches[id].dewHeaterMode == DewHeaterMode::Manual) {
        dewHeaters[id]->setTargetHeaterTemperatureC((float)switches[id].dewTargetManualC);
      } else {
        dewHeaters[id]->setTargetAboveDewPointC((float)switches[id].dewTargetOffsetC);
      }
    }
  }

  void refreshSwitchSensorState(int id) {
    if (!isValidSwitchId(id) || !usesDewHeaterHelper(switches[id].type)) return;

    // Switch 3 (humidity) and switch 8 (dew point) share the ambient DHT22 reader
    if (id == 3 || id == 8) {
      int ambientSource = getAmbientSourceSwitchIndex();
      if (!isValidSwitchId(ambientSource) || !usesDewHeaterHelper(switches[ambientSource].type)) {
        return;
      }
      if (dewHeaters[ambientSource] == nullptr) {
        recreateDewHeater(ambientSource);
      }
      if (dewHeaters[ambientSource] != nullptr) {
        DewHeater *sharedHeater = dewHeaters[ambientSource];
        sharedHeater->update();
        if (id == 3 && sharedHeater->isSensorValid()) {
          switches[id].value = sharedHeater->getHumidityPercent();
        }
        if (id == 8 && sharedHeater->isSensorValid()) {
          switches[id].value = sharedHeater->getDewPointC();
        }
      }
      return;
    }

    // Switch 9/10 expose live heater output percent from switch 4/5
    if ((id == 9 || id == 10) && switches[id].type == SwitchType::DewHeaterOutputPct) {
      int sourceSwitch = (id == 9) ? 4 : 5;
      if (isValidSwitchId(sourceSwitch)) {
        if (dewHeaters[sourceSwitch] == nullptr) {
          recreateDewHeater(sourceSwitch);
        }
        if (dewHeaters[sourceSwitch] != nullptr) {
          DewHeater *sourceHeater = dewHeaters[sourceSwitch];
          sourceHeater->update();
          switches[id].value = sourceHeater->getHeaterPowerPercent();
        }
      }
      return;
    }

    // Switch 6/7 expose live heater temperature from switch 4/5
    if ((id == 6 || id == 7) && switches[id].type == SwitchType::DS18B20Temp) {
      int sourceSwitch = (id == 6) ? 4 : 5;
      if (isValidSwitchId(sourceSwitch)) {
        if (dewHeaters[sourceSwitch] == nullptr) {
          recreateDewHeater(sourceSwitch);
        }
        if (dewHeaters[sourceSwitch] != nullptr) {
          DewHeater *sourceHeater = dewHeaters[sourceSwitch];
          sourceHeater->update();
          if (sourceHeater->isHeaterTemperatureValid()) {
            switches[id].value = sourceHeater->getHeaterTemperatureC();
          }
        }
      }
      return;
    }

    if (dewHeaters[id] == nullptr) {
      recreateDewHeater(id);
    }
    if (dewHeaters[id] == nullptr) return;

    DewHeater *heater = dewHeaters[id];
    heater->update();

    switch (switches[id].type) {
      case SwitchType::DewHeater:
        if (!heater->isPidEnabled()) {
          heater->enablePidControl(true);
        }
        heater->setHeaterTemperatureOffsetC((float)switches[id].heaterTempOffsetC);
        if (switches[id].dewHeaterMode == DewHeaterMode::Manual) {
          heater->setTargetHeaterTemperatureC((float)switches[id].dewTargetManualC);
        } else {
          heater->setTargetAboveDewPointC((float)switches[id].dewTargetOffsetC);
        }
        switches[id].value = switches[id].enabled ? switches[id].maxValue : switches[id].minValue;
        break;
      case SwitchType::DHT22Temp:
        if (heater->isSensorValid()) switches[id].value = heater->getTemperatureC() + switches[id].dhtTempOffsetC;
        break;
      case SwitchType::DHT22Humidity:
        if (heater->isSensorValid()) switches[id].value = heater->getHumidityPercent();
        break;
      case SwitchType::DS18B20Temp:
        if (heater->isHeaterTemperatureValid()) switches[id].value = heater->getHeaterTemperatureC();
        break;
      case SwitchType::DHT22DewPoint:
        if (heater->isSensorValid()) switches[id].value = heater->getDewPointC();
        break;
      default:
        break;
    }
  }

  void scheduleRecreateDewHeater(int id) {
    if (!isValidSwitchId(id) || id >= 32) return;
    pendingRecreateMask |= (1UL << id);
  }

  void scheduleSaveAllToEEPROM() {
    pendingSaveAllEeprom = true;
  }

  void processDeferredMaintenance() {
    if (pendingRecreateMask != 0) {
      for (int i = 0; i < maxSwitch && i < 32; i++) {
        uint32_t bit = (1UL << i);
        if ((pendingRecreateMask & bit) != 0) {
          pendingRecreateMask &= ~bit;
          recreateDewHeater(i);
          applyOutput(i);
          refreshSwitchSensorState(i);
        }
      }
    }

    if (pendingSaveAllEeprom) {
      pendingSaveAllEeprom = false;
      saveAllToEEPROM();
    }
  }

  // ---- EEPROM helpers ----

  void loadFromEEPROM() {
    uint16_t magic;
    EEPROM.get(EEPROM_SW_MAGIC_ADDR, magic);
    if (magic != EEPROM_SW_MAGIC_VAL) {
      LOG_WARN("No valid switch config in EEPROM, using defaults");
      return;
    }
    for (int i = 0; i < maxSwitch && i < EEPROM_SW_MAX; i++) {
      SwitchEEPROMData d;
      EEPROM.get(EEPROM_SW_DATA_ADDR + i * EEPROM_SW_ENTRY_SIZE, d);
      d.name[15]        = '\0';
      d.description[20] = '\0';
      if (d.name[0]        != '\0') switches[i].name        = d.name;
      if (d.description[0] != '\0') switches[i].description = d.description;
      switches[i].canWrite  = (d.flags & 0x01) != 0;
      switches[i].canAsync  = (d.flags & 0x02) != 0;
      switches[i].isPWM     = (d.flags & 0x04) != 0;
      switches[i].enabled   = (d.flags & 0x80) != 0;
      uint8_t typeBits = (d.flags >> 3) & 0x07;
      if (typeBits <= (uint8_t)SwitchType::DewHeaterOutputPct) {
        switches[i].type = (SwitchType)typeBits;
      } else {
        switches[i].type = SwitchType::Default;
      }
      switches[i].dewHeaterMode = ((d.flags & 0x40) != 0) ? DewHeaterMode::Manual : DewHeaterMode::Automatic;
      switches[i].outputPin = (d.outputPin == 0xFF) ? -1 : (int)d.outputPin;
      switches[i].tempInputPin = (d.tempInputPin == 0xFF) ? -1 : (int)d.tempInputPin;
      switches[i].heaterTempPin = (d.heaterTempPin == 0xFF) ? -1 : (int)d.heaterTempPin;
      switches[i].value     = (double)d.value;
      switches[i].minValue  = (double)d.minValue;
      switches[i].maxValue  = (double)d.maxValue;
      switches[i].stepValue = (double)d.stepValue;
      switches[i].dewTargetOffsetC = (double)d.dewTargetOffsetC;
      switches[i].dewTargetManualC = (double)d.dewTargetManualC;
      switches[i].heaterTempOffsetC = (double)d.heaterTempOffsetC;
      switches[i].dhtTempOffsetC = (double)d.dhtTempOffsetC;
      switches[i].heaterTempSensorType = (d.heaterTempSensorType == 1)
                  ? DewHeaterTempSensorType::NTCThermistor
                  : DewHeaterTempSensorType::DS18B20;
      switches[i].ntcSeriesResistorOhm = (double)d.ntcSeriesResistorOhm;
      switches[i].ntcNominalResistanceOhm = (double)d.ntcNominalResistanceOhm;
      switches[i].ntcNominalTemperatureC = (double)d.ntcNominalTemperatureC;
      switches[i].ntcBeta = (double)d.ntcBeta;
      if (switches[i].ntcSeriesResistorOhm <= 0.0) switches[i].ntcSeriesResistorOhm = 10000.0;
      if (switches[i].ntcNominalResistanceOhm <= 0.0) switches[i].ntcNominalResistanceOhm = 10000.0;
      if (switches[i].ntcNominalTemperatureC < -80.0 || switches[i].ntcNominalTemperatureC > 200.0) switches[i].ntcNominalTemperatureC = 25.0;
      if (switches[i].ntcBeta <= 0.0) switches[i].ntcBeta = 3950.0;
      if (switches[i].outputPin >= 0) {
        pinMode(switches[i].outputPin, OUTPUT);
        applyOutput(i);
      }
      if (switches[i].tempInputPin >= 0) {
        pinMode(switches[i].tempInputPin, INPUT);
      }
      if (switches[i].heaterTempPin >= 0) {
        pinMode(switches[i].heaterTempPin, INPUT);
      }
      recreateDewHeater(i);
    }
    LOG_INFO("Switch configuration loaded from EEPROM");
  }

  void saveSwitchToEEPROM(int id) {
    if (!isValidSwitchId(id) || id >= EEPROM_SW_MAX) return;
    SwitchEEPROMData d;
    memset(&d, 0, sizeof(d));
    strncpy(d.name,        switches[id].name.c_str(),        15);
    strncpy(d.description, switches[id].description.c_str(), 20);
    d.outputPin = (switches[id].outputPin < 0) ? 0xFF : (uint8_t)switches[id].outputPin;
    d.tempInputPin = (switches[id].tempInputPin < 0) ? 0xFF : (uint8_t)switches[id].tempInputPin;
    d.heaterTempPin = (switches[id].heaterTempPin < 0) ? 0xFF : (uint8_t)switches[id].heaterTempPin;
    d.flags     = 0;
    if (switches[id].canWrite) d.flags |= 0x01;
    if (switches[id].canAsync) d.flags |= 0x02;
    if (switches[id].isPWM)    d.flags |= 0x04;
    d.flags |= (((uint8_t)switches[id].type) & 0x07) << 3;
    if (switches[id].dewHeaterMode == DewHeaterMode::Manual) d.flags |= 0x40;
    if (switches[id].enabled) d.flags |= 0x80;
    d.value     = (float)switches[id].value;
    d.minValue  = (float)switches[id].minValue;
    d.maxValue  = (float)switches[id].maxValue;
    d.stepValue = (float)switches[id].stepValue;
    d.dewTargetOffsetC = (float)switches[id].dewTargetOffsetC;
    d.dewTargetManualC = (float)switches[id].dewTargetManualC;
    d.heaterTempOffsetC = (float)switches[id].heaterTempOffsetC;
    d.dhtTempOffsetC = (float)switches[id].dhtTempOffsetC;
    d.heaterTempSensorType = (switches[id].heaterTempSensorType == DewHeaterTempSensorType::NTCThermistor) ? 1 : 0;
    d.ntcSeriesResistorOhm = (float)switches[id].ntcSeriesResistorOhm;
    d.ntcNominalResistanceOhm = (float)switches[id].ntcNominalResistanceOhm;
    d.ntcNominalTemperatureC = (float)switches[id].ntcNominalTemperatureC;
    d.ntcBeta = (float)switches[id].ntcBeta;
    EEPROM.put(EEPROM_SW_DATA_ADDR + id * EEPROM_SW_ENTRY_SIZE, d);
  }

  void saveAllToEEPROM() {
    EEPROM.put(EEPROM_SW_MAGIC_ADDR, (uint16_t)EEPROM_SW_MAGIC_VAL);
    for (int i = 0; i < maxSwitch && i < EEPROM_SW_MAX; i++) {
      saveSwitchToEEPROM(i);
    }
    EEPROM.commit();
    LOG_INFO("Switch configuration saved to EEPROM");
  }

  String escapeHtml(const String &input) {
    String out;
    out.reserve(input.length() + 16);
    for (size_t i = 0; i < input.length(); i++) {
      char c = input.charAt(i);
      if (c == '&') out += "&amp;";
      else if (c == '<') out += "&lt;";
      else if (c == '>') out += "&gt;";
      else if (c == '"') out += "&quot;";
      else if (c == '\'') out += "&#39;";
      else out += c;
    }
    return out;
  }

  int getRequestedBank(AsyncWebServerRequest *request, bool fromPostBody) {
    if (request->hasParam("bank", fromPostBody)) {
      int bank = request->getParam("bank", fromPostBody)->value().toInt();
      if (bank == 0 || bank == 1 || bank == 2) return bank;
    }
    return -1;
  }

  int getDewHeaterSwitchIndexFromSlot(int slot) const {
    if (slot == 1) return 4;
    if (slot == 2) return 5;
    return -1;
  }

  int getDewHeaterTempSwitchIndexFromSlot(int slot) const {
    if (slot == 1) return 6;
    if (slot == 2) return 7;
    return -1;
  }

  int getDewHeaterOutputSwitchIndexFromSlot(int slot) const {
    if (slot == 1) return 9;
    if (slot == 2) return 10;
    return -1;
  }

  void enforceReservedSwitchRoles() {
    if (maxSwitch > 2) {
      switches[2].name = "Ambient Temp.";
      switches[2].description = "Ambient Temperature";
      switches[2].type = SwitchType::DHT22Temp;
      switches[2].canWrite = false;
      switches[2].canAsync = false;
      switches[2].enabled = true;
      switches[2].isPWM = false;
      switches[2].outputPin = -1;
      switches[2].heaterTempPin = -1;
      switches[2].heaterTempSensorType = DewHeaterTempSensorType::DS18B20;
      switches[2].ntcSeriesResistorOhm = 10000.0;
      switches[2].ntcNominalResistanceOhm = 10000.0;
      switches[2].ntcNominalTemperatureC = 25.0;
      switches[2].ntcBeta = 3950.0;
      switches[2].minValue = -40.0;
      switches[2].maxValue = 80.0;
      switches[2].stepValue = 0.1;
    }
    if (maxSwitch > 3) {
      switches[3].name = "Ambient Humid.";
      switches[3].description = "Ambient Humidity";
      switches[3].type = SwitchType::DHT22Humidity;
      switches[3].canWrite = false;
      switches[3].canAsync = false;
      switches[3].enabled = true;
      switches[3].isPWM = false;
      switches[3].outputPin = -1;
      switches[3].tempInputPin = switches[2].tempInputPin;
      switches[3].heaterTempPin = -1;
      switches[3].heaterTempSensorType = DewHeaterTempSensorType::DS18B20;
      switches[3].ntcSeriesResistorOhm = 10000.0;
      switches[3].ntcNominalResistanceOhm = 10000.0;
      switches[3].ntcNominalTemperatureC = 25.0;
      switches[3].ntcBeta = 3950.0;
      switches[3].minValue = 0.0;
      switches[3].maxValue = 100.0;
      switches[3].stepValue = 0.1;
    }
    if (maxSwitch > 4) {
      switches[4].name = "Main DewHeater";
      switches[4].description = "Main Scope DewHeater";
      switches[4].type = SwitchType::DewHeater;
      switches[4].canWrite = true;
      switches[4].canAsync = false;
      switches[4].enabled = false;
      switches[4].minValue = 0.0;
      switches[4].maxValue = 1.0;
      switches[4].stepValue = 1.0;
      switches[4].isPWM = true;
      switches[4].value = switches[4].minValue;
    }
    if (maxSwitch > 5) {
      switches[5].name = "Guide DewHeater";
      switches[5].description = "Guide Cam Dew Heater";
      switches[5].type = SwitchType::DewHeater;
      switches[5].canWrite = true;
      switches[5].canAsync = false;
      switches[5].enabled = false;
      switches[5].minValue = 0.0;
      switches[5].maxValue = 1.0;
      switches[5].stepValue = 1.0;
      switches[5].isPWM = true;
      switches[5].value = switches[5].minValue;
    }
    if (maxSwitch > 6) {
      switches[6].name = "Main DH Temp";
      switches[6].description = "Main Scope Dew Temp";
      switches[6].type = SwitchType::DS18B20Temp;
      switches[6].canWrite = false;
      switches[6].canAsync = false;
      switches[6].enabled = true;
      switches[6].isPWM = false;
      switches[6].outputPin = -1;
      switches[6].tempInputPin = -1;
      switches[6].heaterTempPin = switches[4].heaterTempPin;
      switches[6].heaterTempSensorType = switches[4].heaterTempSensorType;
      switches[6].ntcSeriesResistorOhm = switches[4].ntcSeriesResistorOhm;
      switches[6].ntcNominalResistanceOhm = switches[4].ntcNominalResistanceOhm;
      switches[6].ntcNominalTemperatureC = switches[4].ntcNominalTemperatureC;
      switches[6].ntcBeta = switches[4].ntcBeta;
      switches[6].heaterTempOffsetC = switches[4].heaterTempOffsetC;
      switches[6].minValue = -55.0;
      switches[6].maxValue = 125.0;
      switches[6].stepValue = 0.1;
    }
    if (maxSwitch > 7) {
      switches[7].name = "Guide DH Temp";
      switches[7].description = "Guide Cam Dew Temp";
      switches[7].type = SwitchType::DS18B20Temp;
      switches[7].canWrite = false;
      switches[7].canAsync = false;
      switches[7].enabled = true;
      switches[7].isPWM = false;
      switches[7].outputPin = -1;
      switches[7].tempInputPin = -1;
      switches[7].heaterTempPin = switches[5].heaterTempPin;
      switches[7].heaterTempSensorType = switches[5].heaterTempSensorType;
      switches[7].ntcSeriesResistorOhm = switches[5].ntcSeriesResistorOhm;
      switches[7].ntcNominalResistanceOhm = switches[5].ntcNominalResistanceOhm;
      switches[7].ntcNominalTemperatureC = switches[5].ntcNominalTemperatureC;
      switches[7].ntcBeta = switches[5].ntcBeta;
      switches[7].heaterTempOffsetC = switches[5].heaterTempOffsetC;
      switches[7].minValue = -55.0;
      switches[7].maxValue = 125.0;
      switches[7].stepValue = 0.1;
    }
    if (maxSwitch > 8) {
      switches[8].name = "Ambient DewPt";
      switches[8].description = "Calculated Dew Point";
      switches[8].type = SwitchType::DHT22DewPoint;
      switches[8].canWrite = false;
      switches[8].canAsync = false;
      switches[8].enabled = true;
      switches[8].isPWM = false;
      switches[8].outputPin = -1;
      switches[8].heaterTempPin = -1;
      switches[8].tempInputPin = switches[2].tempInputPin;
      switches[8].heaterTempSensorType = DewHeaterTempSensorType::DS18B20;
      switches[8].ntcSeriesResistorOhm = 10000.0;
      switches[8].ntcNominalResistanceOhm = 10000.0;
      switches[8].ntcNominalTemperatureC = 25.0;
      switches[8].ntcBeta = 3950.0;
      switches[8].minValue = -40.0;
      switches[8].maxValue = 80.0;
      switches[8].stepValue = 0.1;
    }
    if (maxSwitch > 9) {
      switches[9].name = "Main DH Out%";
      switches[9].description = "Main Dew Heater output %";
      switches[9].type = SwitchType::DewHeaterOutputPct;
      switches[9].canWrite = false;
      switches[9].canAsync = false;
      switches[9].enabled = true;
      switches[9].isPWM = false;
      switches[9].outputPin = -1;
      switches[9].tempInputPin = -1;
      switches[9].heaterTempPin = -1;
      switches[9].heaterTempSensorType = DewHeaterTempSensorType::DS18B20;
      switches[9].ntcSeriesResistorOhm = 10000.0;
      switches[9].ntcNominalResistanceOhm = 10000.0;
      switches[9].ntcNominalTemperatureC = 25.0;
      switches[9].ntcBeta = 3950.0;
      switches[9].minValue = 0.0;
      switches[9].maxValue = 100.0;
      switches[9].stepValue = 0.1;
    }
    if (maxSwitch > 10) {
      switches[10].name = "Guide DH Out%";
      switches[10].description = "Guide Dew Heater output %";
      switches[10].type = SwitchType::DewHeaterOutputPct;
      switches[10].canWrite = false;
      switches[10].canAsync = false;
      switches[10].enabled = true;
      switches[10].isPWM = false;
      switches[10].outputPin = -1;
      switches[10].tempInputPin = -1;
      switches[10].heaterTempPin = -1;
      switches[10].heaterTempSensorType = DewHeaterTempSensorType::DS18B20;
      switches[10].ntcSeriesResistorOhm = 10000.0;
      switches[10].ntcNominalResistanceOhm = 10000.0;
      switches[10].ntcNominalTemperatureC = 25.0;
      switches[10].ntcBeta = 3950.0;
      switches[10].minValue = 0.0;
      switches[10].maxValue = 100.0;
      switches[10].stepValue = 0.1;
    }
  }

  String getSwitchTypeHelpText(SwitchType type) const {
    switch (type) {
      case SwitchType::DewHeater:
        return "Dedicated dew heater. Configure mode, targets and sensor pins on the main setup page.";
      case SwitchType::DHT22Temp:
        return "Temp Input GPIO = DHT22/AM2302 data pin. DHT Temperature Offset is added to ambient temperature in °C.";
      case SwitchType::DHT22Humidity:
        return "Temp Input GPIO = DHT22/AM2302 data pin. Current Value is read-only live relative humidity in %.";
      case SwitchType::DS18B20Temp:
        return "Heater Temp GPIO = heater temperature sensor pin (DS18B20 or NTC thermistor). Current Value is read-only live temperature in °C.";
      case SwitchType::DHT22DewPoint:
        return "Temp Input GPIO = DHT22/AM2302 data pin. Current Value is read-only calculated dew point in °C.";
      case SwitchType::DewHeaterOutputPct:
        return "Read-only live heater output in %.";
      case SwitchType::Default:
      default:
        return "GPIO Pin controls the output. Current Value is the switch value. Sensor GPIO fields are optional and unused in default mode.";
    }
  }

  /**
   * @brief Validate switch ID
   * @param id Switch ID to validate
   * @return true if valid
   */
  bool isValidSwitchId(int id) const {
    return (id >= 0 && id < maxSwitch);
  }
  
  /**
   * @brief Apply physical output based on switch state
   * @param id Switch ID
   */
  void applyOutput(int id) {
    if (!isValidSwitchId(id)) return;
    
    SwitchData& sw = switches[id];
    if (sw.outputPin < 0) return;

    if (sw.type == SwitchType::DewHeater) {
      if (dewHeaters[id] == nullptr) {
        recreateDewHeater(id);
      }
      if (dewHeaters[id] != nullptr) {
        DewHeater *heater = dewHeaters[id];
        if (!sw.enabled) {
          heater->enablePidControl(false);
          heater->setHeaterPowerPercent(0.0f);
        } else {
          if (!heater->isPidEnabled()) {
            heater->enablePidControl(true);
          }
          heater->setHeaterTemperatureOffsetC((float)sw.heaterTempOffsetC);
          if (sw.dewHeaterMode == DewHeaterMode::Manual) {
            heater->setTargetHeaterTemperatureC((float)sw.dewTargetManualC);
          } else {
            heater->setTargetAboveDewPointC((float)sw.dewTargetOffsetC);
          }
        }
      }
      return;
    }
    
    if (sw.isPWM) {
      // PWM output (analog value)
      int pwmValue = (int)map((long)(sw.value * 100), 
                               (long)(sw.minValue * 100), 
                               (long)(sw.maxValue * 100), 
                               0, 255);
      analogWrite(sw.outputPin, pwmValue);
      LOG_DEBUG("Switch " + String(id) + " PWM output: " + String(pwmValue));
    } else {
      // Digital output (boolean)
      bool state = (sw.value > 0.0);
      digitalWrite(sw.outputPin, state ? HIGH : LOW);
      LOG_DEBUG("Switch " + String(id) + " digital output: " + (state ? "HIGH" : "LOW"));
    }
  }

public:
  /**
   * @brief Constructor for ArduinoSwitch
   * @param devicename Name of the switch device
   * @param devicenumber Device number for Alpaca API
   * @param description Human-readable description
   * @param server Reference to AsyncWebServer
   * @param num_switches Number of switches to manage (default: 8)
   */
  ArduinoSwitch(String devicename, int devicenumber, String description, 
           AsyncWebServer &server, int num_switches = 11)
    : AlpacaDeviceSwitch(devicename, devicenumber, description, server),
      maxSwitch(num_switches),
      pendingRecreateMask(0),
      pendingSaveAllEeprom(false),
      ledDisabled(false) {
    
    // Allocate switch array
    switches = new SwitchData[maxSwitch];
    dewHeaters = new DewHeater*[maxSwitch];
    
    // Initialize switches with default values
    for (int i = 0; i < maxSwitch; i++) {
      switches[i].name = "Custom Switch" + std::to_string(i);
      switches[i].description = "Custom Switch device" + std::to_string(i);
      switches[i].type = SwitchType::Default;
      switches[i].dewHeaterMode = DewHeaterMode::Automatic;
      switches[i].canWrite = true;
      switches[i].canAsync = false;  // Set to true for ISwitchV3+ async support
      switches[i].enabled = false;
      switches[i].value = 0.0;
      switches[i].minValue = 0.0;
      switches[i].maxValue = 1.0;
      switches[i].stepValue = 1.0;
      switches[i].dewTargetOffsetC = 2.0;
      switches[i].dewTargetManualC = 25.0;
      switches[i].heaterTempOffsetC = 0.0;
      switches[i].dhtTempOffsetC = 0.0;
      switches[i].heaterTempSensorType = DewHeaterTempSensorType::DS18B20;
      switches[i].ntcSeriesResistorOhm = 10000.0;
      switches[i].ntcNominalResistanceOhm = 10000.0;
      switches[i].ntcNominalTemperatureC = 25.0;
      switches[i].ntcBeta = 3950.0;
      switches[i].outputPin = -1;
      switches[i].tempInputPin = -1;
      switches[i].heaterTempPin = -1;
      switches[i].isPWM = false;
      switches[i].stateChangeComplete = true;
      dewHeaters[i] = nullptr;
    }

    // Requested default profiles
    // Switch 2: ambient temperature
    if (maxSwitch > 2) {
      switches[2].name = "Ambient Temp.";
      switches[2].description = "Ambient Temperature";
      switches[2].type = SwitchType::DHT22Temp;
      switches[2].canWrite = false;
      switches[2].canAsync = false;
      switches[2].enabled = true;
      switches[2].isPWM = false;
      switches[2].outputPin = -1;
      switches[2].tempInputPin = 13;
      switches[2].heaterTempPin = -1;
      switches[2].minValue = -40.0;
      switches[2].maxValue = 80.0;
      switches[2].stepValue = 0.1;
    }

    // Switch 3: ambient humidity
    if (maxSwitch > 3) {
      switches[3].name = "Ambient Humid.";
      switches[3].description = "Ambient Humidity";
      switches[3].type = SwitchType::DHT22Humidity;
      switches[3].canWrite = false;
      switches[3].canAsync = false;
      switches[3].enabled = true;
      switches[3].isPWM = false;
      switches[3].outputPin = -1;
      switches[3].tempInputPin = 13;
      switches[3].heaterTempPin = -1;
      switches[3].minValue = 0.0;
      switches[3].maxValue = 100.0;
      switches[3].stepValue = 0.1;
    }

    // Dew heater 1 on switch 4
    if (maxSwitch > 4) {
      switches[4].name = "Main DewHeater";
      switches[4].description = "Main Scope DewHeater";
      switches[4].type = SwitchType::DewHeater;
      switches[4].dewHeaterMode = DewHeaterMode::Automatic;
      switches[4].canWrite = true;
      switches[4].canAsync = false;
      switches[4].enabled = false;
      switches[4].isPWM = true;
      switches[4].dewTargetOffsetC = 5.0;
      switches[4].dewTargetManualC = 30.0;
      switches[4].outputPin = 5;
      switches[4].heaterTempPin = 14;
      switches[4].heaterTempSensorType = DewHeaterTempSensorType::DS18B20;
      switches[4].ntcSeriesResistorOhm = 10000.0;
      switches[4].ntcNominalResistanceOhm = 10000.0;
      switches[4].ntcNominalTemperatureC = 25.0;
      switches[4].ntcBeta = 3950.0;
      switches[4].tempInputPin = 13;
    }

    // Dew heater 2 on switch 5
    if (maxSwitch > 5) {
      switches[5].name = "Guide DewHeater";
      switches[5].description = "Guide Cam Dew Heater";
      switches[5].type = SwitchType::DewHeater;
      switches[5].dewHeaterMode = DewHeaterMode::Automatic;
      switches[5].canWrite = true;
      switches[5].canAsync = false;
      switches[5].enabled = false;
      switches[5].isPWM = true;
      switches[5].dewTargetOffsetC = 5.0;
      switches[5].dewTargetManualC = 30.0;
      switches[5].outputPin = 4;
      switches[5].heaterTempPin = 12;
      switches[5].heaterTempSensorType = DewHeaterTempSensorType::DS18B20;
      switches[5].ntcSeriesResistorOhm = 10000.0;
      switches[5].ntcNominalResistanceOhm = 10000.0;
      switches[5].ntcNominalTemperatureC = 25.0;
      switches[5].ntcBeta = 3950.0;
      switches[5].tempInputPin = 13;
    }

    // Dew heater 1 actual temperature on switch 6
    if (maxSwitch > 6) {
      switches[6].name = "Main DH Temp";
      switches[6].description = "Main Scope Dew Temp";
      switches[6].type = SwitchType::DS18B20Temp;
      switches[6].canWrite = false;
      switches[6].canAsync = false;
      switches[6].enabled = true;
      switches[6].isPWM = false;
      switches[6].outputPin = -1;
      switches[6].tempInputPin = -1;
      switches[6].heaterTempPin = 14;
      switches[6].heaterTempSensorType = switches[4].heaterTempSensorType;
      switches[6].ntcSeriesResistorOhm = switches[4].ntcSeriesResistorOhm;
      switches[6].ntcNominalResistanceOhm = switches[4].ntcNominalResistanceOhm;
      switches[6].ntcNominalTemperatureC = switches[4].ntcNominalTemperatureC;
      switches[6].ntcBeta = switches[4].ntcBeta;
      switches[6].minValue = -55.0;
      switches[6].maxValue = 125.0;
      switches[6].stepValue = 0.1;
    }

    // Dew heater 2 actual temperature on switch 7
    if (maxSwitch > 7) {
      switches[7].name = "Guide DH Temp";
      switches[7].description = "Guide Cam Dew Temp";
      switches[7].type = SwitchType::DS18B20Temp;
      switches[7].canWrite = false;
      switches[7].canAsync = false;
      switches[7].enabled = true;
      switches[7].isPWM = false;
      switches[7].outputPin = -1;
      switches[7].tempInputPin = -1;
      switches[7].heaterTempPin = 12;
      switches[7].heaterTempSensorType = switches[5].heaterTempSensorType;
      switches[7].ntcSeriesResistorOhm = switches[5].ntcSeriesResistorOhm;
      switches[7].ntcNominalResistanceOhm = switches[5].ntcNominalResistanceOhm;
      switches[7].ntcNominalTemperatureC = switches[5].ntcNominalTemperatureC;
      switches[7].ntcBeta = switches[5].ntcBeta;
      switches[7].minValue = -55.0;
      switches[7].maxValue = 125.0;
      switches[7].stepValue = 0.1;
    }

    // Additional dew point switch on switch 8
    if (maxSwitch > 8) {
      switches[8].name = "Ambient DewPt";
      switches[8].description = "Calculated Dew Point";
      switches[8].type = SwitchType::DHT22DewPoint;
      switches[8].canWrite = false;
      switches[8].canAsync = false;
      switches[8].enabled = true;
      switches[8].isPWM = false;
      switches[8].outputPin = -1;
      switches[8].tempInputPin = 13;
      switches[8].heaterTempPin = -1;
      switches[8].heaterTempSensorType = DewHeaterTempSensorType::DS18B20;
      switches[8].ntcSeriesResistorOhm = 10000.0;
      switches[8].ntcNominalResistanceOhm = 10000.0;
      switches[8].ntcNominalTemperatureC = 25.0;
      switches[8].ntcBeta = 3950.0;
      switches[8].minValue = -40.0;
      switches[8].maxValue = 80.0;
      switches[8].stepValue = 0.1;
    }

    // Additional read-only heater output % switches
    if (maxSwitch > 9) {
      switches[9].name = "Main DH Out%";
      switches[9].description = "Main Dew Heater output %";
      switches[9].type = SwitchType::DewHeaterOutputPct;
      switches[9].canWrite = false;
      switches[9].canAsync = false;
      switches[9].enabled = true;
      switches[9].isPWM = false;
      switches[9].outputPin = -1;
      switches[9].tempInputPin = -1;
      switches[9].heaterTempPin = -1;
      switches[9].heaterTempSensorType = DewHeaterTempSensorType::DS18B20;
      switches[9].ntcSeriesResistorOhm = 10000.0;
      switches[9].ntcNominalResistanceOhm = 10000.0;
      switches[9].ntcNominalTemperatureC = 25.0;
      switches[9].ntcBeta = 3950.0;
      switches[9].minValue = 0.0;
      switches[9].maxValue = 100.0;
      switches[9].stepValue = 0.1;
    }
    if (maxSwitch > 10) {
      switches[10].name = "Guide DH Out%";
      switches[10].description = "Guide Dew Heater output %";
      switches[10].type = SwitchType::DewHeaterOutputPct;
      switches[10].canWrite = false;
      switches[10].canAsync = false;
      switches[10].enabled = true;
      switches[10].isPWM = false;
      switches[10].outputPin = -1;
      switches[10].tempInputPin = -1;
      switches[10].heaterTempPin = -1;
      switches[10].heaterTempSensorType = DewHeaterTempSensorType::DS18B20;
      switches[10].ntcSeriesResistorOhm = 10000.0;
      switches[10].ntcNominalResistanceOhm = 10000.0;
      switches[10].ntcNominalTemperatureC = 25.0;
      switches[10].ntcBeta = 3950.0;
      switches[10].minValue = 0.0;
      switches[10].maxValue = 100.0;
      switches[10].stepValue = 0.1;
    }
    
    LOG_DEBUG("ArduinoSwitch created with " + String(maxSwitch) + " switches");

    // Load persisted configuration from EEPROM (overwrites defaults where saved)
    loadFromEEPROM();
    enforceReservedSwitchRoles();
    for (int i = 2; i <= 10 && i < maxSwitch; i++) {
      scheduleRecreateDewHeater(i);
    }
    // Force immediate heater creation so sensors are ready before first Alpaca API call
    processDeferredMaintenance();
  }
  
  virtual ~ArduinoSwitch() {
    for (int i = 0; i < maxSwitch; i++) {
      delete dewHeaters[i];
    }
    delete[] dewHeaters;
    delete[] switches;
  }
  
  // ==================== Configuration Methods ====================
  
  /**
   * @brief Configure a specific switch
   * @param id Switch ID (0 to maxSwitch-1)
   * @param name Switch name
   * @param description Switch description
   * @param canWrite Can the switch be written to
   * @param minValue Minimum value
   * @param maxValue Maximum value
   * @param stepValue Step value
   * @param outputPin GPIO pin for output (-1 for none)
   * @param isPWM True for PWM/analog output, false for digital
   */
  void configureSwitch(int id, const std::string& name, const std::string& description,
                       bool canWrite, double minValue, double maxValue, double stepValue,
                       int outputPin = -1, bool isPWM = false) {
    if (!isValidSwitchId(id)) {
      LOG_DEBUG("ERROR: Invalid switch ID: " + String(id));
      return;
    }
    
    switches[id].name = name;
    switches[id].description = description;
    switches[id].type = SwitchType::Default;
    switches[id].dewHeaterMode = DewHeaterMode::Automatic;
    switches[id].canWrite = canWrite;
    switches[id].enabled = false;
    switches[id].minValue = minValue;
    switches[id].maxValue = maxValue;
    switches[id].stepValue = stepValue;
    switches[id].dewTargetOffsetC = 2.0;
    switches[id].dewTargetManualC = 25.0;
    switches[id].heaterTempOffsetC = 0.0;
    switches[id].dhtTempOffsetC = 0.0;
    switches[id].heaterTempSensorType = DewHeaterTempSensorType::DS18B20;
    switches[id].ntcSeriesResistorOhm = 10000.0;
    switches[id].ntcNominalResistanceOhm = 10000.0;
    switches[id].ntcNominalTemperatureC = 25.0;
    switches[id].ntcBeta = 3950.0;
    switches[id].outputPin = outputPin;
    switches[id].tempInputPin = -1;
    switches[id].heaterTempPin = -1;
    switches[id].isPWM = isPWM;
    
    // Initialize output pin if specified
    if (outputPin >= 0) {
      pinMode(outputPin, OUTPUT);
      if (isPWM) {
        analogWrite(outputPin, 0);
      } else {
        digitalWrite(outputPin, LOW);
      }
    }
    
    LOG_DEBUG("Configured switch " + String(id) + ": " + name.c_str() + " on pin " + String(outputPin));
    recreateDewHeater(id);
  }
  
  // ==================== ISwitch Interface Implementation ====================
  
  /**
   * @brief Get the number of switch devices
   * @return Number of switches
   */
  int GetMaxSwitch() override {
    return maxSwitch;
  }
  
  /**
   * @brief Check if switch supports async operation
   * @param switchNumber Switch ID
   * @return true if async supported
   */
  bool GetCanAsync(int switchNumber) override {
    if (!isValidSwitchId(switchNumber)) return false;
    return switches[switchNumber].canAsync;
  }
  
  /**
   * @brief Check if switch can be written to
   * @param switchNumber Switch ID
   * @return true if writable
   */
  bool GetCanWrite(int switchNumber) override {
    if (!isValidSwitchId(switchNumber)) return false;
    return switches[switchNumber].canWrite;
  }
  
  /**
   * @brief Get switch state as boolean
   * @param switchNumber Switch ID
   * @return Switch state (true/false)
   */
  bool GetSwitch(int switchNumber) override {
    if (!isValidSwitchId(switchNumber)) return false;
    if (switches[switchNumber].type == SwitchType::DewHeater) {
      return switches[switchNumber].enabled;
    }
    return (switches[switchNumber].value > 0.0);
  }
  
  /**
   * @brief Get switch description
   * @param switchNumber Switch ID
   * @return Description string
   */
  std::string GetSwitchDescription(int switchNumber) override {
    if (!isValidSwitchId(switchNumber)) return "Invalid switch";
    return switches[switchNumber].description;
  }
  
  /**
   * @brief Get switch name
   * @param switchNumber Switch ID
   * @return Name string
   */
  std::string GetSwitchName(int switchNumber) override {
    if (!isValidSwitchId(switchNumber)) return "Invalid";
    return switches[switchNumber].name;
  }
  
  /**
   * @brief Get switch value
   * @param switchNumber Switch ID
   * @return Value as double
   */
  double GetSwitchValue(int switchNumber) override {
    if (!isValidSwitchId(switchNumber)) return 0.0;
    if (usesDewHeaterHelper(switches[switchNumber].type)) {
      refreshSwitchSensorState(switchNumber);
    }
    if (switches[switchNumber].type == SwitchType::DewHeater) {
      return switches[switchNumber].value;
    }
    return switches[switchNumber].value;
  }
  
  /**
   * @brief Get minimum switch value
   * @param switchNumber Switch ID
   * @return Minimum value
   */
  double GetMinSwitchValue(int switchNumber) override {
    if (!isValidSwitchId(switchNumber)) return 0.0;
    return switches[switchNumber].minValue;
  }
  
  /**
   * @brief Get maximum switch value
   * @param switchNumber Switch ID
   * @return Maximum value
   */
  double GetMaxSwitchValue(int switchNumber) override {
    if (!isValidSwitchId(switchNumber)) return 1.0;
    return switches[switchNumber].maxValue;
  }
  
  /**
   * @brief Check if async state change is complete
   * @param switchNumber Switch ID
   * @return true if complete
   */
  bool GetStateChangeComplete(int switchNumber) override {
    if (!isValidSwitchId(switchNumber)) return true;
    return switches[switchNumber].stateChangeComplete;
  }
  
  /**
   * @brief Get switch step size
   * @param switchNumber Switch ID
   * @return Step size
   */
  double GetSwitchStep(int switchNumber) override {
    if (!isValidSwitchId(switchNumber)) return 1.0;
    return switches[switchNumber].stepValue;
  }
  
  /**
   * @brief Set switch state asynchronously
   * @param switchNumber Switch ID
   * @param state Boolean state
   */
  void SetAsync(int switchNumber, bool state) override {
    if (!isValidSwitchId(switchNumber)) return;
    if (!switches[switchNumber].canWrite) {
      LOG_DEBUG("ERROR: Switch " + String(switchNumber) + " is not writable");
      return;
    }
    
    switches[switchNumber].stateChangeComplete = false;
    switches[switchNumber].enabled = state;
    switches[switchNumber].value = state ? switches[switchNumber].maxValue : switches[switchNumber].minValue;
    applyOutput(switchNumber);
    
    // Simulate async completion (in real implementation, this might take time)
    switches[switchNumber].stateChangeComplete = true;
    
    LOG_DEBUG("Switch " + String(switchNumber) + " async set to: " + (state ? "true" : "false"));
  }
  
  /**
   * @brief Set switch value asynchronously
   * @param switchNumber Switch ID
   * @param value Double value
   */
  void SetAsyncValue(int switchNumber, double value) override {
    if (!isValidSwitchId(switchNumber)) return;
    if (switches[switchNumber].type == SwitchType::DewHeaterOutputPct) {
      LOG_DEBUG("ERROR: Switch " + String(switchNumber) + " is read-only (heater output %)");
      return;
    }
    if (!switches[switchNumber].canWrite) {
      LOG_DEBUG("ERROR: Switch " + String(switchNumber) + " is not writable");
      return;
    }
    
    // Clamp value to valid range
    if (value < switches[switchNumber].minValue) value = switches[switchNumber].minValue;
    if (value > switches[switchNumber].maxValue) value = switches[switchNumber].maxValue;
    
    switches[switchNumber].stateChangeComplete = false;
    if (switches[switchNumber].type == SwitchType::DewHeater) {
      bool turnOn = value > switches[switchNumber].minValue;
      switches[switchNumber].enabled = turnOn;
      switches[switchNumber].value = turnOn ? switches[switchNumber].maxValue : switches[switchNumber].minValue;
      applyOutput(switchNumber);
      switches[switchNumber].stateChangeComplete = true;
      LOG_DEBUG("DewHeater " + String(switchNumber) + " async value interpreted as state: " + (turnOn ? "ON" : "OFF"));
      return;
    }
    switches[switchNumber].value = value;
    applyOutput(switchNumber);
    
    // Simulate async completion
    switches[switchNumber].stateChangeComplete = true;
    
    LOG_DEBUG("Switch " + String(switchNumber) + " async value set to: " + String(value));
  }
  
  /**
   * @brief Set switch state
   * @param switchNumber Switch ID
   * @param state Boolean state
   */
  void SetSwitch(int switchNumber, bool state) override {
    if (!isValidSwitchId(switchNumber)) return;
    if (!switches[switchNumber].canWrite) {
      LOG_DEBUG("ERROR: Switch " + String(switchNumber) + " is not writable");
      return;
    }
    
    switches[switchNumber].enabled = state;
    switches[switchNumber].value = state ? switches[switchNumber].maxValue : switches[switchNumber].minValue;
    applyOutput(switchNumber);
    
    LOG_DEBUG("Switch " + String(switchNumber) + " set to: " + (state ? "true" : "false"));
  }
  
  /**
   * @brief Set switch name
   * @param switchNumber Switch ID
   * @param name New name
   */
  void SetSwitchName(int switchNumber, const std::string& name) override {
    if (!isValidSwitchId(switchNumber)) return;
    switches[switchNumber].name = name;
    LOG_DEBUG("Switch " + String(switchNumber) + " name set to: " + name.c_str());
  }
  
  /**
   * @brief Set switch value
   * @param switchNumber Switch ID
   * @param value Double value
   */
  void SetSwitchValue(int switchNumber, double value) override {
    if (!isValidSwitchId(switchNumber)) return;
    if (switches[switchNumber].type == SwitchType::DewHeaterOutputPct) {
      LOG_DEBUG("ERROR: Switch " + String(switchNumber) + " is read-only (heater output %)");
      return;
    }
    if (!switches[switchNumber].canWrite) {
      LOG_DEBUG("ERROR: Switch " + String(switchNumber) + " is not writable");
      return;
    }

    if (switches[switchNumber].type == SwitchType::DewHeater) {
      bool turnOn = value > switches[switchNumber].minValue;
      switches[switchNumber].enabled = turnOn;
      switches[switchNumber].value = turnOn ? switches[switchNumber].maxValue : switches[switchNumber].minValue;
      applyOutput(switchNumber);
      LOG_DEBUG("DewHeater " + String(switchNumber) + " value interpreted as state: " + (turnOn ? "ON" : "OFF"));
      return;
    }
    
    // Clamp value to valid range
    if (value < switches[switchNumber].minValue) value = switches[switchNumber].minValue;
    if (value > switches[switchNumber].maxValue) value = switches[switchNumber].maxValue;
    
    switches[switchNumber].value = value;
    applyOutput(switchNumber);
    
    LOG_DEBUG("Switch " + String(switchNumber) + " value set to: " + String(value));
  }
  
  // ==================== Public Helper Methods ====================
  
  /**
   * @brief Get current value of a switch
   * @param switchNumber Switch ID
   * @return Current value
   */
  double getSwitchValue(int switchNumber) {
    if (!isValidSwitchId(switchNumber)) return 0.0;
    if (usesDewHeaterHelper(switches[switchNumber].type)) {
      refreshSwitchSensorState(switchNumber);
    }
    if (switches[switchNumber].type == SwitchType::DewHeater) {
      return switches[switchNumber].value;
    }
    return switches[switchNumber].value;
  }
  
  /**
   * @brief Get current state of a switch as boolean
   * @param switchNumber Switch ID
   * @return Current state
   */
  bool getSwitchState(int switchNumber) {
    if (!isValidSwitchId(switchNumber)) return false;
    if (switches[switchNumber].type == SwitchType::DewHeater) {
      return switches[switchNumber].enabled;
    }
    return (switches[switchNumber].value > 0.0);
  }

  void update() {
    enforceReservedSwitchRoles();
    processDeferredMaintenance();
    for (int i = 0; i < maxSwitch; i++) {
      refreshSwitchSensorState(i);
    }
  }
  
  /**
   * @brief Print status of all switches to serial
   */
  void printStatus() {
    Serial.println("\n=== Switch Status ===");
    for (int i = 0; i < maxSwitch; i++) {
      Serial.print("  [");
      Serial.print(i);
      Serial.print("] ");
      Serial.print(switches[i].name.c_str());
      Serial.print(": ");
      if (switches[i].isPWM) {
        Serial.print(switches[i].value, 2);
        Serial.print(" (");
        Serial.print(switches[i].minValue, 0);
        Serial.print("-");
        Serial.print(switches[i].maxValue, 0);
        Serial.print(")");
      } else {
        Serial.print(switches[i].value > 0.0 ? "ON" : "OFF");
      }
      if (switches[i].outputPin >= 0) {
        Serial.print(" [Pin ");
        Serial.print(switches[i].outputPin);
        Serial.print("]");
      }
      if (switches[i].tempInputPin >= 0) {
        Serial.print(" [Sensor ");
        Serial.print(switches[i].tempInputPin);
        Serial.print("]");
      }
      if (switches[i].heaterTempPin >= 0) {
        Serial.print(" [HeaterTemp ");
        Serial.print(switches[i].heaterTempPin);
        Serial.print("]");
      }
      Serial.println();
    }
    Serial.println("===================");
  }

  // ==================== Setup Handler ====================

  /**
   * @brief Web configuration interface for the switch device
   * Covers all SwitchData fields; persists to EEPROM on save.
   */
  void setupHandler(AsyncWebServerRequest *request) override {
    enforceReservedSwitchRoles();
    LOG_INFO("Received switch setup request - Method: " + String(request->method() == HTTP_POST ? "POST" : "GET"));

    String setupUrl = "/setup/v1/switch/" + String(GetDeviceNumber()) + "/setup";

    if (request->method() == HTTP_POST) {
      String message = "";
      int bank = getRequestedBank(request, true);

      // --- WiFi configuration (handled first so we can reset immediately) ---
      if (request->hasParam("led_page", true)) {
        ledDisabled = request->hasParam("led_disable", true);
        pinMode(2, OUTPUT);
        digitalWrite(2, ledDisabled ? HIGH : LOW);

        String ledMessage = ledDisabled ? "LED disabled (GPIO2 set HIGH)." : "LED enabled (GPIO2 set LOW).";
        String html = "<html><head><meta http-equiv='refresh' content='2;url=" + setupUrl + "'></head><body>";
        html += "<h1>Switch Setup</h1><p>" + ledMessage + "</p><p>Redirecting back...</p></body></html>";
        request->send(200, "text/html", html);
        return;
      }

      if (request->hasParam("wifi_ssid", true) && request->hasParam("wifi_password", true)) {
        String newSSID     = request->getParam("wifi_ssid",     true)->value();
        String newPassword = request->getParam("wifi_password", true)->value();
        newSSID.trim();
        if (newSSID.length() > 0) {
          if (wifiConfig.saveToEEPROM(newSSID, newPassword)) {
            String html = "<html><head><meta http-equiv='refresh' content='5;url=" + setupUrl + "'></head><body>";
            html += "<h1>Switch Setup</h1><p>WiFi credentials saved! Restarting...</p></body></html>";
            request->send(200, "text/html", html);
            delay(500);
            ESP.reset();
            return;
          } else {
            message += "Error: Failed to save WiFi credentials.<br>";
          }
        } else {
          message += "Error: SSID cannot be empty.<br>";
        }
      }

      // --- Dedicated dew heater configuration on main page ---
      if (request->hasParam("dh_slot", true)) {
        int slot = request->getParam("dh_slot", true)->value().toInt();
        int i = getDewHeaterSwitchIndexFromSlot(slot);
        int tempSwitchIdx = getDewHeaterTempSwitchIndexFromSlot(slot);
        if (isValidSwitchId(i)) {
          switches[i].type = SwitchType::DewHeater;
          switches[i].canWrite = true;
          switches[i].canAsync = false;
          switches[i].isPWM = true;
          switches[i].dewHeaterMode = request->hasParam("dh_mode_" + String(i), true) &&
                                      request->getParam("dh_mode_" + String(i), true)->value() == "manual"
                                        ? DewHeaterMode::Manual
                                        : DewHeaterMode::Automatic;

          if (request->hasParam("dh_name_" + String(i), true)) {
            String v = request->getParam("dh_name_" + String(i), true)->value();
            v.trim();
            if (v.length() > 0) switches[i].name = v.c_str();
          }
          if (request->hasParam("dh_desc_" + String(i), true)) {
            String v = request->getParam("dh_desc_" + String(i), true)->value();
            v.trim();
            switches[i].description = v.c_str();
          }
          if (request->hasParam("dh_offset_" + String(i), true)) {
            switches[i].dewTargetOffsetC = request->getParam("dh_offset_" + String(i), true)->value().toDouble();
          }
          if (request->hasParam("dh_manual_" + String(i), true)) {
            switches[i].dewTargetManualC = request->getParam("dh_manual_" + String(i), true)->value().toDouble();
          }
          if (request->hasParam("dh_out_" + String(i), true)) {
            int pin = request->getParam("dh_out_" + String(i), true)->value().toInt();
            switches[i].outputPin = (pin >= 0 && pin <= 16) ? pin : -1;
          }
          if (request->hasParam("dh_dht_" + String(i), true)) {
            int pin = request->getParam("dh_dht_" + String(i), true)->value().toInt();
            switches[i].tempInputPin = (pin >= 0 && pin <= 16) ? pin : -1;
          }
          if (request->hasParam("dh_ds_" + String(i), true)) {
            int pin = request->getParam("dh_ds_" + String(i), true)->value().toInt();
            switches[i].heaterTempPin = (pin >= 0 && pin <= 16) ? pin : -1;
          }
          if (request->hasParam("dh_htsens_" + String(i), true)) {
            String sensorType = request->getParam("dh_htsens_" + String(i), true)->value();
            sensorType.toLowerCase();
            switches[i].heaterTempSensorType = (sensorType == "ntc")
                                               ? DewHeaterTempSensorType::NTCThermistor
                                               : DewHeaterTempSensorType::DS18B20;
          }
          if (request->hasParam("dh_dsoffset_" + String(i), true)) {
            switches[i].heaterTempOffsetC = request->getParam("dh_dsoffset_" + String(i), true)->value().toDouble();
          }
          if (request->hasParam("dh_ntc_series_" + String(i), true)) {
            double v = request->getParam("dh_ntc_series_" + String(i), true)->value().toDouble();
            if (v > 0.0) switches[i].ntcSeriesResistorOhm = v;
          }
          if (request->hasParam("dh_ntc_r0_" + String(i), true)) {
            double v = request->getParam("dh_ntc_r0_" + String(i), true)->value().toDouble();
            if (v > 0.0) switches[i].ntcNominalResistanceOhm = v;
          }
          if (request->hasParam("dh_ntc_t0_" + String(i), true)) {
            double v = request->getParam("dh_ntc_t0_" + String(i), true)->value().toDouble();
            if (v > -80.0 && v < 200.0) switches[i].ntcNominalTemperatureC = v;
          }
          if (request->hasParam("dh_ntc_beta_" + String(i), true)) {
            double v = request->getParam("dh_ntc_beta_" + String(i), true)->value().toDouble();
            if (v > 0.0) switches[i].ntcBeta = v;
          }

          if (isValidSwitchId(tempSwitchIdx)) {
            switches[tempSwitchIdx].type = SwitchType::DS18B20Temp;
            switches[tempSwitchIdx].canWrite = false;
            switches[tempSwitchIdx].canAsync = false;
            switches[tempSwitchIdx].isPWM = false;
            switches[tempSwitchIdx].minValue = -55.0;
            switches[tempSwitchIdx].maxValue = 125.0;
            switches[tempSwitchIdx].stepValue = 0.1;
            switches[tempSwitchIdx].outputPin = -1;
            switches[tempSwitchIdx].tempInputPin = -1;
            switches[tempSwitchIdx].heaterTempPin = switches[i].heaterTempPin;
            switches[tempSwitchIdx].heaterTempSensorType = switches[i].heaterTempSensorType;
            switches[tempSwitchIdx].heaterTempOffsetC = switches[i].heaterTempOffsetC;
            switches[tempSwitchIdx].ntcSeriesResistorOhm = switches[i].ntcSeriesResistorOhm;
            switches[tempSwitchIdx].ntcNominalResistanceOhm = switches[i].ntcNominalResistanceOhm;
            switches[tempSwitchIdx].ntcNominalTemperatureC = switches[i].ntcNominalTemperatureC;
            switches[tempSwitchIdx].ntcBeta = switches[i].ntcBeta;
            switches[tempSwitchIdx].name = "DH" + std::to_string(slot) + " Temp";
            switches[tempSwitchIdx].description = "Dew heater " + std::to_string(slot) + " temperature";
            scheduleRecreateDewHeater(tempSwitchIdx);
          }

          scheduleRecreateDewHeater(i);
          scheduleSaveAllToEEPROM();
          message += "Dew heater " + String(slot) + " settings saved.";
        }

        String html = "<html><head><meta http-equiv='refresh' content='2;url=" + setupUrl + "?bank=2'></head><body>";
        html += "<h1>Switch Setup</h1><p>" + message + "</p><p>Redirecting back...</p></body></html>";
        request->send(200, "text/html", html);
        return;
      }

      // --- Dedicated DHT22 sensor configuration page (switches 2-3) ---
      if (request->hasParam("dht22_page", true) || request->hasParam("dth_page", true)) {
        bool anyDthPayload = false;
        for (int i = 2; i <= 3 && i < maxSwitch; i++) {
          bool switchHasPayload = false;

          String nmNew = "dht22_name_" + String(i);
          String nmOld = "dth_name_" + String(i);
          if (request->hasParam(nmNew, true) || request->hasParam(nmOld, true)) {
            anyDthPayload = true;
            switchHasPayload = true;
            String v = request->hasParam(nmNew, true)
                        ? request->getParam(nmNew, true)->value()
                        : request->getParam(nmOld, true)->value();
            v.trim();
            if (v.length() > 0) switches[i].name = v.c_str();
          }

          String dsNew = "dht22_desc_" + String(i);
          String dsOld = "dth_desc_" + String(i);
          if (request->hasParam(dsNew, true) || request->hasParam(dsOld, true)) {
            anyDthPayload = true;
            switchHasPayload = true;
            String v = request->hasParam(dsNew, true)
                        ? request->getParam(dsNew, true)->value()
                        : request->getParam(dsOld, true)->value();
            v.trim();
            switches[i].description = v.c_str();
          }

          // Fixed role mapping: switch 2 = temperature, switch 3 = humidity
          switches[i].type = (i == 2) ? SwitchType::DHT22Temp : SwitchType::DHT22Humidity;

          String pnNew = "dht22_pin_" + String(i);
          String pnOld = "dth_pin_" + String(i);
          if (request->hasParam(pnNew, true) || request->hasParam(pnOld, true)) {
            anyDthPayload = true;
            switchHasPayload = true;
            int pin = request->hasParam(pnNew, true)
                        ? request->getParam(pnNew, true)->value().toInt()
                        : request->getParam(pnOld, true)->value().toInt();
            if (pin < -1 || pin > 16) pin = -1;
            switches[i].tempInputPin = pin;
            if (pin >= 0) {
              pinMode(pin, INPUT);
            }
          }

          String ofNew = "dht22_offset_" + String(i);
          String ofOld = "dth_offset_" + String(i);
          if (request->hasParam(ofNew, true) || request->hasParam(ofOld, true)) {
            anyDthPayload = true;
            switchHasPayload = true;
            switches[i].dhtTempOffsetC = request->hasParam(ofNew, true)
                                          ? request->getParam(ofNew, true)->value().toDouble()
                                          : request->getParam(ofOld, true)->value().toDouble();
          }

          if (switchHasPayload) {
            switches[i].canWrite = false;
            switches[i].canAsync = false;
            switches[i].isPWM = false;
            switches[i].outputPin = -1;
            switches[i].heaterTempPin = -1;
            if (switches[i].type == SwitchType::DHT22Humidity) {
              switches[i].minValue = 0.0;
              switches[i].maxValue = 100.0;
            } else {
              switches[i].minValue = -40.0;
              switches[i].maxValue = 80.0;
            }
            switches[i].stepValue = 0.1;
            scheduleRecreateDewHeater(i);
          }
        }

        String message = anyDthPayload ? "DHT22 sensor settings saved to EEPROM." : "No DHT22 sensor changes detected.";
        if (anyDthPayload) {
          scheduleSaveAllToEEPROM();
        }

        String html = "<html><head><meta http-equiv='refresh' content='2;url=" + setupUrl + "?bank=1'></head><body>";
        html += "<h1>Switch Setup</h1><p>" + message + "</p><p>Redirecting back...</p></body></html>";
        request->send(200, "text/html", html);
        return;
      }

      // --- Per-switch full configuration (bank pages only for switches 0-4) ---
      int startIdx = 0;
      int endIdx = 0;
      if (bank == 0) {
        startIdx = 0;
        endIdx = (maxSwitch < 2) ? maxSwitch : 2;
      } else if (bank == 1) {
        startIdx = 2;
        endIdx = (maxSwitch < 4) ? maxSwitch : 4;
      }

      bool anySwitchPayload = false;
      for (int i = startIdx; i < endIdx; i++) {
        bool switchHasPayload = false;
        // Name
        String nm = "swname_" + String(i);
        if (request->hasParam(nm, true)) {
          anySwitchPayload = true;
          switchHasPayload = true;
          String v = request->getParam(nm, true)->value(); v.trim();
          if (v.length() > 0) switches[i].name = v.c_str();
        }
        // Description
        String ds = "swdesc_" + String(i);
        if (request->hasParam(ds, true)) {
          anySwitchPayload = true;
          switchHasPayload = true;
          String v = request->getParam(ds, true)->value(); v.trim();
          switches[i].description = v.c_str();
        }
        // Type
        String tp = "swtype_" + String(i);
        if (request->hasParam(tp, true)) {
          anySwitchPayload = true;
          switchHasPayload = true;
          int typeInt = request->getParam(tp, true)->value().toInt();
          if (typeInt < (int)SwitchType::Default || typeInt > (int)SwitchType::DewHeaterOutputPct) {
            typeInt = (int)SwitchType::Default;
          }
          // Switches 0-4 are reserved as regular switches; DewHeater is only for dedicated slots 5-7
          if (i <= 4 && typeInt == (int)SwitchType::DewHeater) {
            typeInt = (int)SwitchType::Default;
          }
          switches[i].type = (SwitchType)typeInt;
        }
        // Min / Max / Step
        String mn = "swmin_" + String(i);
        if (request->hasParam(mn, true)) {
          anySwitchPayload = true;
          switchHasPayload = true;
          switches[i].minValue = request->getParam(mn, true)->value().toDouble();
        }
        String mx = "swmax_" + String(i);
        if (request->hasParam(mx, true)) {
          anySwitchPayload = true;
          switchHasPayload = true;
          switches[i].maxValue = request->getParam(mx, true)->value().toDouble();
        }
        String st = "swstep_" + String(i);
        if (request->hasParam(st, true)) {
          anySwitchPayload = true;
          switchHasPayload = true;
          switches[i].stepValue = request->getParam(st, true)->value().toDouble();
        }
        // Current value (only when writable)
        String vl = "swval_" + String(i);
        if (request->hasParam(vl, true)) {
          anySwitchPayload = true;
          switchHasPayload = true;
          double nv = request->getParam(vl, true)->value().toDouble();
          // Clamp to range
          if (nv < switches[i].minValue) nv = switches[i].minValue;
          if (nv > switches[i].maxValue) nv = switches[i].maxValue;
          switches[i].value = nv;
          applyOutput(i);
        }
        // GPIO pin (-1 stored as -1)
        String pn = "swpin_" + String(i);
        if (request->hasParam(pn, true)) {
          anySwitchPayload = true;
          switchHasPayload = true;
          int pin = request->getParam(pn, true)->value().toInt();
          if (pin < 0 || pin > 16) pin = -1;
          if (pin != switches[i].outputPin) {
            switches[i].outputPin = pin;
            if (pin >= 0) {
              pinMode(pin, OUTPUT);
              applyOutput(i);
            }
          }
        }
        // Temperature input GPIO for dew heater
        String tpin = "swtempin_" + String(i);
        if (request->hasParam(tpin, true)) {
          anySwitchPayload = true;
          switchHasPayload = true;
          int pin = request->getParam(tpin, true)->value().toInt();
          if (pin < 0 || pin > 16) pin = -1;
          if (pin != switches[i].tempInputPin) {
            switches[i].tempInputPin = pin;
            if (pin >= 0) {
              pinMode(pin, INPUT);
            }
          }
        }
        // DS18B20 heater temperature GPIO pin
        String hpin = "swheatertemp_" + String(i);
        if (request->hasParam(hpin, true)) {
          anySwitchPayload = true;
          switchHasPayload = true;
          int pin = request->getParam(hpin, true)->value().toInt();
          if (pin < 0 || pin > 16) pin = -1;
          if (pin != switches[i].heaterTempPin) {
            switches[i].heaterTempPin = pin;
            if (pin >= 0) {
              pinMode(pin, INPUT);
            }
          }
        }
        // Checkboxes: absent = unchecked = false
        // only change checkbox values if at least one field for this switch was posted
        if (switchHasPayload) {
          switches[i].canWrite  = request->hasParam("swrw_"    + String(i), true);
          switches[i].canAsync  = request->hasParam("swasync_" + String(i), true);
          switches[i].isPWM     = request->hasParam("swpwm_"   + String(i), true);
          scheduleRecreateDewHeater(i);
          applyOutput(i);
        }
      }

      // Persist changes when switch payload exists
      if (anySwitchPayload) {
        scheduleSaveAllToEEPROM();
        message += "Switch settings saved to EEPROM.";
      }

      String returnUrl = setupUrl;
      if (bank == 0 || bank == 1) {
        returnUrl += "?bank=" + String(bank);
      }

      String html = "<html><head><meta http-equiv='refresh' content='2;url=" + returnUrl + "'></head><body>";
      html += "<h1>Switch Setup</h1><p>" + message + "</p><p>Redirecting back...</p></body></html>";
      request->send(200, "text/html", html);
      return;
    }

    // ---- GET: styled setup page ----
    LOG_INFO("Serving switch setup page");
    int bank = getRequestedBank(request, false);

    // Main page (general settings + links only)
    if (bank < 0) {
      String html;
      if (!html.reserve(3500)) {
        String minimal = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Switch Setup</title></head><body>";
        minimal += "<h2>Switch Setup</h2><p>Low memory fallback page.</p>";
        minimal += "<p><a href='" + setupUrl + "?bank=0'>Open Custom Switch 0-1</a></p>";
        minimal += "<p><a href='" + setupUrl + "?bank=1'>Open DHT22 Sensors 2-3</a></p>";
        minimal += "<p><a href='" + setupUrl + "?bank=2'>Open Dew Heater 1-2</a></p>";
        minimal += "</body></html>";
        request->send(200, "text/html", minimal);
        return;
      }
      html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'><title>Switch Setup</title></head><body>";
      html += "<h1>Switch Setup - " + escapeHtml(GetDeviceName()) + "</h1>";
      html += "<h2>Pages</h2><p><a href='" + setupUrl + "?bank=0'>Custom Switch 0-1</a></p>";
      html += "<p><a href='" + setupUrl + "?bank=1'>DHT22 Sensors 2-3</a></p>";
      html += "<p><a href='" + setupUrl + "?bank=2'>Dew Heater 1-2</a></p>";

      html += "<h2>WiFi</h2><form method='POST' action='" + setupUrl + "'>";
      html += "<label>SSID</label><input type='text' name='wifi_ssid' maxlength='31' value='" + escapeHtml(String(wifiConfig.getSSID())) + "' required>";
      html += "<label>Password</label><input type='password' name='wifi_password' maxlength='63' value='' placeholder='Leave empty to keep current'>";
      html += "<p><input type='submit' value='Save WiFi Settings'></p></form>";

      html += "<h2>LED</h2><form method='POST' action='" + setupUrl + "'>";
      html += "<input type='hidden' name='led_page' value='1'>";
      html += "<label><input type='checkbox' name='led_disable'" + String(ledDisabled ? " checked" : "") + "> Disable onboard LED (GPIO2 HIGH)</label>";
      html += "<p><input type='submit' value='Save LED Setting'></p></form>";
      html += "</body></html>";
      request->send(200, "text/html", html);
      return;
    }

    if (bank == 2) {
      yield(); // let lwIP/WiFi process pending events before allocating response
      if (ESP.getFreeHeap() < 8000) {
        request->send(200, "text/html",
          "<html><body><h2>Dew Heater Setup</h2>"
          "<p>Not enough free memory to render this page right now.</p>"
          "<p><a href='" + setupUrl + "?bank=2'>Reload</a> &nbsp; "
          "<a href='" + setupUrl + "'>Main Setup</a></p></body></html>");
        return;
      }
      AsyncResponseStream *response = request->beginResponseStream("text/html");
      if (response == nullptr) {
        request->send(500, "text/plain", "Failed to create response");
        return;
      }

      response->print("<!DOCTYPE html><html><head>");
      response->print("<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>");
      response->print("<title>Dew Heater Setup</title>");
      response->print("<style>body{font-family:Arial,sans-serif;margin:20px;} .card{border:1px solid #ddd;padding:12px;margin-bottom:14px;} label{display:block;margin:6px 0 2px;} input,select{width:100%;padding:6px;box-sizing:border-box;} .help{font-size:.85em;color:#666;}</style></head><body>");
      response->print("<h1>Dew Heater Setup &mdash; Switches 4-7</h1>");

      for (int slot = 1; slot <= 2; slot++) {
        int i = getDewHeaterSwitchIndexFromSlot(slot);
        int tempSwitchIdx = getDewHeaterTempSwitchIndexFromSlot(slot);
        if (!isValidSwitchId(i)) continue;

        response->print("<div class='card'><h2>Dew Heater ");
        response->print(String(slot));
        response->print("</h2><p class='help'>Uses switch ");
        response->print(String(i));
        response->print(" for control and switch ");
        response->print(String(tempSwitchIdx));
        response->print(" for actual heater temperature.</p>");
        response->print("<form method='POST' action='");
        response->print(setupUrl);
        response->print("'><input type='hidden' name='dh_slot' value='");
        response->print(slot);  // int, no heap
        response->print("'>");

        response->print("<label>Name</label><input type='text' name='dh_name_");
        response->print(i);
        response->print("' maxlength='15' value='");
        response->print(switches[i].name.c_str());
        response->print("'>");

        response->print("<label>Description</label><input type='text' name='dh_desc_");
        response->print(i);
        response->print("' maxlength='20' value='");
        response->print(switches[i].description.c_str());
        response->print("'>");

        response->print("<label>Mode</label><select name='dh_mode_");
        response->print(i);
        response->print("'><option value='automatic'");
        response->print(switches[i].dewHeaterMode == DewHeaterMode::Automatic ? " selected" : "");
        response->print(">Automatic</option><option value='manual'");
        response->print(switches[i].dewHeaterMode == DewHeaterMode::Manual ? " selected" : "");
        response->print(">Manual</option></select>");

        { char fb[16];
        response->print("<label>Target Temp Offset over Dew Point (°C)</label><input type='number' step='0.1' name='dh_offset_");
        response->print(i);
        response->print("' value='");
        dtostrf(switches[i].dewTargetOffsetC, 4, 2, fb); response->print(fb);
        response->print("'>");

        response->print("<label>Target Temp Manual (°C)</label><input type='number' step='0.1' name='dh_manual_");
        response->print(i);
        response->print("' value='");
        dtostrf(switches[i].dewTargetManualC, 4, 2, fb); response->print(fb);
        response->print("'>"); }

        response->print("<label>Output Pin</label><input type='number' min='-1' max='16' name='dh_out_");
        response->print(i);
        response->print("' value='");
        response->print(switches[i].outputPin);
        response->print("'>");

        response->print("<label>DHT22/AM2302 Sensor Pin</label><input type='number' min='-1' max='16' name='dh_dht_");
        response->print(i);
        response->print("' value='");
        response->print(switches[i].tempInputPin);
        response->print("'>");

        response->print("<label>Heater Temperature Sensor Type</label><select name='dh_htsens_");
        response->print(i);
        response->print("'><option value='ds18b20'");
        response->print(switches[i].heaterTempSensorType == DewHeaterTempSensorType::DS18B20 ? " selected" : "");
        response->print(">DS18B20</option><option value='ntc'");
        response->print(switches[i].heaterTempSensorType == DewHeaterTempSensorType::NTCThermistor ? " selected" : "");
        response->print(">NTC Thermistor</option></select>");

        response->print("<label>Heater Temperature Sensor Pin</label><input type='number' min='-1' max='16' name='dh_ds_");
        response->print(i);
        response->print("' value='");
        response->print(switches[i].heaterTempPin);
        response->print("'>");

        { char fb[16];
        response->print("<label>Heater Temperature Offset (°C)</label><input type='number' step='0.1' name='dh_dsoffset_");
        response->print(i);
        response->print("' value='");
        dtostrf(switches[i].heaterTempOffsetC, 4, 2, fb); response->print(fb);
        response->print("'>"); }

        { char fb[16];
        response->print("<label>NTC Fixed Resistor to GND (Ohm)</label><input type='number' step='1' min='1' name='dh_ntc_series_");
        response->print(i);
        response->print("' value='");
        dtostrf(switches[i].ntcSeriesResistorOhm, 6, 1, fb); response->print(fb);
        response->print("'>");

        response->print("<label>NTC Nominal Resistance R0 (Ohm)</label><input type='number' step='1' min='1' name='dh_ntc_r0_");
        response->print(i);
        response->print("' value='");
        dtostrf(switches[i].ntcNominalResistanceOhm, 6, 1, fb); response->print(fb);
        response->print("'>");

        response->print("<label>NTC Nominal Temperature T0 (°C)</label><input type='number' step='0.1' name='dh_ntc_t0_");
        response->print(i);
        response->print("' value='");
        dtostrf(switches[i].ntcNominalTemperatureC, 4, 1, fb); response->print(fb);
        response->print("'>");

        response->print("<label>NTC Beta</label><input type='number' step='1' min='1' name='dh_ntc_beta_");
        response->print(i);
        response->print("' value='");
        dtostrf(switches[i].ntcBeta, 6, 1, fb); response->print(fb); }
        response->print("'><div class='help'>Wiring: VCC -> NTC -> A0 -> fixed resistor to GND. Defaults: fixed resistor=10000, R0=10000, T0=25, Beta=3950.</div>");

        if (dewHeaters[i] != nullptr) {
          char fb[16];
          response->print("<label>Current Heater Temperature</label><input type='text' value='");
          if (dewHeaters[i]->isHeaterTemperatureValid()) { dtostrf(dewHeaters[i]->getHeaterTemperatureC(), 4, 2, fb); response->print(fb); } else { response->print("n/a"); }
          response->print("' readonly>");
          response->print("<label>Current Dew Point</label><input type='text' value='");
          if (dewHeaters[i]->isSensorValid()) { dtostrf(dewHeaters[i]->getDewPointC(), 4, 2, fb); response->print(fb); } else { response->print("n/a"); }
          response->print("' readonly>");
        }

        response->print("<p><input type='submit' value='Save Dew Heater ");
        response->print(slot);  // int, no heap
        response->print("'></p></form></div>");
      }

      response->print("<p><a href='");
      response->print(setupUrl);
      response->print("'>Back to Main Setup</a></p></body></html>");
      request->send(response);
      return;
    }

    if (bank == 1) {
      String html;
      if (!html.reserve(7000)) {
        String minimal = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>DHT22 Setup</title></head><body>";
        minimal += "<h2>DHT22 Setup</h2><p>Not enough memory to render full page. Please reboot device.</p>";
        minimal += "<p><a href='" + setupUrl + "?bank=1'>Reload</a></p></body></html>";
        request->send(200, "text/html", minimal);
        return;
      }

      html = "<!DOCTYPE html><html><head>";
      html += "<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>";
      html += "<title>DHT22 Sensor Setup</title>";
      html += "<style>body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0;}";
      html += ".container{max-width:720px;margin:0 auto;background:#fff;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,.1);}";
      html += ".card{background:#f9f9f9;padding:15px;border-radius:6px;margin-bottom:14px;}";
      html += "label{display:block;margin:8px 0 4px;color:#555;font-size:.9em;}";
      html += "input[type='text'],input[type='number'],select{width:100%;padding:8px;border:1px solid #ccc;border-radius:4px;box-sizing:border-box;}";
      html += "input[type='submit']{margin-top:10px;background:#0066cc;color:#fff;border:none;border-radius:4px;padding:9px 16px;cursor:pointer;}";
      html += ".grid{display:grid;grid-template-columns:1fr 1fr;gap:10px 14px;} .full{grid-column:1/-1;} .help{font-size:.85em;color:#666;margin-top:4px;}";
      html += "</style></head><body><div class='container'>";
      html += "<h1>DHT22 Sensor Setup &mdash; Switches 2-3</h1>";
      html += "<div class='card'><p>Configure fixed DHT22/AM2302 roles: switch 2 = temperature, switch 3 = humidity.</p></div>";
      html += "<form method='POST' action='" + setupUrl + "'>";
      html += "<input type='hidden' name='dht22_page' value='1'>";
      html += "<input type='hidden' name='bank' value='1'>";

      for (int i = 2; i <= 3 && i < maxSwitch; i++) {
        html += "<div class='card'><h2>Switch " + String(i) + " (" + String(i == 2 ? "Temperature" : "Humidity") + ")</h2><div class='grid'>";
        html += "<div class='full'><label>Name</label><input type='text' name='dht22_name_" + String(i) + "' maxlength='15' value='" + escapeHtml(String(switches[i].name.c_str())) + "'></div>";
        html += "<div class='full'><label>Description</label><input type='text' name='dht22_desc_" + String(i) + "' maxlength='20' value='" + escapeHtml(String(switches[i].description.c_str())) + "'></div>";
        html += "<div><label>Mode</label><input type='text' value='" + String(i == 2 ? "Temperature" : "Humidity") + "' readonly></div>";
        html += "<div><label>DHT22/AM2302 Data Pin</label><input type='number' min='-1' max='16' name='dht22_pin_" + String(i) + "' value='" + String(switches[i].tempInputPin) + "'></div>";
        html += "<div><label>Temperature Offset (°C)</label><input type='number' step='0.1' name='dht22_offset_" + String(i) + "' value='" + String(switches[i].dhtTempOffsetC, 2) + "'></div>";
        html += "<div><label>Current Value</label><input type='text' value='" + String(switches[i].value, 2) + "' readonly></div>";
        html += "<div class='full help'>Temperature offset is applied only on switch 2 (temperature).</div>";
        html += "</div></div>";
      }

      if (maxSwitch > 8) {
        html += "<div class='card'><h2>Switch 8 (Dew Point)</h2><div class='grid'>";
        html += "<div class='full'><label>Name</label><input type='text' value='" + escapeHtml(String(switches[8].name.c_str())) + "' readonly></div>";
        html += "<div class='full'><label>Description</label><input type='text' value='" + escapeHtml(String(switches[8].description.c_str())) + "' readonly></div>";
        html += "<div><label>Mode</label><input type='text' value='Calculated Dew Point' readonly></div>";
        html += "<div><label>DHT22/AM2302 Data Pin</label><input type='number' value='" + String(switches[8].tempInputPin) + "' readonly></div>";
        html += "<div><label>Current Value (°C)</label><input type='text' value='" + String(switches[8].value, 2) + "' readonly></div>";
        html += "<div class='full help'>This switch is read-only and automatically calculated from ambient temperature and humidity.</div>";
        html += "</div></div>";
      }

      html += "<input type='submit' value='Save DHT22 Sensor Settings'>";
      html += "</form>";
      html += "<p><a href='" + setupUrl + "'>Back to Main Setup</a></p>";
      html += "<p><a href='" + setupUrl + "?bank=0'>Open Custom Switch 0-1</a></p>";
      html += "</div></body></html>";
      request->send(200, "text/html", html);
      return;
    }

    String html;
    if (!html.reserve(8500)) {
      String minimal = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>Switch Setup</title></head><body>";
      minimal += "<h2>Switch Setup</h2><p>Not enough memory to render full page. Please reduce switch count or reboot device.</p>";
      minimal += "<p><a href='" + setupUrl + "'>Reload</a></p></body></html>";
      request->send(200, "text/html", minimal);
      return;
    }
    html = "<!DOCTYPE html><html><head>";
    html += "<meta charset='UTF-8'>";
    html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
    html += "<title>Switch Setup</title>";
    html += "<style>";
    html += "body{font-family:Arial,sans-serif;margin:20px;background-color:#f0f0f0;}";
    html += "h1{color:#333;}";
    html += ".container{max-width:720px;margin:0 auto;background-color:white;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,.1);}";
    html += ".info-section{background-color:#e8f4f8;padding:15px;border-radius:5px;margin-bottom:20px;}";
    html += ".info-row{display:flex;justify-content:space-between;margin:6px 0;}";
    html += ".info-label{font-weight:bold;color:#555;}";
    html += ".info-value{color:#0066cc;}";
    html += ".form-section{background-color:#f9f9f9;padding:15px;border-radius:5px;margin-bottom:15px;}";
    html += "h2{color:#555;font-size:1.15em;margin-top:0;}";
    html += ".sw-card{border:1px solid #ddd;border-radius:6px;padding:12px;margin-bottom:12px;background:#fff;}";
    html += ".sw-title{font-weight:bold;font-size:1em;color:#0066cc;margin-bottom:8px;}";
    html += ".sw-grid{display:grid;grid-template-columns:1fr 1fr;gap:6px 14px;}";
    html += ".sw-full{grid-column:1/-1;}";
    html += "label{font-size:.85em;color:#555;display:block;margin-bottom:1px;}";
    html += "input[type='text'],input[type='number']{width:100%;padding:5px 7px;border:1px solid #ccc;border-radius:4px;box-sizing:border-box;font-size:.9em;}";
    html += "input[type='password']{width:100%;padding:6px 8px;border:1px solid #ccc;border-radius:4px;box-sizing:border-box;}";
    html += ".cb-row{display:flex;gap:16px;align-items:center;margin-top:4px;}";
    html += ".cb-row label{font-size:.85em;color:#444;display:flex;align-items:center;gap:4px;margin:0;}";
    html += "input[type='submit']{background-color:#0066cc;color:white;padding:9px 20px;border:none;border-radius:4px;cursor:pointer;margin-top:10px;font-size:.95em;}";
    html += "input[type='submit']:hover{background-color:#0052a3;}";
    html += ".on{color:#00aa00;font-weight:bold;} .off{color:#cc0000;font-weight:bold;}";
    html += ".help-text{font-size:.85em;color:#888;margin-top:3px;}";
    html += ".type-help{background:#eef6ff;border-left:4px solid #0066cc;padding:8px 10px;border-radius:4px;color:#355;font-size:.84em;grid-column:1/-1;}";
    html += "</style>";
    html += "</head><body><div class='container'>";
    html += "<h1>Switch Setup &mdash; " + GetDeviceName() + "</h1>";

    // Status
    html += "<div class='info-section'><h2>Device Status</h2>";
    html += "<div class='info-row'><span class='info-label'>Page:</span><span class='info-value'>Custom Switch 0-1</span></div>";
    html += "<div class='info-row'><span class='info-label'>Device Number:</span><span class='info-value'>" + String(GetDeviceNumber()) + "</span></div>";
    html += "<div class='info-row'><span class='info-label'>Switches:</span><span class='info-value'>" + String(maxSwitch) + "</span></div>";
    html += "</div>";

    // WiFi status
    html += "<div class='info-section'><h2>WiFi Status</h2>";
    if (WiFi.getMode() == WIFI_AP) {
      html += "<div class='info-row'><span class='info-label'>Mode:</span><span class='info-value' style='color:#ff9900;'>Access Point</span></div>";
      html += "<div class='info-row'><span class='info-label'>AP SSID:</span><span class='info-value'>" + String(WiFi.softAPSSID()) + "</span></div>";
      html += "<div class='info-row'><span class='info-label'>IP:</span><span class='info-value'>" + WiFi.softAPIP().toString() + "</span></div>";
    } else {
      html += "<div class='info-row'><span class='info-label'>Mode:</span><span class='info-value' style='color:#00aa00;'>Station</span></div>";
      html += "<div class='info-row'><span class='info-label'>SSID:</span><span class='info-value'>" + String(WiFi.SSID()) + "</span></div>";
      html += "<div class='info-row'><span class='info-label'>IP:</span><span class='info-value'>" + WiFi.localIP().toString() + "</span></div>";
      html += "<div class='info-row'><span class='info-label'>Signal:</span><span class='info-value'>" + String(WiFi.RSSI()) + " dBm</span></div>";
    }
    html += "<div class='info-row'><span class='info-label'>Hostname:</span><span class='info-value'>" + String(WiFi.hostname()) + "</span></div>";
    html += "</div>";

    // Switch configuration form (one form, all switches as cards)
    html += "<div class='form-section'>";
    html += "<form method='POST' action='" + setupUrl + "'>";
    html += "<input type='hidden' name='bank' value='" + String(bank) + "'>";
    html += "<h2>Switch Configuration</h2>";
    html += "<p class='help-text'>All changes are saved to EEPROM and survive reboots.</p>";

    int fromIdx = 0;
    int toIdx = 2;
    if (fromIdx > maxSwitch) fromIdx = maxSwitch;
    if (toIdx > maxSwitch) toIdx = maxSwitch;

    for (int i = fromIdx; i < toIdx; i++) {
      bool isOn = switches[i].value > 0.0;
      html += "<div class='sw-card'>";
      html += "<div class='sw-title'>Switch " + String(i) + " &nbsp;<span class='" + String(isOn?"on":"off") + "'>" + String(isOn?"ON":"OFF") + "</span></div>";
      html += "<div class='sw-grid'>";

      // Name
      html += "<div class='sw-full'><label>Name (max 15 chars)</label>";
      html += "<input type='text' name='swname_" + String(i) + "' value='" + escapeHtml(String(switches[i].name.c_str())) + "' maxlength='15'></div>";

      // Description
      html += "<div class='sw-full'><label>Description (max 20 chars)</label>";
      html += "<input type='text' name='swdesc_" + String(i) + "' value='" + escapeHtml(String(switches[i].description.c_str())) + "' maxlength='20'></div>";

      // Type
      html += "<div class='sw-full'><label>Switch Type</label>";
      html += "<select name='swtype_" + String(i) + "' style='width:100%;padding:5px 7px;border:1px solid #ccc;border-radius:4px;box-sizing:border-box;font-size:.9em;'>";
      html += "<option value='0'" + String(switches[i].type == SwitchType::Default ? " selected" : "") + ">Default</option>";
      html += "<option value='2'" + String(switches[i].type == SwitchType::DHT22Temp ? " selected" : "") + ">DHT22 Temp</option>";
      html += "<option value='3'" + String(switches[i].type == SwitchType::DHT22Humidity ? " selected" : "") + ">DHT22 Humidity</option>";
      html += "<option value='4'" + String(switches[i].type == SwitchType::DS18B20Temp ? " selected" : "") + ">DS18B20 Temp</option>";
      html += "</select></div>";
      html += "<div class='type-help' id='swtypehelp_" + String(i) + "'>" + escapeHtml(getSwitchTypeHelpText(switches[i].type)) + "</div>";

      // Min / Max / Step / Value
      html += "<div><label>Min Value</label>";
      html += "<input type='number' step='any' name='swmin_" + String(i) + "' value='" + String(switches[i].minValue, 4) + "'></div>";

      html += "<div><label>Max Value</label>";
      html += "<input type='number' step='any' name='swmax_" + String(i) + "' value='" + String(switches[i].maxValue, 4) + "'></div>";

      html += "<div><label>Step Size</label>";
      html += "<input type='number' step='any' name='swstep_" + String(i) + "' value='" + String(switches[i].stepValue, 4) + "'></div>";

      html += "<div><label>Current Value</label>";
      if (switches[i].canWrite) {
        html += "<input type='number' step='any' name='swval_" + String(i) + "' value='" + String(switches[i].value, 4) + "'>";
      } else {
        html += "<input type='number' step='any' name='swval_" + String(i) + "' value='" + String(switches[i].value, 4) + "' readonly>";
      }
      html += "</div>";

      // GPIO pin
      html += "<div><label>GPIO Pin (-1 = none)</label>";
      html += "<input type='number' name='swpin_" + String(i) + "' value='" + String(switches[i].outputPin) + "' min='-1' max='16'></div>";

      // Checkboxes
      html += "<div class='sw-grid' style='grid-column:1/-1;'>";
      html += "<div class='cb-row'>";
      html += "<label><input type='checkbox' name='swrw_"    + String(i) + "'" + String(switches[i].canWrite  ? " checked" : "") + "> Can Write (R/W)</label>";
      html += "<label><input type='checkbox' name='swasync_" + String(i) + "'" + String(switches[i].canAsync  ? " checked" : "") + "> Async (ISwitchV3+)</label>";
      html += "<label><input type='checkbox' name='swpwm_"   + String(i) + "'" + String(switches[i].isPWM     ? " checked" : "") + "> PWM (analog) output</label>";
      html += "</div></div>";

      html += "</div></div>"; // sw-grid / sw-card
    }

    html += "<input type='submit' value='Save All Switches'>";
    html += "</form></div>";

    html += "<p><a href='" + setupUrl + "'>Back to Main Setup</a></p>";
    html += "<p><a href='" + setupUrl + "?bank=1'>Open DHT22 Sensors 2-3</a></p>";

    html += "<script>";
    html += "function getSwitchTypeHelp(v){switch(String(v)){";
    html += "case '1': return 'GPIO Pin = heater PWM output. Temp Input GPIO = DHT22/AM2302 ambient sensor. Heater Temp GPIO = DS18B20 on heater. Current Value = target offset above dew point in °C.';";
    html += "case '2': return 'Temp Input GPIO = DHT22/AM2302 data pin. Current Value is read-only live ambient temperature in °C.';";
    html += "case '3': return 'Temp Input GPIO = DHT22/AM2302 data pin. Current Value is read-only live relative humidity in %.';";
    html += "case '4': return 'Heater Temp GPIO = DS18B20 data pin. Current Value is read-only live temperature in °C.';";
    html += "default: return 'GPIO Pin controls the output. Current Value is the switch value. Sensor GPIO fields are optional and unused in default mode.';";
    html += "}}";
    html += "function bindSwitchTypeHelp(i){var sel=document.getElementsByName('swtype_'+i)[0];var box=document.getElementById('swtypehelp_'+i);if(!sel||!box)return;var upd=function(){box.textContent=getSwitchTypeHelp(sel.value);};sel.addEventListener('change',upd);upd();}";
    for (int i = fromIdx; i < toIdx; i++) {
      html += "bindSwitchTypeHelp(" + String(i) + ");";
    }
    html += "</script>";

    html += "<p style='text-align:center;color:#aaa;font-size:.85em;margin-top:24px;'>";
    html += "<a href='/management/v1/description' style='color:#0066cc;text-decoration:none;'>Back to Management API</a>";
    html += "</p>";
    html += "</div></body></html>";
    request->send(200, "text/html", html);
  }
};

#endif // ARDUINO_SWITCH_H
