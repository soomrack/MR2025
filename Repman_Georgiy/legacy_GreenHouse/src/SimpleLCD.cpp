#include "SimpleLCD.h"

// Конструктор
GreenhouseDisplay::GreenhouseDisplay(uint8_t lcdAddr, uint8_t lcdCols, uint8_t lcdRows)
        : isInitialized(false), currentMode(MODE_CLOCK), blinkState(false) {

    lcd = new LiquidCrystal_I2C(lcdAddr, lcdCols, lcdRows);

    // Инициализация данных нулевыми значениями
    memset(&data, 0, sizeof(data));
    data.isAutoMode = true;
    data.hasError = false;
}

// Инициализация дисплея
void GreenhouseDisplay::begin() {
    lcd->begin();
    lcd->backlight();
    lcd->clear();

    isInitialized = true;
    lastModeChange = millis();
    lastUpdate = millis();
    lastBlink = millis();

    // Приветственное сообщение
    lcd->setCursor(0, 0);
    lcd->print("Smart Greenhouse");
    lcd->setCursor(0, 1);
    lcd->print("ver. 0");
    delay(1500);
    lcd->clear();
}

// Основной метод обновления
void GreenhouseDisplay::update() {
    if (!isInitialized) return;

    unsigned long currentTime = millis();

    // Автопереключение режимов каждые 8 секунд
    if (currentTime - lastModeChange > 8000) {
        if (!data.hasError) { // Не переключаем при ошибке
            nextMode();
        }
        lastModeChange = currentTime;
    }

    // Обновление дисплея каждую секунду
    if (currentTime - lastUpdate > 1000) {
        updateDisplay();
        lastUpdate = currentTime;
    }

    // Мигание для индикаторов
    if (currentTime - lastBlink > 500) {
        blinkState = !blinkState;
        lastBlink = currentTime;
    }
}

// Обновление отображения
void GreenhouseDisplay::updateDisplay() {
    lcd->clear();

    if (data.hasError) {
        showError();
    } else {
        switch (currentMode) {
            case MODE_CLOCK:
                showClock();
                break;
            case MODE_TEMP_HUM:
                showTempHum();
                break;
            case MODE_SOIL:
                showSoil();
                break;
            case MODE_LIGHT_WATER:
                showLightWater();
                break;
            case MODE_SYSTEM:
                showSystem();
                break;
            case MODE_ERROR:
                showError();
                break;
        }
    }

    // Отображение индикаторов состояния
    drawStatusIndicators();
}

// Режим: Часы и дата
void GreenhouseDisplay::showClock() {
    lcd->setCursor(0, 0);
    lcd->print("Time: ");
    lcd->print(formatTime());

    lcd->setCursor(0, 1);
    lcd->print("Date: ");
    lcd->print(formatDate());
}

// Режим: Температура и влажность
void GreenhouseDisplay::showTempHum() {
    lcd->setCursor(0, 0);
    lcd->print("T:");
    lcd->print(formatTemperature());
    lcd->print("C");

    lcd->setCursor(8, 0);
    lcd->print("H:");
    lcd->print(formatHumidity());
    lcd->print("%");

    // Качество воздуха на второй строке
    lcd->setCursor(0, 1);
    lcd->print("AQI:");
    lcd->print(data.airQuality, 0);

    // Индикатор качества
    lcd->setCursor(9, 1);
    if (data.airQuality < 50) {
        lcd->print("Good");
    } else if (data.airQuality < 100) {
        lcd->print("Fair");
    } else {
        lcd->print("Poor");
    }
}

