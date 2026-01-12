#include <DHT.h>

// Глобальные объекты
#define TEMP_HUMID_SENSOR_PIN 12
#define SOIL_MOISTURE_SENSOR_PIN_1 1
#define SOIL_MOISTURE_SENSOR_PIN_2 2  
#define SOIL_MOISTURE_SENSOR_PIN_3 3
#define LIGHT_SENSOR_PIN 0
#define PUMP_PIN 5 
#define LIGHT_PIN 6
#define HEAT_PIN 4
#define VENT_PIN 7

// Константы для диагностики датчиков
const int SENSOR_ERROR_VALUE = -1;
const int SENSOR_OUT_OF_RANGE = 4096; // Для Arduino 12-bit ADC
const unsigned long SENSOR_TIMEOUT = 5000; // 5 секунд для обнаружения неисправности

// Класс датчика температуры
class Thermometer {
public:
    int temperature;
    int humidity;
    
    void get_data(DHT& dht) {
        temperature = dht.readTemperature();
        humidity = dht.readHumidity();
    }
};

// Класс умного датчика влажности почвы с диагностикой
class SmartSoilMoistureSensor {
private:
    int pin;
    int lastValidValue;
    unsigned long lastValidReadTime;
    bool isFunctional;
    
public:
    int currentValue;
    bool low_moisture;
    String name;
    
    SmartSoilMoistureSensor(int p, String sensorName) : pin(p), name(sensorName) {
        lastValidValue = 0;
        lastValidReadTime = 0;
        isFunctional = true;
        currentValue = 0;
        low_moisture = false;
    }
    
    bool read_data() {
        int rawValue = analogRead(pin);
        
        // Проверка на аппаратные ошибки
        if (rawValue == SENSOR_ERROR_VALUE || rawValue >= SENSOR_OUT_OF_RANGE) {
            Serial.print("ОШИБКА датчика ");
            Serial.print(name);
            Serial.print(": значение=");
            Serial.println(rawValue);
            isFunctional = false;
            return false;
        }
        
        // Проверка на залипание значения (не меняется длительное время)
        if (abs(rawValue - lastValidValue) < 10) { // Минимальное изменение
            if (millis() - lastValidReadTime > SENSOR_TIMEOUT) {
                Serial.print("ПРЕДУПРЕЖДЕНИЕ: Датчик ");
                Serial.print(name);
                Serial.println(" возможно залип");
            }
        } else {
            lastValidReadTime = millis();
        }
        
        currentValue = rawValue;
        lastValidValue = rawValue;
        isFunctional = true;
        return true;
    }
    
    bool is_healthy() {
        return isFunctional;
    }
    
    void reset_health() {
        isFunctional = true;
        lastValidReadTime = millis();
    }
};

// Класс массива из трех датчиков с резервированием
class SoilMoistureSensorArray {
private:
    static const int SENSOR_COUNT = 3;
    SmartSoilMoistureSensor* sensors[SENSOR_COUNT];
    int activeSensors[SENSOR_COUNT]; // Индексы работающих датчиков
    int activeCount;
    
public:
    int moisture1, moisture2, moisture3;
    bool low_moisture1, low_moisture2, low_moisture3;
    
    SoilMoistureSensorArray() : activeCount(0) {
        for (int i = 0; i < SENSOR_COUNT; i++) {
            sensors[i] = nullptr;
            activeSensors[i] = -1;
        }
    }
    
    void add_sensor(SmartSoilMoistureSensor* sensor, int index) {
        if (index >= 0 && index < SENSOR_COUNT) {
            sensors[index] = sensor;
        }
    }

    
    void update_sensor_health() {
    activeCount = 0;
    for (int i = 0; i < SENSOR_COUNT; i++) {
        if (sensors[i] && sensors[i]->is_healthy()) {
            activeSensors[activeCount++] = i;
        }
    }
    
    // Просто считаем количество рабочих датчиков
    // Восстановление аппаратных датчиков невозможно
    if (activeCount == 0) {
        Serial.println("ОШИБКА: Все датчики влажности почвы неисправны");
    }
}
    
