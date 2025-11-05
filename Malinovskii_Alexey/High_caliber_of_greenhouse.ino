#include "DHT.h"

#define PUMP_PIN 5
#define LIGHT_BULB_PIN 6
#define HEAT_GENERATOR_PIN 4
#define VENT_PIN 7

#define TEMP_HUMID_SENSOR_PIN 12
#define LIGHT_SENSOR_PIN 0
#define SOIL_MOISTURE_SENSOR_PIN 3


bool pump_has_been_act = true;
int pump_work_time = 1000;

int level_of_humidity;
int illumination_level;
int temperature_receivings;
int level_of_moisturing;

bool light_bulb_activate;
bool vent_activate;
bool heat_generator_activate;
bool water_pump_activate;
bool light_bulb_activate_3;
bool light_bulb_activate_2;
bool light_bulb_activate_1;

bool night;


int current_time;
int end_of_day = 10000;
int end_of_night = 10000;
int twenty_four_hours = end_of_day + end_of_night;
int timer = 0;


struct State_of_greenhouse {
  int level_of_humidity;
  int illumination_level;
  int temperature_receivings;
  int level_of_moisturing;
};

struct State_of_greenhouse extreme_parameters;
struct State_of_greenhouse miserable_parameters;



DHT dht(TEMP_HUMID_SENSOR_PIN, DHT11);




void einstellung_optimaler_Parameter() {

  miserable_parameters.level_of_humidity = 20;
  miserable_parameters.illumination_level = 200;
  miserable_parameters.temperature_receivings = 20;
  miserable_parameters.level_of_moisturing = 900;

  extreme_parameters.level_of_humidity = 40;
  extreme_parameters.illumination_level = 20;
  extreme_parameters.temperature_receivings = 36;
  extreme_parameters.level_of_moisturing = 10;

}



void zeit() {


  current_time = millis();



  if (current_time - timer < end_of_day) {
    night = false;
  }

  if (current_time - timer > end_of_day && current_time - timer < twenty_four_hours) {
    night = true;
  }

  if (current_time - timer > twenty_four_hours) {
    night = false;
    timer = current_time;
  }

}


// проверка температуры
void temperaturprufung() {

  temperature_receivings = dht.readTemperature();

  if (temperature_receivings <= miserable_parameters.temperature_receivings) {
    heat_generator_activate = true;
  }

  else if (temperature_receivings > extreme_parameters.temperature_receivings) {
    heat_generator_activate = false;
  }

  else {
    heat_generator_activate = true;
  }

}


//проверка удобренности
void uberprufungderBodendungung() {
  level_of_moisturing = analogRead(SOIL_MOISTURE_SENSOR_PIN);

  if (level_of_moisturing < miserable_parameters.level_of_moisturing) {
    water_pump_activate = true;
  }

  else if (level_of_moisturing >= extreme_parameters.level_of_moisturing) {
    water_pump_activate = false;
  }

}


//проверка освещенности
void beleuchtungprufen() {

  illumination_level = analogRead(LIGHT_SENSOR_PIN);

  if (miserable_parameters.illumination_level <= illumination_level && night == false) {
    light_bulb_activate_1 = true;
  }

  else {
    light_bulb_activate_1 = false;
  }

}

void beleuchtungprufenfalsch() {

  illumination_level = analogRead(LIGHT_SENSOR_PIN);

  if (miserable_parameters.illumination_level <= illumination_level && night == false) {
    light_bulb_activate_2 = false;
  }

  else {
    light_bulb_activate_2 = true;
  }

}

void beleuchtungprufenquest() {

  illumination_level = analogRead(LIGHT_SENSOR_PIN);

  if (miserable_parameters.illumination_level <= illumination_level && night == false) {
    light_bulb_activate_3 = true;
  }

  else {
    light_bulb_activate_3 = false;
  }

}

