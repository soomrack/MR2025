#include <DHT.h>
#include <Adafruit_Sensor.h>

// === Аппаратная распиновка  ===
#define PUMP_PIN 5
#define LIGHT_PIN 6
#define HEAT_PIN 4
#define VENT_PIN 7

#define DHT_PIN 12
#define DHT_PIN2 12
#define DHT_PIN3 11
#define DHT_PIN4 11
#define DEFAULT_LIGHT_SENSOR_PIN A0
#define DEFAULT_SOIL_SENSOR_PIN A3

// === Параметры: сколько датчиков поддерживаем максимум ===
const int MAX_LIGHT_SENSORS = 4;
const int MAX_SOIL_SENSORS  = 4;
const int MAX_DHT_SENSORS = 4;

// === Вспомогательная структура для диапазонов ===
struct Range {
  int minVal; // нижняя граница 
  int maxVal; // верхняя граница 
};

// === Структура: воздушный датчик (DHT) ===
struct AirSensor {
  int pin;
  int temperature;   // последняя температура (°C)
  int humidity;      // последняя влажность (%) 
  bool is_normal;    // флаг валидности чтений
};

// === Структура: отдельный датчик температуры DHT ===
struct TemperatureSensor {
  int pin;
  int temperature;   // последняя температура (°C)
  DHT* dht;          // указатель на объект DHT
  
  // Конструктор
  TemperatureSensor(int p) : pin(p), temperature(0) {
    dht = new DHT(pin, DHT11);
  }

  void read() {
      int temperature = dht->readTemperature();
    }


// === Структура: группа температурных датчиков (наследник) ===
struct TemperatureSensorArray : public TemperatureSensor {
  TemperatureSensor* sensors[MAX_DHT_SENSORS];
  int count;
  int temps[MAX_DHT_SENSORS];
  int filteredTemp;

  int getTemperature() {
    return temperature;
  }
};
  
  TemperatureSensorArray() : TemperatureSensor(0), count(0), filteredTemp(0) {}
  
  // Добавление датчика в массив
  void addSensor(TemperatureSensor* sensor) {
    if (count < MAX_DHT_SENSORS) {
      sensors[count] = sensor;
      count++;
    }
  }
  
  void readAllTemperatures(TemperatureSensor& sensor, TemperatureSensor& sensor, TemperatureSensor& sensor, TemperatureSensor& sensor) {
    for (int i = 0; i < count; i++) {
      // sensors[i]->read();
      temps[0] = sensor.getTemperature();
      temps[1] = sensor.getTemperature();
      temps[2] = sensor.getTemperature();
      temps[3] = sensor.getTemperature();
    }
  }
  
  // Фильтрация выбросов и усреднение температур
  int filterAverageTemperature() {

    float sum = 0;
    for (int i = 0; i < count; i++) sum += temps[i];
    float mean = sum / count;

    float sq = 0;
    for (int i = 0; i < count; i++)
      sq += pow(temps[i] - mean, 2);
    float stddev = sqrt(sq / count);

    float filteredSum = 0;
    int filteredCount = 0;
    for (int i = 0; i < count; i++) {
      if (fabs(temps[i] - mean) <= 1.5 * stddev) {
        filteredSum += temps[i];
        filteredCount++;
      }
    }

    if (filteredCount == 0)
      return (int)mean;
    return (int)(filteredSum / filteredCount);
  }
};
  
// === Группа аналоговых датчиков освещённости ===
struct LightSensors {
  int pins[MAX_LIGHT_SENSORS];
  int values[MAX_LIGHT_SENSORS];
  int count;
  Range limits;     // пороги "недостаточно/слишком много света" (условные ед.)
  bool shouldBeOn;  // рассчитанный флаг по освещению
};

// === Структура: группа датчиков влажности почвы (аналоговые) ===
struct SoilSensors {
  int pins[MAX_SOIL_SENSORS];
  int values[MAX_SOIL_SENSORS];
  int count;
  Range limits;    // пороги для полива (min = сухо, max = влажно)
  bool pumpNeeded; // рассчитанный флаг
  unsigned long lastPumpStart; // когда стартовал полив
  unsigned long pumpMaxDuration; // макс длительность полива (ms)
  unsigned long sleepStart;
};

