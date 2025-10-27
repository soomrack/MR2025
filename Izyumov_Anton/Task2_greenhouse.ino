#include <DHT11.h>


#define TEMP_HUM_SENSOR_PIN 12     // temperature and humidity sensor


DHT11 dht11(TEMP_HUM_SENSOR_PIN);


#define HOUR_LENGTH 3600000UL // 1 hour in millis
#define LIGHT_ON_HOUR 6       // turn light on at 6:00
#define LIGHT_OFF_HOUR 22     // turn light off at 22:00


#define VENT_PERIOD 3600000UL // period of ventilation: 1 hour (3600000 millis)
#define VENT_DURATION 300000UL // duration of ventilation: 5 minutes (300000 millis)


#define WATER_PERIOD 10000UL // period of watering: 30 seconds (30000 millis)
#define WATERING_DURATION 5000UL // duration of watering: 5 seconds (5000 millis)


unsigned long TIME = 0;


struct ClimateConditions {
  int minLight;
  int minMoisture;
  int tempMin;
  int tempMax;
  int maxHum;
};


ClimateConditions climate = {400, 500, 20, 30, 60};


class Fan {
  public:
    const int pin = 7;
    bool on_heater;
    bool on_timetable;
    bool on_hot;
    bool on_humidity;
  public:
    void power();
};


class Heater {
  public:
    const int pin = 4;
    bool on_temperature;
  public:
    void power();
};


class Lamp {
  public:
    const int pin = 6;
    bool on_lamp;
  public:
    void power();
};


class Pump {
  public:
    const int pin = 5;
    bool on_pump;
  public:
    void power();
};


class Thermometer {
  public:
    int temperature;   // Celsius
  public:
    void get_data();
};


class Thermometer_ran {
  public:
    int temperature;   // Celsius
  public:
    void get_data();
};


class Thermometer_merge: public Thermometer {
  public:
    int* t1;
    int* t2;
    int* t3;
  public:
    void calculate();
};


class LightSen {
  public:
    const int pin = A0;
    int lightValue;
  public:
    void get_data();
};


class Air_humidity {
  public:
    int humidity;
  public:
    void get_data();
};


class Dirt_humidity {
  public:
    const int pin = A3;
    int humidity;
  public:
   void get_data();
};


void Fan::power()
{
  if( on_heater || on_timetable || on_hot || on_humidity ) {
        digitalWrite(pin, HIGH);
  }
  else {
    digitalWrite(pin, LOW);
  }
}


void Heater::power()
{
  if( on_temperature ) {
    digitalWrite(pin, HIGH);
  }
  else {
    digitalWrite(pin, LOW);
  }  
}


void Lamp::power()
{
  if( on_lamp ) {
    analogWrite(pin, 255);
  }
  else {
    analogWrite(pin, 0);
  }  
}


void Pump::power()
{
  if( on_pump ) {
    analogWrite(pin, 255);
  }
  else {
    digitalWrite(pin, 0);
  }  
}


void Thermometer::get_data()
{
  temperature = dht11.readTemperature();

  if (temperature == DHT11::ERROR_CHECKSUM || temperature == DHT11::ERROR_TIMEOUT) {
       Serial.println(DHT11::getErrorString(temperature));
  }
}


void Thermometer_ran::get_data()
{
  temperature = random(0, 50);
}


void LightSen::get_data()
{
  lightValue = analogRead(pin);
}


void Air_humidity::get_data()
{
  humidity = dht11.readHumidity();

  if (humidity == DHT11::ERROR_CHECKSUM || humidity == DHT11::ERROR_TIMEOUT) {
    Serial.println(DHT11::getErrorString(humidity));
  }
}


void Dirt_humidity::get_data()
{
  humidity = analogRead(pin);
}


void control_temperature(Thermometer &thermometer, Fan &fan, Heater &heater) {
  if (thermometer.temperature > climate.tempMax) {
    fan.on_hot = true;
    heater.on_temperature = false;
  }
  else if (thermometer.temperature < climate.tempMin) {
    fan.on_hot = false;
    fan.on_heater = true;
    heater.on_temperature = true;
  }
  else
  {
    heater.on_temperature = false;
  }
}


void ventilation_by_time(Fan &fan)
{
  static unsigned long lastVentTime = 0;
  static unsigned long startVentTime = 0;
  static bool venting = false;

  if ( !venting && TIME - lastVentTime >= VENT_PERIOD ) {
    venting = true;
    startVentTime = TIME;
  }

  if ( venting ) {
    if (TIME - startVentTime < VENT_DURATION) {
      fan.on_timetable = true;
    } else {
      fan.on_timetable = false;
      venting = false; // end of ventilation
      lastVentTime = TIME;
    }
  } else {
    fan.on_timetable = false;
  }
}


