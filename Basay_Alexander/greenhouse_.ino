#include <DHT11.h>

#define TEMP_HUM_SENSOR_PIN 12
#define LAMP_PIN 6
#define LIGHT_SEN_PIN A0
#define DIRT_HUMIDITY_PIN A3
#define HEATER_PIN 4
#define FAN_PIN 7
#define PUMP_PIN 5

DHT11 dht11(TEMP_HUM_SENSOR_PIN);

// Параметры климата
struct Climate {
  int minLight = 400;
  int minMoisture = 500;
  int tempMin = 20;
  int tempMax = 30;
  int maxHum = 60;
} climate;

// Переменные для данных датчиков
int temperature, humidity, light, soilMoisture;
unsigned long lastWaterTime = 0;
bool watering = false;
unsigned long wateringStart = 0;

void readSensors() {
  temperature = dht11.readTemperature();
  humidity = dht11.readHumidity();
  light = analogRead(LIGHT_SEN_PIN);
  soilMoisture = analogRead(DIRT_HUMIDITY_PIN);
}

void controlSystems() {
  // Контроль температуры
  if (temperature > climate.tempMax) {
    digitalWrite(FAN_PIN, HIGH);
    digitalWrite(HEATER_PIN, LOW);
  } else if (temperature < climate.tempMin) {
    digitalWrite(FAN_PIN, LOW);
    digitalWrite(HEATER_PIN, HIGH);
  } else {
    digitalWrite(FAN_PIN, LOW);
    digitalWrite(HEATER_PIN, LOW);
  }
  
  // Контроль влажности воздуха
  if (humidity > climate.maxHum) {
    digitalWrite(FAN_PIN, HIGH);
  }
  
  // Контроль освещения (упрощенный)
  int currentHour = (millis() / 3600000UL) % 24;
  if (currentHour >= 6 && currentHour < 22 && light < climate.minLight) {
    digitalWrite(LAMP_PIN, HIGH);
  } else {
    digitalWrite(LAMP_PIN, LOW);
  }
  
  // Контроль полива
  if (!watering && soilMoisture < climate.minMoisture && 
      millis() - lastWaterTime > 300000) {
    watering = true;
    wateringStart = millis();
    digitalWrite(PUMP_PIN, HIGH);
  }
  
  if (watering && millis() - wateringStart > 5000) {
    digitalWrite(PUMP_PIN, LOW);
    watering = false;
    lastWaterTime = millis();
  }
}

void setup() {
  pinMode(FAN_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(LAMP_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  Serial.begin(9600);
}

void loop() {
  readSensors();
  controlSystems();
  
  Serial.print("Temp: "); Serial.print(temperature);
  Serial.print(" Hum: "); Serial.print(humidity);
  Serial.print(" Light: "); Serial.print(light);
  Serial.print(" Soil: "); Serial.println(soilMoisture);
  
  delay(1000);
}