#include <EduIntro.h>

#define PIN_DHT 12
#define PIN_SOIL_HUMIDITY 1
#define PIN_LIGHT_SENSOR 0
#define PIN_RELAY_HEATER 4
#define PIN_RELAY_PUMP 5
#define PIN_RELAY_LIGHT 6
#define PIN_RELAY_FAN 7

DHT11 dht11(PIN_DHT);

struct Data_parameters {
  float humidity;
  float temperature;
  int soilhumidity;
  int light;
};

struct Data_parameters Current_Parameters;
struct Data_parameters Min_Parameters = {20, 20, 100, 20};
struct Data_parameters Max_Parameters = {40, 36, 900, 200};
unsigned long pump_work_time = 3*1000;
unsigned long pump_pause_time = 3*1000;
unsigned long data_update_rate = 5*1000;

unsigned long pump_time = 0;
unsigned long data_time = 0;

bool is_cooling_active = false;
bool is_pump_active = false;


void init_parameters() {
  dht11.update();
  Current_Parameters.humidity = dht11.readHumidity();
  Current_Parameters.temperature = dht11.readCelsius();
  Current_Parameters.soilhumidity = 1023 - analogRead(PIN_SOIL_HUMIDITY); //1023 для проверки
  Current_Parameters.light = analogRead(PIN_LIGHT_SENSOR);
}


void light_regulation() {
  if (Current_Parameters.light >= Min_Parameters.light) {
    digitalWrite(PIN_RELAY_LIGHT, LOW);
  } else {
    digitalWrite(PIN_RELAY_LIGHT, HIGH);
  };
}


void soil_humidity_regulation() {
  // Проверка влажности почвы
  if (Current_Parameters.soilhumidity < Min_Parameters.soilhumidity)
    is_pump_active = true;
  else if (Current_Parameters.soilhumidity > Max_Parameters.soilhumidity)
    is_pump_active = false;

  // Подкачка с интервалами
  if (is_pump_active && millis() - pump_time < pump_work_time)
    digitalWrite(PIN_RELAY_PUMP, HIGH);
  else if (is_pump_active && millis() - pump_time >= pump_pause_time + pump_work_time)
    pump_time = millis();
  else
    digitalWrite(PIN_RELAY_PUMP, LOW);
}


void thermoregulation() {
  // При перегреве дополнительно включается вентилятор
  if (Current_Parameters.temperature >= Max_Parameters.temperature) {
    digitalWrite(PIN_RELAY_HEATER, LOW);
    is_cooling_active = true;
  }
  else if (Current_Parameters.temperature < Max_Parameters.temperature)
    digitalWrite(PIN_RELAY_HEATER, HIGH);
  else is_cooling_active = false;
}


void humidity_regulation() {
  if (Current_Parameters.humidity >= Max_Parameters.humidity || is_cooling_active)
    digitalWrite(PIN_RELAY_FAN, HIGH);
  else
    digitalWrite(PIN_RELAY_FAN, LOW);
}


void data_output() {
  if (millis() - data_time > data_update_rate) {
    Serial.print("Температура: ");
    Serial.println(Current_Parameters.temperature);
    Serial.print("Влажность воздуха: ");
    Serial.println(Current_Parameters.humidity);
    Serial.print("Освещенность: ");
    Serial.println(Current_Parameters.light);
    Serial.print("Влажность почвы: ");
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
  init_parameters();
  light_regulation();
  soil_humidity_regulation();
  thermoregulation();
  humidity_regulation();
  data_output();
}