    void get_data() {
        // Чтение данных со всех датчиков
        for (int i = 0; i < SENSOR_COUNT; i++) {
            if (sensors[i]) {
                sensors[i]->read_data();
            }
        }
        
        // Обновление статуса работоспособности
        update_sensor_health();
        
        // Стратегия использования данных в зависимости от количества работающих датчиков
        if (activeCount == 3) {
            // Все три датчика работают - используем их как есть
            moisture1 = sensors[0]->currentValue;
            moisture2 = sensors[1]->currentValue;
            moisture3 = sensors[2]->currentValue;
        } else if (activeCount == 2) {
            // Два работающих датчика - распределяем их значения
            moisture1 = sensors[activeSensors[0]]->currentValue;
            moisture2 = sensors[activeSensors[1]]->currentValue;
            // Для третьего значения используем среднее из двух работающих
            moisture3 = (moisture1 + moisture2) / 2;
            Serial.println("ВНИМАНИЕ: Используется только 2 датчика, 3-е значение - среднее");
        } else if (activeCount == 1) {
            // Один работающий датчик - используем его значение для всех позиций
            moisture1 = moisture2 = moisture3 = sensors[activeSensors[0]]->currentValue;
            Serial.println("ВНИМАНИЕ: Используется только 1 датчик для всех позиций");
        } else {
            // Аварийный режим - безопасные значения (предполагаем, что почва влажная)
            moisture1 = moisture2 = moisture3 = 500;
            Serial.println("ОШИБКА: Нет работающих датчиков влажности! Используются безопасные значения");
        }
    }
    
    void check_moisture(int min_moisture, int max_moisture) {
        low_moisture1 = (moisture1 < min_moisture);
        low_moisture2 = (moisture2 < min_moisture);
        low_moisture3 = (moisture3 < min_moisture);
        
        // Также обновляем статус в индивидуальных датчиках
        for (int i = 0; i < activeCount; i++) {
            sensors[activeSensors[i]]->low_moisture = (sensors[activeSensors[i]]->currentValue < min_moisture);
        }
    }
    
    bool needs_watering() {
        // Полив нужен если большинство датчиков показывают сухость
        int dryCount = 0;
        if (low_moisture1) dryCount++;
        if (low_moisture2) dryCount++;
        if (low_moisture3) dryCount++;
        
        // Если 2 из 3 датчиков показывают сухость - включаем полив
        return dryCount >= 2;
    }
    
    void print_sensor_status() {
        Serial.println("=== СТАТУС ДАТЧИКОВ ВЛАЖНОСТИ ===");
        for (int i = 0; i < SENSOR_COUNT; i++) {
            if (sensors[i]) {
                Serial.print("Датчик ");
                Serial.print(sensors[i]->name);
                Serial.print(": ");
                Serial.print(sensors[i]->is_healthy() ? "РАБОТАЕТ" : "НЕИСПРАВЕН");
                Serial.print(" (значение=");
                Serial.print(sensors[i]->currentValue);
                Serial.print(", пин=");
                Serial.print(SOIL_MOISTURE_SENSOR_PIN_1 + i);
                Serial.println(")");
            }
        }
        Serial.print("Активных датчиков: ");
        Serial.println(activeCount);
        Serial.println("=================================");
    }
    
    int get_active_sensor_count() {
        return activeCount;
    }
    
    // Метод для ручного сброса всех датчиков
    void reset_all_sensors() {
        for (int i = 0; i < SENSOR_COUNT; i++) {
            if (sensors[i]) {
                sensors[i]->reset_health();
            }
        }
        Serial.println("Все датчики сброшены в рабочее состояние");
    }
};

// Класс датчика освещенности
class LightSensor {
private:
    int pin;
public:
    int light_level;
    
    LightSensor(int p) : pin(p) {}
    
    void get_data() {
        light_level = analogRead(pin);
    }
};

// Класс системы освещения
class Light {
private:
    int pin;
public:
    bool on_night;
    bool on_dark;
    
    Light(int p) : pin(p) {}
    
    void power() {
        if (on_night || on_dark) {
            digitalWrite(pin, HIGH);
        } else {
            digitalWrite(pin, LOW);
        }
    }
};

