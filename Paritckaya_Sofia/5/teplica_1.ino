#include <DHT.h>

#define PUMP_PIN 5 
#define LIGHT_PIN_1 6  // Первая лампа
#define LIGHT_PIN_2 8  // Вторая лампа
#define LIGHT_PIN_3 9  // Третья лампа
#define HEAT_PIN 4
#define VENT_PIN 7

#define TEMP_HUMID_SENSOR_PIN 12
#define SOIL_MOISTURE_SENSOR_PIN 3
#define LIGHT_SENSOR_PIN 0

// Класс для управления одной лампой
class Lamp {
private:
    int pin;
    bool is_on;
    
public:
    Lamp(int lampPin) : pin(lampPin), is_on(false) {
      pinMode(pin, OUTPUT);
      }
    
    void power() {
        digitalWrite(pin, is_on ? HIGH : LOW);
    }
    
    void set_state(bool state) {
        is_on = state;
    }
    
    bool get_state() const {
        return is_on;
    }
};

// Класс для управления массивом ламп
class LampArray {
private:
    Lamp* lamps[3];
    
public:
    LampArray(int pin1, int pin2, int pin3) {
        lamps[0] = new Lamp(pin1);
        lamps[1] = new Lamp(pin2);
        lamps[2] = new Lamp(pin3);
    }
    
    ~LampArray() {
        delete lamps[0];
        delete lamps[1];
        delete lamps[2];
    }
    
    void power() {
        lamps[0]->power();
        lamps[1]->power();
        lamps[2]->power();
    }
    
    void set_state(int index, bool state) {
        if (index >= 0 && index < 3) {
            lamps[index]->set_state(state);
        }
    }
    
    bool get_state(int index) const {
        if (index >= 0 && index < 3) {
            return lamps[index]->get_state();
        }
        return false;
    }
};

// Создаем объект для управления лампами
LampArray myLamps(LIGHT_PIN_1, LIGHT_PIN_2, LIGHT_PIN_3);

// Переменные для показаний датчиков
int airHumidity;
int tempValue;
int soilMoisture;
int lightLevel;

// Состояния устройств
bool heatActive;
bool ventActive;
bool soilLowMoisture;
bool pumpActive;

unsigned long lastTime = 0;
unsigned long currentTime; 
const unsigned long dayTime = 12*1000;
const unsigned long nightTime = 12*1000;
bool isNight;

unsigned long lastSerialTime = 0;
const unsigned long serialInterval = 5000;

unsigned long startWateringTime = 0;
unsigned long stopWateringTime = 0;   
const unsigned long wateringDuration = 2000;
const unsigned long wateringDelay = 2000;
bool wateringInProgress = false;
bool pumpCurrentlyActive;

struct SensorLimits{
  int temperature;
  int air_humidity;
  int soil_moisture;
  int light_low;     // Порог для включения 1 лампы
  int light_medium;  // Порог для включения 2 ламп
  int light_high;    // Порог для включения 3 ламп
};

struct SensorLimits minValues;
struct SensorLimits maxValues;

DHT dht(TEMP_HUMID_SENSOR_PIN, DHT11);

void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(HEAT_PIN, OUTPUT);
  pinMode(VENT_PIN, OUTPUT);
  initializeLimits();
}

void updateDayNightCycle(){
  currentTime = millis();
  if (currentTime - lastTime < dayTime){
    isNight = false;
  }
  else if (currentTime - lastTime > dayTime && currentTime - lastTime < nightTime + dayTime){
    isNight = true;
  }
  else if (currentTime - lastTime > nightTime){
    isNight = false;
    lastTime = currentTime;
  }
}

void control_light(LampArray& lamps, bool is_night, int light_level) {
  if (is_night) {
    // Ночью включаем все три лампы
    lamps.set_state(0, true);
    lamps.set_state(1, true);
    lamps.set_state(2, true);
  } else {
    // Днем регулируем освещение в зависимости от уровня освещенности
    if (light_level >= maxValues.light_high) {
      // Очень темно - включаем все три лампы
      lamps.set_state(0, true);
      lamps.set_state(1, true);
      lamps.set_state(2, true);
    } else if (light_level >= maxValues.light_medium) {
      // Средняя освещенность - включаем две лампы
      lamps.set_state(0, true);
      lamps.set_state(1, true);
      lamps.set_state(2, false);
    } else if (light_level >= maxValues.light_low) {
      // Немного темно - включаем одну лампу
      lamps.set_state(0, true);
      lamps.set_state(1, false);
      lamps.set_state(2, false);
    } else {
      // Достаточно светло - выключаем все лампы
      lamps.set_state(0, false);
      lamps.set_state(1, false);
      lamps.set_state(2, false);
    }
  }
}

