#include <DHT11.h>

#define TEMP_HUM_SENSOR_PIN 12    
#define FAN_PIN 7
#define HEATER_PIN 4
#define LAMP_PIN 6
#define PUMP_PIN 5
#define LIGHT_SENS_PIN 14
#define DIRT_HUMIDITY_1_PIN 17
#define DIRT_HUMIDITY_2_PIN 17
#define DIRT_HUMIDITY_3_PIN 17


DHT11 dht11(TEMP_HUM_SENSOR_PIN);

#define HOUR_LENGTH 3600000UL // 1 hour in millis
#define LIGHT_ON_HOUR 6       // turn light on at 6:00
#define LIGHT_OFF_HOUR 22     // turn light off at 22:00

#define VENT_PERIOD 3600000UL // period of ventilation: 1 hour (3600000 millis)
#define VENT_DURATION 300000UL // duration of ventilation: 5 minutes (300000 millis)

#define WATER_PERIOD 10000UL // period of watering: 30 seconds (30000 millis)
#define WATERING_DURATION 5000UL // duration of watering: 5 seconds (5000 millis)

unsigned long TIME = 0;
int temperature = 0;
int air_humidity = 0;
int dirt_humidity = 0;
int light_value = 0;

bool is_too_hot;
bool is_too_cold;
bool is_dark;
bool is_wet;
bool is_venting;
bool pump_on;

struct ClimateConditions {
  int minLight;
  int minMoisture;
  int tempMin;
  int tempMax;
  int maxHum;
};

ClimateConditions climate = {400, 500, 20, 30, 60};


class Sensor {
public:
  Sensor(int pin);
  int pin;
  int data;
  void get_data();
};


class Sensor_merged : public Sensor {
public:
  Sensor_merged() : Sensor(DIRT_HUMIDITY_1_PIN) {}
  void get_data(Sensor& sens1, Sensor& sens2, Sensor& sens3);
};


class Thermometr {
public:
  int data;
  void get_data();
};


class Air_humidity {
public:
  int data;
  void get_data();
};


class Actuator {
public:
  Actuator(int pin);
  int pin;
  void turn_on();
  void turn_off();
};


class Fan : public Actuator{
public:
  Fan(int pin);
  bool is_wet = false;
  bool is_venting = false;
  bool is_too_hot = false;
  void power();
};


class Pump : public Actuator{
public:
  Pump(int pin);
  bool pump_on;
  void power();
};


class Lamp : public Actuator{
public:
  Lamp(int pin);
  bool is_dark = false;
  void power();
};


class Heater : public Actuator{
public:
  Heater(int pin);
  bool is_too_cold = false;
  void power();
};


Sensor::Sensor(int pin) : pin{pin} {
  pinMode(pin, INPUT);
}


void Sensor::get_data() {
  data = analogRead(pin);
}


void Sensor_merged::get_data(Sensor& sens1, Sensor& sens2, Sensor& sens3) {
  sens1.get_data();
  sens2.get_data();
  sens3.get_data();
  data = (sens1.data + sens2.data + sens3.data) / 3;
}


void Thermometr::get_data() {
  int result = dht11.readTemperature();

  if (result == DHT11::ERROR_CHECKSUM || result == DHT11::ERROR_TIMEOUT) {
    Serial.println(DHT11::getErrorString(result));
    result = data;
  }

  data = result;
}


void Air_humidity::get_data() {
  int result = dht11.readHumidity();

  if (result == DHT11::ERROR_CHECKSUM || result == DHT11::ERROR_TIMEOUT) {
    Serial.println(DHT11::getErrorString(result));
    result = data;
  }

  data = result;
}


Actuator::Actuator(int pin) : pin{pin} {
  pinMode(pin, OUTPUT);
}


void Actuator::turn_on() {
  digitalWrite(pin, HIGH);
}


void Actuator::turn_off() {
  digitalWrite(pin, LOW);
}


Fan::Fan(int pin) : Actuator(pin) {}
Heater::Heater(int pin) : Actuator(pin) {}
Pump::Pump(int pin) : Actuator(pin) {}
Lamp::Lamp(int pin) : Actuator(pin) {}