// Класс обогревателя
class Heater {
private:
    int pin;
public:
    bool on_temperature;
    
    Heater(int p) : pin(p) {}
    
    void power() {
        if (on_temperature) {
            digitalWrite(pin, HIGH);
        } else {
            digitalWrite(pin, LOW);
        }
    }
};

// Класс вентилятора
class Fan {
private:
    int pin;
public:
    bool on_heater;
    bool on_humidity;
    
    Fan(int p) : pin(p) {}
    
    void power() {
        if (on_heater || on_humidity) {
            digitalWrite(pin, HIGH);
        } else {
            digitalWrite(pin, LOW);
        }
    }
};

// Класс системы полива
class WaterPump {
private:
    int pin;
    unsigned long startWateringTime = 0;
    unsigned long stopWateringTime = 0;
    const unsigned long wateringDuration = 2000;
    const unsigned long wateringDelay = 2000;
    bool wateringInProgress = false;
    bool pumpCurrentlyActive = false;
public:
    bool active;
    
    WaterPump(int p) : pin(p) {}
    
    void power() {
        if (active) {
            digitalWrite(pin, HIGH);
        } else {
            digitalWrite(pin, LOW);
        }
    }
    
    void decide_activation(bool moisture_low) {
        unsigned long currentTime = millis();
        
        if (!moisture_low) {
            active = false;
            wateringInProgress = false;
        } else {
            if (wateringInProgress && pumpCurrentlyActive) {
                if (currentTime - startWateringTime < wateringDuration) {
                    active = true;
                } else {
                    active = false;
                    pumpCurrentlyActive = false;
                    stopWateringTime = currentTime;
                }
            } else if (wateringInProgress && !pumpCurrentlyActive) {
                if (currentTime - stopWateringTime < wateringDelay) {
                    active = false;
                } else {
                    wateringInProgress = false;
                }
            } else if (!wateringInProgress) {
                pumpCurrentlyActive = true;
                startWateringTime = currentTime;
                wateringInProgress = true;
            }
        }
    }
};

// Класс суточного цикла
class DayNightCycle {
private:
    unsigned long lastTime = 0;
    const unsigned long dayTime = 12 * 1000;
    const unsigned long nightTime = 12 * 1000;
public:
    bool is_night;
    
    void update() {
        unsigned long currentTime = millis();
        if (currentTime - lastTime < dayTime) {
            is_night = false;
        } else if (currentTime - lastTime > dayTime && currentTime - lastTime < nightTime + dayTime) {
            is_night = true;
        } else if (currentTime - lastTime > nightTime) {
            is_night = false;
            lastTime = currentTime;
        }
    }
};

// Структура предельных значений
struct SensorLimits {
    int temperature_min = 29;
    int temperature_max = 30;
    int air_humidity_min = 20;
    int air_humidity_max = 40;
    int soil_moisture_min = 900;
    int soil_moisture_max = 10;
    int light_min = 400;
    int light_max = 20;
};

DHT dht(TEMP_HUMID_SENSOR_PIN, DHT11);
Thermometer thermometer;

// Создаем умные датчики
SmartSoilMoistureSensor sensor1(SOIL_MOISTURE_SENSOR_PIN_1, "Датчик_1_низ");
SmartSoilMoistureSensor sensor2(SOIL_MOISTURE_SENSOR_PIN_2, "Датчик_2_середина");
SmartSoilMoistureSensor sensor3(SOIL_MOISTURE_SENSOR_PIN_3, "Датчик_3_верх");

// Создаем массив из трех датчиков
SoilMoistureSensorArray soil_moisture_array;

LightSensor light_sensor(LIGHT_SENSOR_PIN);
Light light(LIGHT_PIN);
Heater heater(HEAT_PIN);
Fan fan(VENT_PIN);
WaterPump pump(PUMP_PIN);
DayNightCycle day_night;
SensorLimits limits;

unsigned long lastSerialTime = 0;
const unsigned long serialInterval = 5000;
unsigned long lastHealthCheckTime = 0;
const unsigned long healthCheckInterval = 10000; // Проверка здоровья каждые 10 секунд

