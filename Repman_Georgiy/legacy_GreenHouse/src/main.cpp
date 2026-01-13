#include "main.h"

// Пины
/*
const uint8_t ENC_CLK = 2;
const uint8_t ENC_DT = 3;
const uint8_t ENC_SW = 4;
*/
const uint8_t LIGHT_PIN = 6;
const uint8_t FAN_PIN = 5;
const uint8_t PUMP_PIN = 7;
/*
const uint8_t LED_PIN = 8;
const uint8_t LCD_ADDR = 0x27;
const uint8_t LCD_COLS = 16;
const uint8_t LCD_ROWS = 4;
*/
// Глобальные объекты
SensorManager sensors;
DeviceManager devices(LIGHT_PIN, FAN_PIN, PUMP_PIN);
GreenhouseDisplay display(0x27, 16, 2);

bool systemAutoMode = true;

void runAutoMode();

void setup() {
    Serial.begin(9600);
    //delay(1000);
    sensors.init();
    devices.init();
    display.begin();
}

void updateDisplayWithSensorData() {
    // Передача всех данных сенсоров
    display.setTemperature(sensors.get_air_temp());
    display.setHumidity(sensors.get_air_humidity());
    display.setSoilMoisture1(sensors.get_soil_moisture_1());
    display.setSoilMoisture2(sensors.get_soil_moisture_2());
    display.setLightLevel(sensors.get_light_level());
    display.setWaterVolume(sensors.get_water_volume());
    display.setAirQuality(sensors.get_air_quality());
    display.setTime(sensors.get_hour(),sensors.get_minute());
    // Передача состояния устройств
    display.setLightState(devices.isLightOn());
    display.setFanState(devices.isFanOn());
    display.setPumpState(devices.isPumpOn());
    display.setDate(sensors.get_day(), sensors.get_month(), sensors.get_year());
    // Установка режима работы
    display.setAutoMode(systemAutoMode);
}

void loop() {
    static uint32_t lastSensorUpdate = 0;
    if (millis() - lastSensorUpdate > 1000) {
        sensors.update_all();
        updateDisplayWithSensorData();
        lastSensorUpdate = millis();
        /*
        // Передача данных в UI
        data.temperature = sensors.get_air_temp();
        data.humidity = sensors.get_air_humidity();
        data.soilMoisture = sensors.get_soil_moisture_1();
        data.waterLevel = sensors.get_water_volume();
        data.lightLevel = sensors.get_light_level();
         */
    }
    /*
    // 2. Обновление состояния устройств в UI
    UIController::DeviceState devState;
    devState.lightOn = devices.isLightOn();
    devState.fanOn = devices.isFanOn();
    devState.pumpOn = devices.isPumpOn();
    
    ui.setDeviceState(devState);
    ui.setSystemMode(systemAutoMode);
    */
    // 3. Обновление UI
    display.update();
    //4. Обновление устройств (для насоса с автостопом)
    devices.update();

    if (systemAutoMode) {
        runAutoMode();
    }
    delay(50);
}

// TODO: complete autologic with watering, complete error handler
void runAutoMode() {
    // Простейшая логика автоматического режима
    static uint32_t lastAutoAction = 0;
    if (millis() - lastAutoAction > 10000) {  // Каждые 10 секунд
        lastAutoAction = millis();

        if ((sensors.get_light_level() < 50 || sensors.get_hour() > 20 || sensors.get_hour() < 8)&& !devices.isLightOn()) {
            devices.setLight(true);
        } else if (sensors.get_light_level() > 500 && devices.isLightOn()) {
            devices.setLight(false);
        }

        if (((sensors.get_air_temp() > 28) || (sensors.get_air_humidity() > 75) || (sensors.get_air_quality() > 60)) && !devices.isFanOn()) {
            devices.setFan(true);
        } else if ((sensors.get_air_temp() < 25  || (sensors.get_air_humidity() < 60) || (sensors.get_air_quality() < 50)) && devices.isFanOn()) {
            devices.setFan(false);
        }

        //TODO:Fix magic number usage
        if (((sensors.get_soil_moisture_1() + sensors.get_soil_moisture_2() / 2 < 45)) && !devices.isPumpOn()) {
            devices.startPump(100);
        }

    }
}



/*
void handleUserAction(int actionId, int value) {
    Serial.print(F("Действие: "));
    Serial.print(actionId);
    Serial.print(F(" = "));
    Serial.println(value);
    
    switch (actionId) {
        case 1:  // Свет
            devices.setLight(!devices.isLightOn());
            break;
            
        case 2:  // Вентилятор
            devices.setFan(!devices.isFanOn());
            break;
            
        case 3:  // Полив
            devices.startPump(100);  // 100 мл
            break;
            
        case 4:  // Режим
            systemAutoMode = !systemAutoMode;
            ui.showMessage(systemAutoMode ? F("Авто режим") : F("Ручной режим"));
            break;
            
        case 5:  // Настройки
            ui.showMessage(F("Настройки"), F("в разработке"));
            break;
    }
}
*/