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
  double safetyMinTempC;     // Heater temp safety minimum (°C) — shuts off below this
  double safetyMaxTempC;     // Heater temp safety maximum (°C) — shuts off above this
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
  static const int      EEPROM_LED_ADDR      = EEPROM_SW_DATA_ADDR + EEPROM_SW_MAX * EEPROM_SW_ENTRY_SIZE; // 1 byte: 0xAB=disabled
  static const int      EEPROM_DH_SAFETY_ADDR = EEPROM_LED_ADDR + 1; // 4 floats: slot1_min, slot1_max, slot2_min, slot2_max (16 bytes)

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
        // Safety temperature check — shut off if heater temp is out of configured range
        if (heater->isHeaterTemperatureValid()) {
          float htC = heater->getHeaterTemperatureC();
          if (switches[id].enabled &&
              (htC < (float)switches[id].safetyMinTempC || htC > (float)switches[id].safetyMaxTempC)) {
            LOG_WARN("DewHeater " + String(id) + " safety shutoff: T=" + String(htC) +
                     " range=[" + String(switches[id].safetyMinTempC) + "," +
                     String(switches[id].safetyMaxTempC) + "]");
            switches[id].enabled = false;
            heater->enablePidControl(false);
            heater->setHeaterPowerPercent(0.0f);
          }
        }
        if (!switches[id].enabled) {
          // Switch disabled (Alpaca API or safety shutoff) — ensure heater output is off
          if (heater->isPidEnabled()) heater->enablePidControl(false);
          heater->setHeaterPowerPercent(0.0f);
        } else {
          if (!heater->isPidEnabled()) {
            heater->enablePidControl(true);
          }
          heater->setHeaterTemperatureOffsetC((float)switches[id].heaterTempOffsetC);
          if (switches[id].dewHeaterMode == DewHeaterMode::Manual) {
            heater->setTargetHeaterTemperatureC((float)switches[id].dewTargetManualC);
          } else {
            heater->setTargetAboveDewPointC((float)switches[id].dewTargetOffsetC);
          }
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
    // LED disable flag
    uint8_t ledFlag = 0;
    EEPROM.get(EEPROM_LED_ADDR, ledFlag);
    ledDisabled = (ledFlag == 0xAB);
    if (ledDisabled) { pinMode(2, OUTPUT); digitalWrite(2, HIGH); }
    // Safety temperature limits for dew heater slots
    for (int slot = 1; slot <= 2; slot++) {
      int si = getDewHeaterSwitchIndexFromSlot(slot);
      if (!isValidSwitchId(si)) continue;
      float sMin = 0.0f, sMax = 50.0f;
      EEPROM.get(EEPROM_DH_SAFETY_ADDR + (slot - 1) * 2 * sizeof(float), sMin);
      EEPROM.get(EEPROM_DH_SAFETY_ADDR + (slot - 1) * 2 * sizeof(float) + sizeof(float), sMax);
      if (sMin > -50.0f && sMin < 200.0f) switches[si].safetyMinTempC = (double)sMin;
      if (sMax > -50.0f && sMax < 200.0f) switches[si].safetyMaxTempC = (double)sMax;
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
    // LED disable flag
    EEPROM.put(EEPROM_LED_ADDR, (uint8_t)(ledDisabled ? 0xAB : 0x00));
    // Safety temperature limits for dew heater slots
    for (int slot = 1; slot <= 2; slot++) {
      int si = getDewHeaterSwitchIndexFromSlot(slot);
      if (!isValidSwitchId(si)) continue;
      float sMin = (float)switches[si].safetyMinTempC;
      float sMax = (float)switches[si].safetyMaxTempC;
      EEPROM.put(EEPROM_DH_SAFETY_ADDR + (slot - 1) * 2 * sizeof(float), sMin);
      EEPROM.put(EEPROM_DH_SAFETY_ADDR + (slot - 1) * 2 * sizeof(float) + sizeof(float), sMax);
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
      switches[4].minValue = 0.0;
      switches[4].maxValue = 1.0;
      switches[4].stepValue = 1.0;
      switches[4].isPWM = true;
    }
    if (maxSwitch > 5) {
      switches[5].name = "Guide DewHeater";
      switches[5].description = "Guide Cam Dew Heater";
      switches[5].type = SwitchType::DewHeater;
      switches[5].canWrite = true;
      switches[5].canAsync = false;
      switches[5].minValue = 0.0;
      switches[5].maxValue = 1.0;
      switches[5].stepValue = 1.0;
      switches[5].isPWM = true;
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
      switches[i].safetyMinTempC = 0.0;
      switches[i].safetyMaxTempC = 50.0;
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
      switches[4].heaterTempPin = 17;
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
      switches[5].heaterTempPin = 17;
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

    // Register dedicated per-slot dew heater setup endpoints so each page is small
    String dhBase = "/setup/v1/switch/" + String(GetDeviceNumber()) + "/dewheater";
    server.on((dhBase + "/1").c_str(), HTTP_GET,
      [this](AsyncWebServerRequest *r){ this->dewHeaterSlotGetHandler(1, r); });
    server.on((dhBase + "/1").c_str(), HTTP_POST,
      [this](AsyncWebServerRequest *r){ this->dewHeaterSlotPostHandler(1, r); });
    server.on((dhBase + "/2").c_str(), HTTP_GET,
      [this](AsyncWebServerRequest *r){ this->dewHeaterSlotGetHandler(2, r); });
    server.on((dhBase + "/2").c_str(), HTTP_POST,
      [this](AsyncWebServerRequest *r){ this->dewHeaterSlotPostHandler(2, r); });

    // Dedicated DHT22 sensor setup endpoint
    String dht22Base = "/setup/v1/switch/" + String(GetDeviceNumber()) + "/dht22";
    server.on(dht22Base.c_str(), HTTP_GET,
      [this](AsyncWebServerRequest *r){ this->dht22GetHandler(r); });
    server.on(dht22Base.c_str(), HTTP_POST,
      [this](AsyncWebServerRequest *r){ this->dht22PostHandler(r); });

    // Dedicated custom switch setup endpoints (switch 0 and 1)
    String csBase = "/setup/v1/switch/" + String(GetDeviceNumber()) + "/customswitch";
    server.on((csBase + "/0").c_str(), HTTP_GET,
      [this](AsyncWebServerRequest *r){ this->customSwitchGetHandler(0, r); });
    server.on((csBase + "/0").c_str(), HTTP_POST,
      [this](AsyncWebServerRequest *r){ this->customSwitchPostHandler(0, r); });
    server.on((csBase + "/1").c_str(), HTTP_GET,
      [this](AsyncWebServerRequest *r){ this->customSwitchGetHandler(1, r); });
    server.on((csBase + "/1").c_str(), HTTP_POST,
      [this](AsyncWebServerRequest *r){ this->customSwitchPostHandler(1, r); });
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

  // ==================== Dew Heater Slot Handlers ====================

  /** Dedicated GET handler for a single dew heater slot (1 or 2). */
  void dewHeaterSlotGetHandler(int slot, AsyncWebServerRequest *request) {
    enforceReservedSwitchRoles();
    int i = getDewHeaterSwitchIndexFromSlot(slot);
    int tempSwitchIdx = getDewHeaterTempSwitchIndexFromSlot(slot);
    String setupUrl = "/setup/v1/switch/" + String(GetDeviceNumber()) + "/setup";
    String slotUrl  = "/setup/v1/switch/" + String(GetDeviceNumber()) + "/dewheater/" + String(slot);

    if (!isValidSwitchId(i)) {
      request->send(400, "text/plain", "Invalid dew heater slot");
      return;
    }
    if (ESP.getFreeHeap() < 5000) {
      request->send(200, "text/html",
        "<html><body><h2>Dew Heater " + String(slot) + " Setup</h2>"
        "<p>Low memory. <a href='" + slotUrl + "'>Reload</a></p></body></html>");
      return;
    }

    AsyncResponseStream *response = request->beginResponseStream("text/html");
    if (response == nullptr) { request->send(500, "text/plain", "Failed to create response"); return; }

    response->print("<!DOCTYPE html><html><head>");
    response->print("<meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>");
    response->print("<title>Dew Heater "); response->print(slot); response->print(" Setup</title>");
    response->print("<style>body{font-family:Arial,sans-serif;margin:20px;} .card{border:1px solid #ddd;padding:12px;margin-bottom:14px;} label{display:block;margin:6px 0 2px;} input,select{width:100%;padding:6px;box-sizing:border-box;} .help{font-size:.85em;color:#666;}</style></head><body>");
    response->print("<h1>Dew Heater "); response->print(slot); response->print(" Setup</h1>");
    response->print("<p><a href='"); response->print(setupUrl); response->print("?bank=2'>Back to Dew Heater Overview</a> &nbsp; <a href='"); response->print(setupUrl); response->print("'>Main Setup</a></p>");

    response->print("<div class='card'>");
    response->print("<p class='help'>Switch "); response->print(i);
    response->print(" = heater control &nbsp;|&nbsp; Switch "); response->print(tempSwitchIdx);
    response->print(" = heater temperature</p>");
    response->print("<form method='POST' action='"); response->print(slotUrl); response->print("'>");

    response->print("<label>Name</label><input type='text' name='dh_name' maxlength='15' value='");
    response->print(switches[i].name.c_str()); response->print("'>");

    response->print("<label>Description</label><input type='text' name='dh_desc' maxlength='20' value='");
    response->print(switches[i].description.c_str()); response->print("'>");

    response->print("<label>Mode</label><select name='dh_mode'>");
    response->print(switches[i].dewHeaterMode == DewHeaterMode::Automatic
      ? "<option value='automatic' selected>Automatic</option><option value='manual'>Manual</option>"
      : "<option value='automatic'>Automatic</option><option value='manual' selected>Manual</option>");
    response->print("</select>");

    { char fb[16];
      response->print("<label>Target Temp Offset over Dew Point (&deg;C)</label><input type='number' step='0.1' name='dh_offset' value='");
      dtostrf(switches[i].dewTargetOffsetC, 4, 2, fb); response->print(fb); response->print("'>");

      response->print("<label>Target Temp Manual (&deg;C)</label><input type='number' step='0.1' name='dh_manual' value='");
      dtostrf(switches[i].dewTargetManualC, 4, 2, fb); response->print(fb); response->print("'>");
    }

    response->print("<label>Output Pin</label><input type='number' min='-1' max='16' name='dh_out' value='");
    response->print(switches[i].outputPin); response->print("'>");

    response->print("<label>DHT22/AM2302 Sensor Pin</label><input type='number' min='-1' max='16' name='dh_dht' value='");
    response->print(switches[i].tempInputPin); response->print("'>");

    response->print("<label>Heater Temperature Sensor Type</label><select name='dh_htsens'>");
    response->print(switches[i].heaterTempSensorType == DewHeaterTempSensorType::DS18B20
      ? "<option value='ds18b20' selected>DS18B20</option><option value='ntc'>NTC Thermistor</option>"
      : "<option value='ds18b20'>DS18B20</option><option value='ntc' selected>NTC Thermistor</option>");
    response->print("</select>");

    response->print("<label>Heater Temperature Sensor Pin (17 = A0/NTC)</label><input type='number' min='-1' max='17' name='dh_ds' value='");
    response->print(switches[i].heaterTempPin); response->print("'>");

    { char fb[16];
      response->print("<label>Heater Temperature Offset (&deg;C)</label><input type='number' step='0.1' name='dh_dsoffset' value='");
      dtostrf(switches[i].heaterTempOffsetC, 4, 2, fb); response->print(fb); response->print("'>");

      response->print("<label>NTC Fixed Resistor to GND (Ohm)</label><input type='number' step='1' min='1' name='dh_ntc_series' value='");
      dtostrf(switches[i].ntcSeriesResistorOhm, 6, 1, fb); response->print(fb); response->print("'>");

      response->print("<label>NTC Nominal Resistance R0 (Ohm)</label><input type='number' step='1' min='1' name='dh_ntc_r0' value='");
      dtostrf(switches[i].ntcNominalResistanceOhm, 6, 1, fb); response->print(fb); response->print("'>");

      response->print("<label>NTC Nominal Temperature T0 (&deg;C)</label><input type='number' step='0.1' name='dh_ntc_t0' value='");
      dtostrf(switches[i].ntcNominalTemperatureC, 4, 1, fb); response->print(fb); response->print("'>");

      response->print("<label>NTC Beta</label><input type='number' step='1' min='1' name='dh_ntc_beta' value='");
      dtostrf(switches[i].ntcBeta, 6, 1, fb); response->print(fb);
    }
    response->print("'><div class='help'>Wiring: VCC -&gt; fixed resistor -&gt; A0 -&gt; NTC -&gt; GND. Use pin 17 for A0. Defaults: fixed=10000, R0=10000, T0=25, Beta=3950.</div>");

    { char fb[16];
      response->print("<label>Safety Min Heater Temp (&deg;C) &mdash; shuts off below this</label><input type='number' step='0.5' name='dh_safety_min' value='");
      dtostrf(switches[i].safetyMinTempC, 4, 1, fb); response->print(fb); response->print("'>");
      response->print("<label>Safety Max Heater Temp (&deg;C) &mdash; shuts off above this</label><input type='number' step='0.5' name='dh_safety_max' value='");
      dtostrf(switches[i].safetyMaxTempC, 4, 1, fb); response->print(fb); response->print("'>");
      response->print("<div class='help'>If the heater temperature sensor reads outside [min..max], the heater is automatically disabled. Default: 0 to 50 &deg;C.</div>");
    }

    if (dewHeaters[i] != nullptr) {
      char fb[16];
      response->print("<label>Current Heater Temperature</label><input type='text' value='");
      if (dewHeaters[i]->isHeaterTemperatureValid()) { dtostrf(dewHeaters[i]->getHeaterTemperatureC(), 4, 2, fb); response->print(fb); } else { response->print("n/a"); }
      response->print("' readonly>");
      response->print("<label>Current Dew Point</label><input type='text' value='");
      if (dewHeaters[i]->isSensorValid()) { dtostrf(dewHeaters[i]->getDewPointC(), 4, 2, fb); response->print(fb); } else { response->print("n/a"); }
      response->print("' readonly>");
    }

    response->print("<p><input type='submit' value='Save Dew Heater "); response->print(slot); response->print("'></p>");
    response->print("</form></div>");
    response->print("<p><a href='"); response->print(setupUrl); response->print("?bank=2'>Back to Dew Heater Overview</a></p>");
    response->print("</body></html>");
    request->send(response);
  }

  /** Dedicated POST handler for a single dew heater slot (1 or 2). */
  void dewHeaterSlotPostHandler(int slot, AsyncWebServerRequest *request) {
    enforceReservedSwitchRoles();
    int i = getDewHeaterSwitchIndexFromSlot(slot);
    int tempSwitchIdx = getDewHeaterTempSwitchIndexFromSlot(slot);
    String slotUrl  = "/setup/v1/switch/" + String(GetDeviceNumber()) + "/dewheater/" + String(slot);

    if (!isValidSwitchId(i)) {
      request->send(400, "text/plain", "Invalid dew heater slot");
      return;
    }

    switches[i].type = SwitchType::DewHeater;
    switches[i].canWrite = true;
    switches[i].canAsync = false;
    switches[i].isPWM = true;

    if (request->hasParam("dh_mode", true)) {
      String v = request->getParam("dh_mode", true)->value();
      switches[i].dewHeaterMode = (v == "manual") ? DewHeaterMode::Manual : DewHeaterMode::Automatic;
    }
    if (request->hasParam("dh_name", true)) {
      String v = request->getParam("dh_name", true)->value(); v.trim();
      if (v.length() > 0) switches[i].name = v.c_str();
    }
    if (request->hasParam("dh_desc", true)) {
      String v = request->getParam("dh_desc", true)->value(); v.trim();
      switches[i].description = v.c_str();
    }
    if (request->hasParam("dh_offset", true))
      switches[i].dewTargetOffsetC = request->getParam("dh_offset", true)->value().toDouble();
    if (request->hasParam("dh_manual", true))
      switches[i].dewTargetManualC = request->getParam("dh_manual", true)->value().toDouble();
    if (request->hasParam("dh_out", true)) {
      int pin = request->getParam("dh_out", true)->value().toInt();
      switches[i].outputPin = (pin >= 0 && pin <= 16) ? pin : -1;
    }
    if (request->hasParam("dh_dht", true)) {
      int pin = request->getParam("dh_dht", true)->value().toInt();
      switches[i].tempInputPin = (pin >= 0 && pin <= 16) ? pin : -1;
    }
    if (request->hasParam("dh_ds", true)) {
      int pin = request->getParam("dh_ds", true)->value().toInt();
      switches[i].heaterTempPin = (pin >= 0 && pin <= 17) ? pin : -1;  // 17 = A0 (NTC)
    }
    if (request->hasParam("dh_htsens", true)) {
      String s = request->getParam("dh_htsens", true)->value(); s.toLowerCase();
      switches[i].heaterTempSensorType = (s == "ntc") ? DewHeaterTempSensorType::NTCThermistor : DewHeaterTempSensorType::DS18B20;
    }
    if (request->hasParam("dh_dsoffset", true))
      switches[i].heaterTempOffsetC = request->getParam("dh_dsoffset", true)->value().toDouble();
    if (request->hasParam("dh_ntc_series", true)) {
      double v = request->getParam("dh_ntc_series", true)->value().toDouble();
      if (v > 0.0) switches[i].ntcSeriesResistorOhm = v;
    }
    if (request->hasParam("dh_ntc_r0", true)) {
      double v = request->getParam("dh_ntc_r0", true)->value().toDouble();
      if (v > 0.0) switches[i].ntcNominalResistanceOhm = v;
    }
    if (request->hasParam("dh_ntc_t0", true)) {
      double v = request->getParam("dh_ntc_t0", true)->value().toDouble();
      if (v > -80.0 && v < 200.0) switches[i].ntcNominalTemperatureC = v;
    }
    if (request->hasParam("dh_ntc_beta", true)) {
      double v = request->getParam("dh_ntc_beta", true)->value().toDouble();
      if (v > 0.0) switches[i].ntcBeta = v;
    }
    if (request->hasParam("dh_safety_min", true)) {
      double v = request->getParam("dh_safety_min", true)->value().toDouble();
      if (v > -50.0 && v < 200.0) switches[i].safetyMinTempC = v;
    }
    if (request->hasParam("dh_safety_max", true)) {
      double v = request->getParam("dh_safety_max", true)->value().toDouble();
      if (v > -50.0 && v < 200.0) switches[i].safetyMaxTempC = v;
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

    request->redirect(slotUrl);
  }

  // ==================== DHT22 Dedicated Handlers ====================

  /** GET /setup/v1/switch/{n}/dht22 — dedicated small page for ambient sensor setup. */
  void dht22GetHandler(AsyncWebServerRequest *request) {
    enforceReservedSwitchRoles();
    String setupUrl = "/setup/v1/switch/" + String(GetDeviceNumber()) + "/setup";
    String selfUrl  = "/setup/v1/switch/" + String(GetDeviceNumber()) + "/dht22";

    AsyncResponseStream *response = request->beginResponseStream("text/html");
    if (response == nullptr) { request->send(500, "text/plain", "Failed to create response"); return; }

    response->print("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>");
    response->print("<title>DHT22 Sensor Setup</title>");
    response->print("<style>body{font-family:Arial,sans-serif;margin:20px;} .card{border:1px solid #ddd;padding:12px;margin-bottom:14px;background:#f9f9f9;} label{display:block;margin:6px 0 2px;} input[type='text'],input[type='number']{width:100%;padding:6px;box-sizing:border-box;} input[type='submit']{background:#0066cc;color:#fff;border:none;border-radius:4px;padding:9px 16px;cursor:pointer;} .help{font-size:.85em;color:#666;}</style></head><body>");
    response->print("<h1>DHT22 Sensor Setup &mdash; Switches 2-3</h1>");
    response->print("<p><a href='"); response->print(setupUrl); response->print("?bank=1'>&#8592; DHT22 Overview</a> &nbsp;|&nbsp; <a href='"); response->print(setupUrl); response->print("'>Main Setup</a></p>");

    response->print("<form method='POST' action='"); response->print(selfUrl); response->print("'>");
    for (int i = 2; i <= 3 && i < maxSwitch; i++) {
      response->print("<div class='card'><h2>Switch "); response->print(i);
      response->print(i == 2 ? " &mdash; Temperature" : " &mdash; Humidity"); response->print("</h2>");
      response->print("<label>Name</label><input type='text' name='dht22_name_"); response->print(i);
      response->print("' maxlength='15' value='"); response->print(escapeHtml(String(switches[i].name.c_str()))); response->print("'>");
      response->print("<label>Description</label><input type='text' name='dht22_desc_"); response->print(i);
      response->print("' maxlength='20' value='"); response->print(escapeHtml(String(switches[i].description.c_str()))); response->print("'>");
      response->print("<label>DHT22/AM2302 Data Pin</label><input type='number' min='-1' max='16' name='dht22_pin_"); response->print(i);
      response->print("' value='"); response->print(switches[i].tempInputPin); response->print("'>");
      if (i == 2) {
        char fb[12];
        response->print("<label>Temperature Offset (&deg;C)</label><input type='number' step='0.1' name='dht22_offset_2' value='");
        dtostrf(switches[2].dhtTempOffsetC, 4, 2, fb); response->print(fb); response->print("'>");
      }
      { char fb[12];
        response->print("<label>Current Value (read-only)</label><input type='text' value='");
        dtostrf(switches[i].value, 4, 2, fb); response->print(fb); response->print("' readonly>"); }
      response->print("</div>");
    }
    if (maxSwitch > 8) {
      char fb[12];
      response->print("<div class='card'><h2>Switch 8 &mdash; Dew Point (read-only)</h2>");
      response->print("<div class='help'>Calculated from switches 2+3. No configuration needed.</div>");
      response->print("<label>Current Value (&deg;C)</label><input type='text' value='");
      dtostrf(switches[8].value, 4, 2, fb); response->print(fb); response->print("' readonly></div>");
    }
    response->print("<p><input type='submit' value='Save DHT22 Settings'></p></form>");
    response->print("<p><a href='"); response->print(setupUrl); response->print("'>&#8592; Back to Main Setup</a></p>");
    response->print("</body></html>");
    request->send(response);
  }

  /** POST /setup/v1/switch/{n}/dht22 — save ambient sensor settings. */
  void dht22PostHandler(AsyncWebServerRequest *request) {
    enforceReservedSwitchRoles();
    String selfUrl = "/setup/v1/switch/" + String(GetDeviceNumber()) + "/dht22";
    bool anyChange = false;

    for (int i = 2; i <= 3 && i < maxSwitch; i++) {
      bool thisChange = false;
      String si = String(i);
      if (request->hasParam("dht22_name_" + si, true)) {
        String v = request->getParam("dht22_name_" + si, true)->value(); v.trim();
        if (v.length() > 0) { switches[i].name = v.c_str(); thisChange = true; }
      }
      if (request->hasParam("dht22_desc_" + si, true)) {
        String v = request->getParam("dht22_desc_" + si, true)->value(); v.trim();
        switches[i].description = v.c_str(); thisChange = true;
      }
      if (request->hasParam("dht22_pin_" + si, true)) {
        int pin = request->getParam("dht22_pin_" + si, true)->value().toInt();
        int np = (pin >= 0 && pin <= 16) ? pin : -1;
        if (np != switches[i].tempInputPin) {
          switches[i].tempInputPin = np;
          if (np >= 0) pinMode(np, INPUT);
        }
        thisChange = true;
      }
      if (i == 2 && request->hasParam("dht22_offset_2", true)) {
        switches[i].dhtTempOffsetC = request->getParam("dht22_offset_2", true)->value().toDouble();
        thisChange = true;
      }
      if (thisChange) {
        anyChange = true;
        switches[i].type      = (i == 2) ? SwitchType::DHT22Temp : SwitchType::DHT22Humidity;
        switches[i].canWrite  = false;
        switches[i].canAsync  = false;
        switches[i].isPWM     = false;
        switches[i].outputPin = -1;
        switches[i].heaterTempPin = -1;
        switches[i].minValue  = (i == 2) ? -40.0 : 0.0;
        switches[i].maxValue  = (i == 2) ?  80.0 : 100.0;
        switches[i].stepValue = 0.1;
        scheduleRecreateDewHeater(i);
      }
    }
    if (anyChange) scheduleSaveAllToEEPROM();

    String html = "<html><head><meta http-equiv='refresh' content='2;url=" + selfUrl + "'></head><body>";
    html += "<h1>DHT22 Sensor Setup</h1><p>Settings saved. Redirecting...</p></body></html>";
    request->send(200, "text/html", html);
  }

  // ==================== Custom Switch Dedicated Handlers ====================

  /** GET /setup/v1/switch/{n}/customswitch/{idx} — single custom switch config page. */
  void customSwitchGetHandler(int idx, AsyncWebServerRequest *request) {
    enforceReservedSwitchRoles();
    String setupUrl = "/setup/v1/switch/" + String(GetDeviceNumber()) + "/setup";
    String selfUrl  = "/setup/v1/switch/" + String(GetDeviceNumber()) + "/customswitch/" + String(idx);

    if (!isValidSwitchId(idx)) { request->send(400, "text/plain", "Invalid switch index"); return; }

    AsyncResponseStream *response = request->beginResponseStream("text/html");
    if (response == nullptr) { request->send(500, "text/plain", "Failed to create response"); return; }

    bool isOn = switches[idx].value > 0.0;
    response->print("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>");
    response->print("<title>Custom Switch "); response->print(idx); response->print(" Setup</title>");
    response->print("<style>body{font-family:Arial,sans-serif;margin:20px;} .card{border:1px solid #ddd;padding:12px;margin-bottom:14px;background:#f9f9f9;} label{display:block;margin:6px 0 2px;} input[type='text'],input[type='number'],select{width:100%;padding:6px;box-sizing:border-box;} .cb-row{display:flex;gap:16px;margin:8px 0;flex-wrap:wrap;} .cb-row label{display:flex;align-items:center;gap:4px;width:auto;} input[type='submit']{background:#0066cc;color:#fff;border:none;border-radius:4px;padding:9px 16px;cursor:pointer;margin-top:8px;} .on{color:#00aa00;font-weight:bold;} .off{color:#cc0000;font-weight:bold;} .help{font-size:.85em;color:#666;} .type-help{background:#eef6ff;border-left:4px solid #0066cc;padding:8px;border-radius:4px;font-size:.85em;color:#355;margin-top:4px;}</style></head><body>");
    response->print("<h1>Custom Switch "); response->print(idx);
    response->print(" &nbsp;<span class='"); response->print(isOn ? "on" : "off"); response->print("'>"); response->print(isOn ? "ON" : "OFF"); response->print("</span></h1>");
    response->print("<p><a href='"); response->print(setupUrl); response->print("?bank=0'>&#8592; Custom Switch Overview</a> &nbsp;|&nbsp; <a href='"); response->print(setupUrl); response->print("'>Main Setup</a></p>");

    response->print("<form method='POST' action='"); response->print(selfUrl); response->print("'>");
    response->print("<div class='card'>");

    response->print("<label>Name (max 15 chars)</label><input type='text' name='swname_"); response->print(idx);
    response->print("' value='"); response->print(escapeHtml(String(switches[idx].name.c_str()))); response->print("' maxlength='15'>");

    response->print("<label>Description (max 20 chars)</label><input type='text' name='swdesc_"); response->print(idx);
    response->print("' value='"); response->print(escapeHtml(String(switches[idx].description.c_str()))); response->print("' maxlength='20'>");

    response->print("<label>Switch Type</label><select name='swtype_"); response->print(idx); response->print("'>");
    response->print("<option value='0'"); response->print(switches[idx].type == SwitchType::Default      ? " selected" : ""); response->print(">Default</option>");
    response->print("<option value='2'"); response->print(switches[idx].type == SwitchType::DHT22Temp    ? " selected" : ""); response->print(">DHT22 Temp</option>");
    response->print("<option value='3'"); response->print(switches[idx].type == SwitchType::DHT22Humidity ? " selected" : ""); response->print(">DHT22 Humidity</option>");
    response->print("<option value='4'"); response->print(switches[idx].type == SwitchType::DS18B20Temp  ? " selected" : ""); response->print(">DS18B20 Temp</option>");
    response->print("</select>");
    response->print("<div class='type-help' id='typehelp'>"); response->print(getSwitchTypeHelpText(switches[idx].type)); response->print("</div>");

    { char fb[16];
      response->print("<label>Min Value</label><input type='number' step='any' name='swmin_"); response->print(idx);
      response->print("' value='"); dtostrf(switches[idx].minValue, 6, 4, fb); response->print(fb); response->print("'>");
      response->print("<label>Max Value</label><input type='number' step='any' name='swmax_"); response->print(idx);
      response->print("' value='"); dtostrf(switches[idx].maxValue, 6, 4, fb); response->print(fb); response->print("'>");
      response->print("<label>Step Size</label><input type='number' step='any' name='swstep_"); response->print(idx);
      response->print("' value='"); dtostrf(switches[idx].stepValue, 6, 4, fb); response->print(fb); response->print("'>");
      response->print("<label>Current Value"); if (!switches[idx].canWrite) response->print(" (read-only)"); response->print("</label>");
      response->print("<input type='number' step='any' name='swval_"); response->print(idx);
      response->print("' value='"); dtostrf(switches[idx].value, 6, 4, fb); response->print(fb);
      response->print("'"); if (!switches[idx].canWrite) response->print(" readonly"); response->print(">");
    }

    response->print("<label>GPIO Output Pin (-1 = none)</label><input type='number' name='swpin_"); response->print(idx);
    response->print("' value='"); response->print(switches[idx].outputPin); response->print("' min='-1' max='16'>");
    response->print("<label>Temp Input GPIO (DHT22 data pin)</label><input type='number' name='swtempin_"); response->print(idx);
    response->print("' value='"); response->print(switches[idx].tempInputPin); response->print("' min='-1' max='16'>");
    response->print("<label>Heater Temp GPIO (DS18B20/NTC; 17=A0)</label><input type='number' name='swheatertemp_"); response->print(idx);
    response->print("' value='"); response->print(switches[idx].heaterTempPin); response->print("' min='-1' max='17'>");

    response->print("<div class='cb-row'>");
    response->print("<label><input type='checkbox' name='swrw_");    response->print(idx); response->print("'"); response->print(switches[idx].canWrite  ? " checked" : ""); response->print("> Can Write (R/W)</label>");
    response->print("<label><input type='checkbox' name='swasync_"); response->print(idx); response->print("'"); response->print(switches[idx].canAsync  ? " checked" : ""); response->print("> Async</label>");
    response->print("<label><input type='checkbox' name='swpwm_");   response->print(idx); response->print("'"); response->print(switches[idx].isPWM     ? " checked" : ""); response->print("> PWM output</label>");
    response->print("</div>");
    response->print("</div>");
    response->print("<p><input type='submit' value='Save Switch "); response->print(idx); response->print("'></p></form>");

    response->print("<script>(function(){");
    response->print("var sel=document.getElementsByName('swtype_"); response->print(idx); response->print("')[0];");
    response->print("var box=document.getElementById('typehelp');");
    response->print("var h={'1':'GPIO Pin=heater PWM. Temp Input=DHT22 ambient. Heater Temp=DS18B20. Value=offset above dew point.',");
    response->print("'2':'Temp Input GPIO=DHT22 data. Value=read-only ambient temp.','3':'Temp Input GPIO=DHT22 data. Value=read-only humidity.',");
    response->print("'4':'Heater Temp GPIO=DS18B20. Value=read-only temperature.'};");
    response->print("sel.addEventListener('change',function(){box.textContent=h[this.value]||'GPIO Pin controls output. Value is the switch value.';});");
    response->print("})();</script>");
    response->print("<p><a href='"); response->print(setupUrl); response->print("?bank=0'>&#8592; Back to Custom Switch Overview</a></p>");
    response->print("</body></html>");
    request->send(response);
  }

  /** POST /setup/v1/switch/{n}/customswitch/{idx} — save single custom switch settings. */
  void customSwitchPostHandler(int idx, AsyncWebServerRequest *request) {
    enforceReservedSwitchRoles();
    String selfUrl = "/setup/v1/switch/" + String(GetDeviceNumber()) + "/customswitch/" + String(idx);

    if (!isValidSwitchId(idx)) { request->send(400, "text/plain", "Invalid switch index"); return; }

    String si = String(idx);
    bool changed = false;
    if (request->hasParam("swname_" + si, true)) {
      String v = request->getParam("swname_" + si, true)->value(); v.trim();
      if (v.length() > 0) { switches[idx].name = v.c_str(); changed = true; }
    }
    if (request->hasParam("swdesc_" + si, true)) {
      String v = request->getParam("swdesc_" + si, true)->value(); v.trim();
      switches[idx].description = v.c_str(); changed = true;
    }
    if (request->hasParam("swtype_" + si, true)) {
      int t = request->getParam("swtype_" + si, true)->value().toInt();
      if (t < (int)SwitchType::Default || t > (int)SwitchType::DewHeaterOutputPct) t = (int)SwitchType::Default;
      if (t == (int)SwitchType::DewHeater) t = (int)SwitchType::Default; // DewHeater reserved for slots 4-5
      switches[idx].type = (SwitchType)t; changed = true;
    }
    if (request->hasParam("swmin_"  + si, true)) { switches[idx].minValue  = request->getParam("swmin_"  + si, true)->value().toDouble(); changed = true; }
    if (request->hasParam("swmax_"  + si, true)) { switches[idx].maxValue  = request->getParam("swmax_"  + si, true)->value().toDouble(); changed = true; }
    if (request->hasParam("swstep_" + si, true)) { switches[idx].stepValue = request->getParam("swstep_" + si, true)->value().toDouble(); changed = true; }
    if (request->hasParam("swval_"  + si, true)) {
      double nv = request->getParam("swval_" + si, true)->value().toDouble();
      if (nv < switches[idx].minValue) nv = switches[idx].minValue;
      if (nv > switches[idx].maxValue) nv = switches[idx].maxValue;
      switches[idx].value = nv; applyOutput(idx); changed = true;
    }
    if (request->hasParam("swpin_" + si, true)) {
      int pin = request->getParam("swpin_" + si, true)->value().toInt();
      if (pin < 0 || pin > 16) pin = -1;
      if (pin != switches[idx].outputPin) {
        switches[idx].outputPin = pin;
        if (pin >= 0) { pinMode(pin, OUTPUT); applyOutput(idx); }
      }
      changed = true;
    }
    if (request->hasParam("swtempin_" + si, true)) {
      int pin = request->getParam("swtempin_" + si, true)->value().toInt();
      if (pin < 0 || pin > 16) pin = -1;
      if (pin != switches[idx].tempInputPin) { switches[idx].tempInputPin = pin; if (pin >= 0) pinMode(pin, INPUT); }
      changed = true;
    }
    if (request->hasParam("swheatertemp_" + si, true)) {
      int pin = request->getParam("swheatertemp_" + si, true)->value().toInt();
      if (pin < 0 || pin > 16) pin = -1;
      if (pin != switches[idx].heaterTempPin) { switches[idx].heaterTempPin = pin; if (pin >= 0) pinMode(pin, INPUT); }
      changed = true;
    }
    if (changed) {
      switches[idx].canWrite = request->hasParam("swrw_"    + si, true);
      switches[idx].canAsync = request->hasParam("swasync_" + si, true);
      switches[idx].isPWM    = request->hasParam("swpwm_"   + si, true);
      scheduleRecreateDewHeater(idx);
      applyOutput(idx);
      scheduleSaveAllToEEPROM();
    }

    String html = "<html><head><meta http-equiv='refresh' content='2;url=" + selfUrl + "'></head><body>";
    html += "<h1>Switch " + String(idx) + " Setup</h1><p>Settings saved. Redirecting...</p></body></html>";
    request->send(200, "text/html", html);
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
        scheduleSaveAllToEEPROM(); // persist LED state across reboots
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

      // --- Dew heater POST is now handled by dedicated /dewheater/{slot} endpoints ---
      // Legacy dh_slot param: redirect to the appropriate dedicated endpoint
      if (request->hasParam("dh_slot", true)) {
        int slot = request->getParam("dh_slot", true)->value().toInt();
        if (slot < 1 || slot > 2) slot = 1;
        String redirect = "/setup/v1/switch/" + String(GetDeviceNumber()) + "/dewheater/" + String(slot);
        request->redirect(redirect);
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
      if (ESP.getFreeHeap() < 6000) {
        request->send(200, "text/html",
          "<html><body><h2>Switch Setup</h2><p>Low memory. <a href='" + setupUrl + "'>Reload</a></p></body></html>");
        return;
      }
      AsyncResponseStream *response = request->beginResponseStream("text/html");
      if (response == nullptr) {
        request->send(500, "text/plain", "Failed to create response");
        return;
      }
      response->print("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'><title>Switch Setup</title></head><body>");
      response->print("<h1>Switch Setup - ");
      response->print(escapeHtml(GetDeviceName()));
      response->print("</h1>");
      response->print("<h2>Pages</h2>");
      { String csBase = "/setup/v1/switch/" + String(GetDeviceNumber()) + "/customswitch";
        String dht22Url2 = "/setup/v1/switch/" + String(GetDeviceNumber()) + "/dht22";
        String dhBase2  = "/setup/v1/switch/" + String(GetDeviceNumber()) + "/dewheater";
        response->print("<p><a href='"); response->print(csBase); response->print("/0'>Custom Switch 0</a></p>");
        response->print("<p><a href='"); response->print(csBase); response->print("/1'>Custom Switch 1</a></p>");
        response->print("<p><a href='"); response->print(dht22Url2); response->print("'>DHT22 Sensors 2-3</a></p>");
        response->print("<p><a href='"); response->print(dhBase2); response->print("/1'>Dew Heater 1</a></p>");
        response->print("<p><a href='"); response->print(dhBase2); response->print("/2'>Dew Heater 2</a></p>"); }
      response->print("<h2>WiFi</h2><form method='POST' action='"); response->print(setupUrl); response->print("'>");
      response->print("<label>SSID</label><input type='text' name='wifi_ssid' maxlength='31' value='");
      response->print(escapeHtml(String(wifiConfig.getSSID())));
      response->print("' required>");
      response->print("<label>Password</label><input type='password' name='wifi_password' maxlength='63' value='' placeholder='Leave empty to keep current'>");
      response->print("<p><input type='submit' value='Save WiFi Settings'></p></form>");
      response->print("<h2>LED</h2><form method='POST' action='"); response->print(setupUrl); response->print("'>");
      response->print("<input type='hidden' name='led_page' value='1'>");
      response->print(ledDisabled
        ? "<label><input type='checkbox' name='led_disable' checked> Disable onboard LED (GPIO2 HIGH)</label>"
        : "<label><input type='checkbox' name='led_disable'> Disable onboard LED (GPIO2 HIGH)</label>");
      response->print("<p><input type='submit' value='Save LED Setting'></p></form>");
      response->print("</body></html>");
      request->send(response);
      return;
    }

    if (bank == 2) {
      // Each dew heater now has its own dedicated small-page endpoint
      String dh1 = "/setup/v1/switch/" + String(GetDeviceNumber()) + "/dewheater/1";
      String dh2 = "/setup/v1/switch/" + String(GetDeviceNumber()) + "/dewheater/2";
      AsyncResponseStream *response = request->beginResponseStream("text/html");
      if (response == nullptr) { request->send(500, "text/plain", "Failed to create response"); return; }
      response->print("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>");
      response->print("<title>Dew Heater Setup</title>");
      response->print("<style>body{font-family:Arial,sans-serif;margin:20px;} a{display:block;margin:10px 0;font-size:1.1em;}</style></head><body>");
      response->print("<h1>Dew Heater Setup</h1>");
      response->print("<p><a href='"); response->print(dh1); response->print("'>&#9654; Dew Heater 1 (Switch 4+6)</a>");
      response->print("<a href='"); response->print(dh2); response->print("'>&#9654; Dew Heater 2 (Switch 5+7)</a>");
      response->print("<a href='"); response->print(setupUrl); response->print("'>&#8592; Back to Main Setup</a></p>");
      response->print("</body></html>");
      request->send(response);
      return;
      // (dead code removed - old per-slot forms now handled by /dewheater/{slot} endpoints)
      #if 0
      AsyncResponseStream *_dh_unused = request->beginResponseStream("text/html");
      _dh_unused->print("<!DOCTYPE html><html><head>");
      _dh_unused->print("<meta charset='UTF-8'><meta name='viewport' content='width=device-width, initial-scale=1.0'>");
      _dh_unused->print("<title>Dew Heater Setup</title>");
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

        response->print("<label>Heater Temperature Sensor Pin (17 = A0/NTC)</label><input type='number' min='-1' max='17' name='dh_ds_");
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
      #endif // end dead code
    }

    if (bank == 1) {
      String dht22Url = "/setup/v1/switch/" + String(GetDeviceNumber()) + "/dht22";
      AsyncResponseStream *response = request->beginResponseStream("text/html");
      if (response == nullptr) { request->send(500, "text/plain", "Failed to create response"); return; }
      response->print("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>");
      response->print("<title>DHT22 Sensor Setup</title>");
      response->print("<style>body{font-family:Arial,sans-serif;margin:20px;} a{display:block;margin:10px 0;font-size:1.1em;}</style></head><body>");
      response->print("<h1>DHT22 Sensor Setup</h1>");
      response->print("<p><a href='"); response->print(dht22Url); response->print("'>&#9654; Configure DHT22 Sensors (Switches 2-3)</a>");
      response->print("<a href='"); response->print(setupUrl); response->print("'>&#8592; Back to Main Setup</a></p>");
      response->print("</body></html>");
      request->send(response);
      return;
    }

    // bank==0: custom switch nav page
    if (bank == 0) {
      String cs0Url = "/setup/v1/switch/" + String(GetDeviceNumber()) + "/customswitch/0";
      String cs1Url = "/setup/v1/switch/" + String(GetDeviceNumber()) + "/customswitch/1";
      AsyncResponseStream *response = request->beginResponseStream("text/html");
      if (response == nullptr) { request->send(500, "text/plain", "Failed to create response"); return; }
      response->print("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>");
      response->print("<title>Custom Switch Setup</title>");
      response->print("<style>body{font-family:Arial,sans-serif;margin:20px;} a{display:block;margin:10px 0;font-size:1.1em;}</style></head><body>");
      response->print("<h1>Custom Switch Setup</h1>");
      response->print("<p><a href='"); response->print(cs0Url); response->print("'>&#9654; Custom Switch 0</a>");
      response->print("<a href='"); response->print(cs1Url); response->print("'>&#9654; Custom Switch 1</a>");
      response->print("<a href='"); response->print(setupUrl); response->print("'>&#8592; Back to Main Setup</a></p>");
      response->print("</body></html>");
      request->send(response);
      return;
      // (dead code removed - old DHT22 form, was unreachable after bank==1 return)
      AsyncResponseStream *response1 = nullptr;
      response1->print("<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1.0'>");
      response1->print("<title>DHT22 Sensor Setup</title>");
      response1->print("<style>body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0;}");
      response1->print(".container{max-width:720px;margin:0 auto;background:#fff;padding:20px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,.1);}");
      response1->print(".card{background:#f9f9f9;padding:15px;border-radius:6px;margin-bottom:14px;}");
      response1->print("label{display:block;margin:8px 0 4px;color:#555;font-size:.9em;}");
      response1->print("input[type='text'],input[type='number']{width:100%;padding:8px;border:1px solid #ccc;border-radius:4px;box-sizing:border-box;}");
      response1->print("input[type='submit']{margin-top:10px;background:#0066cc;color:#fff;border:none;border-radius:4px;padding:9px 16px;cursor:pointer;}");
      response1->print(".grid{display:grid;grid-template-columns:1fr 1fr;gap:10px 14px;} .full{grid-column:1/-1;} .help{font-size:.85em;color:#666;margin-top:4px;}");
      response1->print("</style></head><body><div class='container'>");
      response1->print("<h1>DHT22 Sensor Setup &mdash; Switches 2-3</h1>");
      response1->print("<div class='card'><p>Configure fixed DHT22/AM2302 roles: switch 2 = temperature, switch 3 = humidity.</p></div>");
      response1->print("<form method='POST' action='"); response1->print(setupUrl); response1->print("'>");
      response1->print("<input type='hidden' name='dht22_page' value='1'>");
      response1->print("<input type='hidden' name='bank' value='1'>");
      for (int i = 2; i <= 3 && i < maxSwitch; i++) {
        response1->print("<div class='card'><h2>Switch "); response1->print(i);
        response1->print(i == 2 ? " (Temperature)" : " (Humidity)");
        response1->print("</h2><div class='grid'>");
        response1->print("<div class='full'><label>Name</label><input type='text' name='dht22_name_"); response1->print(i);
        response1->print("' maxlength='15' value='"); response1->print(escapeHtml(String(switches[i].name.c_str()))); response1->print("'></div>");
        response1->print("<div class='full'><label>Description</label><input type='text' name='dht22_desc_"); response1->print(i);
        response1->print("' maxlength='20' value='"); response1->print(escapeHtml(String(switches[i].description.c_str()))); response1->print("'></div>");
        response1->print("<div><label>Mode</label><input type='text' value='");
        response1->print(i == 2 ? "Temperature" : "Humidity"); response1->print("' readonly></div>");
        response1->print("<div><label>DHT22/AM2302 Data Pin</label><input type='number' min='-1' max='16' name='dht22_pin_"); response1->print(i);
        response1->print("' value='"); response1->print(switches[i].tempInputPin); response1->print("'></div>");
        { char fb[12]; response1->print("<div><label>Temperature Offset (&deg;C)</label><input type='number' step='0.1' name='dht22_offset_"); response1->print(i);
        response1->print("' value='"); dtostrf(switches[i].dhtTempOffsetC, 4, 2, fb); response1->print(fb); response1->print("'></div>"); }
        { char fb[12]; response1->print("<div><label>Current Value</label><input type='text' value='");
        dtostrf(switches[i].value, 4, 2, fb); response1->print(fb); response1->print("' readonly></div>"); }
        response1->print("<div class='full help'>Temperature offset is applied only on switch 2 (temperature).</div>");
        response1->print("</div></div>");
      }
      if (maxSwitch > 8) {
        response1->print("<div class='card'><h2>Switch 8 (Dew Point)</h2><div class='grid'>");
        response1->print("<div class='full'><label>Name</label><input type='text' value='"); response1->print(escapeHtml(String(switches[8].name.c_str()))); response1->print("' readonly></div>");
        response1->print("<div class='full'><label>Description</label><input type='text' value='"); response1->print(escapeHtml(String(switches[8].description.c_str()))); response1->print("' readonly></div>");
        response1->print("<div><label>Mode</label><input type='text' value='Calculated Dew Point' readonly></div>");
        response1->print("<div><label>DHT22/AM2302 Data Pin</label><input type='number' value='"); response1->print(switches[8].tempInputPin); response1->print("' readonly></div>");
        { char fb[12]; response1->print("<div><label>Current Value (&deg;C)</label><input type='text' value='");
        dtostrf(switches[8].value, 4, 2, fb); response1->print(fb); response1->print("' readonly></div>"); }
        response1->print("<div class='full help'>Read-only, calculated from ambient temperature and humidity.</div>");
        response1->print("</div></div>");
      }
      response1->print("<input type='submit' value='Save DHT22 Sensor Settings'></form>");
      response1->print("<p><a href='"); response1->print(setupUrl); response1->print("'>Back to Main Setup</a></p>");
      response1->print("<p><a href='"); response1->print(setupUrl); response1->print("?bank=0'>Open Custom Switch 0-1</a></p>");
      response1->print("</div></body></html>");
      request->send(response1);
      return;
    }

  }
};

#endif // ARDUINO_SWITCH_H