void chekingsomesensors() {

  if (light_bulb_activate_1 == false && light_bulb_activate_3 == false && light_bulb_activate_2 == true) {
    light_bulb_activate = false;
  }

  else if (light_bulb_activate_1 == true && light_bulb_activate_3 == true && light_bulb_activate_2 == false) {
    light_bulb_activate = true;
  }

  else if (light_bulb_activate_1 == false && light_bulb_activate_3 == true && light_bulb_activate_2 == false) {
    light_bulb_activate = true;
  }

  else if (light_bulb_activate_1 == false && light_bulb_activate_3 == true && light_bulb_activate_2 == true) {
    light_bulb_activate = false;
  }

  else if (light_bulb_activate_1 == true && light_bulb_activate_3 == false && light_bulb_activate_2 == true) {
    light_bulb_activate = false;
  }

  else if (light_bulb_activate_1 == true && light_bulb_activate_3 == false && light_bulb_activate_2 == false) {
    light_bulb_activate = true;
  }

  else if (light_bulb_activate_1 == false && light_bulb_activate_3 == false && light_bulb_activate_2 == false) {
    light_bulb_activate = false;
  }

  else if (light_bulb_activate_1 == true && light_bulb_activate_3 == true && light_bulb_activate_2 == true) {
    light_bulb_activate = true;
  }

}

//проверка влажности
void feuchtigkeitsprufung() {

  level_of_humidity = dht.readHumidity();

  if (extreme_parameters.level_of_humidity <= level_of_humidity) {
    vent_activate = true;
  }

  else if (miserable_parameters.level_of_humidity >= level_of_humidity) {
    vent_activate = false;
  }

  else {
    vent_activate = true;
  }

}


// контроль освещенности
void lichtsteuerung1true() {

  if (light_bulb_activate == true) {
    digitalWrite(LIGHT_BULB_PIN, 1);
  }
  else {
    digitalWrite(LIGHT_BULB_PIN, 0);
  }

}


//контроль влажности
void feuchtigkeitsregelung() {

  if (vent_activate == true || heat_generator_activate == true) {
    digitalWrite(VENT_PIN, 1);
  }

  else {
    digitalWrite(VENT_PIN, 0);
  }

}


//контроль температуры
void temperaturregelung() {

  if (heat_generator_activate == true) {
    digitalWrite(HEAT_GENERATOR_PIN, 1);
  }

  else {
    digitalWrite(HEAT_GENERATOR_PIN, 0);
  }

}


//контроль удобренности
void dungemittelkontrolle() {


  if (water_pump_activate == false) {
    digitalWrite(PUMP_PIN, 0);
  }

  else if (water_pump_activate == true && current_time - timer < 2000) {
    digitalWrite(PUMP_PIN, 1);
  }

  else if (current_time - timer >= 2000) {
     digitalWrite(PUMP_PIN, 0);
  }

}

void transmission() {

  if (current_time - timer == 5000) {
    Serial.print("Температура: ");
    Serial.println(level_of_humidity);
    Serial.print("Влажность воздуха: ");
    Serial.println(illumination_level);
    Serial.print("Освещенность: ");
    Serial.println(temperature_receivings);
    Serial.print("Влажность почвы: ");
    Serial.println(level_of_moisturing);
    Serial.print("-------------------");
    Serial.println();
  }

}



void setup() {
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(LIGHT_BULB_PIN, OUTPUT);
  pinMode(HEAT_GENERATOR_PIN, OUTPUT);
  pinMode(VENT_PIN, OUTPUT);
  pinMode(TEMP_HUMID_SENSOR_PIN, INPUT);
  pinMode(LIGHT_SENSOR_PIN, INPUT);
  pinMode(SOIL_MOISTURE_SENSOR_PIN, INPUT);
  dht.begin();
  einstellung_optimaler_Parameter();
}

void loop() {
  zeit();  // otschet vremeny
  
  temperaturprufung();        // проверка температуры

  uberprufungderBodendungung();     //проверка удобренности

  beleuchtungprufen();    //проверка освещенности 1

  beleuchtungprufenfalsch(); //проверка освещенности 2

  beleuchtungprufenquest(); //проверка освещенности 3

  chekingsomesensors();

  feuchtigkeitsprufung();     //проверка влажности

  lichtsteuerung1true();   ////контроль освещенности

  feuchtigkeitsregelung();    //  контроль влажности

  temperaturregelung(); // контроль temp

  dungemittelkontrolle(); // control' udobreniy

  transmission(); // peredacha infy na komp

}
