#ifndef MY_OBSERVING_CONDITIONS_H
#define MY_OBSERVING_CONDITIONS_H

#include "alpaca_api/Alpaca_Device_ObservingConditions.h"
#include <math.h>

/**
 * @file MyObservingConditions.h
 * @brief Example implementation of ObservingConditions device
 * 
 * This example demonstrates how to create a concrete ObservingConditions device
 * by extending the AlpacaDeviceObservingConditions class and implementing
 * environmental monitoring for temperature, humidity, pressure, wind, rain, etc.
 */

class MyObservingConditions : public AlpacaDeviceObservingConditions {
private:
  // Averaging configuration
  double averagePeriod;           // Hours for averaging readings
  
  // Sensor data
  double temperature;             // Celsius
  double humidity;                // Percentage (0-100)
  double dewPoint;                // Celsius (calculated)
  double pressure;                // hPa (hectopascals)
  double cloudCover;              // Percentage (0-100)
  double rainRate;                // mm/hr
  double skyBrightness;           // Lux
  double skyQuality;              // mag/arcsec^2
  double skyTemperature;          // Celsius
  double starFWHM;                // arcseconds
  double windDirection;           // Degrees (0-360, 0=North)
  double windSpeed;               // m/s
  double windGust;                // m/s (peak 3-sec gust in last 2 min)
  
  // Sensor availability flags
  bool hasTemperatureSensor;
  bool hasHumiditySensor;
  bool hasPressureSensor;
  bool hasRainSensor;
  bool hasWindSensors;
  bool hasCloudSensor;
  bool hasSkyQualitySensor;
  bool hasSkyBrightnessSensor;
  bool hasSkyTemperatureSensor;
  
  // Timing
  unsigned long lastUpdateTime;   // millis() of last sensor read
  unsigned long lastRefreshTime;  // millis() of last manual refresh
  unsigned long autoRefreshInterval; // Auto-refresh interval (ms)
  
  // Sensor pins (optional)
  int temperatureHumidityPin;     // DHT22 data pin
  int rainSensorPin;              // Rain detector analog pin
  int windSpeedPin;               // Anemometer pulse pin
  int windDirectionPin;           // Wind vane analog pin
  
  // Wind tracking
  volatile unsigned long windPulseCount;
  unsigned long lastWindCheckTime;
  double maxWindGust;
  unsigned long gustStartTime;
  
  /**
   * @brief Calculate dew point from temperature and humidity
   */
  double calculateDewPoint(double tempC, double rh) {
    if (rh <= 0.0 || rh > 100.0) return -999.0;
    
    // Magnus formula approximation
    double a = 17.27;
    double b = 237.7;
    double alpha = ((a * tempC) / (b + tempC)) + log(rh / 100.0);
    double dewPt = (b * alpha) / (a - alpha);
    
    return dewPt;
  }
  
  /**
   * @brief Read temperature and humidity sensor (e.g., DHT22)
   */
  void readTemperatureHumidity() {
    if (!hasTemperatureSensor && !hasHumiditySensor) return;
    
    // In real implementation, read from DHT22 or similar sensor
    // For simulation, generate realistic values with small variations
    static double baseTemp = 15.0;
    static double baseHumidity = 60.0;
    
    temperature = baseTemp + (random(-50, 50) / 100.0);
    humidity = baseHumidity + (random(-100, 100) / 100.0);
    
    if (humidity < 0.0) humidity = 0.0;
    if (humidity > 100.0) humidity = 100.0;
    
    // Calculate dew point
    dewPoint = calculateDewPoint(temperature, humidity);
  }
  
  /**
   * @brief Read pressure sensor (e.g., BMP280)
   */
  void readPressure() {
    if (!hasPressureSensor) return;
    
    // In real implementation, read from BMP280 or similar
    // For simulation, generate realistic pressure (1013 hPa average)
    static double basePressure = 1013.0;
    pressure = basePressure + (random(-100, 100) / 10.0);
  }
  
