#include "DHT.h"

#define TEMP_HUMID_PIN 12
#define SOIL_MOISTURE_PIN A1
#define LIGHT_PIN A2

DHT dht(TEMP_HUMID_PIN, DHT11);

float temperature;
float humidity;
int soil_moisture;
int light;

void setup() {
  Serial.begin(9600);
  dht.begin();
}

void readSensors(){
  temperature = dht.readTemperature();
  humidity = dht.readHumidity();

  soil_moisture = analogRead(SOIL_MOISTURE_PIN);
  light = analogRead(LIGHT_PIN);
}

void send_readings(){
  String data = "TEMP:" + String(temperature) +
                " HUM:" + String(humidity) +
                " LIGHT:" + String(light);

  Serial.println(data);
}

void loop() {

  readSensors();        // <-- ВАЖНО
  send_readings();      // <-- сразу шлём

  delay(2000);          // DHT11 требует паузу
}
