#include <DHT.h>


#define PUMP_PIN 5
#define LIGHT_PIN 6
#define HEAT_PIN 4
#define VENT_PIN 7

#define TEMP_AND_HUMID_PIN 12
#define DHT_TYPE DHT11
#define SOIL_PIN A3

const uint8_t LIGHT_PINS[] = { A0, A1, A2 };
const uint8_t LIGHT_COUNT = sizeof(LIGHT_PINS) / sizeof(LIGHT_PINS[0]);

const bool RELAY_ACTIVE_LOW = false;

const int LIGHT_ON_THRESHOLD  = 300;
const int LIGHT_OFF_THRESHOLD = 360;

const unsigned long LAMP_MIN_CHANGE_MS = 2000UL;

const float TEMP_ON_BELOW = 20.0;
const float TEMP_OFF_ABOVE = 22.0;

const int HUMIDITY_ON_ABOVE = 75;
const int HUMIDITY_OFF_BELOW = 60;

const int SOIL_DRY_THRESHOLD = 600;
const int SOIL_WET_THRESHOLD = 450;

const unsigned long PUMP_MAX_DURATION = 1000UL;
const unsigned long PUMP_MIN_INTERVAL = 10UL * 1000UL;

const unsigned long VENT_SCHEDULE_INTERVAL = 3600UL * 1000UL;
const unsigned long VENT_SCHEDULE_DURATION = 60UL * 1000UL;

const unsigned long DAY_DURATION_MS = 10UL * 1000UL;
const unsigned long NIGHT_DURATION_MS = 10UL * 1000UL;

const unsigned long SERIAL_INTERVAL = 5000UL;


DHT dht(TEMP_AND_HUMID_PIN, DHT_TYPE);


unsigned long now_ms = 0;
unsigned long last_serial_ms = 0;

unsigned long cycle_start_ms = 0;
bool isNight = false;

unsigned long last_vent_ms = 0;
bool vent_timer_active = false;
unsigned long vent_timer_start = 0;

bool pump_state = false;
unsigned long pump_start_ms = 0;
unsigned long pump_last_end_ms = 0;

float air_temp = NAN;
float air_hum = NAN;
int light_values[LIGHT_COUNT];
int soil_value = 0;

bool lamp_state = false;
unsigned long lamp_last_change_ms = 0;  

void writeOutput(uint8_t pin, bool on) {
    if (RELAY_ACTIVE_LOW) {
        digitalWrite(pin, on ? LOW : HIGH);
    }
    else {
        digitalWrite(pin, on ? HIGH : LOW);
    }
}

void readSensors() {
  float h = dht.readHumidity();
  float t = dht.readTemperature();

  if (!isnan(h) && !isnan(t)) { air_hum = h; air_temp = t; }

  for (uint8_t i = 0; i < LIGHT_COUNT; ++i) light_values[i] = analogRead(LIGHT_PINS[i]);

  soil_value = analogRead(SOIL_PIN);
}


void updateDayNight() {
    unsigned long elapsed = now_ms - cycle_start_ms;
    if (elapsed < DAY_DURATION_MS) {
        isNight = false;
    }
    else if (elapsed < DAY_DURATION_MS + NIGHT_DURATION_MS) {
        isNight = true;
    }
    else {
        cycle_start_ms = now_ms;
        isNight = false;
    }
}

bool decideLampDesired(bool isNightLocal) {
  if (isNightLocal) {
    return true;
  }
  long sum = 0;
  for (uint8_t i = 0; i < LIGHT_COUNT; ++i) sum += light_values[i];
  int avg = LIGHT_COUNT ? (sum / LIGHT_COUNT) : 1023;

  if (lamp_state) {
    return !(avg > LIGHT_OFF_THRESHOLD);
  } else {
    return (avg < LIGHT_ON_THRESHOLD);
  }
}

bool decideHeaterOn() {
    if (isnan(air_temp)) return false;
    static bool last_state = false;
    if (air_temp < TEMP_ON_BELOW) last_state = true;
    else if (air_temp >= TEMP_OFF_ABOVE) last_state = false;
    return last_state;
}

bool decideVentOn(bool heater_on, bool pump_on_flag) {
    static bool last_state = false;
    if (!isnan(air_hum)) {
        if (air_hum > HUMIDITY_ON_ABOVE) last_state = true;
        else if (air_hum <= HUMIDITY_OFF_BELOW) last_state = false;
    }
    if (heater_on || pump_on_flag) last_state = true;
    if (vent_timer_active) {
        last_state = true;
    }
    return last_state;
}

void updateVentSchedule() {
    if (!vent_timer_active && (now_ms - last_vent_ms >= VENT_SCHEDULE_INTERVAL)) {
        vent_timer_active = true;
        vent_timer_start = now_ms;
        last_vent_ms = now_ms;
    }
    if (vent_timer_active && (now_ms - vent_timer_start >= VENT_SCHEDULE_DURATION)) {
        vent_timer_active = false;
    }
}