  /**
   * @brief Read rain sensor
   */
  void readRainSensor() {
    if (!hasRainSensor) return;
    
    // In real implementation, read rain detector
    if (rainSensorPin >= 0) {
      int rainValue = analogRead(rainSensorPin);
      // Convert analog reading to rain rate (mm/hr)
      // Higher reading = more rain
      if (rainValue > 512) {
        rainRate = (rainValue - 512) / 512.0 * 10.0; // 0-10 mm/hr
      } else {
        rainRate = 0.0;
      }
    } else {
      // Simulated: mostly no rain, occasional rain
      rainRate = (random(0, 100) > 95) ? random(1, 50) / 10.0 : 0.0;
    }
  }
  
  /**
   * @brief Read wind sensors
   */
  void readWindSensors() {
    if (!hasWindSensors) return;
    
    unsigned long currentTime = millis();
    unsigned long elapsed = currentTime - lastWindCheckTime;
    
    if (windSpeedPin >= 0) {
      // Calculate wind speed from pulse count
      // Typical anemometer: 1 pulse per rotation, calibrated to m/s
      double pulsesPerSec = windPulseCount / (elapsed / 1000.0);
      windSpeed = pulsesPerSec * 0.5; // Example calibration factor
      
      // Track wind gust (max speed in last 2 minutes)
      if (windSpeed > maxWindGust) {
        maxWindGust = windSpeed;
        gustStartTime = currentTime;
      }
      
      // Reset gust if older than 2 minutes
      if (currentTime - gustStartTime > 120000) {
        maxWindGust = windSpeed;
        gustStartTime = currentTime;
      }
      
      windGust = maxWindGust;
      
      // Reset counters
      windPulseCount = 0;
      lastWindCheckTime = currentTime;
    } else {
      // Simulated wind
      windSpeed = random(0, 100) / 10.0; // 0-10 m/s
      windGust = windSpeed * (1.0 + random(0, 50) / 100.0); // Gusts 0-50% higher
    }
    
    // Read wind direction
    if (windDirectionPin >= 0) {
      int dirValue = analogRead(windDirectionPin);
      // Convert analog reading to degrees (0-360)
      windDirection = (dirValue / 1023.0) * 360.0;
    } else {
      // Simulated direction
      static double baseDirection = 180.0;
      windDirection = baseDirection + random(-30, 30);
      if (windDirection < 0.0) windDirection += 360.0;
      if (windDirection >= 360.0) windDirection -= 360.0;
    }
  }
  
  /**
   * @brief Read sky sensors (cloud, brightness, quality, temperature)
   */
  void readSkySensors() {
    // Cloud cover (requires IR sky temperature sensor)
    if (hasCloudSensor) {
      // In real implementation, compare sky IR temp to ambient
      // Clear sky is much colder than cloudy sky
      double skyAmbientDiff = skyTemperature - temperature;
      if (skyAmbientDiff < -20.0) {
        cloudCover = 0.0; // Clear
      } else if (skyAmbientDiff > -5.0) {
        cloudCover = 100.0; // Overcast
      } else {
        cloudCover = ((skyAmbientDiff + 20.0) / 15.0) * 100.0;
      }
    } else {
      // Simulated
      cloudCover = random(0, 100);
    }
    
    // Sky temperature (IR sensor pointing up)
    if (hasSkyTemperatureSensor) {
      // In real implementation, read MLX90614 or similar IR sensor
      // Sky is typically -20 to -40°C below ambient on clear nights
      skyTemperature = temperature - random(15, 40);
    } else {
      skyTemperature = temperature - 25.0;
    }
    
    // Sky brightness (lux meter)
    if (hasSkyBrightnessSensor) {
      // In real implementation, read light sensor
      // Darker is better for astronomy
      skyBrightness = random(0, 1000) / 10.0; // 0-100 lux
    } else {
      skyBrightness = 10.0;
    }
    
    // Sky quality (mag/arcsec^2)
    if (hasSkyQualitySensor) {
      // In real implementation, read Sky Quality Meter (SQM)
      // Range: ~16 (city) to ~22 (excellent dark site)
      // Higher is better
      skyQuality = 18.0 + random(0, 40) / 10.0; // 18.0-22.0
    } else {
      skyQuality = 20.0;
    }
    
    // Star FWHM (seeing - requires auto-guider or similar)
    // Lower is better (sharper stars)
    starFWHM = 2.0 + random(0, 30) / 10.0; // 2.0-5.0 arcseconds
  }
  
