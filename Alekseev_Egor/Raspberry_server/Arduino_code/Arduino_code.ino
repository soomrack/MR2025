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

void send_readings(){
  String data = "Temperature: " + String(temperature) + "," +
                "Humidity: " + String(humidity) + "," +
                "Light" + String(light) + "," +
                "Soil moisture: " + String(soil_moisture);

  Serial.println(data);
}

void loop() {
  if (Serial.available() > 0) { 
        incomingByte = Serial.read();
        if (incomingByte == "1"){
          send_readings();
        }
    }
}
