#include "DewHeater.h"

#include <math.h>
#include "DebugLog.h"

namespace {
struct DhtAmbientCacheEntry {
  bool initialized;
  bool valid;
  unsigned long lastReadMs;
  float temperatureC;
  float humidityPercent;
  float dewPointC;
};

DhtAmbientCacheEntry g_dhtAmbientCache[17] = {};
}

DewHeater::DewHeater(int heater_pin, int dht22_pin, unsigned long interval_ms, int heater_temp_pin,
           uint8_t heater_temp_sensor_type)
    : heaterOutputPin(heater_pin),
  dhtPin(dht22_pin),
    heaterTempSensorPin(heater_temp_pin),
    heaterTempSensorType((heater_temp_sensor_type == 1)
                 ? HeaterTempSensorType::NTCThermistor
                 : HeaterTempSensorType::DS18B20),
  dht(dht22_pin, DHT22),
      heaterOneWire(nullptr),
      heaterSensors(nullptr),
      heaterPowerPercent(0.0f),
      temperatureC(NAN),
      humidityPercent(NAN),
      dewPointC(NAN),
      heaterTemperatureC(NAN),
      heaterTemperatureOffsetC(0.0f),
      sensorValid(false),
      heaterTemperatureValid(false),
      ntcSeriesResistorOhm(10000.0f),
      ntcNominalResistanceOhm(10000.0f),
      ntcNominalTemperatureC(25.0f),
      ntcBeta(3950.0f),
      ntcSupplyVoltage(3.3f),
      ntcAdcReferenceVoltage(3.3f),
      pidEnabled(false),
      targetHeaterTemperatureC(NAN),
      dewPointOffsetC(2.0f),
      pidKp(12.0f),
      pidKi(0.15f),
      pidKd(2.5f),
      pidIntegral(0.0f),
      pidLastError(0.0f),
      lastReadMs(0),
      lastPidMs(0),
      readIntervalMs(interval_ms) {}

DewHeater::~DewHeater() {
  delete heaterSensors;
  delete heaterOneWire;
}

void DewHeater::begin() {
  if (heaterOutputPin >= 0) {
    pinMode(heaterOutputPin, OUTPUT);
    LOG_INFO("Pin mode set for heater output pin %d", heaterOutputPin);
    analogWrite(heaterOutputPin, 0);
  }

  if (dhtPin >= 0) {
    dht.begin();
    lastReadMs = millis(); // DHT22 needs ~2 s warm-up; delay first read by readIntervalMs
  }

  if (heaterTempSensorPin >= 0) {
    if (heaterTempSensorType == HeaterTempSensorType::DS18B20 && heaterSensors == nullptr) {
      heaterOneWire = new OneWire(heaterTempSensorPin);
      heaterSensors = new DallasTemperature(heaterOneWire);
      heaterSensors->begin();
    } else if (heaterTempSensorType == HeaterTempSensorType::NTCThermistor) {
      pinMode(heaterTempSensorPin, INPUT);
      LOG_INFO("Pin mode set for NTC thermistor heater temperature sensor pin %d", heaterTempSensorPin);
    }
  }
}

