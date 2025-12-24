#include <DHT11.h>

#define PUMP_PIN 5 
#define LIGHT_PIN 6
#define HEAT_PIN 4
#define FAN_PIN 7
#define TEMP_HUMID_SENSOR_PIN 12
#define SOIL_MOISTURE_SENSOR_PIN 3

#define LIGHT_SENSOR_1_PIN 0
#define LIGHT_SENSOR_2_PIN 1
#define LIGHT_SENSOR_3_PIN 2
#define LIGHT_SENSOR_4_PIN 8
#define LIGHT_SENSOR_5_PIN 9


const unsigned long DAY_DURATION 12*1000;    
const unsigned long NIGHT_DURATION 12*1000;  
const unsigned long SERIAL_INTERVAL 1*1000;    

const unsigned long WATERING_DURATION 2*1000;   
const unsigned long WATERING_DELAY 2*1000;         

unsigned long TIME = 0;


struct ClimateConditions {
  int minTemperature;
  int maxTemperature;
  int minAirHumidity;
  int maxAirHumidity;
  int minSoilMoisture;  // Чем выше значение, тем суше почва
  int maxSoilMoisture;  // Чем ниже значение, тем влажнее почва
  int minLight;  
  int maxLight;
};


ClimateConditions climate = {29, 30, 20, 40, 900, 10, 400, 20};


class Fan {
  public:
    const int pin = FAN_PIN;
    bool on_heater;      
    bool on_humidity;
    bool on_hot;          
  public:
    void power();
};


class Heater {
  public:
    const int pin = HEAT_PIN;
    bool on_temperature; 
  public:
    void power(); 
};


class Lamp {
  public:
    const int pin = LIGHT_PIN;
    bool on_light;       
    bool on_night;       
  public:
    void power(); 
};


class Pump {
  public:
    const int pin = PUMP_PIN;
    bool on_pump;       
    unsigned long startWateringTime = 0;
    unsigned long stopWateringTime = 0;
    bool wateringInProgress = false;
    bool pumpCurrentlyActive = false;
  public:
    void power(); 
};


class Thermometer {
  public:
    int temperature;   
  public:
    void get_data(); 
};


class LightSensor {
  public:
    int pin;
    int lightLevel;    
  public:
    void get_data(); 
};


class LightSensor_merge: public LightSensor {
  public:
    LightSensor sensors[5];
    int averageLightLevel;
  public:
    void calculate();
    void initPins();
    void readAllSensors();
};


class AirHumiditySensor {
  public:
    int humidity;  
  public:    
    void get_data(); 
};


class SoilMoistureSensor {
  public:
    const int pin = SOIL_MOISTURE_SENSOR_PIN;
    int moisture;      
  public:
    void get_data(); 
};


class DayNightCycle {
  public:
    bool isNight;                  
    unsigned long dayDuration;     
    unsigned long nightDuration;   
    unsigned long cycleStartTime;  
    DayNightCycle(unsigned long day, unsigned long night) : dayDuration(day), nightDuration(night), isNight(false), cycleStartTime(0) {}
    void update(); 
};


void Fan::power()
{
  if (on_heater || on_humidity || on_hot) {
    digitalWrite(pin, HIGH);
  } 
  else {
    digitalWrite(pin, LOW);
  }
}


void Heater::power()
{ 
  if (on_temperature) {
    digitalWrite(pin, HIGH);
  } 
  else {
    digitalWrite(pin, LOW);
  }
}


void Lamp::power()
{
  if (on_light || on_night) {
    digitalWrite(pin, HIGH);
  } 
  else {
    digitalWrite(pin, LOW);
  }
}

void Pump::power()
{
  if (on_pump) {
    digitalWrite(pin, HIGH);
  } 
  else {
    digitalWrite(pin, LOW);
  }
}


void Thermometer::get_data()
{
  temperature = dht11.readTemperature(); 
}


void LightSensor::get_data()
{
  lightLevel = analogRead(pin);
}


void AirHumiditySensor::get_data()
{
  humidity = dht11.readHumidity();
}


void SoilMoistureSensor::get_data()
{
  moisture = analogRead(pin);
}


void DayNightCycle::update()
{
  unsigned long cycleTime = TIME - cycleStartTime;
  if (!isNight && cycleTime >= dayDuration) {
    isNight = true;
    cycleStartTime = TIME;
  } 
  else if (isNight && cycleTime >= nightDuration) {
    isNight = false;
    cycleStartTime = TIME;
  }
}


void LightSensor_merge::initPins() {
  sensors[0].pin = LIGHT_SENSOR_1_PIN;
  sensors[1].pin = LIGHT_SENSOR_2_PIN;
  sensors[2].pin = LIGHT_SENSOR_3_PIN;
  sensors[3].pin = LIGHT_SENSOR_4_PIN;
  sensors[4].pin = LIGHT_SENSOR_5_PIN;
}