// Режим: Влажность почвы
void GreenhouseDisplay::showSoil() {
    uint8_t avgSoil = (data.soilMoisture1 + data.soilMoisture2) / 2;

    lcd->setCursor(0, 0);
    lcd->print("Soil Moisture");

    lcd->setCursor(0, 1);
    lcd->print("S1:");
    lcd->print(data.soilMoisture1);
    lcd->print("% ");

    lcd->setCursor(8, 1);
    lcd->print("S2:");
    lcd->print(data.soilMoisture2);
    lcd->print("%");

    // Графический индикатор средней влажности
    lcd->setCursor(14, 1);
    uint8_t level = map(avgSoil, 0, 100, 0, 5);
    char levelChar;
    switch (level) {
        case 0: levelChar = ' '; break;
        case 1: levelChar = 1; break;   // Нижний блок
        case 2: levelChar = 2; break;   // 2 блока
        case 3: levelChar = 3; break;   // 3 блока
        case 4: levelChar = 4; break;   // 4 блока
        case 5: levelChar = 5; break;   // Полный
        default: levelChar = '?';
    }
    lcd->write(levelChar);
}

// Режим: Освещенность и вода
void GreenhouseDisplay::showLightWater() {
    lcd->setCursor(0, 0);
    lcd->print("Light: ");
    lcd->print(formatLight());

    lcd->setCursor(0, 1);
    lcd->print("Water: ");
    lcd->print(formatWater());

    // Индикатор уровня воды (графический)
    lcd->setCursor(14, 1);
    if (data.waterVolume > 2000) {
        lcd->write(5);  // Полный
    } else if (data.waterVolume > 1000) {
        lcd->write(4);  // Высокий
    } else if (data.waterVolume > 500) {
        lcd->write(3);  // Средний
    } else if (data.waterVolume > 100) {
        lcd->write(2);  // Низкий
    } else if (data.waterVolume > 0) {
        lcd->write(1);  // Очень низкий
    } else {
        lcd->print("!"); // Пусто
    }
}

// Режим: Состояние системы
void GreenhouseDisplay::showSystem() {
    lcd->setCursor(0, 0);
    lcd->print("Mode: ");
    lcd->print(data.isAutoMode ? "Auto" : "Manual");

    lcd->setCursor(0, 1);
    lcd->print("L:");
    lcd->print(data.lightOn ? "ON " : "OFF");

    lcd->setCursor(5, 1);
    lcd->print("F:");
    lcd->print(data.fanOn ? "ON " : "OFF");

    lcd->setCursor(10, 1);
    lcd->print("P:");
    lcd->print(data.pumpOn ? "ON" : "OFF");
}

// Режим: Ошибки
void GreenhouseDisplay::showError() {
    lcd->setCursor(0, 0);
    lcd->print("! ERROR !");

    // Прокрутка текста ошибки на второй строке
    static uint8_t scrollPos = 0;
    String displayText;

    if (data.errorMessage.length() <= 16) {
        displayText = data.errorMessage;
    } else {
        displayText = data.errorMessage.substring(scrollPos, scrollPos + 16);
        scrollPos++;
        if (scrollPos > data.errorMessage.length() - 16) {
            scrollPos = 0;
        }
    }

    lcd->setCursor(0, 1);
    lcd->print(displayText);

    // Мигание подсветкой при ошибке
    if (blinkState) {
        lcd->backlight();
    } else {
        lcd->noBacklight();
    }
}

// Индикаторы состояния в углу экрана
void GreenhouseDisplay::drawStatusIndicators() {
    if (data.hasError) return;

    // Индикатор режима в правом верхнем углу
    lcd->setCursor(15, 0);
    if (data.isAutoMode) {
        lcd->print("A");
    } else {
        lcd->print("M");
    }

    // Индикатор ошибки или состояния
    lcd->setCursor(15, 1);
    if (data.hasError) {
        lcd->print("!");
    } else if (data.pumpOn) {
        lcd->print("P");
    } else if (data.lightOn && blinkState) {
        lcd->print("*");
    } else if (data.fanOn && blinkState) {
        lcd->print("~");
    } else {
        lcd->print(" ");
    }
}

// Форматирование времени
String GreenhouseDisplay::formatTime() {
    char buffer[9];
    sprintf(buffer, "%02d:%02d", data.hour, data.minute);
    return String(buffer);
}

