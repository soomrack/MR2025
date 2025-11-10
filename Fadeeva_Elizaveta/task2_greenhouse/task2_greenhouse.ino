#include <DHT11.h>


#define SENSOR_PIN 12     // сенсор температуры и влажности DHT11
DHT11 dht11(SENSOR_PIN);


// включение и выключение света
#define LIGHT_ON_HOUR 6       
#define LIGHT_OFF_HOUR 21     


// время переведенное в миллисекунды
#define HOUR_LENGTH 3600000UL // 1 час 
#define VENT_PERIOD 3600000UL // 1 час (промежуток между вентиляциями)
#define VENT_DURATION 600000UL // 10 минут (время вентиляции)
#define WATER_PERIOD 120000UL // 2 минуты (промежуток между поливом)
#define WATERING_DURATION 10000UL // 10 секунд (время полива)


unsigned long TIME = 0;


struct ClimateLimits {
  int minLight;
  int minDirtHum;
  int tempMin;
  int tempMax;
  int maxAirHum;
};


ClimateLimits climate = {400, 1000, 20, 30, 60};


class Fan {
  public:
    const int pin = 7;
    bool _heater = false;
    bool _timing = false;
    bool _hotness = false;
    bool _humidity = false;

    void power();
};


void Fan::power() {
  if( _heater || _timing || _hotness || _humidity ) 
    digitalWrite(pin, HIGH);
    else 
      digitalWrite(pin, LOW);
}


class Heater {
  public:
    const int pin = 4;
    bool _temperature = 0;
    void power();
};


void Heater::power() {
  if( _temperature ) 
    digitalWrite(pin, HIGH);
    else 
      digitalWrite(pin, LOW);  
}


class Lamp {
  public:
    const int pin = 6;
    bool _lamp = false;
    void power();
};


void Lamp::power() {
  if( _lamp ) 
    analogWrite(pin, 255);
    else 
      analogWrite(pin, 0);  
}


class Pump {
  public:
    const int pin = 5;
    bool _pump = false;
    void power();
};


void Pump::power() {
  if( _pump ) 
    analogWrite(pin, 255);
    else 
      digitalWrite(pin, 0);  
}


class Light {
  public:
    const int pin = A0;
    int lightValue;

    void read() {
      lightValue = analogRead(pin);
    }
};


class Air_humidity {
  public:
    int humidity;
    void get_data();
};


void Air_humidity::get_data() {
  humidity = dht11.readHumidity();

  if (humidity == DHT11::ERROR_CHECKSUM || humidity == DHT11::ERROR_TIMEOUT) {
    Serial.println(DHT11::getErrorString(humidity));
  }
}


class Dirt_humidity {
  public:
    const int pin = A3;
    int humidity;

   void read(){
    humidity = analogRead(pin);
  }
};


class Thermometer {
  public:
    int temperature = 0;   
    void get_data();
};


void Thermometer::get_data()
{
  temperature = dht11.readTemperature();

  if (temperature == DHT11::ERROR_CHECKSUM || temperature == DHT11::ERROR_TIMEOUT) {
       Serial.println(DHT11::getErrorString(temperature));
  }
}


class Thermometer_random {
  public:
    int temperature = 0;  
     
    void read() {
      temperature = random(0, 50);
    }
};


class Thermometer_mean: public Thermometer {
  public:
    void calculate(Thermometer & t01, Thermometer & t02, Thermometer_random & t03);
};


void Thermometer_mean::calculate(Thermometer & t01, Thermometer & t02, Thermometer_random & t03) {
  int a = t01.temperature;
  int b = t02.temperature;
  int c = t03.temperature;
  int temp;
  if (a > b) { temp = a; a = b; b = temp; }
  if (b > c) { temp = b; b = c; c = temp; }
  if (a > b) { temp = a; a = b; b = temp; }
  temperature = b;
}


void control_temp(Thermometer & thermometer, Fan & fan, Heater & heater) {
  if (thermometer.temperature > climate.tempMax) {
    fan._hotness = true;
    heater._temperature = false;
  }
  else if (thermometer.temperature < climate.tempMin) {
    fan._hotness = false;
    fan._heater = true;
    heater._temperature = true;
  }
  else{
    heater._temperature = false;
  }
}


