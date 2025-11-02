#include <DHT11.h>


#define TEMP_HUM_SENSOR_PIN 12
#define LAMP_PIN            6
#define LIGHT_SEN_PIN       A0
#define DIRT_HUMIDITY_PIN   A3
#define HEATER_PIN          4
#define FAN_PIN             7


DHT11 dht11(TEMP_HUM_SENSOR_PIN);


#define HOUR_LENGTH 3600000UL   // 1 hour in millis
#define LIGHT_ON_HOUR 6         // turn light on at 6:00
#define LIGHT_OFF_HOUR 22       // turn light off at 22:00


unsigned long TIME = 0;


struct Conditions {
  int minLight;
  int minMoisture;
  int tempMin;
  int tempMax;
  int maxHum;
};


Conditions climate = {400, 500, 20, 30, 60};


class Fan {
  public:
    void power();
    void set_pin(int pin);
    void set_on_heater(bool data);
    void set_on_timetable(bool data);
    void set_on_hot(bool data);
    void set_on_humidity(bool data);
  private:
    int pin;
    bool on_heater;
    bool on_timetable;
    bool on_hot;
    bool on_humidity;
};


class Heater {
  public:
    void power();
    void set_on_temperature(bool data);
    void set_pin(int pin);
  private:
    int pin;
    bool on_temperature;
};


class Lamp {
  public:
    void power();
    void set_pin(int pin);
    void set_on_lamp(bool data);
  private:
    int pin;
    bool on_lamp;
};


class Pump {
  public:
    void power();
    void set_pin(int pin);
    void set_on_pump(bool data);
  private:
    int pin;
    bool on_pump;
};


class Thermometer_AirHumidity {
  public:
    int get_temperature();
    int get_humidity();
  private:
};


class Light_Sen {
  public:
    void set_pin(int pin);
    int get_light_value();
  private:
    int pin;
    int light_value;
};


class Dirt_Humidity {
  public:
    int get_data();
    void set_pin(int pin);
  private:
    int pin;
    int humidity;
};



void Fan::power() {
  if( on_heater || on_timetable || on_hot || on_humidity ) {
    digitalWrite(pin, HIGH);
  } else {
    digitalWrite(pin, LOW);
  }
}


void Fan::set_pin(int pin) {
  pin = pin;
  pinMode(pin, OUTPUT);
}


void Fan::set_on_heater(bool data) {
  on_heater = data;
}


void Fan::set_on_timetable(bool data) {
  on_timetable = data;
}


void Fan::set_on_hot(bool data) {
  on_hot = data;
}


void Fan::set_on_humidity(bool data) {
  on_humidity = data;
}


void Heater::power() {
  if ( on_temperature ) {
    digitalWrite(pin, HIGH);
  } else {
    digitalWrite(pin, LOW);
  }
}


void Heater::set_pin(int pin) {
  pin = pin;
  pinMode(pin, OUTPUT);
}


void Heater::set_on_temperature(bool data) {
  on_temperature = data;
}


void Lamp::power() {
  if ( on_lamp ) {
    analogWrite(pin, 255);
  } else {
    analogWrite(pin, 0);
  }
}


void Lamp::set_pin(int pin) {
  pin = pin;
  pinMode(pin, OUTPUT);
}


void Lamp::set_on_lamp(bool data) {
  on_lamp = data;
}


void Pump::power() {
  if ( on_pump ) {
    analogWrite(pin, 255);
  } else {
    analogWrite(pin, 0);
  }
}


void Pump::set_pin(int pin) {
  pin = pin;
  pinMode(pin, OUTPUT);
}


void Pump::set_on_pump(bool data) {
  on_pump = data;
}


int Thermometer_AirHumidity::get_temperature() {
  return dht11.readTemperature();
}


int Thermometer_AirHumidity::get_humidity() {
  return dht11.readHumidity();
}


void Light_Sen::set_pin(int pin) {
  pin = pin;
  pinMode(pin, INPUT);
}


int Light_Sen::get_light_value() {
  light_value = analogRead(pin);
  return light_value;
}


void Dirt_Humidity::set_pin(int pin) {
  pin = pin;
  pinMode(pin, INPUT);
}


int Dirt_Humidity::get_data() {
  humidity = analogRead(pin);
  return humidity;
}




void control_temperature (Thermometer_AirHumidity &thermometer, Fan &fan, Heater &heater) {
  if (thermometer.get_temperature() > climate.tempMax) {
    fan.set_on_hot(true);
    fan.set_on_heater(false);
    heater.set_on_temperature(false);
  } else if (thermometer.get_temperature() < climate.tempMin) {
    fan.set_on_hot(false);
    fan.set_on_heater(true);
    heater.set_on_temperature(true);
  } else {
    heater.set_on_temperature(false);
  }
}


void control_light (Light_Sen &lightSen, Lamp &lamp) {
  if ( TIME / HOUR_LENGTH > LIGHT_ON_HOUR && TIME / HOUR_LENGTH < LIGHT_OFF_HOUR ) {
    if ( lightSen.get_light_value() < climate.minLight) {
      lamp.set_on_lamp(true);
    } else {
      lamp.set_on_lamp(false);
    }
  } else {
    lamp.set_on_lamp(false);
  }
}


void control_air_humidity (Fan &fan, Thermometer_AirHumidity &thermometer) {
  if ( thermometer.get_humidity() > climate.maxHum ) {
    fan.set_on_humidity(true);
  } else {
    fan.set_on_humidity(false);
  }
}


void control_dirt_humidity(Pump &pump, Dirt_Humidity &dirt_humidity) {
  if (dirt_humidity.get_data() < climate.minMoisture) {
    pump.set_on_pump(true);
  } else {
    pump.set_on_pump(false);
  }
}


void multi_control(Thermometer_AirHumidity &thermometer, Fan &fan, Heater &heater, Light_Sen &lightSen, Lamp &lamp, Pump &pump, Dirt_Humidity &dirt_humidity ) {
  control_light(lightSen, lamp);
  control_air_humidity(fan, thermometer);
  control_temperature(thermometer, fan, heater);
  control_dirt_humidity(pump, dirt_humidity);
}


void multi_power( Fan &fan, Heater &heater, Lamp &lamp, Pump &pump ){
  fan.power();
  heater.power();
  lamp.power();
  pump.power();
}


Fan fan;
Heater heater;
Thermometer_AirHumidity thermometer;
Lamp lamp;
Pump pump;
Light_Sen lightSen;
Dirt_Humidity dirt_humidity;


void setup() {
  fan.set_pin(FAN_PIN);
  heater.set_pin(HEATER_PIN);
  lamp.set_pin(LAMP_PIN);
  lightSen.set_pin(LIGHT_SEN_PIN);
  dirt_humidity.set_pin(DIRT_HUMIDITY_PIN);
}


void loop() {
  TIME = millis();

  multi_control(thermometer, fan, heater, lightSen, lamp, pump, dirt_humidity);

  multi_power(fan, heater, lamp, pump);

  delay(10);
}