bool DewHeater::update() {
  unsigned long now = millis();
  if (now - lastReadMs < readIntervalMs) {
    if (pidEnabled) {
      updatePidControl(now);
    }
    return sensorValid && (heaterTempSensorPin < 0 || heaterTemperatureValid);
  }
  lastReadMs = now;

  if (dhtPin >= 0) {
    bool useCachedReading = false;
    if (dhtPin <= 16) {
      DhtAmbientCacheEntry &cache = g_dhtAmbientCache[dhtPin];
      if (cache.valid && cache.initialized && (now - cache.lastReadMs) < readIntervalMs) {
        temperatureC = cache.temperatureC;
        humidityPercent = cache.humidityPercent;
        dewPointC = cache.dewPointC;
        sensorValid = true;
        useCachedReading = true;
      }
    }

    if (!useCachedReading) {
      // ESP8266 WiFi activity can corrupt DHT timing — retry once on failure
      float newHumidity = dht.readHumidity();
      float newTemperature = dht.readTemperature();
      if (isnan(newHumidity) || isnan(newTemperature)) {
        delay(20);
        newHumidity = dht.readHumidity();
        newTemperature = dht.readTemperature();
        LOG_WARN("DHT retry read on pin %d: humidity=%.1f temperature=%.1f", dhtPin, newHumidity, newTemperature);
      }

      if (isnan(newHumidity) || isnan(newTemperature) ||
          newHumidity < 0.0f || newHumidity > 100.0f) {
        sensorValid = false;
        temperatureC = -273.15f;
        humidityPercent = NAN;
        dewPointC = NAN;
        LOG_ERROR("Failed to read from DHT sensor on pin " + String(dhtPin) + ": humidity=" + String(newHumidity) + " temperature=" + String(newTemperature));

        if (dhtPin <= 16) {
          DhtAmbientCacheEntry &cache = g_dhtAmbientCache[dhtPin];
          if (cache.valid && cache.initialized) {
            temperatureC = cache.temperatureC;
            humidityPercent = cache.humidityPercent;
            dewPointC = cache.dewPointC;
            sensorValid = true;
            LOG_WARN("Using cached DHT values for pin " + String(dhtPin) + " (last read " + String(now - cache.lastReadMs) + " ms ago)");
          }
        }
      } else {
        humidityPercent = newHumidity;
        temperatureC = newTemperature;
        dewPointC = (float)calculateDewPointMagnus(temperatureC, humidityPercent);
        sensorValid = !isnan(dewPointC) && !isinf(dewPointC);

        if (dhtPin <= 16) {
          DhtAmbientCacheEntry &cache = g_dhtAmbientCache[dhtPin];
          cache.initialized = true;
          cache.valid = sensorValid;
          cache.lastReadMs = now;
          cache.temperatureC = temperatureC;
          cache.humidityPercent = humidityPercent;
          cache.dewPointC = dewPointC;
        }
      }
    } else {
      // cached values already applied
      LOG_INFO("Using cached DHT values for pin %d", dhtPin);
    }
  } else {
    sensorValid = false;
    temperatureC = NAN;
    humidityPercent = NAN;
    dewPointC = NAN;
    LOG_ERROR("DHT pin not configured, cannot read temperature/humidity");
  }

  if (heaterTempSensorType == HeaterTempSensorType::DS18B20 && heaterSensors != nullptr) {
    heaterSensors->requestTemperatures();
    float newHeaterTemperature = heaterSensors->getTempCByIndex(0);

    if (newHeaterTemperature == DEVICE_DISCONNECTED_C ||
        newHeaterTemperature == 85.0f ||
        isnan(newHeaterTemperature) ||
        isinf(newHeaterTemperature) ||
        newHeaterTemperature < -55.0f ||
        newHeaterTemperature > 125.0f) {
      heaterTemperatureValid = false;
      heaterTemperatureC = -273.15f;
    } else {
      heaterTemperatureC = newHeaterTemperature + heaterTemperatureOffsetC;
      heaterTemperatureValid = true;
    }
  } else if (heaterTempSensorType == HeaterTempSensorType::NTCThermistor && heaterTempSensorPin >= 0) {
    float newHeaterTemperature = readNtcTemperatureC();
    if (isnan(newHeaterTemperature) || isinf(newHeaterTemperature) ||
        newHeaterTemperature < -55.0f || newHeaterTemperature > 125.0f) {
      heaterTemperatureValid = false;
      heaterTemperatureC = -273.15f;
    } else {
      heaterTemperatureC = newHeaterTemperature + heaterTemperatureOffsetC;
      heaterTemperatureValid = true;
    }
  } else {
    heaterTemperatureValid = false;
    heaterTemperatureC = NAN;
  }

  if (pidEnabled) {
    updatePidControl(now);
  }

  return sensorValid && (heaterTempSensorPin < 0 || heaterTemperatureValid);
}

double DewHeater::calculateDewPointMagnus(double tempC, double relativeHumidityPercent) {
  if (isnan(tempC) || isnan(relativeHumidityPercent) ||
      relativeHumidityPercent <= 0.0 || relativeHumidityPercent > 100.0) {
    return NAN;
  }

  // Magnus formula approximation over water
  const double a = 17.27;
  const double b = 237.7;
  const double alpha = ((a * tempC) / (b + tempC)) + log(relativeHumidityPercent / 100.0);
  return (b * alpha) / (a - alpha);
}

void DewHeater::setHeaterPowerPercent(float percent) {
  if (percent < 0.0f) percent = 0.0f;
  if (percent > 100.0f) percent = 100.0f;
  heaterPowerPercent = percent;

  if (heaterOutputPin >= 0) {
    int pwmValue = map((long)(heaterPowerPercent * 100.0f), 0L, 10000L, 0L, 255L);
    analogWrite(heaterOutputPin, pwmValue);
  }
}