  /**
   * @brief Update all sensor readings
   */
  void updateAllSensors() {
    readTemperatureHumidity();
    readPressure();
    readRainSensor();
    readWindSensors();
    readSkySensors();
    
    lastUpdateTime = millis();
    LOG_DEBUG("Sensors updated - Temp: " + String(temperature) + " C, Humidity: " + String(humidity) + " %");
  }

public:
  /**
   * @brief Constructor for MyObservingConditions
   * @param devicename Name of the device
   * @param devicenumber Device number for Alpaca API
   * @param description Human-readable description
   * @param server Reference to AsyncWebServer
   * @param has_temp_sensor Temperature sensor available
   * @param has_humidity_sensor Humidity sensor available
   * @param has_pressure_sensor Pressure sensor available
   * @param has_rain_sensor Rain detector available
   * @param has_wind_sensors Wind speed/direction sensors available
   * @param temp_humidity_pin DHT22 data pin (-1 if not used)
   * @param rain_sensor_pin Rain detector analog pin (-1 if not used)
   * @param wind_speed_pin Anemometer pulse pin (-1 if not used)
   * @param wind_dir_pin Wind vane analog pin (-1 if not used)
   */
  MyObservingConditions(String devicename, int devicenumber, String description, 
                        AsyncWebServer &server,
                        bool has_temp_sensor = true,
                        bool has_humidity_sensor = true,
                        bool has_pressure_sensor = true,
                        bool has_rain_sensor = true,
                        bool has_wind_sensors = false,
                        int temp_humidity_pin = -1,
                        int rain_sensor_pin = -1,
                        int wind_speed_pin = -1,
                        int wind_dir_pin = -1)
    : AlpacaDeviceObservingConditions(devicename, devicenumber, description, server),
      averagePeriod(0.0),
      temperature(15.0),
      humidity(60.0),
      dewPoint(10.0),
      pressure(1013.0),
      cloudCover(0.0),
      rainRate(0.0),
      skyBrightness(10.0),
      skyQuality(20.0),
      skyTemperature(-10.0),
      starFWHM(3.0),
      windDirection(180.0),
      windSpeed(0.0),
      windGust(0.0),
      hasTemperatureSensor(has_temp_sensor),
      hasHumiditySensor(has_humidity_sensor),
      hasPressureSensor(has_pressure_sensor),
      hasRainSensor(has_rain_sensor),
      hasWindSensors(has_wind_sensors),
      hasCloudSensor(false),
      hasSkyQualitySensor(false),
      hasSkyBrightnessSensor(false),
      hasSkyTemperatureSensor(false),
      lastUpdateTime(0),
      lastRefreshTime(0),
      autoRefreshInterval(60000), // 60 seconds
      temperatureHumidityPin(temp_humidity_pin),
      rainSensorPin(rain_sensor_pin),
      windSpeedPin(wind_speed_pin),
      windDirectionPin(wind_dir_pin),
      windPulseCount(0),
      lastWindCheckTime(0),
      maxWindGust(0.0),
      gustStartTime(0) {
    
    // Initialize sensor pins
    if (rainSensorPin >= 0) {
      pinMode(rainSensorPin, INPUT);
    }
    if (windDirectionPin >= 0) {
      pinMode(windDirectionPin, INPUT);
    }
    if (windSpeedPin >= 0) {
      pinMode(windSpeedPin, INPUT_PULLUP);
      // In real implementation, attach interrupt for pulse counting
      // attachInterrupt(digitalPinToInterrupt(windSpeedPin), windPulseISR, FALLING);
    }
    
    // Initial sensor read
    updateAllSensors();
    
    LOG_DEBUG("MyObservingConditions created");
    LOG_DEBUG("  Temperature sensor: " + String(hasTemperatureSensor ? "Yes" : "No"));
    LOG_DEBUG("  Humidity sensor: " + String(hasHumiditySensor ? "Yes" : "No"));
    LOG_DEBUG("  Pressure sensor: " + String(hasPressureSensor ? "Yes" : "No"));
    LOG_DEBUG("  Rain sensor: " + String(hasRainSensor ? "Yes" : "No"));
    LOG_DEBUG("  Wind sensors: " + String(hasWindSensors ? "Yes" : "No"));
  }
  