// === Устройство: насос ===
struct Pump {
  int pin;
  bool is_on;
  bool time_to_sleep;
};

// === Устройство: освещение ===
struct LightCtrl {
  int pin;
  bool is_on;
  Range limits;
};

// === Устройство: нагреватель ===
struct Heater {
  int pin;
  bool is_on;
  Range limits; 
};

// === Устройство: вентиляция ===
struct Ventilation {
  int pin;
  bool is_on;
  Range humidityLimits;  // пороги влажности воздуха
  unsigned long lastVentTime;
  unsigned long ventInterval; // интервал между плановыми проветриваниями (ms)
  unsigned long ventDuration; // длительность планового проветривания (ms)
  bool timerActive;
};

// === Глобальные объекты ===
unsigned long currentMillis = 0;  // Единый момент времени для всех функций


TemperatureSensor tempSensor1(DHT_PIN);
TemperatureSensor tempSensor2(DHT_PIN2);
TemperatureSensor tempSensor3(DHT_PIN3);
TemperatureSensor tempSensor4(DHT_PIN4);

TemperatureSensorArray tempSensors;

DHT dht1(DHT_PIN, DHT11);
AirSensor air = { DHT_PIN, 0, 0, false };

LightSensors lights;
SoilSensors soils;

Pump pump = { PUMP_PIN, false };
LightCtrl lamp = { LIGHT_PIN, false, {0,0} };
Heater heater = { HEAT_PIN, false, {0,0} };
Ventilation vent = { VENT_PIN, false, {30,75}, 0, 3600000UL, 60000UL, false };

// === Время / дневной цикл (симуляция) ===
unsigned long cycleStart = 0;
const unsigned long dayDuration = 10UL * 1000UL;   // 10s день (тест)
const unsigned long nightDuration = 10UL * 1000UL; // 10s ночь (тест)
bool isNight = false;

// === Serial таймер ===
unsigned long lastSerial = 0;
const unsigned long serialInterval = 5000UL;

// -------------------------------------------------------------
// ИНИЦИАЛИЗАЦИЯ ПАРАМЕТРОВ (пороги, пины датчиков и т.д.)
// -------------------------------------------------------------
void Parameter_Initialization() {

  tempSensor1.dht->begin();
  tempSensor2.dht->begin();
  tempSensor3.dht->begin();
  tempSensor4.dht->begin();
  
  tempSensors.addSensor(&tempSensor1);
  tempSensors.addSensor(&tempSensor2);
  tempSensors.addSensor(&tempSensor3);
  tempSensors.addSensor(&tempSensor4);

  // Массивы световых датчиков (по умолчанию 1 датчик A0)
  lights.count = 1;
  lights.pins[0] = DEFAULT_LIGHT_SENSOR_PIN;
  lights.values[0] = 0;
  lights.limits.minVal = 405;
  lights.limits.maxVal = 800;
  lights.shouldBeOn = false;

  // Почвенные датчики — по умолчанию 1
  soils.count = 1;
  soils.pins[0] = DEFAULT_SOIL_SENSOR_PIN;
  soils.values[0] = 0;
  soils.limits.minVal = 500;
  soils.limits.maxVal = 600;
  soils.pumpNeeded = false;
  soils.lastPumpStart = 0;
  soils.pumpMaxDuration = 5000UL;

  // Лампа — дублируем пороги в контроллере освещения 
  lamp.limits = lights.limits;

  // Нагреватель
  heater.limits.minVal = 20;
  heater.limits.maxVal = 27;
  heater.is_on = false;

  // Вентиляция
  vent.lastVentTime = currentMillis;
  vent.timerActive = false;

  // Цикл день/ночь
  cycleStart = currentMillis;
  isNight = false;

  // Pump/lamp default off
  pump.is_on = false;
  lamp.is_on = false;
  pump.time_to_sleep = false;
}

// -------------------------------------------------------------
// ЧТЕНИЕ ВСЕХ ДАТЧИКОВ (возвращаем оригинальные функции)
// -------------------------------------------------------------
void readDHT(){
  int humid_data = dht1.readHumidity();
  int temp_data = dht1.readTemperature();
  if (isnan(humid_data) || isnan(temp_data)) {
    air.humidity = air.temperature = 0;
    air.is_normal = false;
  } else {
    air.humidity = humid_data;
    air.temperature = temp_data;
    air.is_normal = true;
  }
}

