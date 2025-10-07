#define PUMP 1
#define LIGHT 2
#define HEAT 3
#define VENT 4
#include "DHT.h"

#define DHTPIN 2
#define LIGHT_SENSOR 5
#define SOIL_MOISTURE_SENSOR 6
#define TEMPERATURE_SENSOR 7
#define HUMIDITY_SENSOR 8

struct Values{
  int temperature;
  int air_humidity;
  int soil_moisture;
  int light;
};

struct Values minimal;
struct Values maximum;

void setup() {
  Serial.begin(9600);
  pinMode(PUMP, OUTPUT);
  pinMode(LIGHT, OUTPUT);
  pinMode(HEAT, OUTPUT);
  pinMode(VENT, OUTPUT);
}


void air_humidity_control(){

  int air_humidity = analogRead(HUMIDITY_SENSOR);
  if (air_humidity < minimal.air_humidity){
    digitalWrite(VENT, LOW);
  }
  else if (air_humidity > minimal.air_humidity){
    digitalWrite(VENT, HIGH);
  }
}


void soil_moisture_control(){
  int soil_moisture = analogRead(SOIL_MOISTURE_SENSOR);
  if (soil_moisture < minimal.soil_moisture){
    digitalWrite(PUMP, HIGH);
  }
  else if (soil_moisture > minimal.soil_moisture){
    digitalWrite(PUMP, LOW);
  }
}

void vent_control(){
  int air_humidity = analogRead(HUMIDITY_SENSOR);
  int temperature = 

}


void loop() {
  

}
