#include <EduIntro.h>

#define PIN_DHT 12
#define PIN_SOIL_HUMIDITY 3
#define PIN_LIGHT_SENSOR 0
#define PIN_RELAY_HEATER 4
#define PIN_RELAY_PUMP 5
#define PIN_RELAY_LIGHT 6
#define PIN_RELAY_FAN 7

DHT11 dht11(PIN_DHT);

class Thermometer {
public:
  int temperature;
public:
  void read();
  void error_read();
};

Thermometer thermometer_1;
Thermometer thermometer_2;
Thermometer thermometer_3;
Thermometer thermometer_4;
Thermometer thermometer_5;

class Thermometer_merge : Thermometer {
public:
  int themperatures[5];
public:
  void Get_data_merge ();
  void Get_massive (Thermometer therm1, Thermometer therm2, Thermometer therm3, Thermometer therm4, Thermometer therm5);
};

Thermometer_merge thermometer_merge;


struct Data_parameters {
  int humidity;
  float temperature;
  int soilhumidity;
  int light;
};

struct Data_parameters Current_Parameters;
struct Data_parameters Min_Parameters = {20, 20, 50, 100};
struct Data_parameters Max_Parameters = {40, 36, 950, 1023};
unsigned long pump_work_time = 3*1000;
unsigned long pump_pause_time = 3*1000;
unsigned long ventilation_lenght_time = 5*60*1000;
unsigned long ventilation_pause_time = 60*60*1000;
unsigned long data_update_rate = 5*1000;

unsigned long pump_time = 0;
unsigned long data_time = 0;
unsigned long ventilation_time = 0;

bool is_light_active = false;
bool is_pump_active = false;
bool is_cooling_active = false;
bool is_heater_active = false;
bool is_stuffy_active = false;
bool is_ventilation_active = false;


int Light_sensor_read() {
  Current_Parameters.light = analogRead(PIN_LIGHT_SENSOR);
  return (Current_Parameters.light);
}


int Soil_humidity_sensor_read() {
  Current_Parameters.soilhumidity = analogRead(PIN_SOIL_HUMIDITY);
  return (Current_Parameters.soilhumidity);
}


void Thermometer::read() {
  dht11.update();
  temperature = dht11.readCelsius() + random(-3, 3);
}


void Thermometer::error_read() {
  temperature = random(100);
}


void Air_humidity_sensor_read() {
  dht11.update();
  Current_Parameters.humidity = dht11.readHumidity();
  return (Current_Parameters.humidity);
}


void Light_regulation() {
  if (Current_Parameters.light >= Min_Parameters.light) {
    is_light_active = true;
  } else {
    is_light_active = false;
  };
}


int Soil_humidity_regulation() {
  if (Current_Parameters.soilhumidity < Min_Parameters.soilhumidity)
    is_pump_active = true;
  else if (Current_Parameters.soilhumidity > Max_Parameters.soilhumidity)
    is_pump_active = false;
}


void Thermometer_merge::Get_massive (Thermometer therm1, Thermometer therm2, Thermometer therm3, Thermometer therm4, Thermometer therm5) {
  themperatures[0] = therm1.temperature;
  themperatures[1] = therm2.temperature;
  themperatures[2] = therm3.temperature;
  themperatures[3] = therm4.temperature;
  themperatures[4] = therm5.temperature;
}


void Thermometer_merge::Get_data_merge () {

  for (int i=0; i < 5; i++) {
    temperature += themperatures[i];
  }

  float error_avarage = temperature/5;
  int max_error = 0;
  bool is_error_positive = true;

  // Вычисление ошибочного датчика
  for (int i=0; i < 5; i++) {
    if (abs(themperatures[i] - error_avarage) > max_error) {
      max_error = abs(themperatures[i] - error_avarage);
      if (themperatures[i] - error_avarage > 0) 
        is_error_positive = true;
      else
        is_error_positive = false;
    }
  }

  //Убераем ошибочный 
  if (is_error_positive) temperature -= max_error;
  else temperature += max_error;

  //Вычисление среднего арифметического
  temperature /=4;
}


int Termoregulation(Thermometer& therm) {
  if (therm.temperature >= Max_Parameters.temperature) {
    is_heater_active = false;
    is_cooling_active = true;
  }
  else if (therm.temperature < Max_Parameters.temperature)
    is_heater_active = true;
  else is_cooling_active = false;
}


int Air_humidity_regulation() {
  if (Current_Parameters.humidity >= Max_Parameters.humidity)
    is_stuffy_active = true;
  else
    is_stuffy_active = false;
}


void Ventilation_cycle() {
  if (millis() - ventilation_time < ventilation_lenght_time)
    is_ventilation_active = true;
  else if (millis() - ventilation_time >= ventilation_lenght_time + ventilation_pause_time) {
    is_ventilation_active = false;
    ventilation_time = millis();
  }
}


void Light_contorl() {
  if (is_light_active) {
    digitalWrite(PIN_RELAY_LIGHT, LOW);
  } else {
    digitalWrite(PIN_RELAY_LIGHT, HIGH);
  };
}


void Pump_control() {
  if (is_pump_active && millis() - pump_time < pump_work_time)
    digitalWrite(PIN_RELAY_PUMP, HIGH);
  else if (is_pump_active && millis() - pump_time >= pump_pause_time + pump_work_time)
    pump_time = millis();
  else
    digitalWrite(PIN_RELAY_PUMP, LOW);
}


void Heater_control() {
  if (is_heater_active) {
    digitalWrite(PIN_RELAY_LIGHT, LOW);
  } else {
    digitalWrite(PIN_RELAY_LIGHT, HIGH);
  };
}


void Fan_control() {
  if (is_stuffy_active || is_cooling_active || is_ventilation_active)
    digitalWrite(PIN_RELAY_FAN, HIGH);
  else
    digitalWrite(PIN_RELAY_FAN, LOW);
}


void data_output() {
  if (millis() - data_time > data_update_rate) {
    Serial.print("Temperature: ");
    Serial.println(Current_Parameters.temperature);
    Serial.print("Air humidity: ");
    Serial.println(Current_Parameters.humidity);
    Serial.print("lightning level: ");
    Serial.println(Current_Parameters.light);
    Serial.print("Soil humidity: ");
    Serial.println(Current_Parameters.soilhumidity);
    Serial.println("-------------------");
    data_time = millis();
  }
}


void setup()
{
  Serial.begin(9600);
  pinMode(PIN_DHT, INPUT);
  pinMode(PIN_SOIL_HUMIDITY, INPUT);
  pinMode(PIN_LIGHT_SENSOR, INPUT);
  pinMode(PIN_RELAY_HEATER, OUTPUT);
  pinMode(PIN_RELAY_PUMP, OUTPUT);
  pinMode(PIN_RELAY_LIGHT, OUTPUT);
  pinMode(PIN_RELAY_FAN, OUTPUT);
}


void loop()
{
  Light_sensor_read();
  Soil_humidity_sensor_read();
  thermometer_1.read();
  thermometer_2.read();
  thermometer_3.read();
  thermometer_4.read();
  thermometer_5.error_read();
  Air_humidity_sensor_read();
  thermometer_merge.Get_massive(thermometer_1, thermometer_2, thermometer_3, thermometer_4, thermometer_5);
  thermometer_merge.Get_data_merge();

  Light_regulation();
  Soil_humidity_regulation();
  Termoregulation(thermometer_1);
  Air_humidity_regulation();
  Ventilation_cycle();

  Light_contorl();
  Pump_control();
  Heater_control();
  Fan_control();

  data_output();
}