void read_Multiple_Light_sensors(){
  for (int k = 0; k < lights.count; ++k) {
    lights.values[k] = analogRead(lights.pins[k]);
  }
}

void read_Multiple_Soil_sensors(){
  for (int k = 0; k < soils.count; ++k) {
    soils.values[k] = analogRead(soils.pins[k]);
  }
}

// -------------------------------------------------------------
// ДНЕВНО-НОЧНОЙ ЦИКЛ (симуляция по millis())
// -------------------------------------------------------------
void updateDayNight() {
  unsigned long elapsed = currentMillis - cycleStart;
  if (elapsed < dayDuration) {
    isNight = false;
  } else if (elapsed < dayDuration + nightDuration) {
    isNight = true;
  } else {
    cycleStart = currentMillis;
    isNight = false;
  }
}

// -------------------------------------------------------------
// ПРОВЕРКА ОСВЕЩЕНИЯ (по всем датчикам, берём среднее)
// -------------------------------------------------------------
void Algorithm_Lighting() {
  long sum = 0;
  for (int k = 0; k < lights.count; ++k) sum += lights.values[k];
  int avg = (lights.count > 0) ? (sum / lights.count) : 0;
  lights.shouldBeOn = (!isNight && avg > lights.limits.minVal);
}

// -------------------------------------------------------------
// ПРОВЕРКА ТЕМПЕРАТУРЫ (DHT)
// -------------------------------------------------------------
void Algorithm_Temperature(TemperatureSensor& sensor) {
  int effectiveTemp = sensor.getTemperature();
  heater.is_on = (effectiveTemp < heater.limits.minVal);
}

// -------------------------------------------------------------
// ПРОВЕРКА ВЛАЖНОСТИ ПОЧВЫ (по всем датчикам — берем среднее)
// -------------------------------------------------------------
void Algorithm_Soil() {
  if (soils.count == 0) return;

  long sum = 0;
  for (int i = 0; i < soils.count; ++i) sum += soils.values[i];
  int avg = sum / soils.count;
  
  soils.pumpNeeded = avg > soils.limits.minVal;
  
  if (pump.is_on && (currentMillis - soils.lastPumpStart > soils.pumpMaxDuration)) {
    pump.is_on = false;
    soils.pumpNeeded = false;
  }
}

// -------------------------------------------------------------
// УПРАВЛЕНИЕ ВЕНТИЛЯЦИЕЙ ПО РАСПИСАНИЮ
// -------------------------------------------------------------
void Algorithm_vent_schedule_control(){
  if (!vent.timerActive && (currentMillis - vent.lastVentTime >= vent.ventInterval)) {
    vent.timerActive = true;
    vent.lastVentTime = currentMillis;
  }

  if (vent.timerActive) {
    if (currentMillis - vent.lastVentTime >= vent.ventDuration) {
      vent.timerActive = false;
      vent.lastVentTime = currentMillis;
    }
  }
}

long sleepStart = 0;
void Algorithm_pump_state_control(){
  if (soils.count == 0) return;

  long sum = 0;
  for (int i = 0; i < soils.count; ++i) sum += soils.values[i];
  int avg = sum / soils.count;
  
  // Нужно поливать, когда СУХО (значение ниже порога)
  soils.pumpNeeded = avg > soils.limits.minVal; 

  // Если насос уже работает, проверяем таймаут
  if (pump.is_on) {
    if (!soils.pumpNeeded) {
      pump.is_on = false;
    }
    else if (currentMillis - soils.lastPumpStart > soils.pumpMaxDuration) {
      pump.is_on = false;  // Выключаем по таймауту
      soils.sleepStart = currentMillis;
      pump.time_to_sleep = true;
    }
  } 
  // Если насос выключен, проверяем нужно ли включить
  else if (soils.pumpNeeded && !pump.time_to_sleep) {
    pump.is_on = true;
    soils.lastPumpStart = currentMillis;
  }
  
  // Управление флагом "сна" - сбрасываем через 5 секунд
  if ((currentMillis - soils.sleepStart > 5000UL)) {
    pump.time_to_sleep = false;
  }
}