void LightSensor_merge::calculate() {
  int sum = 0;
  int count = 0;

  for (int i = 0; i < 5; i++) {
    sensors[i].get_data();
    if (sensors[i].lightLevel > 0 && sensors[i].lightLevel < 1024) {
      sum += sensors[i].lightLevel;
      count++;
    }
  }
  
  if (count > 0) {
    averageLightLevel = sum / count;
  } 
  else {
    averageLightLevel = 0;
  }
}


void LightSensor_merge::readAllSensors() {
  for (int i = 0; i < 5; i++) {
    sensors[i].get_data();
  }
}


void control_temperature(Thermometer &thermometer, Fan &fan, Heater &heater) {
  if (thermometer.temperature >= climate.maxTemperature) {
    fan.on_hot = false;
    heater.on_temperature = false;
  } 
  else if (thermometer.temperature < climate.minTemperature) {
    fan.on_hot = false;
    fan.on_heater = true;
    heater.on_temperature = true;
  } 
  else {
    fan.on_hot = false;
    heater.on_temperature = false;
  }
}


void control_air_humidity(Fan &fan, AirHumiditySensor &airHumiditySensor) {
  if (airHumiditySensor.humidity > climate.maxAirHumidity) {
    fan.on_humidity = true;
  } 
  else if (airHumiditySensor.humidity <= climate.minAirHumidity) {
    fan.on_humidity = false;
  }
}


void control_light(LightSensor &lightSensor, Lamp &lamp, DayNightCycle &dayNight) {
  if (dayNight.isNight) { 
    lamp.on_night = true;
    lamp.on_light = false;
  } 
  else {
    if (lightSensor.averageLightLevel >= climate.minLight) {// Чем выше значение, тем темнее
      lamp.on_light = true;
    } 
    else {
      lamp.on_light = false;
    }
    lamp.on_night = false;
  }
}


void control_soil_moister(Pump &pump, SoilMoistureSensor &soilMoistureSensor) {
  if (soilMoistureSensor.moisture > climate.minSoilMoisture) {
    if (!pump.wateringInProgress) { // Начинаем новый цикл полива
      pump.wateringInProgress = true;
      pump.pumpCurrentlyActive = true;
      pump.startWateringTime = TIME;
      pump.on_pump = true;
    } 

    else if (pump.wateringInProgress && pump.pumpCurrentlyActive) { // Полив в процессе, насос работает
      if (TIME - pump.startWateringTime >= WATERING_DURATION) { // Завершаем полив
        pump.on_pump = false;
        pump.pumpCurrentlyActive = false;
        pump.stopWateringTime = TIME;
      }
    } 
    
    else if (pump.wateringInProgress && !pump.pumpCurrentlyActive) { // Пауза между поливами
      if (TIME - pump.stopWateringTime >= WATERING_DELAY) { 
        pump.wateringInProgress = false;
      }
    }

  } else {
    pump.on_pump = false;
    pump.wateringInProgress = false;
    pump.pumpCurrentlyActive = false;
  }
}


void printStatus(Thermometer &thermometer, LightSensorMerge &lightSensor, 
                 AirHumiditySensor &airHumiditySensor, SoilMoistureSensor &soilMoistureSensor,
                 DayNightCycle &dayNight) {
  static unsigned long lastPrintTime = 0;
  
  if (TIME - lastPrintTime >= SERIAL_INTERVAL) {
    Serial.print("Температура: ");
    Serial.print(thermometer.temperature);
    Serial.print("°C, Влажность воздуха: ");
    Serial.print(airHumiditySensor.humidity);
    Serial.print("%, Влажность почвы: ");
    Serial.print(soilMoistureSensor.moisture);
    Serial.print(", Освещенность: ");
    Serial.print(lightSensor.lightLevel);
    lastPrintTime = TIME;
  }
}


Fan fan;
Heater heater;
Lamp lamp;
Pump pump;
Thermometer thermometer;
LightSensor lightSensor;
LightSensor_merge lightSensor;
AirHumiditySensor airHumiditySensor;
SoilMoistureSensor soilMoistureSensor;
DayNightCycle dayNight(DAY_DURATION, NIGHT_DURATION);


void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(fan.pin, OUTPUT);
  pinMode(heater.pin, OUTPUT);
  pinMode(lamp.pin, OUTPUT);
  pinMode(pump.pin, OUTPUT);
  lightSensor.initPins();
  pinMode(lightSensor.pin, OUTPUT);
  Serial.println("Система умной теплицы инициализирована");
  Serial.println("======================================");
}

void loop() {
  TIME = millis(); 
  dayNight.update();
  thermometer.get_data();
  lightSensor.readAllSensors();
  lightSensor.calculate();
  airHumiditySensor.get_data();
  soilMoistureSensor.get_data();
  
  control_temperature(thermometer, fan, heater);
  control_air_humidity(fan, airHumiditySensor);
  control_light(lightSensor, lamp, dayNight);
  control_soil_moister(pump, soilMoistureSensor);
  
  fan.power();
  heater.power();
  lamp.power();
  pump.power();

  //Serial.print(lightSensor.sensors[2].lightLevel);
  
  printStatus(thermometer, lightSensor, airHumiditySensor, 
              soilMoistureSensor, dayNight);
  
  delay(100); 
}