// Форматирование даты
String GreenhouseDisplay::formatDate() {
    char buffer[11];
    //TODO: Fix (everything)
    sprintf(buffer, "%02d.%02d.%02d", data.day, data.month, data.year%100 - 8);
    return String(buffer);
}

// Форматирование температуры
String GreenhouseDisplay::formatTemperature() {
    char buffer[6];
    dtostrf(data.temperature, 4, 1, buffer);
    return String(buffer);
}

// Форматирование влажности
String GreenhouseDisplay::formatHumidity() {
    char buffer[6];
    dtostrf(data.humidity, 3, 0, buffer);
    return String(buffer);
}

// Форматирование освещенности
String GreenhouseDisplay::formatLight() {
    char buffer[8];
    if (data.lightLevel < 1000) {
        dtostrf(data.lightLevel, 4, 0, buffer);
        return String(buffer) + "lx";
    } else {
        dtostrf(data.lightLevel / 1000.0, 4, 1, buffer);
        return String(buffer) + "klx";
    }
}

// Форматирование объема воды
String GreenhouseDisplay::formatWater() {
    char buffer[8];
    if (data.waterVolume < 1000) {
        dtostrf(data.waterVolume, 4, 0, buffer);
        return String(buffer) + "ml";
    } else {
        dtostrf(data.waterVolume / 1000.0, 4, 1, buffer);
        return String(buffer) + "L";
    }
}

// Методы установки данных
void GreenhouseDisplay::setTime(uint8_t h, uint8_t m) {
    data.hour = h;
    data.minute = m;
}

void GreenhouseDisplay::setDate(uint8_t d, uint8_t mo, uint16_t y) {
    data.day = d;
    data.month = mo;
    data.year = y;
}

void GreenhouseDisplay::setTemperature(float temp) {
    data.temperature = temp;
}

void GreenhouseDisplay::setHumidity(float hum) {
    data.humidity = hum;
}

void GreenhouseDisplay::setSoilMoisture1(uint8_t moisture) {
    data.soilMoisture1 = moisture;
}

void GreenhouseDisplay::setSoilMoisture2(uint8_t moisture) {
    data.soilMoisture2 = moisture;
}

void GreenhouseDisplay::setLightLevel(float level) {
    data.lightLevel = level;
}

void GreenhouseDisplay::setWaterVolume(float volume) {
    data.waterVolume = volume;
}

void GreenhouseDisplay::setAirQuality(float quality) {
    data.airQuality = quality;
}

void GreenhouseDisplay::setAutoMode(bool autoMode) {
    data.isAutoMode = autoMode;
}

void GreenhouseDisplay::setLightState(bool state) {
    data.lightOn = state;
}

void GreenhouseDisplay::setFanState(bool state) {
    data.fanOn = state;
}

void GreenhouseDisplay::setPumpState(bool state) {
    data.pumpOn = state;
}

void GreenhouseDisplay::setError(const String& message) {
    data.hasError = true;
    data.errorMessage = message;
    setMode(MODE_ERROR);
}

void GreenhouseDisplay::clearError() {
    data.hasError = false;
    data.errorMessage = "";
    lcd->backlight();
}

void GreenhouseDisplay::clear() {
    lcd->clear();
}

void GreenhouseDisplay::backlightOn() {
    lcd->backlight();
}

void GreenhouseDisplay::backlightOff() {
    lcd->noBacklight();
}

void GreenhouseDisplay::setMode(DisplayMode mode) {
    currentMode = mode;
    lastModeChange = millis();
}

void GreenhouseDisplay::nextMode() {
    currentMode = static_cast<DisplayMode>((currentMode + 1) % 5); // 5 режимов кроме ERROR
    lcd->clear();
}

void GreenhouseDisplay::showMessage(const String& line1, const String& line2, uint16_t duration) {
    lcd->clear();

    if (line1.length() > 0) {
        lcd->setCursor(0, 0);
        lcd->print(line1);
    }

    if (line2.length() > 0) {
        lcd->setCursor(0, 1);
        lcd->print(line2);
    }

    delay(duration);
    lcd->clear();
}