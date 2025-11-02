#include <DHT.h>
#include <Adafruit_Sensor.h>

// === Аппаратная распиновка ===
#define PUMP_PIN 5
#define LIGHT_PIN 6
#define HEAT_PIN 4
#define VENT_PIN 7

#define DHT_PIN 12
#define DEFAULT_LIGHT_SENSOR_PIN A0
#define DEFAULT_SOIL_SENSOR_PIN A3

// === Параметры: сколько датчиков поддерживаем максимум ===
const int MAX_LIGHT_SENSORS = 4;
const int MAX_SOIL_SENSORS  = 4;

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
  bool is_normal;           // флаг валидности чтений
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
};

// === Устройство: насос ===
struct Pump {
  int pin;
  bool is_on;
  // связывается с SoilSensors.pumpNeeded
};

// === Устройство: освещение ===
struct LightCtrl {
  int pin;
  bool is_on;
  Range limits;      // можно дублировать или брать из LightSensors.limits
  unsigned long nightStart; // для симуляции дня/ночи (не обязательно)
};

// === Устройство: нагреватель ===
struct Heater {
  int pin;
  bool is_on;
  Range limits; // min = включить при < min, max = выключить при > max
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
DHT dht(DHT_PIN, DHT11);
AirSensor air = { DHT_PIN, 0, 0, false };

LightSensors lights;
SoilSensors soils;

Pump pump = { PUMP_PIN, false };
LightCtrl lamp = { LIGHT_PIN, false, {0,0}, 0 };
Heater heater = { HEAT_PIN, false, {0,0} };
Ventilation vent = { VENT_PIN, false, {30,75}, 0, 3600000UL, 60000UL, false }; // 1 час и 60s

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
  // Массивы световых датчиков (по умолчанию 1 датчик A0)
  lights.count = 1;
  lights.pins[0] = DEFAULT_LIGHT_SENSOR_PIN;
  lights.values[0] = 0;
  lights.limits.minVal = 200;  // если ниже — темно
  lights.limits.maxVal = 800;  // если выше — ярко
  lights.shouldBeOn = false;

  // Почвенные датчики — по умолчанию 1
  soils.count = 1;
  soils.pins[0] = DEFAULT_SOIL_SENSOR_PIN;
  soils.values[0] = 0;
  soils.limits.minVal = 850;  // сухо (больше — суше)
  soils.limits.maxVal = 300;  // влажно (меньше — влажно)
  soils.pumpNeeded = false;
  soils.lastPumpStart = 0;
  soils.pumpMaxDuration = 30000UL; // 30s макс полив (безопасность)

  // Лампа — дублируем пороги в контроллере освещения (по желанию)
  lamp.limits = lights.limits;

  // Нагреватель
  heater.limits.minVal = 20;
  heater.limits.maxVal = 27;
  heater.is_on = false;

  // Вентиляция — уже инициализирована при объявлении (ventInterval, ventDuration)
  vent.lastVentTime = millis();
  vent.timerActive = false;

  // Цикл день/ночь
  cycleStart = millis();
  isNight = false;

  // Pump/lamp default off
  pump.is_on = false;
  lamp.is_on = false;
}

// -------------------------------------------------------------
// ЧТЕНИЕ ВСЕХ ДАТЧИКОВ
// -------------------------------------------------------------
void readAllSensors() {
  // DHT
  int h = dht.readHumidity();//может переименовать?Но как?
  int t = dht.readTemperature();//<-------|same
  if (isnan(h) || isnan(t)) {
    air.humidity = air.temperature = 0;
    air.is_normal = false;
  } else {
    air.humidity = h;
    air.temperature = t;
    air.is_normal = true;
  }

  // Multiple Light sensors
  for (int i = 0; i < lights.count; ++i) {
    lights.values[i] = analogRead(lights.pins[i]);
  }

  //Multiple Soil sensors
  for (int i = 0; i < soils.count; ++i) {
    soils.values[i] = analogRead(soils.pins[i]);
  }
}

// -------------------------------------------------------------
// ДНЕВНО-НОЧНОЙ ЦИКЛ (симуляция по millis())
// -------------------------------------------------------------
void updateDayNight() {
  unsigned long now = millis();
  unsigned long elapsed = now - cycleStart;
  if (elapsed < dayDuration) {
    isNight = false;
  } else if (elapsed < dayDuration + nightDuration) {
    isNight = true;
  } else {
    cycleStart = now; // сбросим цикл
    isNight = false;
  }
}

// -------------------------------------------------------------
// ПРОВЕРКА ОСВЕЩЕНИЯ (по всем датчикам, берём среднее)
// -------------------------------------------------------------
void evaluateLighting() {
  long sum = 0;
  for (int i = 0; i < lights.count; ++i) sum += lights.values[i];
  int avg = (lights.count > 0) ? (sum / lights.count) : 0;
  // если день и средняя освещённость ниже лимита — нужно включить
  lights.shouldBeOn = (!isNight && avg < lights.limits.minVal);
  // также обновляем lamp.on по lights.shouldBeOn 
}