  // ==================== IObservingConditions Interface Implementation ====================
  
  double GetAveragePeriod() override {
    return averagePeriod;
  }
  
  void SetAveragePeriod(double hours) override {
    if (hours < 0.0) {
      LOG_DEBUG("Invalid average period: " + String(hours));
      return;
    }
    averagePeriod = hours;
    LOG_DEBUG("Average period set to: " + String(averagePeriod) + " hours");
  }
  
  double GetCloudCover() override {
    return cloudCover;
  }
  
  double GetDewPoint() override {
    return dewPoint;
  }
  
  double GetHumidity() override {
    return humidity;
  }
  
  double GetPressure() override {
    return pressure;
  }
  
  double GetRainRate() override {
    return rainRate;
  }
  
  double GetSkyBrightness() override {
    return skyBrightness;
  }
  
  double GetSkyQuality() override {
    return skyQuality;
  }
  
  double GetSkyTemperature() override {
    return skyTemperature;
  }
  
  double GetStarFWHM() override {
    return starFWHM;
  }
  
  double GetTemperature() override {
    return temperature;
  }
  
  double GetWindDirection() override {
    return windDirection;
  }
  
  double GetWindGust() override {
    return windGust;
  }
  
  double GetWindSpeed() override {
    return windSpeed;
  }
  
  void Refresh() override {
    LOG_DEBUG("Manual refresh requested");
    updateAllSensors();
    lastRefreshTime = millis();
  }
  
  std::string GetSensorDescription(const std::string& sensorName) override {
    if (sensorName == "Temperature" || sensorName == "temperature") {
      return hasTemperatureSensor ? 
        "DHT22 temperature sensor, accuracy ±0.5°C" : 
        "Simulated temperature sensor";
    }
    else if (sensorName == "Humidity" || sensorName == "humidity") {
      return hasHumiditySensor ? 
        "DHT22 humidity sensor, accuracy ±2% RH" : 
        "Simulated humidity sensor";
    }
    else if (sensorName == "DewPoint" || sensorName == "dewpoint") {
      return "Calculated from temperature and humidity using Magnus formula";
    }
    else if (sensorName == "Pressure" || sensorName == "pressure") {
      return hasPressureSensor ? 
        "BMP280 barometric pressure sensor, accuracy ±1 hPa" : 
        "Simulated pressure sensor";
    }
    else if (sensorName == "CloudCover" || sensorName == "cloudcover") {
      return hasCloudSensor ? 
        "IR sky temperature comparison for cloud detection" : 
        "Simulated cloud cover";
    }
    else if (sensorName == "RainRate" || sensorName == "rainrate") {
      return hasRainSensor ? 
        "Capacitive rain detector with analog output" : 
        "Simulated rain detector";
    }
    else if (sensorName == "SkyBrightness" || sensorName == "skybrightness") {
      return hasSkyBrightnessSensor ? 
        "Light sensor measuring sky brightness in lux" : 
        "Simulated sky brightness";
    }
    else if (sensorName == "SkyQuality" || sensorName == "skyquality") {
      return hasSkyQualitySensor ? 
        "Sky Quality Meter (SQM) measuring mag/arcsec²" : 
        "Simulated sky quality";
    }
    else if (sensorName == "SkyTemperature" || sensorName == "skytemperature") {
      return hasSkyTemperatureSensor ? 
        "MLX90614 IR temperature sensor pointing at sky" : 
        "Simulated sky temperature";
    }
    else if (sensorName == "StarFWHM" || sensorName == "starfwhm") {
      return "Calculated seeing from auto-guider or simulated";
    }
    else if (sensorName == "WindDirection" || sensorName == "winddirection") {
      return hasWindSensors ? 
        "Wind vane with analog output, 0-360° (0=North)" : 
        "Simulated wind direction";
    }
    else if (sensorName == "WindSpeed" || sensorName == "windspeed") {
      return hasWindSensors ? 
        "Anemometer with pulse output, calibrated to m/s" : 
        "Simulated wind speed";
    }
    else if (sensorName == "WindGust" || sensorName == "windgust") {
      return hasWindSensors ? 
        "Peak 3-second wind gust over last 2 minutes from anemometer" : 
        "Simulated wind gust";
    }
    else {
      return "Unknown sensor: " + sensorName;
    }
  }
  
