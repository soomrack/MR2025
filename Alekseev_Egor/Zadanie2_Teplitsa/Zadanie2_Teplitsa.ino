#include "DHT.h"

#define PUMP 1
#define LIGHT 2
#define HEAT 3
#define VENT 4

#define TEMP_HUMID_SENSOR 2
#define LIGHT_SENSOR 5
#define SOIL_MOISTURE_SENSOR 6


int air_humidity;
int temperature;
int soil_moisture;
int lighting;

bool heat_on;

struct Values{
  int temperature;
  int air_humidity;
  int soil_moisture;
  int light;
};

struct Values minimal;
struct Values maximum;

DHT dht(TEMP_HUMID_SENSOR, DHT22);

void setup() {
  dht.begin();
  pinMode(PUMP, OUTPUT);
  pinMode(LIGHT, OUTPUT);
  pinMode(HEAT, OUTPUT);
  pinMode(VENT, OUTPUT);
}

void read_sensors(){
  air_humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  soil_moisture = analogRead(SOIL_MOISTURE_SENSOR);
  lighting = analogRead(LIGHT_SENSOR);
}


void heat_control(const int temperature){
  if (temperature < minimal.temperature){
    digitalWrite(HEAT, HIGH);
    heat_on = HIGH;
  }

  else if (temperature > maximum.temperature){
    digitalWrite(HEAT, LOW);
    heat_on = LOW;
  }
}


void vent_control(const int air_humidity, const int heat_on){
  if (heat_on == HIGH){
    digitalWrite(VENT, HIGH);
  }

  else if (air_humidity > maximum.air_humidity){
    digitalWrite(VENT, HIGH);
  }

  else if (air_humidity < minimal.air_humidity){
    digitalWrite(VENT, LOW);
  }
  
}


void pump_control(const int soil_moisture){
  if (soil_moisture < minimal.soil_moisture){
    while (soil_moisture <= maximum.soil_moisture){
      digitalWrite(PUMP, HIGH);
    }
    
  }
  else if (soil_moisture > minimal.soil_moisture){
    digitalWrite(PUMP, LOW);
  }
}


void light_control(const int lighting){
  if (lighting < minimal.light){
    digitalWrite(LIGHT, HIGH);
  }
  else{
    digitalWrite(LIGHT, LOW);
  }
}

void initialization(){
  minimal.temperature = 20;
  minimal.air_humidity = 50;
  minimal.soil_moisture = 100;
  minimal.light = 100;

  maximum.temperature = 25;
  maximum.air_humidity = 80;
  maximum.soil_moisture = 200;
  maximum.light = 200;
}


void loop() {
  initialization();
  read_sensors();
  heat_control(temperature);
  vent_control(air_humidity, heat_on);
  pump_control(soil_moisture);
  light_control(lighting);
}