// -------------------------------------------------------------
// ПРИМЕНЕНИЕ РЕШЕНИЙ К УСТРОЙСТВАМ (включаем/выключаем пины)
// -------------------------------------------------------------
void Lamp_power() {
  lamp.is_on = lights.shouldBeOn;
  digitalWrite(lamp.pin, lamp.is_on ? HIGH : LOW);
}

void Heater_power() {
  digitalWrite(heater.pin, heater.is_on ? HIGH : LOW);
}

void Pump_power() {
  digitalWrite(pump.pin, pump.is_on ? HIGH : LOW);
}

void Ventilation_power() {
  bool highHumidity = (air.is_normal 
                       && air.humidity > vent.humidityLimits.maxVal);
  bool needVentilation = (vent.timerActive || highHumidity || heater.is_on || pump.is_on);
  
  vent.is_on = needVentilation;
  digitalWrite(vent.pin, vent.is_on ? HIGH : LOW);
}

// -------------------------------------------------------------
// SERIAL / ЛОГИРОВАНИЕ
// -------------------------------------------------------------
void serialLog() {
  if (currentMillis - lastSerial < serialInterval) return;
  lastSerial = currentMillis;

  bool highHumidity = (air.is_normal && air.humidity > vent.humidityLimits.maxVal);
  
  Serial.println("=== STATUS ===");
  Serial.print("DHT ok: "); Serial.println(air.is_normal ? "YES" : "NO");
  Serial.print("Temp: "); Serial.print(air.temperature); Serial.print("C  Hum: "); Serial.print(air.humidity); Serial.println("%");
  Serial.print("Light sensors (avg): ");
  long sumL = 0; for (int i=0;i<lights.count;i++) sumL += lights.values[i];
  Serial.print( lights.count ? sumL / lights.count : 0 );
  Serial.print("  Lamp on: "); Serial.println(lamp.is_on ? "YES" : "NO");
  Serial.print("Soil sensors (avg): ");
  long sumS = 0; for (int i=0;i<soils.count;i++) sumS += soils.values[i];
  Serial.print( soils.count ? sumS / soils.count : 0 );
  Serial.print("  Pump on: "); Serial.println(pump.is_on ? "YES" : "NO");
  Serial.print("Vent on: "); Serial.println(vent.is_on ? "YES" : "NO");
  Serial.print("Vent conditions - Timer: "); Serial.print(vent.timerActive ? "YES" : "NO");
  Serial.print(" HighHumidity: "); Serial.print(highHumidity ? "YES" : "NO");
  Serial.print(" Heating: "); Serial.print(heater.is_on ? "YES" : "NO");
  Serial.print(" Pumping: "); Serial.println(pump.is_on ? "YES" : "NO");
  Serial.println("==============");
}

// -------------------------------------------------------------
// SETUP
// -------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  dht1.begin();

  pinMode(pump.pin, OUTPUT);
  pinMode(lamp.pin, OUTPUT);
  pinMode(heater.pin, OUTPUT);
  pinMode(vent.pin, OUTPUT);

  Parameter_Initialization();
  
  digitalWrite(pump.pin, LOW);
  digitalWrite(lamp.pin, LOW);
  digitalWrite(heater.pin, LOW);
  digitalWrite(vent.pin, LOW);
}

// -------------------------------------------------------------
// LOOP — только вызовы функций 
// -------------------------------------------------------------
void loop() {
  currentMillis = millis();
  
  updateDayNight();
  
  readDHT();
  tempSensor1.readTemperature();
  tempSensor2.readTemperature();
  tempSensor3.readTemperature();
  tempSensor4.readTemperature();
  tempSensors.readAllTemperatures(tempSensor1, tempSensor2, tempSensor3, tempSensor4);
  
  read_Multiple_Light_sensors();
  read_Multiple_Soil_sensors();
  
  Algorithm_Lighting();
  Algorithm_Temperature(tempSensors);
  Algorithm_vent_schedule_control();
  Algorithm_pump_state_control();
  
  Lamp_power();
  Heater_power();
  Pump_power();
  Ventilation_power();
  
  serialLog();
}
