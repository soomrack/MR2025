#include "DHT.h"

#define PUMP 5
#define LIGHT 6
#define HEAT 4
#define VENT 7

#define TEMP_HUMID_SENSOR 12
#define LIGHT_SENSOR 0
#define SOIL_MOISTURE_SENSOR 3


int air_humidity;
int temperature;    //Переменные для показаний датчиков
int soil_moisture;
int lighting;

bool light_on;
bool heat_on;     //Флаги (== true - значит нужно включить соответствующий актуатор)
bool vent_on;
bool pump_on;

unsigned long last_time = 0;
unsigned long current_time; 
const unsigned long day_time = 10*1000;       //Переменные для условной реализации отсчета времени (чтобы ночью свет не включался)
const unsigned long night_time = 10*1000;
bool night;

unsigned long last_serial_time = 0;             //Для реализации вывода данных в серийный порт раз в 5 секунд
const unsigned long serial_interval = 5000;

struct Values{
  int temperature;
  int air_humidity;
  int soil_moisture;
  int light;
};

struct Values minimal;
struct Values maximum;    //Минимальные и максимальные допустимые параметры климата

DHT dht(TEMP_HUMID_SENSOR, DHT11);

void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(PUMP, OUTPUT);
  pinMode(LIGHT, OUTPUT);
  pinMode(HEAT, OUTPUT);
  pinMode(VENT, OUTPUT);
  initialization();
}

void read_sensors(){
  air_humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  soil_moisture = analogRead(SOIL_MOISTURE_SENSOR);
  lighting = analogRead(LIGHT_SENSOR);
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


void check_light(const int lighting){
  if (night == false && lighting >= minimal.light){
    light_on = true;
  }
  else{
    light_on = false;
  }
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


void check_soil_moisture(const int humidity){ //Сейчас при замкнутых контактах включается насос
  if (soil_moisture < minimal.soil_moisture){
    pump_on = true;
  }
  else if (soil_moisture >= maximum.soil_moisture){
    pump_on = false;
  }
}


void light_control(const bool light_on){
  if (light_on == true){
    digitalWrite(LIGHT , HIGH);
  }
  else{
    digitalWrite(LIGHT , LOW);
  }
}
//Добавить три датчика освещенности

void heat_control(const bool heat_on){
  if (heat_on == true){
    digitalWrite(HEAT , HIGH);
  }
  else{
    digitalWrite(HEAT , LOW);
  }
}


void vent_control(const bool vent_on, const bool heat_on){
  if (vent_on == true || heat_on == true){
    digitalWrite(VENT , HIGH);
  }
  else{
    digitalWrite(VENT , LOW);
  }
}


void pump_control(const bool pump_on){
  if (pump_on == true){
    digitalWrite(PUMP , HIGH);
  }
  else{
    digitalWrite(PUMP , LOW);
  }
}


void initialization(){
  minimal.temperature = 29;
  minimal.air_humidity = 20;
  minimal.soil_moisture = 900; //1020 - значение при разомкнутых контактах (абсолютно сухая почва)
  minimal.light = 200; //Значение минимальной освещенности (фонарик)

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
  Serial.print("Освещенность: ");
  Serial.println(lighting);
  Serial.print("Влажность почвы: ");
  Serial.println(soil_moisture);
  Serial.print("-------------------");
  Serial.println();
}


void loop(){
  air_humidity = dht.readHumidity();
  temperature = dht.readTemperature();
  soil_moisture = analogRead(SOIL_MOISTURE_SENSOR);
  lighting = analogRead(LIGHT_SENSOR);
  clock();
  check_light(lighting);
  check_temperature(temperature);
  check_air_humidity(air_humidity);
  check_soil_moisture(soil_moisture);
  light_control(light_on);
  heat_control(heat_on);
  vent_control(vent_on, heat_on);
  pump_control(pump_on);

  if (current_time - last_serial_time >= serial_interval) {
    serial_output();
    last_serial_time = current_time;
  }
}
