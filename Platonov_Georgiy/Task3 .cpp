#include "DHT.h"

#define PUMP_PIN 5 
#define LIGHT_PIN 6
#define HEAT_PIN 4
#define VENT_PIN 7

#define TEMP_HUMID_SENSOR_PIN 12
#define SOIL_MOISTURE_SENSOR_PIN 3

const int light_sensors_count = 3;
const int LIGHT_SENSORS_PINS[light_sensors_count] = {0, 1, 2}; 

// Переменные для показаний датчиков
int air_humidity;
int temperature;
int soil_moisture;
int lighting[light_sensors_count];


bool light_on;
bool heat_on;
bool vent_on;

bool moisture_low;
bool pump_on;

unsigned long last_time = 0;
unsigned long current_time; 
const unsigned long day_time = 10*1000;
const unsigned long night_time = 10*1000;
bool night;

unsigned long last_serial_time = 0;
const unsigned long serial_interval = 5000;

unsigned long start_watering_time = 0;
unsigned long stop_watering_time = 0;   
const unsigned long watering_duration = 2000;
const unsigned long watering_delay = 2000;
bool watering_in_process = false;
bool pump_active;

struct Values{
  int temperature;
  int air_humidity;
  int soil_moisture;
  int light;
};

struct Values minimal;
struct Values maximum;

DHT dht(TEMP_HUMID_SENSOR_PIN, DHT11);

void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(HEAT_PIN, OUTPUT);
  pinMode(VENT_PIN, OUTPUT);
  initialization();
}


void clock(){
  current_time = millis();
  if (current_time - last_time < day_time){
    night = false;
  }
  else if (current_time - last_time > day_time && current_time - last_time < night_time + day_time){
    night = true;
  }
  else if (current_time - last_time > night_time){
    night = false;
    last_time = current_time;
  }
}


void readLightSensors() {
  for (int i = 0; i < light_sensors_count; i++) {
    lighting[i] = analogRead(LIGHT_SENSORS_PINS[i]);
  }
}


bool decide_light_control() { //Решение принимается по большинству
  int low_light_count = 0;
  for (int i = 0; i < light_sensors_count; i++) {
    if (lighting[i] >= minimal.light) {
      low_light_count = low_light_count + 1;
    }
  }

  int majority_threshold = light_sensors_count / 2 + 1;
  return (low_light_count >= majority_threshold && night == false);
}


void check_light(){
  light_on = decide_light_control();
}


void check_temperature(const int temperature){
  if (temperature < minimal.temperature){
    heat_on = true;
  }
  else if (temperature >= maximum.temperature){
    heat_on = false;
  }
}


void check_air_humidity(const int air_humidity){
  if (air_humidity > maximum.air_humidity){
    vent_on = true;
  }
  else if (air_humidity <= minimal.air_humidity){
    vent_on = false;
  }
}


void check_soil_moisture(const int humidity){
  if (soil_moisture < minimal.soil_moisture){
    moisture_low = true;
  }
  else if (soil_moisture >= maximum.soil_moisture){
    moisture_low = false;
  }
}


void light_control(const bool light_on){
  if (light_on == true){
    digitalWrite(LIGHT_PIN , HIGH);
  }
  else{
    digitalWrite(LIGHT_PIN , LOW);
  }
}


void heat_control(const bool heat_on){
  if (heat_on == true){
    digitalWrite(HEAT_PIN , HIGH);
  }
  else{
    digitalWrite(HEAT_PIN , LOW);
  }
}


void vent_control(const bool vent_on, const bool heat_on){
  if (vent_on == true || heat_on == true){
    digitalWrite(VENT_PIN , HIGH);
  }
  else{
    digitalWrite(VENT_PIN , LOW);
  }
}
void decide_pump_on(const bool moisture_low){
   if (moisture_low == false){  //Полив не нужен
    pump_on = false;
    watering_in_process = false;
  }
  else {
    if (watering_in_process == true && pump_active == true){  //Полив в процессе, работает насос
      if (current_time - start_watering_time < watering_duration){  //Период полива
        pump_on = true;
      }
      else{
        pump_on = false;  //Переход в режим ожидания
        pump_active = false;
        stop_watering_time = current_time;
      }
    }
    else if (watering_in_process == true && pump_active == false){  //Режим ожидания
      if (current_time - stop_watering_time < watering_delay){
          pump_on = false;
      }
      else{
          watering_in_process = false;  //Почва была влажной, стала сухой
      }
    }
    else if (watering_in_process == false){
      pump_active = true;
      start_watering_time = current_time;
      watering_in_process = true;
    }
  }
}


void new_pump_control_2(const bool pump_on){
    if (pump_on == true){
    digitalWrite(PUMP_PIN , HIGH);
  }
  else{
    digitalWrite(PUMP_PIN , LOW);
  }
}


void initialization(){
  minimal.temperature = 29;
  minimal.air_humidity = 20;
  minimal.soil_moisture = 900;  //1020 - значение при разомкнутых контактах (абсолютно сухая почва)
  minimal.light = 200;  //Значение минимальной освещенности (фонарик)

  maximum.temperature = 30;
  maximum.air_humidity = 40;
  maximum.soil_moisture = 10; //Значение абсолютно влажной почвы (контакты замкнуты)
  maximum.light = 20; //Значение максимальной освещенности (комнатный свет)
}


void serial_output(){
  Serial.print("Температура: ");
  Serial.println(temperature);
  Serial.print("Влажность воздуха: ");
  Serial.println(air_humidity);
  
  // Вывод показаний всех датчиков освещенности
  for (int i = 0; i < light_sensors_count; i++) {
    Serial.print("Освещенность ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(lighting[i]);
  }
  
  Serial.print("Влажность почвы: ");
  Serial.println(soil_moisture);
  Serial.print("-------------------");
  Serial.println();
}


void loop(){
  air_humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  soil_moisture = analogRead(SOIL_MOISTURE_SENSOR_PIN);
  
  readLightSensors();

  clock();
  check_light();
  check_temperature(temperature);
  check_air_humidity(air_humidity);
  check_soil_moisture(soil_moisture);

  light_control(light_on);
  heat_control(heat_on);
  vent_control(vent_on, heat_on);

  decide_pump_on(moisture_low);
  new_pump_control_2(pump_on);

  if (current_time - last_serial_time >= serial_interval) {
    serial_output();
    last_serial_time = current_time;
  }
}