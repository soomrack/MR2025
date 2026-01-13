#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
#include "Adafruit_VEML7700.h"
#include "AHTxx.h"
#include "DS1307RTC.h"
#include "ScioSense_ENS160.h"
#include "HCSR04.h"

class SensorManager {
private:
    struct SensorReadings {
        float light_lux;
        float air_temp;
        float air_qual;
        float air_hum;
        float water_dist_cm;
        float water_volume_ml;
        uint8_t soil_moist_1;
        uint8_t soil_moist_2;
        uint8_t hour, minute, second;
        uint8_t day, month, year;

        bool light_sensor_ok;
        bool air_temp_sensor_ok;
        bool air_qual_sensor_ok;
        bool soil_sensor_1_ok;
        bool soil_sensor_2_ok;
        bool rtc_ok;
        bool water_sensor_ok;
    };

    SensorReadings readings{};

    Adafruit_VEML7700 veml;
    AHTxx aht20;
    ScioSense_ENS160 ens160;
    HCSR04 hc;
    tmElements_t tm{};

    static const uint8_t TRIG_PIN = 11;
    static const uint8_t ECHO_PIN = 12;
    static const uint8_t SOIL_1_PIN = A0;
    static const uint8_t SOIL_2_PIN = A1;
    static const uint16_t LIGHT_LOW_THRESHOLD = 10000U;
    static const uint16_t LIGHT_HIGH_THRESHOLD = 10000U;
    static const uint16_t TANK_DIAMETER_CM = 100U;
    static const uint16_t TANK_HEIGHT_CM = 30U;
    static const uint16_t SOIL_DRY_VALUE = 470U;
    static const uint16_t SOIL_WET_VALUE = 350U;

    bool init_light_sensor();
    bool init_air_temp_hum_sensor();
    bool init_air_qual_sensor();
    bool init_rtc();

    float read_light_sensor();
    float read_air_temp_sensor();
    float read_air_hum_sensor();
    float read_air_quality_sensor();
    static uint8_t read_soil_sensor(uint8_t pin);
    void read_rtc_time();

    // Вспомогательные
    static bool check_soil_sensor(uint8_t pin);
    static float calculate_water_volume(float distance_cm);
    static uint8_t convert_soil_reading(int raw_value);

public:
    SensorManager();

    bool init();
    void update_all();

    SensorReadings get_readings() const { return readings; }

    bool is_light_sensor_ok() const { return readings.light_sensor_ok; }
    bool is_air_temp_sensor_ok() const { return readings.air_temp_sensor_ok; }
    bool is_air_qual_sensor_ok() const { return readings.air_qual_sensor_ok; }
    bool is_soil_sensor_1_ok() const { return readings.soil_sensor_1_ok; }
    bool is_soil_sensor_2_ok() const { return readings.soil_sensor_2_ok; }
    bool is_rtc_ok() const { return readings.rtc_ok; }
    bool is_water_sensor_ok() const { return readings.water_sensor_ok; }

    float get_light_level() const { return readings.light_lux; }
    float get_air_temp() const { return readings.air_temp; }
    float get_air_humidity() const { return readings.air_hum; }
    float get_air_quality() const { return readings.air_qual; }
    uint16_t get_soil_moisture_1() const { return readings.soil_moist_1; }
    uint16_t get_soil_moisture_2() const { return readings.soil_moist_2; }
    float get_water_distance() const { return readings.water_dist_cm; }
    float get_water_volume() const { return readings.water_volume_ml; }
    uint8_t get_hour() const { return readings.hour;}
    uint8_t get_minute() const { return readings.minute;}
    uint8_t get_second() const { return readings.second;}
    uint16_t get_year() const  { return readings.year;}
    uint8_t get_month() const { return readings.month;}
    uint8_t get_day() const {return readings.day;}
    // I don't know how to fix this -_- //
    float read_water_distance_sensor() {
        float val = hc.dist()/10;
        delay(60);
        return (val);
    }
};

#endif