void Fan::power() {
  if(is_venting || is_too_hot || is_wet) turn_on();
  else turn_off();
}


void Lamp::power() {
  if (is_dark) turn_on();
  else turn_off();
}


void Pump::power() {
  if (pump_on) turn_on();
  else turn_off();
}


void Heater::power() {
  if (is_too_cold) turn_on();
  else turn_off();
}


int get_air_humidity() {
  int result = dht11.readHumidity();

  if (result == DHT11::ERROR_CHECKSUM || result == DHT11::ERROR_TIMEOUT) {
    Serial.println(DHT11::getErrorString(result));
    result = air_humidity;
  }

  return result;
}


Fan fan(FAN_PIN);
Heater heater(HEATER_PIN);
Pump pump(PUMP_PIN);
Lamp lamp(LAMP_PIN);

Thermometr thermometr;
Air_humidity air_humidity_sens;
Sensor light_sen(LIGHT_SENS_PIN);
Sensor dirt_humidity_sens_1(DIRT_HUMIDITY_1_PIN);
Sensor dirt_humidity_sens_2(DIRT_HUMIDITY_2_PIN);
Sensor dirt_humidity_sens_3(DIRT_HUMIDITY_3_PIN);
Sensor_merged dirt_humidity_merged;


void control_temperature(Thermometr& term, Heater& heater, Fan& fan) {
  if (term.data > climate.tempMax) {
    fan.is_too_hot = true;
    heater.is_too_cold = false;
  }
  else if (term.data < climate.tempMin) {
    fan.is_too_hot = false;
    heater.is_too_cold = true;
  }
  else {
    fan.is_too_hot = false;
    heater.is_too_cold = false;
  }
}


void control_light(Sensor& light_sen, Lamp& lamp) {
  if (light_sen.data > climate.minLight) lamp.is_dark = true;
  else lamp.is_dark = false;
}


void control_air_humidity(Air_humidity& sens, Fan& fan){
  if (sens.data > climate.maxHum) {
    fan.is_wet = true;
  } else {
    fan.is_wet = false;
  }
}


void ventilation_by_time(Fan& fan){
  static unsigned long lastVentTime = 0;
  static unsigned long startVentTime = 0;
  static bool venting = false;

  if ( !venting && TIME - lastVentTime >= VENT_PERIOD ) {
    venting = true;
    startVentTime = TIME;
  }

  if ( venting ) {
    if (TIME - startVentTime < VENT_DURATION) {
      fan.is_venting = true;
    } else {
      fan.is_venting = false;
      venting = false;
      lastVentTime = TIME;
    }
  } else {
    fan.is_venting = false;
  }
}



void control_dirt_humidity(Pump& pump, Sensor& sens) {
  static unsigned long lastWaterTime = 0;
  static unsigned long startWaterTime = 0;
  static bool watering = false;

  if ( !watering && sens.data < climate.minMoisture && TIME - lastWaterTime >= WATER_PERIOD) {
    watering = true;
    startWaterTime = TIME;
  }

  if ( watering ) {
    if (TIME - startWaterTime < WATERING_DURATION) {
      pump.pump_on = true;
    } else {
      pump.pump_on = false;
      watering = false; // end of watering
      lastWaterTime = TIME;
    }
  } else {
    pump.pump_on = false;
  }
}


void setup() {
  Serial.begin(9600);
}
 

void loop() {
  TIME = millis()*1000;
  thermometr.get_data();
  air_humidity_sens.get_data();
  light_sen.get_data();
  dirt_humidity_sens_1.get_data();
  //dirt_humidity_sens_2.get_data();
  //dirt_humidity_sens_3.get_data();
  dirt_humidity_merged.get_data(dirt_humidity_sens_1, dirt_humidity_sens_2, dirt_humidity_sens_3);


  control_temperature(thermometr, heater, fan);
  control_light(light_sen, lamp);
  control_air_humidity(air_humidity_sens, fan);
  ventilation_by_time(fan);
  // control_dirt_humidity(pump, dirt_humidity_sens_1);
  control_dirt_humidity(pump, dirt_humidity_merged);


  fan.power();
  heater.power();
  lamp.power();
  pump.power();
}