void setup() {
    Serial.begin(9600);
    dht.begin();
    pinMode(PUMP_PIN, OUTPUT);
    pinMode(LIGHT_PIN, OUTPUT);
    pinMode(HEAT_PIN, OUTPUT);
    pinMode(VENT_PIN, OUTPUT);
    
    // Добавляем три датчика в массив
    soil_moisture_array.add_sensor(&sensor1, 0);
    soil_moisture_array.add_sensor(&sensor2, 1);
    soil_moisture_array.add_sensor(&sensor3, 2);
    
    Serial.println("Система автоматического полива с тремя датчиками запущена");
    soil_moisture_array.print_sensor_status();
}

void control_light() {
    light_sensor.get_data();
    
    if ((light_sensor.light_level >= limits.light_min && !day_night.is_night) || day_night.is_night) {
        light.on_dark = true;
    } else {
        light.on_dark = false;
    }
    light.on_night = day_night.is_night;
}

void control_temperature() {
    if (thermometer.temperature < limits.temperature_min) {
        heater.on_temperature = true;
        fan.on_heater = true;
    } else if (thermometer.temperature >= limits.temperature_max) {
        heater.on_temperature = false;
        fan.on_heater = false;
    }
}

void control_air_humidity() {
    if (thermometer.humidity > limits.air_humidity_max) {
        fan.on_humidity = true;
    } else if (thermometer.humidity <= limits.air_humidity_min) {
        fan.on_humidity = false;
    }
}

void control_soil_moisture() {
    soil_moisture_array.check_moisture(limits.soil_moisture_min, limits.soil_moisture_max);
}

void print_sensor_data() {
    Serial.print("Температура: ");
    Serial.println(thermometer.temperature);
    Serial.print("Влажность воздуха: ");
    Serial.println(thermometer.humidity);
    
    Serial.print("Влажность почвы 1: ");
    Serial.print(soil_moisture_array.moisture1);
    Serial.print(" (");
    Serial.print(soil_moisture_array.low_moisture1 ? "СУХО" : "норм");
    Serial.println(")");
    
    Serial.print("Влажность почвы 2: ");
    Serial.print(soil_moisture_array.moisture2);
    Serial.print(" (");
    Serial.print(soil_moisture_array.low_moisture2 ? "СУХО" : "норм");
    Serial.println(")");
    
    Serial.print("Влажность почвы 3: ");
    Serial.print(soil_moisture_array.moisture3);
    Serial.print(" (");
    Serial.print(soil_moisture_array.low_moisture3 ? "СУХО" : "норм");
    Serial.println(")");
    
    Serial.print("Активных датчиков: ");
    Serial.println(soil_moisture_array.get_active_sensor_count());
    
    Serial.print("Статус полива: ");
    Serial.println(soil_moisture_array.needs_watering() ? "НУЖЕН" : "не нужен");
    
    Serial.print("Освещенность: ");
    Serial.println(light_sensor.light_level);
    Serial.println();
}

void loop() {
    thermometer.get_data(dht);
    soil_moisture_array.get_data(); // Чтение данных с автоматическим резервированием
    light_sensor.get_data();
 
    day_night.update();
    
    control_light();
    control_temperature();
    control_air_humidity();
    control_soil_moisture();
    
    pump.decide_activation(soil_moisture_array.needs_watering());
    
    light.power();
    heater.power();
    fan.power();
    pump.power();

    if (millis() - lastSerialTime >= serialInterval) {
        print_sensor_data();
        lastSerialTime = millis();
    }
    
    // Периодическая проверка здоровья датчиков
    if (millis() - lastHealthCheckTime >= healthCheckInterval) {
        soil_moisture_array.update_sensor_health();
        lastHealthCheckTime = millis();
    }
    
    // Вывод статуса датчиков каждые 30 секунд
    if (millis() % 30000 == 0) {
        soil_moisture_array.print_sensor_status();
    }
    
    delay(100);
}

int main() {
    setup();
    while(1) {
        loop();
    }
    return 0;
}
