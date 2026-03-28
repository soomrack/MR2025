#include "DHT.h"
#define TEMP_HUMID_PIN 2
#define SOIL_MOISTURE_PIN 3
#define LIGHT_PIN 4

DHT dht(TEMP_HUMID_PIN, DHT11);

int temperature;
int humidity;
int soil_moisture;
int light;

int incomingByte = 0;

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

void convert_lightness(){
  light = map(light, 0, 1023, 10, 600);
}

void convert_soil_moisture(){
  soil_moisture = map(soil_moisture, 0, 1023, 0, 100);
}

void send_readings(){
  String data = String(temperature) + String(humidity) + String(light) + String(soil_moisture);
  Serial.println(data);
}

void loop() {
  if (Serial.available() > 0) { 
        incomingByte = Serial.read();
        if (incomingByte == "1"){
          readSensors();
          convert_lightness();
          convert_soil_moisture();
          send_readings();
        }
    }
}
