#include <DHT11.h>


#define LIGHT_SENSOR A0        // light sensor
#define DIRT_HUM_SENSOR A1     // soil moisture sensor
#define TEMP_HUM_SENSOR 12     // temperature and humidity sensor
#define FAN 7                  // fan
#define LIGHTING 6             // lighting (shim)
#define PUMP 5                 // pump power (shim)
#define HEATER 4               // air heater


DHT11 dht11(TEMP_HUM_SENSOR);


#define HOUR_LENGTH 3600000UL // 1 hour in millis
#define LIGHT_ON_HOUR 6       // turn light on at 6:00
#define LIGHT_OFF_HOUR 22     // turn light off at 22:00


#define VENT_PERIOD 3600000UL // period of ventilation: 1 hour (3600000 millis)
#define VENT_DURATION 300000UL // duration of ventilation: 5 minutes (300000 millis)


#define WATER_PERIOD 10000UL // period of watering: 30 seconds (30000 millis)
#define WATERING_DURATION 5000UL // duration of watering: 5 seconds (5000 millis)


unsigned long TIME = 0;


#define MIN_LIGHT 700
#define MIN_MOISTURE 500
#define TEMP_MIN 20
#define TEMP_MAX 30


class Fan {
  public:
    bool on_heater;
    bool on_timetable;
    bool on_hot;
    bool on_humidity;
  public:
    void power();
};


class Heater {
  public:
    bool on_temperature;
  public:
    void power();
};


class Lamp {
  public:
    bool on_lamp;
  public:
    void power();
};


class Pump {
  public:
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


class LightSen {
  public:
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
    int humidity;
  public:
   void get_data();
};


void Fan::power()
{
  if( on_heater || on_timetable || on_hot || on_humidity ) {
        digitalWrite(FAN, HIGH);
  }
  else {
    digitalWrite(FAN, LOW);
  }
}


void Heater::power()
{
  if( on_temperature ) {
    digitalWrite(HEATER, HIGH);
  }
  else {
    digitalWrite(HEATER, LOW);
  }  
}


void Lamp::power()
{
  if( on_lamp ) {
    analogWrite(LIGHTING, 255);
  }
  else {
    digitalWrite(LIGHTING, 0);
  }  
}


void Pump::power()
{
  if( on_pump ) {
    analogWrite(PUMP, 255);
  }
  else {
    digitalWrite(PUMP, 0);
  }  
}


void Thermometer::get_data()
{
  temperature = dht11.readTemperature();

  if (temperature == DHT11::ERROR_CHECKSUM || temperature == DHT11::ERROR_TIMEOUT) {
       Serial.println(DHT11::getErrorString(temperature));
  }
}


void LightSen::get_data()
{
  lightValue = analogRead(LIGHT_SENSOR);
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
  humidity = analogRead(DIRT_HUM_SENSOR);
}


bool control_temperature(Thermometer &thermometer, Fan &fan, Heater &heater) {
  if (thermometer.temperature > TEMP_MAX) {
    fan.on_hot = true;
    heater.on_temperature = false;
  }
  else if (thermometer.temperature < TEMP_MIN) {
    fan.on_hot = false;
    fan.on_heater = true;
    heater.on_temperature = true;
  }
  else
  {
    heater.on_temperature = false;
  }
}


bool ventilation_by_time(Fan &fan)
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


bool control_light(LightSen &lightSen, Lamp &lamp) 
{
  if ( TIME / HOUR_LENGTH > LIGHT_ON_HOUR && TIME / HOUR_LENGTH < LIGHT_OFF_HOUR ) {
    if (lightSen.lightValue > MIN_LIGHT) {
      lamp.on_lamp = true;
    }
    else {
      lamp.on_lamp = false;
  }
  } else {
    lamp.on_lamp = false;
  }
}


bool control_air_humidity(Fan &fan, Air_humidity &air_humidity) {
  if ( air_humidity.humidity > 60) {
    fan.on_humidity = true;
  } else {
    fan.on_humidity = false;
  }
}


bool control_dirt_humidity(Pump &pump, Dirt_humidity &dirt_humidity) {
  static unsigned long lastWaterTime = 0;
  static unsigned long startWaterTime = 0;
  static bool watering = false;

  if ( !watering && dirt_humidity.humidity < MIN_MOISTURE && TIME - lastWaterTime >= WATER_PERIOD) {
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


Fan fan;
Heater heater;
Thermometer thermometer;
Lamp lamp;
Pump pump;
LightSen lightSen;
Air_humidity air_humidity;
Dirt_humidity dirt_humidity;


void setup() {
  pinMode(LIGHT_SENSOR, INPUT);
  pinMode(DIRT_HUM_SENSOR, INPUT);

  pinMode(FAN, OUTPUT);
  pinMode(LIGHTING, OUTPUT);
  pinMode(PUMP, OUTPUT);
  pinMode(HEATER, OUTPUT);

  Serial.begin(9600);
}


void loop() {
  TIME = millis();

  thermometer.get_data();
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

  Serial.print("Temperature: ");
  Serial.print(thermometer.temperature);
  Serial.print(" Light: ");
  Serial.print(lightSen.lightValue);
  Serial.print(" Air_hum: ");
  Serial.print(air_humidity.humidity);
  Serial.print(" Dirt_hum: ");
  Serial.print(dirt_humidity.humidity);
  Serial.print(" Time (hour): ");
  Serial.println(TIME);
}
