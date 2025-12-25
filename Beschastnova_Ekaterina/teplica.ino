#include <DHT.h>

#define PUMP_PIN 5 
#define LIGHT_PIN 6
#define HEAT_PIN 4
#define VENT_PIN 7

#define TEMP_HUMID_SENSOR_PIN 12
#define SOIL_MOISTURE_SENSOR_PIN 3
#define LIGHT_SENSOR_PIN 0


// Переменные для показаний датчиков
int airHumidity;
int tempValue;
int soilMoisture;
int lightLevel;


bool lightActive;
bool heatActive;
bool ventActive;
bool soilLowMoisture;
bool pumpActive;

unsigned long lastTime = 0;
unsigned long currentTime; 
unsigned long dayTime = 12*1000;
unsigned long nightTime = 12*1000;
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
  int light;
};

struct SensorLimits minValues;
struct SensorLimits maxValues;

enum Season {
  SPRING,
  SUMMER,
  AUTUMN,
  WINTER
};

Season currentSeason = SPRING;
int seasonDayCounter = 0;
const int daysInSeason = 30;

DHT dht(TEMP_HUMID_SENSOR_PIN, DHT11);

void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(HEAT_PIN, OUTPUT);
  pinMode(VENT_PIN, OUTPUT);
  adjustLimitsForSeason();
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

void checkLightConditions(){
  lightLevel = analogRead(LIGHT_SENSOR_PIN);
  if ((lightLevel >= minValues.light && !isNight)||(isNight))
  {
    lightActive = true;
  }
  else
  {
    lightActive = false;
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


void controlLight(const bool light_on){
  if (light_on == true){
    digitalWrite(LIGHT_PIN , HIGH);
  }
  else{
    digitalWrite(LIGHT_PIN , LOW);
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


  
  
  adjustLimitsForSeason(); //Параметры в зависимости от сезона
}

void adjustLimitsForSeason() {
  switch(currentSeason) {
    case SPRING:
      minValues.temperature = 20;
      maxValues.temperature = 25;
      minValues.air_humidity = 40;
      maxValues.air_humidity = 60;
      minValues.light = 500; 
      minValues.soil_moisture = 700; 
      dayTime = 14 * 1000; 
      nightTime = 10 * 1000; 
      break;
      
    case SUMMER:
      minValues.temperature = 25;
      maxValues.temperature = 32;
      minValues.air_humidity = 30;
      maxValues.air_humidity = 50;
      minValues.light = 300;
      minValues.soil_moisture = 600;
      dayTime = 16 * 1000;
      nightTime = 8 * 1000;  
      break;
      
    case AUTUMN:
      minValues.temperature = 18;
      maxValues.temperature = 23;
      minValues.air_humidity = 35;
      maxValues.air_humidity = 55;
      minValues.light = 400;
      minValues.soil_moisture = 800;
      dayTime = 12 * 1000;  
      nightTime = 12 * 1000; 
      break;
      
    case WINTER:
      minValues.temperature = 15;
      maxValues.temperature = 20;
      minValues.air_humidity = 30;
      maxValues.air_humidity = 45;
      minValues.light = 600;
      minValues.soil_moisture = 900;
      dayTime = 10 * 1000;  
      nightTime = 14 * 1000; 
      break;
  }
  
  // Информация о смене параметров
  Serial.print("Установлены параметры для ");
  switch(currentSeason) {
    case SPRING: Serial.println("ВЕСНЫ"); break;
    case SUMMER: Serial.println("ЛЕТА"); break;
    case AUTUMN: Serial.println("ОСЕНИ"); break;
    case WINTER: Serial.println("ЗИМЫ"); break;
  }
}

void updateSeason() {
  static unsigned long lastSeasonCheck = 0;
  const unsigned long seasonCheckInterval = 60000;
  
  if (millis() - lastSeasonCheck >= seasonCheckInterval) {
    lastSeasonCheck = millis();
    
    seasonDayCounter++;
    
    if (seasonDayCounter >= daysInSeason) {
      seasonDayCounter = 0;
      
      switch(currentSeason) {
        case SPRING:
          currentSeason = SUMMER;
          Serial.println("\n=== НАСТУПИЛО ЛЕТО ===");
          break;
        case SUMMER:
          currentSeason = AUTUMN;
          Serial.println("\n=== НАСТУПИЛА ОСЕНЬ ===");
          break;
        case AUTUMN:
          currentSeason = WINTER;
          Serial.println("\n=== НАСТУПИЛА ЗИМА ===");
          break;
        case WINTER:
          currentSeason = SPRING;
          Serial.println("\n=== НАСТУПИЛА ВЕСНА ===");
          break;
      }
      adjustLimitsForSeason();
    }
  }
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
  Serial.println();

}


void loop(){
  airHumidity = dht.readHumidity();
  tempValue = dht.readTemperature();
  
  soilMoisture = analogRead(SOIL_MOISTURE_SENSOR_PIN);
  lightLevel = analogRead(LIGHT_SENSOR_PIN);

  updateDayNightCycle();
  updateSeason();
  
  checkLightConditions();
  checkTemperature(tempValue);
  checkAirHumidity(airHumidity);
  checkSoilMoisture(soilMoisture);

  controlLight(lightActive);
  controlHeat(heatActive);
  controlVent(ventActive, heatActive);

  decidePumpActivation(soilLowMoisture);
  controlWaterPump(pumpActive);

  if (currentTime - lastSerialTime >= serialInterval) {
    printSensorData();
    lastSerialTime = currentTime;
  }
}