void DewHeater::enablePidControl(bool enable) {
  pidEnabled = enable;
  pidIntegral = 0.0f;
  pidLastError = 0.0f;
  lastPidMs = 0;
}

void DewHeater::setPidTunings(float kp, float ki, float kd) {
  if (kp >= 0.0f) pidKp = kp;
  if (ki >= 0.0f) pidKi = ki;
  if (kd >= 0.0f) pidKd = kd;
}

void DewHeater::setTargetHeaterTemperatureC(float targetC) {
  targetHeaterTemperatureC = targetC;
}

void DewHeater::setTargetAboveDewPointC(float offsetC) {
  dewPointOffsetC = offsetC;
  targetHeaterTemperatureC = NAN;
}

void DewHeater::setHeaterTemperatureOffsetC(float offsetC) {
  heaterTemperatureOffsetC = offsetC;
}

float DewHeater::getActiveTargetTemperatureC() const {
  if (!isnan(targetHeaterTemperatureC)) {
    return targetHeaterTemperatureC;
  }

  if (sensorValid) {
    return dewPointC + dewPointOffsetC;
  }

  return NAN;
}

float DewHeater::readNtcTemperatureC() const {
  // Wiring model:
  // VCC -> fixed resistor (ntcSeriesResistorOhm) -> ADC node -> NTC thermistor -> GND
  // This matches the standard Arduino NTC circuit: VCC - 10k - A0 - NTC - GND
  // Formula: R_ntc = R_series * adc / (adcMax - adc)
  const float adcMax = 1023.0f;
  const float seriesResistorOhm = ntcSeriesResistorOhm;
  const float nominalResistanceOhm = ntcNominalResistanceOhm;
  const float nominalTemperatureK = ntcNominalTemperatureC + 273.15f;
  const float beta = ntcBeta;

  if (seriesResistorOhm <= 0.0f || nominalResistanceOhm <= 0.0f || nominalTemperatureK <= 0.0f || beta <= 0.0f) {
    LOG_ERROR("Invalid NTC parameters");
    return NAN;
  }

  int adc = analogRead(heaterTempSensorPin);

  // Rate-limited debug log (max once every 30 s) — uses stack buffer, no heap allocation
  static unsigned long lastNtcLogMs = 0;
  unsigned long nowMs = millis();
  if (nowMs - lastNtcLogMs >= 30000UL) {
    lastNtcLogMs = nowMs;
    char buf[96];
    snprintf(buf, sizeof(buf), "NTC adc=%d pin=%d R_series=%.0f R0=%.0f T0K=%.2f beta=%.0f Vcc=%.2f Vref=%.2f",
             adc, heaterTempSensorPin, seriesResistorOhm, nominalResistanceOhm, nominalTemperatureK, beta,
             ntcSupplyVoltage, ntcAdcReferenceVoltage);
    LOG_INFO(buf);
  }

  if (adc <= 0 || adc >= (int)adcMax) {
    LOG_ERROR("Invalid ADC reading for NTC: " + String(adc));
    return NAN;
  }

  // VCC -> R_series -> A0 -> NTC -> GND
  // When supply VCC differs from ADC reference voltage (e.g. 5V NTC supply + 3.2V Wemos D1 Mini ADC):
  //   R_ntc = R_series * (adc * V_ref) / (adcMax * VCC - adc * V_ref)
  // When VCC == V_ref this correctly reduces to R_series * adc / (adcMax - adc)
  float adcVref = ntcAdcReferenceVoltage;
  float vcc = ntcSupplyVoltage;
  float numerator = (float)adc * adcVref;
  float denominator = adcMax * vcc - (float)adc * adcVref;
  if (denominator <= 0.0f) {
    LOG_ERROR("NTC denominator <= 0, check VCC/Vref settings");
    return NAN;
  }
  float resistance = seriesResistorOhm * numerator / denominator;
  if (resistance <= 0.0f || isnan(resistance) || isinf(resistance)) {
    LOG_ERROR("Calculated invalid resistance for NTC: " + String(resistance));
    return NAN;
  }

  float lnRatio = log(resistance / nominalResistanceOhm);
  float invT = (1.0f / nominalTemperatureK) + (lnRatio / beta);
  if (invT <= 0.0f || isnan(invT) || isinf(invT)) {
    LOG_ERROR("Calculated invalid inverse temperature for NTC: " + String(invT));
    return NAN;
  }

  float temperatureK = 1.0f / invT;
  float temperatureC = temperatureK - 273.15f;

  // Log the result at the same rate
  if (nowMs == lastNtcLogMs) {
    char buf2[48];
    snprintf(buf2, sizeof(buf2), "NTC resistance=%.1f => %.2f C", resistance, temperatureC);
    LOG_INFO(buf2);
  }

  return temperatureC;
}