void control_light(LightSen &lightSen, Lamp &lamp) 
{
  if ( TIME / HOUR_LENGTH > LIGHT_ON_HOUR && TIME / HOUR_LENGTH < LIGHT_OFF_HOUR ) {
    if (lightSen.lightValue > climate.minLight) {
      lamp.on_lamp = true;
    }
    else {
      lamp.on_lamp = false;
  }
  } else {
    lamp.on_lamp = false;
  }
}


void control_air_humidity(Fan &fan, Air_humidity &air_humidity) {
  if ( air_humidity.humidity > climate.maxHum) {
    fan.on_humidity = true;
  } else {
    fan.on_humidity = false;
  }
}


void control_dirt_humidity(Pump &pump, Dirt_humidity &dirt_humidity) {
  static unsigned long lastWaterTime = 0;
  static unsigned long startWaterTime = 0;
  static bool watering = false;

  if ( !watering && dirt_humidity.humidity < climate.minMoisture && TIME - lastWaterTime >= WATER_PERIOD) {
    watering = true;
    startWaterTime = TIME;
  }

  if ( watering ) {
    if (TIME - startWaterTime < WATERING_DURATION) {
      pump.on_pump = true;
    } else {
      pump.on_pump = false;
      watering = false; // end of watering
      lastWaterTime = TIME;
    }
  } else {
    pump.on_pump = false;
  }
}


void Thermometer_merge::calculate() {
  int a = *t1;
  int b = *t2;
  int c = *t3;
  int temp;
  if (a > b) { temp = a; a = b; b = temp; }
  if (b > c) { temp = b; b = c; c = temp; }
  if (a > b) { temp = a; a = b; b = temp; }
  temperature = b;
}


void printStatus(Thermometer &thermometer, LightSen &lightSen, Air_humidity &air_humidity, Dirt_humidity &dirt_humidity, unsigned long TIME) {
  Serial.print("Temperature: ");
  Serial.print(thermometer.temperature);
  Serial.print(" Light: ");
  Serial.print(lightSen.lightValue);
  Serial.print(" Air_hum: ");
  Serial.print(air_humidity.humidity);
  Serial.print(" Dirt_hum: ");
  Serial.print(dirt_humidity.humidity);
  Serial.print(" Time (hour): ");
  Serial.println(TIME/HOUR_LENGTH);    // при ускорении времени / HOUR_LENGTH
}


Fan fan;
Heater heater;
Thermometer thermometer1;
Thermometer thermometer2;
Thermometer_ran thermometer3;
Thermometer_merge thermometer;
Lamp lamp;
Pump pump;
LightSen lightSen;
Air_humidity air_humidity;
Dirt_humidity dirt_humidity;


void setup() {
  pinMode(lightSen.pin, INPUT);
  pinMode(dirt_humidity.pin, INPUT);

  pinMode(fan.pin, OUTPUT);
  pinMode(lamp.pin, OUTPUT);
  pinMode(pump.pin, OUTPUT);
  pinMode(heater.pin, OUTPUT);

  thermometer.t1 = &thermometer1.temperature;
  thermometer.t2 = &thermometer2.temperature;
  thermometer.t3 = &thermometer3.temperature;

  Serial.begin(9600);
}


void loop() {
  TIME = millis()*900;  // * 900 для ускорения

  thermometer1.get_data();
  thermometer2.get_data();
  thermometer3.get_data();
  thermometer.calculate();
  lightSen.get_data();
  air_humidity.get_data();
  dirt_humidity.get_data();

  control_light(lightSen, lamp);
  control_air_humidity(fan, air_humidity);
  ventilation_by_time(fan);
  control_temperature(thermometer, fan, heater);
  control_dirt_humidity(pump, dirt_humidity);

  fan.power();
  heater.power();
  lamp.power();
  pump.power();

  delay(10);

  // Serial.print("Temperature1: ");
  // Serial.print(thermometer1.temperature);
  // Serial.print(" Temperature2: ");
  // Serial.print(thermometer2.temperature);
  // Serial.print(" Temperature3: ");
  // Serial.print(thermometer3.temperature);
  // Serial.print(" Temperature: ");
  // Serial.println(thermometer.temperature);

  printStatus(thermometer, lightSen, air_humidity, dirt_humidity, TIME);
}