void updatePumpControl() {
    static bool soil_was_dry = false;
    if (soil_value > SOIL_DRY_THRESHOLD) soil_was_dry = true;
    else if (soil_value <= SOIL_WET_THRESHOLD) soil_was_dry = false;

    bool ready_for_new_watering = (now_ms - pump_last_end_ms >= PUMP_MIN_INTERVAL);
    if (!pump_state) {
        if (soil_was_dry && ready_for_new_watering) {
            pump_state = true;
            pump_start_ms = now_ms;
        }
    }
    else {
        if (now_ms - pump_start_ms >= PUMP_MAX_DURATION) {
            pump_state = false;
            pump_last_end_ms = now_ms;
        }
        if (!soil_was_dry) {
            pump_state = false;
            pump_last_end_ms = now_ms;
        }
    }
}

void applyOutputs(bool heater_on, bool vent_on_flag, bool pump_on_flag) {
  writeOutput(LIGHT_PIN, lamp_state);
  writeOutput(HEAT_PIN, heater_on);
  writeOutput(VENT_PIN, vent_on_flag);

  if (pump_on_flag && (now_ms - pump_start_ms >= PUMP_MAX_DURATION)) {
    pump_on_flag = false;
    pump_state = false;
    pump_last_end_ms = now_ms;
    Serial.println(F("PUMP: forced stop at applyOutputs"));
  }
  writeOutput(PUMP_PIN, pump_on_flag);
}

void serialLog(bool heater_on, bool vent_on_flag, bool pump_on_flag) {
  if (now_ms - last_serial_ms < SERIAL_INTERVAL) return;
  last_serial_ms = now_ms;

    Serial.println(F("=== СОСТОЯНИЕ ПАРНИКА ==="));
    Serial.print(F("Цикл: "));
    Serial.println(isNight ? F("НОЧЬ") : F("ДЕНЬ"));

    Serial.print(F("DHT: "));
    if (isnan(air_temp) || isnan(air_hum)) {
        Serial.println(F("READ_FAIL"));
    }
    else {
        Serial.print(air_temp, 1); Serial.print(F(" C, "));
        Serial.print(air_hum, 1); Serial.println(F(" %"));
    }

    long sum = 0;
  for (uint8_t i = 0; i < LIGHT_COUNT; ++i) { Serial.print(F("Light[")); Serial.print(i); Serial.print(F("]: ")); Serial.println(light_values[i]); sum += light_values[i]; }
  int avgLight = LIGHT_COUNT ? (sum / LIGHT_COUNT) : 0;
  Serial.print(F("Средняя освещенность: ")); Serial.println(avgLight);
  Serial.print(F("Лампа: ")); Serial.println(lamp_state ? F("ON") : F("OFF"));
    

    Serial.print(F("Почва: ")); Serial.println(soil_value);
    Serial.print(F("Насос: ")); Serial.println(pump_on_flag ? F("ON") : F("OFF"));

    if (pump_on_flag) Serial.print(F("Время работы насоса истекло (сек): ")), Serial.println((now_ms - pump_start_ms) / 1000);

    else Serial.print(F("Как давно насос не работает (сек): ")), Serial.println((now_ms - pump_last_end_ms) / 1000);

    Serial.print(F("Нагреватель: ")); Serial.println(heater_on ? F("ON") : F("OFF"));
    Serial.print(F("Вентиляция: ")); Serial.println(vent_on_flag ? F("ON") : F("OFF"));
    Serial.println(F("========================="));
}

void setup() {
    Serial.begin(9600);
    dht.begin();

    pinMode(PUMP_PIN, OUTPUT);
    pinMode(LIGHT_PIN, OUTPUT);
    pinMode(HEAT_PIN, OUTPUT);
    pinMode(VENT_PIN, OUTPUT);

    writeOutput(PUMP_PIN, false);
    writeOutput(LIGHT_PIN, false);
    writeOutput(HEAT_PIN, false);
    writeOutput(VENT_PIN, false);

    now_ms = millis();
    cycle_start_ms = now_ms;
    last_serial_ms = now_ms;
    last_vent_ms = now_ms;

    Serial.println(F("Система запущена. Конфигурация:"));
    Serial.print(F("ПОРОГ СУХОСТИ ПОЧВЫ = ")); Serial.println(SOIL_DRY_THRESHOLD);
    Serial.print(F("МАКСИМАЛЬНАЯ ПРОДОЛЖИТЕЛЬНОСТЬ РАБОТЫ НАСОСА (секунды) = ")); Serial.println(PUMP_MAX_DURATION / 1000);
    Serial.println(F("=== СИСТЕМА ГОТОВА ==="));
}

void loop() {
    now_ms = millis();
    
    updateDayNight();
    readSensors();
    updateVentSchedule();

  bool desiredLamp = decideLampDesired(isNight);
  if (desiredLamp != lamp_state) {
    if (now_ms - lamp_last_change_ms >= LAMP_MIN_CHANGE_MS) {
      lamp_state = desiredLamp;
      lamp_last_change_ms = now_ms;
      Serial.print(F("LAMP: state changed to ")); Serial.println(lamp_state ? F("ON") : F("OFF"));
    }
  }

  bool heater_on = decideHeaterOn();

  updatePumpControl();

  bool pump_on_flag = pump_state;
  bool vent_on_flag = decideVentOn(heater_on, pump_on_flag);

  applyOutputs(heater_on, vent_on_flag, pump_on_flag);
  serialLog(heater_on, vent_on_flag, pump_on_flag);

  delay(100);
}