void DewHeater::updatePidControl(unsigned long nowMs) {
  if (!pidEnabled || !heaterTemperatureValid) {
    return;
  }

  float targetC = getActiveTargetTemperatureC();
  if (isnan(targetC) || isinf(targetC)) {
    return;
  }

  if (lastPidMs == 0) {
    lastPidMs = nowMs;
    pidLastError = targetC - heaterTemperatureC;
    return;
  }

  float dt = (float)(nowMs - lastPidMs) / 1000.0f;
  if (dt <= 0.0f) {
    return;
  }
  lastPidMs = nowMs;

  float error = targetC - heaterTemperatureC;
  pidIntegral += error * dt;

  if (pidIntegral > 100.0f) pidIntegral = 100.0f;
  if (pidIntegral < -100.0f) pidIntegral = -100.0f;

  float derivative = (error - pidLastError) / dt;
  pidLastError = error;

  float output = pidKp * error + pidKi * pidIntegral + pidKd * derivative;
  if (output < 0.0f) output = 0.0f;
  if (output > 100.0f) output = 100.0f;

  setHeaterPowerPercent(output);
}

void DewHeater::setNtcParameters(float seriesResistorOhm, float nominalResistanceOhm, float nominalTemperatureC, float beta,
                                  float supplyVoltage, float adcReferenceVoltage) {
  if (seriesResistorOhm > 0.0f) ntcSeriesResistorOhm = seriesResistorOhm;
  if (nominalResistanceOhm > 0.0f) ntcNominalResistanceOhm = nominalResistanceOhm;
  if (nominalTemperatureC > -80.0f && nominalTemperatureC < 200.0f) ntcNominalTemperatureC = nominalTemperatureC;
  if (beta > 0.0f) ntcBeta = beta;
  if (supplyVoltage > 0.0f) ntcSupplyVoltage = supplyVoltage;
  if (adcReferenceVoltage > 0.0f) ntcAdcReferenceVoltage = adcReferenceVoltage;
}

int DewHeater::getHeaterOutputPin() const {
  return heaterOutputPin;
}

int DewHeater::getDhtPin() const {
  return dhtPin;
}

int DewHeater::getHeaterTempSensorPin() const {
  return heaterTempSensorPin;
}

uint8_t DewHeater::getHeaterTempSensorType() const {
  return (heaterTempSensorType == HeaterTempSensorType::NTCThermistor) ? 1 : 0;
}

float DewHeater::getHeaterPowerPercent() const {
  return heaterPowerPercent;
}

float DewHeater::getTemperatureC() const {
  return temperatureC;
}

float DewHeater::getHumidityPercent() const {
  return humidityPercent;
}

float DewHeater::getDewPointC() const {
  return dewPointC;
}

float DewHeater::getHeaterTemperatureC() const {
  return heaterTemperatureC;
}

float DewHeater::getHeaterTemperatureOffsetC() const {
  return heaterTemperatureOffsetC;
}

float DewHeater::getTargetHeaterTemperatureC() const {
  return getActiveTargetTemperatureC();
}

float DewHeater::getNtcSeriesResistorOhm() const {
  return ntcSeriesResistorOhm;
}

float DewHeater::getNtcNominalResistanceOhm() const {
  return ntcNominalResistanceOhm;
}

float DewHeater::getNtcNominalTemperatureC() const {
  return ntcNominalTemperatureC;
}

float DewHeater::getNtcBeta() const {
  return ntcBeta;
}

float DewHeater::getNtcSupplyVoltage() const {
  return ntcSupplyVoltage;
}

float DewHeater::getNtcAdcReferenceVoltage() const {
  return ntcAdcReferenceVoltage;
}

bool DewHeater::isSensorValid() const {
  return sensorValid;
}

bool DewHeater::isHeaterTemperatureValid() const {
  return heaterTemperatureValid;
}

bool DewHeater::isPidEnabled() const {
  return pidEnabled;
}