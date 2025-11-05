#include "TinyDHT.h"  

// Pin definitions
#define DHT_PIN         8
#define LIGHT_SENSOR    A0
#define MOISTURE_SENSOR A1

#define LED_GROW        6
#define WATER_PUMP      5 
#define HEATER          3 
#define FAN             7

// Optimal values
#define OPT_HUMIDITY    50
#define OPT_TEMP        25
#define OPT_MOISTURE    400
#define OPT_LIGHT       500

// Safety timers (seconds)
#define PUMP_SAFETIME   30
#define HEATER_SAFETIME 60
#define FAN_SAFETIME    120

// Control thresholds
#define LIGHT_THRESHOLD 2
#define TEMP_THRESHOLD  2
#define HUMID_THRESHOLD 2
#define MOIST_THRESHOLD 2

// System state
struct {
  int temp;
  int humidity;
  int moisture;
  int light;
  
  bool isLightOn;
  bool isPumpOn;
  bool isHeating;
  bool isFanOn;
  
  uint32_t lightTimer;
  uint32_t pumpTimer;
  uint32_t heatTimer;
  uint32_t fanTimer;
} greenhouse;

// Инициализация датчика ПЕРЕД использованием
DHT envSensor(DHT_PIN, DHT11);  // Убедитесь, что TinyDHT поддерживает такой синтаксис

void setup() {
  Serial.begin(115200);
  
  pinMode(LED_GROW, OUTPUT);
  pinMode(WATER_PUMP, OUTPUT);
  pinMode(HEATER, OUTPUT);
  pinMode(FAN, OUTPUT);
  
  digitalWrite(LED_GROW, LOW);
  digitalWrite(WATER_PUMP, HIGH); // Pump off (active low)
  digitalWrite(HEATER, LOW);
  digitalWrite(FAN, LOW);
  
  // Инициализация таймеров
  greenhouse.lightTimer = millis();
  greenhouse.pumpTimer = millis();
  greenhouse.heatTimer = millis();
  greenhouse.fanTimer = millis();
}

void readSensors() {
  // В зависимости от используемой библиотеки:
  greenhouse.temp = envSensor.readTemperature();
  greenhouse.humidity = envSensor.readHumidity();
  
  
  greenhouse.light = analogRead(LIGHT_SENSOR);
  greenhouse.moisture = analogRead(MOISTURE_SENSOR);
}

void controlLight() {
  if (abs(greenhouse.light - OPT_LIGHT) >= LIGHT_THRESHOLD && millis() - greenhouse.lightTimer >= 1500) {
    greenhouse.isLightOn = greenhouse.light < OPT_LIGHT;
    digitalWrite(LED_GROW, greenhouse.isLightOn);
    greenhouse.lightTimer = millis();
  }
}

void controlWater() {
  if (greenhouse.moisture > OPT_MOISTURE + MOIST_THRESHOLD && millis() - greenhouse.pumpTimer >= 5000) {
    digitalWrite(WATER_PUMP, LOW); // Turn pump on
    greenhouse.isPumpOn = true;
    greenhouse.pumpTimer = millis();
  } 
  else if (greenhouse.moisture < OPT_MOISTURE - MOIST_THRESHOLD) {
    digitalWrite(WATER_PUMP, HIGH); // Turn pump off
    greenhouse.isPumpOn = false;
  }
}

void controlTemperature() {
  greenhouse.isHeating = greenhouse.temp < OPT_TEMP - TEMP_THRESHOLD;
  digitalWrite(HEATER, greenhouse.isHeating);
}

void controlVentilation() {
  bool needCooling = greenhouse.humidity > OPT_HUMIDITY + HUMID_THRESHOLD || 
                    greenhouse.isHeating || 
                    greenhouse.temp > OPT_TEMP + 1;
  
  bool canTurnOff = greenhouse.humidity < OPT_HUMIDITY - HUMID_THRESHOLD &&
                   !greenhouse.isHeating && 
                   greenhouse.temp < OPT_TEMP + 3;
  
  greenhouse.isFanOn = needCooling || !canTurnOff;
  digitalWrite(FAN, greenhouse.isFanOn);
}

void logData() {
  Serial.print("H: "); Serial.print(greenhouse.humidity); Serial.print("% ");
  Serial.print("T: "); Serial.print(greenhouse.temp); Serial.print("C ");
  Serial.print("L: "); Serial.print(greenhouse.light); Serial.print(" ");
  Serial.print("M: "); Serial.print(greenhouse.moisture); Serial.print(" ");
  Serial.print("STATUS: ");
  Serial.print(greenhouse.isLightOn ? "L_ON " : "L_OFF ");
  Serial.print(greenhouse.isPumpOn ? "P_ON " : "P_OFF ");
  Serial.print(greenhouse.isHeating ? "H_ON " : "H_OFF ");
  Serial.println(greenhouse.isFanOn ? "F_ON" : "F_OFF");
}

void loop() {
  readSensors();
  
  controlLight();
  controlWater();
  controlTemperature();
  controlVentilation();
  
  logData();
  
  delay(100);
}