#include "DewHeater.h"

#include <math.h>

DewHeater::DewHeater(int heater_pin, int dht11_pin, unsigned long interval_ms, int ds18b20_pin)
    : heaterOutputPin(heater_pin),
      dhtPin(dht11_pin),
      heaterTempSensorPin(ds18b20_pin),
      dht(dht11_pin, DHT11),
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
    analogWrite(heaterOutputPin, 0);
  }

  if (dhtPin >= 0) {
    dht.begin();
  }

  if (heaterTempSensorPin >= 0 && heaterSensors == nullptr) {
    heaterOneWire = new OneWire(heaterTempSensorPin);
    heaterSensors = new DallasTemperature(heaterOneWire);
    heaterSensors->begin();
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
    float newHumidity = dht.readHumidity();
    float newTemperature = dht.readTemperature();

    if (isnan(newHumidity) || isnan(newTemperature) ||
        newHumidity < 0.0f || newHumidity > 100.0f) {
      sensorValid = false;
      temperatureC = NAN;
      humidityPercent = NAN;
      dewPointC = NAN;
    } else {
      humidityPercent = newHumidity;
      temperatureC = newTemperature;
      dewPointC = (float)calculateDewPointMagnus(temperatureC, humidityPercent);
      sensorValid = !isnan(dewPointC) && !isinf(dewPointC);
    }
  } else {
    sensorValid = false;
    temperatureC = NAN;
    humidityPercent = NAN;
    dewPointC = NAN;
  }

  if (heaterSensors != nullptr) {
    heaterSensors->requestTemperatures();
    float newHeaterTemperature = heaterSensors->getTempCByIndex(0);

    if (newHeaterTemperature == DEVICE_DISCONNECTED_C ||
        newHeaterTemperature == 85.0f ||
        isnan(newHeaterTemperature) ||
        isinf(newHeaterTemperature) ||
        newHeaterTemperature < -55.0f ||
        newHeaterTemperature > 125.0f) {
      heaterTemperatureValid = false;
      heaterTemperatureC = NAN;
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

int DewHeater::getHeaterOutputPin() const {
  return heaterOutputPin;
}

int DewHeater::getDhtPin() const {
  return dhtPin;
}

int DewHeater::getHeaterTempSensorPin() const {
  return heaterTempSensorPin;
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

bool DewHeater::isSensorValid() const {
  return sensorValid;
}

bool DewHeater::isHeaterTemperatureValid() const {
  return heaterTemperatureValid;
}

bool DewHeater::isPidEnabled() const {
  return pidEnabled;
}