// -------------------------------------------------------------
// ПРОВЕРКА ТЕМПЕРАТУРЫ (DHT)
// -------------------------------------------------------------
void evaluateTemperature() {
  if (!air.is_normal) return; // если нет корректных данных
  if (air.temperature < heater.limits.minVal) heater.is_on = true;
  else if (air.temperature > heater.limits.maxVal) heater.is_on = false;
}

// -------------------------------------------------------------
// ПРОВЕРКА ВЛАЖНОСТИ ВОЗДУХА (DHT)
// -------------------------------------------------------------
void evaluateAirHumidity() {
  if (!air.is_normal) return;
  if (air.humidity > vent.humidityLimits.maxVal) vent.is_on = true;
  else if (air.humidity < vent.humidityLimits.minVal) vent.is_on = false;
}

// -------------------------------------------------------------
// ПРОВЕРКА ВЛАЖНОСТИ ПОЧВЫ (по всем датчикам — берем среднее)
// -------------------------------------------------------------
void evaluateSoil() {
  long sum = 0;
  for (int i = 0; i < soils.count; ++i) sum += soils.values[i];
  int avg = (soils.count > 0) ? (sum / soils.count) : 0;
  // если avg больше порога minVal — почва сухая => нужен полив
  soils.pumpNeeded = (avg > soils.limits.minVal);
  // если насос уже включён дольше, чем максимальное время — выключаем
  if (pump.is_on && (millis() - soils.lastPumpStart > soils.pumpMaxDuration)) {
    pump.is_on = false;
    soils.pumpNeeded = false;
  }
}

// -------------------------------------------------------------
// ПРИМЕНЕНИЕ РЕШЕНИЙ К УСТРОЙСТВАМ (включаем/выключаем пины)
// -------------------------------------------------------------
void controlDevices() {
  // Lamp follows lights.shouldBeOn
  lamp.is_on = lights.shouldBeOn;
  digitalWrite(lamp.pin, lamp.is_on ? HIGH : LOW);

  // Heater uses heater.on
  digitalWrite(heater.pin, heater.is_on ? HIGH : LOW);

  // Pump: связываем pump.state с soils.pumpNeeded
  if (soils.pumpNeeded && !pump.is_on) {
    pump.is_on = true;
    soils.lastPumpStart = millis();
  } else if (!soils.pumpNeeded && pump.is_on) {
    pump.is_on = false;
  }
  digitalWrite(pump.pin, pump.is_on ? HIGH : LOW);
}

// -------------------------------------------------------------
// УПРАВЛЕНИЕ ВЕНТИЛЯЦИЕЙ (по таймеру + по состояниям)
// -------------------------------------------------------------
void controlVentilation() {
  unsigned long now = millis();

  // Запускаем плановый таймер
  if (!vent.timerActive && (now - vent.lastVentTime >= vent.ventInterval)) {
    vent.timerActive = true;
    vent.lastVentTime = now;
  }

  // Если плановый таймер активен — включаем вентилятор на ventDuration
  if (vent.timerActive) {
    vent.is_on = true;
    if (now - vent.lastVentTime >= vent.ventDuration) {
      vent.timerActive = false; // закончилось плановое проветривание
      vent.lastVentTime = now;  // обновляем для следующего цикла
      // После окончания планового проветривания — продолжаем учитывать текущие флаги
    }
  } else {
    // Включаем вентилятор, если высокая влажность, или если включен нагрев, или если идёт полив
    vent.is_on = ( (air.is_normal && air.humidity > vent.humidityLimits.maxVal) || heater.is_on || pump.is_on );
  }

  digitalWrite(vent.pin, vent.is_on ? HIGH : LOW);
}

// -------------------------------------------------------------
// ЛОГИРОВАНИЕ
// -------------------------------------------------------------
void serialLog() {
  if (millis() - lastSerial < serialInterval) return;
  lastSerial = millis();

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
  Serial.println("==============");
}

// -------------------------------------------------------------
// SETUP
// -------------------------------------------------------------
void setup() {
  Serial.begin(9600);
  // DHT init
  dht.begin();

  // pinMode для устройств
  pinMode(pump.pin, OUTPUT);
  pinMode(lamp.pin, OUTPUT);
  pinMode(heater.pin, OUTPUT);
  pinMode(vent.pin, OUTPUT);

  // Инициализация параметров, порогов и т.п.
  Parameter_Initialization();
  // Установим все реле в ноль для запуска по условиям
  digitalWrite(pump.pin, LOW);
  digitalWrite(lamp.pin, LOW);
  digitalWrite(heater.pin, LOW);
  digitalWrite(vent.pin, LOW);
}

// -------------------------------------------------------------
// LOOP — только вызовы функций 
// -------------------------------------------------------------
void loop() {
  readAllSensors();
  updateDayNight();
  evaluateLighting();
  evaluateTemperature();
  evaluateAirHumidity();
  evaluateSoil();
  controlDevices();
  controlVentilation();
  serialLog();
}
