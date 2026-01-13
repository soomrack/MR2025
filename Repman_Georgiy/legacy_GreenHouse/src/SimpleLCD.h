#ifndef GREENHOUSE_DISPLAY_H
#define GREENHOUSE_DISPLAY_H

#include <Arduino.h>
#include <Wire.h>
#include "LiquidCrystal_I2C.h"

class GreenhouseDisplay {
private:
    LiquidCrystal_I2C* lcd;
    bool isInitialized;

    // Режимы отображения
    enum DisplayMode {
        MODE_CLOCK,        // Часы и дата
        MODE_TEMP_HUM,     // Температура и влажность
        MODE_SOIL,         // Влажность почвы
        MODE_LIGHT_WATER,  // Освещенность и вода
        MODE_SYSTEM,       // Состояние системы
        MODE_ERROR         // Ошибки
    };

    DisplayMode currentMode;
    unsigned long lastModeChange;
    unsigned long lastUpdate;
    unsigned long lastBlink;
    bool blinkState;

    // Структура для хранения данных
    struct DisplayData {
        // Время
        uint8_t hour;
        uint8_t minute;
        uint8_t day;
        uint8_t month;
        uint16_t year;

        // Данные датчиков
        float temperature;
        float humidity;
        uint8_t soilMoisture1;
        uint8_t soilMoisture2;
        float lightLevel;
        float waterVolume;
        float airQuality;

        // Состояние системы
        bool isAutoMode;
        bool lightOn;
        bool fanOn;
        bool pumpOn;

        // Ошибки
        bool hasError;
        String errorMessage;
    };

    DisplayData data;

    // Приватные методы
    void updateDisplay();
    void showClock();
    void showTempHum();
    void showSoil();
    void showLightWater();
    void showSystem();
    void showError();

    void printTwoLines(const String& line1, const String& line2);
    void printCenter(const String& text, uint8_t row);
    void scrollText(const String& text, uint8_t row);

    // Вспомогательные функции форматирования
    String formatTime();
    String formatDate();
    String formatTemperature();
    String formatHumidity();
    String formatLight();
    String formatWater();

    // Индикаторы состояния
    void drawStatusIndicators();

public:
    GreenhouseDisplay(uint8_t lcdAddr = 0x27, uint8_t lcdCols = 16, uint8_t lcdRows = 2);

    void begin();
    void update();

    // Установка данных
    void setTime(uint8_t h, uint8_t m);
    void setDate(uint8_t d, uint8_t mo, uint16_t y);
    void setTemperature(float temp);
    void setHumidity(float hum);
    void setSoilMoisture1(uint8_t moisture);
    void setSoilMoisture2(uint8_t moisture);
    void setLightLevel(float level);
    void setWaterVolume(float volume);
    void setAirQuality(float quality);

    // Установка состояния системы
    void setAutoMode(bool autoMode);
    void setLightState(bool state);
    void setFanState(bool state);
    void setPumpState(bool state);

    // Управление ошибками
    void setError(const String& message);
    void clearError();

    // Управление дисплеем
    void clear();
    void backlightOn();
    void backlightOff();
    void setMode(DisplayMode mode);
    void nextMode();

    // Показать сообщение
    void showMessage(const String& line1, const String& line2 = "", uint16_t duration = 2000);

    // Информация о дисплее
    bool isReady() const { return isInitialized; }
};

#endif