void checkTemperature(const int temperature){
  if (temperature < minValues.temperature){
    heatActive = true;
  }
  else if (temperature >= maxValues.temperature){
    heatActive = false;
  }
}

void checkAirHumidity(const int air_humidity){
  if (air_humidity > maxValues.air_humidity){
    ventActive = true;
  }
  else if (air_humidity <= minValues.air_humidity){
    ventActive = false;
  }
}

void checkSoilMoisture(const int humidity){
  if (soilMoisture < minValues.soil_moisture){
    soilLowMoisture = true;
  }
  else if (soilMoisture >= maxValues.soil_moisture){
    soilLowMoisture = false;
  }
}

void controlHeat(const bool heat_on){
  if (heat_on == true){
    digitalWrite(HEAT_PIN , HIGH);
  }
  else{
    digitalWrite(HEAT_PIN , LOW);
  }
}

void controlVent(const bool vent_on, const bool heat_on){
  if (vent_on == true || heat_on == true){
    digitalWrite(VENT_PIN , HIGH);
  }
  else{
    digitalWrite(VENT_PIN , LOW);
  }
}

void decidePumpActivation(const bool moisture_low){
   if (moisture_low == false){  //Полив не нужен
    pumpActive = false;
    wateringInProgress = false;
  }
  else {
    if (wateringInProgress == true && pumpCurrentlyActive == true){  //Полив в процессе, работает насос
      if (currentTime - startWateringTime < wateringDuration){  //Период полива
        pumpActive = true;
      }
      else{
        pumpActive = false;  //Переход в режим ожидания
        pumpCurrentlyActive = false;
        stopWateringTime = currentTime;
      }
    }
    else if (wateringInProgress == true && pumpCurrentlyActive == false){  //Режим ожидания
      if (currentTime - stopWateringTime < wateringDelay){
          pumpActive = false;
      }
      else{
          wateringInProgress = false;  //Почва была влажной, стала сухой
      }
    }
    else if (wateringInProgress == false){
      pumpCurrentlyActive = true;
      startWateringTime = currentTime;
      wateringInProgress = true;
    }
  }
}

void controlWaterPump(const bool pump_on){
    if (pump_on == true){
    digitalWrite(PUMP_PIN , HIGH);
  }
  else{
    digitalWrite(PUMP_PIN , LOW);
  }
}

void initializeLimits(){
  minValues.temperature = 29;
  minValues.air_humidity = 20;
  minValues.soil_moisture = 900;  

  // Пороги освещенности для включения разного количества ламп
  // Чем выше значение с датчика, тем темнее
  maxValues.light_low = 300;    // Включение 1 лампы
  maxValues.light_medium = 500; // Включение 2 ламп
  maxValues.light_high = 700;   // Включение 3 ламп

  maxValues.temperature = 30;
  maxValues.air_humidity = 40;
  maxValues.soil_moisture = 10;
}

void printSensorData(){
  Serial.print("Температура: ");
  Serial.println(tempValue);
  Serial.print("Влажность воздуха: ");
  Serial.println(airHumidity);
  Serial.print("Влажность почвы: ");
  Serial.println(soilMoisture);
  Serial.print("Освещенность: ");
  Serial.println(lightLevel);
  Serial.print("Лампа 1: ");
  Serial.println(myLamps.get_state(0) ? "ВКЛ" : "ВЫКЛ");
  Serial.print("Лампа 2: ");
  Serial.println(myLamps.get_state(1) ? "ВКЛ" : "ВЫКЛ");
  Serial.print("Лампа 3: ");
  Serial.println(myLamps.get_state(2) ? "ВКЛ" : "ВЫКЛ");
  Serial.println();
}

void loop(){
  // Чтение данных с датчиков
  airHumidity = dht.readHumidity();
  tempValue = dht.readTemperature();
  soilMoisture = analogRead(SOIL_MOISTURE_SENSOR_PIN);
  lightLevel = analogRead(LIGHT_SENSOR_PIN);

  // Обновление состояний
  updateDayNightCycle();
  checkTemperature(tempValue);
  checkAirHumidity(airHumidity);
  checkSoilMoisture(soilMoisture);

  // Управление устройствами
  control_light(myLamps, isNight, lightLevel);
  controlHeat(heatActive);
  controlVent(ventActive, heatActive);
  decidePumpActivation(soilLowMoisture);
  controlWaterPump(pumpActive);

  // Применяем состояния к лампам
  myLamps.power();

  if (currentTime - lastSerialTime >= serialInterval) {
    printSensorData();
    lastSerialTime = currentTime;
  }
}