void control_vent(Fan & fan) {
  static unsigned long lastVentTime = 0;
  static unsigned long startVentTime = 0;
  static bool venting = false;

  if ( !venting && TIME - lastVentTime >= VENT_PERIOD ) {
    venting = true;
    startVentTime = TIME;
  }

  if (venting) {
    if (TIME - startVentTime < VENT_DURATION) {
      fan._timing = true;
    } else {
      fan._timing = false;
      venting = false;
      lastVentTime = TIME;
    }
  } else {
    fan._timing = false;
  }
}


void control_light(Light & light, Lamp & lamp) {
  if ( TIME / HOUR_LENGTH > LIGHT_ON_HOUR && TIME / HOUR_LENGTH < LIGHT_OFF_HOUR ) {
    if (light.lightValue > climate.minLight) {
      lamp._lamp = true;
    }
    else {
      lamp._lamp = false;
  }
  } else {
    lamp._lamp = false;
  }
}


void control_air(Fan & fan, Air_humidity & air_humidity) {
  if ( air_humidity.humidity > climate.maxAirHum) {
    fan._humidity = true;
  } else {
    fan._humidity = false;
  }
}


void control_dirt(Pump & pump, Dirt_humidity & dirt_humidity) {
  static unsigned long lastWaterTime = 0;
  static unsigned long startWaterTime = 0;
  static bool watering = false;

  if (!watering && dirt_humidity.humidity > climate.minDirtHum && TIME - lastWaterTime >= WATER_PERIOD) {
    watering = true;
    startWaterTime = TIME;
  }

  pump._pump = watering && (TIME - startWaterTime < WATERING_DURATION);

  if (watering && TIME - startWaterTime >= WATERING_DURATION) {
    watering = false;
    lastWaterTime = TIME;
  }
}


Fan fan;
Heater heater;
Thermometer thermometer1;
Thermometer thermometer2;
Thermometer_random thermometer3;
Thermometer_mean thermometer;
Lamp lamp;
Pump pump;
Light light;
Air_humidity air_humidity;
Dirt_humidity dirt_humidity;


void printStatus(Thermometer &thermometer, Light &light, Air_humidity &air_humidity, Dirt_humidity &dirt_humidity, unsigned long TIME) {
  Serial.print("Temperature: ");
  Serial.print(thermometer.temperature);
  Serial.print(" Light: ");
  Serial.print(light.lightValue);
  if (lamp._lamp) Serial.print(" Lamp ON"); else Serial.print(" Lamp OFF");
  Serial.print(" Air_hum: ");
  Serial.print(air_humidity.humidity);
  Serial.print(" Dirt_hum: ");
  Serial.print(dirt_humidity.humidity);
  Serial.print(" Time (hour): ");
  Serial.println(TIME/HOUR_LENGTH);    // при ускорении времени 
}


void setup() {
  pinMode(light.pin, INPUT);
  pinMode(dirt_humidity.pin, INPUT);

  pinMode(fan.pin, OUTPUT);
  pinMode(lamp.pin, OUTPUT);
  pinMode(pump.pin, OUTPUT);
  pinMode(heater.pin, OUTPUT);

  Serial.begin(9600);
}


void loop() {
  TIME = millis()*600;  // *600 ускорение

  light.read ();
  dirt_humidity.read ();
  thermometer3.read ();

  thermometer1.get_data();
  thermometer2.get_data();
  thermometer.calculate(thermometer1, thermometer2, thermometer3);
  air_humidity.get_data();

  control_light(light, lamp);
  control_vent(fan);
  control_temp(thermometer, fan, heater);
  control_air(fan, air_humidity);
  control_dirt(pump, dirt_humidity);

  fan.power();
  heater.power();
  lamp.power();
  pump.power();

  delay(10);

  printStatus(thermometer, light, air_humidity, dirt_humidity, TIME);
}