  double GetTimeSinceLastUpdate(const std::string& sensorName) override {
    // All sensors are updated together, so return same time for all
    unsigned long currentTime = millis();
    double timeSinceSec = (currentTime - lastUpdateTime) / 1000.0;
    
    LOG_DEBUG("Time since last update for " + sensorName + ": " + String(timeSinceSec) + " seconds");
    return timeSinceSec;
  }
  
  // ==================== Additional Methods ====================
  
  /**
   * @brief Update sensor readings periodically
   * Call this from loop() to enable automatic refresh
   */
  void update() {
    unsigned long currentTime = millis();
    
    // Auto-refresh sensors at configured interval
    if (currentTime - lastUpdateTime >= autoRefreshInterval) {
      updateAllSensors();
    }
  }
  
  /**
   * @brief Set auto-refresh interval
   * @param intervalMs Interval in milliseconds
   */
  void setAutoRefreshInterval(unsigned long intervalMs) {
    autoRefreshInterval = intervalMs;
    LOG_DEBUG("Auto-refresh interval set to: " + String(autoRefreshInterval) + " ms");
  }
  
  /**
   * @brief Enable/disable sky sensors
   */
  void setSkySensorsAvailable(bool cloud, bool quality, bool brightness, bool temperature) {
    hasCloudSensor = cloud;
    hasSkyQualitySensor = quality;
    hasSkyBrightnessSensor = brightness;
    hasSkyTemperatureSensor = temperature;
    LOG_DEBUG("Sky sensors availability updated");
  }
  
  /**
   * @brief Check if conditions are safe for observing
   * @return true if safe, false if unsafe
   */
  bool isSafeToObserve() const {
    bool safe = true;
    
    // Check for rain
    if (hasRainSensor && rainRate > 0.1) {
      LOG_DEBUG("Unsafe: Rain detected - " + String(rainRate) + " mm/hr");
      safe = false;
    }
    
    // Check for high winds
    if (hasWindSensors && windSpeed > 15.0) {
      LOG_DEBUG("Unsafe: High wind - " + String(windSpeed) + " m/s");
      safe = false;
    }
    
    // Check humidity (high humidity = dew risk)
    if (hasHumiditySensor && humidity > 85.0) {
      LOG_DEBUG("Warning: High humidity - " + String(humidity) + " %");
    }
    
    return safe;
  }
  
  /**
   * @brief Get weather summary string
   */
  String getWeatherSummary() const {
    String summary = "Weather: ";
    summary += String(temperature, 1) + "°C, ";
    summary += String(humidity, 0) + "% RH, ";
    summary += String(pressure, 1) + " hPa";
    
    if (hasWindSensors) {
      summary += ", Wind: " + String(windSpeed, 1) + " m/s @ " + String(windDirection, 0) + "°";
    }
    
    if (hasRainSensor && rainRate > 0.0) {
      summary += ", Rain: " + String(rainRate, 1) + " mm/hr";
    }
    
    return summary;
  }
};

#endif // MY_OBSERVING_CONDITIONS_H
