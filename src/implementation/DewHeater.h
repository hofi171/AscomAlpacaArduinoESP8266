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
 * @brief Helper class for dew heater control using a DHT22/AM2302 and DS18B20 sensor.
 *
 * This class reads ambient temperature and relative humidity from a DHT22/AM2302
 * sensor, reads heater temperature from a DS18B20 sensor, calculates the
 * dew point (Taupunkt) using the Magnus formula, and can regulate the heater
 * output using a PID controller over PWM.
 */
class DewHeater {
private:
  enum class HeaterTempSensorType : uint8_t {
    DS18B20 = 0,
    NTCThermistor = 1
  };

  int heaterOutputPin;
  int dhtPin;
  int heaterTempSensorPin;
  HeaterTempSensorType heaterTempSensorType;
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

  float ntcSeriesResistorOhm;
  float ntcNominalResistanceOhm;
  float ntcNominalTemperatureC;
  float ntcBeta;
  float ntcSupplyVoltage;       // VCC applied to the NTC divider (e.g. 5.0 or 3.3)
  float ntcAdcReferenceVoltage; // ADC full-scale voltage at the MCU pin (e.g. 3.2 for Wemos D1 Mini, 3.3 for plain ESP8266)

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
  float readNtcTemperatureC() const;

public:
  DewHeater(int heater_pin = -1, int dht22_pin = -1, unsigned long interval_ms = 2000UL, int heater_temp_pin = -1,
            uint8_t heater_temp_sensor_type = 0);
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
  void setNtcParameters(float seriesResistorOhm, float nominalResistanceOhm, float nominalTemperatureC, float beta,
                         float supplyVoltage = 3.3f, float adcReferenceVoltage = 3.3f);

  int getHeaterOutputPin() const;
  int getDhtPin() const;
  int getHeaterTempSensorPin() const;
  uint8_t getHeaterTempSensorType() const;
  float getHeaterPowerPercent() const;
  float getTemperatureC() const;
  float getHumidityPercent() const;
  float getDewPointC() const;
  float getHeaterTemperatureC() const;
  float getHeaterTemperatureOffsetC() const;
  float getTargetHeaterTemperatureC() const;
  float getNtcSeriesResistorOhm() const;
  float getNtcNominalResistanceOhm() const;
  float getNtcNominalTemperatureC() const;
  float getNtcBeta() const;
  float getNtcSupplyVoltage() const;
  float getNtcAdcReferenceVoltage() const;
  bool isSensorValid() const;
  bool isHeaterTemperatureValid() const;
  bool isPidEnabled() const;
};

#endif // ARDUINO_DEW_HEATER_H


#endif /* A5E91EB7_4A79_42FC_8BCD_5B6E0FBB3933 */
