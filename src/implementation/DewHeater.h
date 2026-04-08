#ifndef A5E91EB7_4A79_42FC_8BCD_5B6E0FBB3933
#define A5E91EB7_4A79_42FC_8BCD_5B6E0FBB3933
#ifndef ARDUINO_DEW_HEATER_H
#define ARDUINO_DEW_HEATER_H

#include <Arduino.h>
#include <DHT.h>
#include <DallasTemperature.h>
#include <OneWire.h>

/**
 * @file DewHeater.h
 * @brief Helper class for dew heater control using a DHT11 and DS18B20 sensor.
 *
 * This class reads ambient temperature and relative humidity from a DHT11
 * sensor, reads heater temperature from a DS18B20 sensor, calculates the
 * dew point (Taupunkt) using the Magnus formula, and can regulate the heater
 * output using a PID controller over PWM.
 */
class DewHeater {
private:
  int heaterOutputPin;
  int dhtPin;
  int heaterTempSensorPin;
  DHT dht;
  OneWire *heaterOneWire;
  DallasTemperature *heaterSensors;

  float heaterPowerPercent;
  float temperatureC;
  float humidityPercent;
  float dewPointC;
  float heaterTemperatureC;
  float heaterTemperatureOffsetC;
  bool sensorValid;
  bool heaterTemperatureValid;

  bool pidEnabled;
  float targetHeaterTemperatureC;
  float dewPointOffsetC;
  float pidKp;
  float pidKi;
  float pidKd;
  float pidIntegral;
  float pidLastError;
  unsigned long lastReadMs;
  unsigned long lastPidMs;
  unsigned long readIntervalMs;

  void updatePidControl(unsigned long nowMs);
  float getActiveTargetTemperatureC() const;

public:
  DewHeater(int heater_pin = -1, int dht11_pin = -1, unsigned long interval_ms = 2000UL, int ds18b20_pin = -1);
  ~DewHeater();

  void begin();
  bool update();

  static double calculateDewPointMagnus(double tempC, double relativeHumidityPercent);

  void setHeaterPowerPercent(float percent);
  void enablePidControl(bool enable);
  void setPidTunings(float kp, float ki, float kd);
  void setTargetHeaterTemperatureC(float targetC);
  void setTargetAboveDewPointC(float offsetC);
  void setHeaterTemperatureOffsetC(float offsetC);

  int getHeaterOutputPin() const;
  int getDhtPin() const;
  int getHeaterTempSensorPin() const;
  float getHeaterPowerPercent() const;
  float getTemperatureC() const;
  float getHumidityPercent() const;
  float getDewPointC() const;
  float getHeaterTemperatureC() const;
  float getHeaterTemperatureOffsetC() const;
  float getTargetHeaterTemperatureC() const;
  bool isSensorValid() const;
  bool isHeaterTemperatureValid() const;
  bool isPidEnabled() const;
};

#endif // ARDUINO_DEW_HEATER_H


#endif /* A5E91EB7_4A79_42FC_8BCD_5B6E0FBB3933 */
