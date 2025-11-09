#define THERMOMETER_PIN 9
#define DIRT_HUMIDITY_SENSOR_PIN 10
#define AIR_HUMIDITY_SENSOR_PIN 11
#define LIGHT_SENSOR_PIN A0 //done
#define FAN_PIN 7 //done
#define HEATER_PIN 5
#define LIGHT_PIN 6
#define PUMP_PIN 5 //done

#define MAX_TEMPERATURE 40
#define MIN_TEMPERATURE 20
#define MAX_LUMINOSITY 700
#define MIN_LUMINOSITY 500
#define MAX_AIR_HUMIDITY 600
#define MIN_AIR_HUMIDITY 400
#define MAX_DIRT_HUMIDITY 800
#define MIN_DIRT_HUMIDITY 600

typedef int celsius;

class Thermometer{
private: 
  int pin;
public:
  celsius temperature;
  Thermometer(int new_pin);
  int get_data();
};

Thermometer::Thermometer(int new_pin){
  pin = new_pin;
}

int Thermometer::get_data(){
  temperature = 10;
}

class Humidity_sensor{
  private:
    int pin;
  public:
    int humidity;
    Humidity_sensor(int new_pin);
    int get_data();
};

Humidity_sensor::Humidity_sensor(int new_pin){
  pin = new_pin;
}

int Humidity_sensor::get_data(){
  humidity = 10;
}

class Light_sensor{
  private:
    int pin;
  public:
    int luminosity;
    Light_sensor(int new_pin);
    int get_data();
};

Light_sensor::Light_sensor(int new_pin){
  pin = new_pin;
}

int Light_sensor::get_data(){
  luminosity = 10;
}

//Actuators
class Fan{
  private:
    int pin;
  public:
    bool on_heater;
    bool on_timetable;
    bool on_hot;
    bool on_humidity;

    Fan(int new_pin);
    void power();
};

Fan::Fan(int new_pin){
  pin = new_pin;
}

void Fan::power(){
  if (on_heater || on_timetable || on_hot || on_humidity){
    digitalWrite(pin, HIGH);
  } else {
    digitalWrite(pin, LOW);
  }
}


class Heater{
  private:
    int pin;
  public:
    bool on;

    Heater(int new_pin);
    void power();
};

Heater::Heater(int new_pin){
  pin = new_pin;
}

void Heater::power(){
  if (on){
    digitalWrite(pin, HIGH);
  } else {
    digitalWrite(pin, LOW);
  }
}


class Light{
  private:
    int pin;
  public:
    bool on;

    Light(int new_pin);
    void power();
};

Light::Light(int new_pin){
  pin = new_pin;
}

void Light::power(){
  if (on){
    digitalWrite(pin, HIGH);
  } else {
    digitalWrite(pin, LOW);
  }
}


class Pump{
  private:
    int pin;
  public:
    bool on;

    Pump(int new_pin);
    void power();
};

Pump::Pump(int new_pin){
  pin = new_pin;
}

void Pump::power(){
  if (on){
    digitalWrite(pin, HIGH);
    delay(5000);
    digitalWrite(pin, LOW);
  } else {
    digitalWrite(pin, LOW);
  }
}

void control_temperature(Thermometer thermometer, Fan &fan, Heater &heater){
  if (thermometer.temperature > MAX_TEMPERATURE){
    fan.on_hot = true;
  }
  if (thermometer.temperature < MIN_TEMPERATURE){
    fan.on_hot = false;
    fan.on_heater = true;
    heater.on = true;
  }
}

void control_luminosity(Light_sensor light_sensor, Light &light){
  if (light_sensor.luminosity > MAX_LUMINOSITY){
    light.on = true;
  }
  if (light_sensor.luminosity < MIN_LUMINOSITY){
    light.on = false;
  }
}

void control_air_humidity(Humidity_sensor air_hum_sensor, Fan &fan){
  if (air_hum_sensor.humidity > MAX_AIR_HUMIDITY){
    fan.on_humidity = true;
  }
  if (air_hum_sensor.humidity < MIN_AIR_HUMIDITY){
    fan.on_humidity = false;
  }
}

void control_dirt_humidity(Humidity_sensor dirt_hum_sensor, Pump &pump){
  int time;
  if (dirt_hum_sensor.humidity > MAX_DIRT_HUMIDITY){
    pump.on = false;
  }
  if (dirt_hum_sensor.humidity < MIN_DIRT_HUMIDITY){
    pump.on = true;
  }
}

int mins(){
  return millis()/1000/60;
}

void ventilation_by_time(Fan &fan){
  int minutes = mins()%(24*60);
  if ((minutes>(6*60) && minutes<(6*60+10)) || (minutes>(18*60) && minutes<(18*60+10))){
    fan.on_timetable = true;
  } else{
    fan.on_timetable = false;
  }
}

void log(Thermometer thermometer, Light_sensor light_sensor, Humidity_sensor air_humidity_sensor, Humidity_sensor dirt_humidity_sensor, Fan fan, Heater heater, Light light, Pump pump){
  Serial.print("temperature: ");
  Serial.print(thermometer.temperature);
  Serial.print(" luminosity: ");
  Serial.print(light_sensor.luminosity);
  Serial.print(" air_humidity: ");
  Serial.print(air_humidity_sensor.humidity);
  Serial.print(" dirt_humidity: ");
  Serial.println(dirt_humidity_sensor.humidity);
}

void setup() {
  Serial.begin(9600);
}

Thermometer thermometer = Thermometer(THERMOMETER_PIN);
Light_sensor light_sensor = Light_sensor(LIGHT_SENSOR_PIN);
Humidity_sensor air_humidity_sensor = Humidity_sensor(AIR_HUMIDITY_SENSOR_PIN);
Humidity_sensor dirt_humidity_sensor = Humidity_sensor(DIRT_HUMIDITY_SENSOR_PIN);

Fan fan = Fan(FAN_PIN);
Heater heater = Heater(HEATER_PIN);
Light light = Light(LIGHT_PIN);
Pump pump = Pump(PUMP_PIN);

void loop() {
  thermometer.get_data();
  light_sensor.get_data();
  air_humidity_sensor.get_data();
  dirt_humidity_sensor.get_data();

  log(thermometer, light_sensor, air_humidity_sensor, dirt_humidity_sensor, fan, heater, light, pump);

  control_temperature(thermometer, fan, heater);
  control_luminosity(light_sensor, light);
  control_air_humidity(air_humidity_sensor, fan);
  control_dirt_humidity(dirt_humidity_sensor, pump);
  ventilation_by_time(fan);

  fan.power();
  heater.power();
  light.power();
  pump.power();
  delay(10);
}
