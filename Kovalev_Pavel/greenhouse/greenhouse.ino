#include <DHT11.h>

// аппаратные настройки
const int SENSOR_PIN = 12; // DHT11
DHT11 dht(SENSOR_PIN);

// временные константы (в миллисекундах)
const unsigned long HOUR_MS      = 3600 * 1000UL; // 1 час
const unsigned long VENT_PERIOD  = 3600 * 1000UL; // период вентиляции: 1 час
const unsigned long VENT_TIME    = 300  * 1000UL; // время вентиляции: 5 минут
const unsigned long WATER_PERIOD = 10   * 1000UL; // период полива: 10 секунд 
const unsigned long WATER_TIME   = 5    * 1000UL; // время полива: 5 секунд
const          int  DAY_HOURS    = 24           ; // количество часов в сутках

// часы включения/выключения света (целые часы)
const int LIGHT_ON_HOUR  = 6;
const int LIGHT_OFF_HOUR = 22;

// параметры климата
struct EnvLimits {
  int minLight;
  int minSoilMoisture;
  int tempMin;
  int tempMax;
  int maxAirHum;
};

EnvLimits limits{ 400, 500, 20, 30, 60 };

// устройства
class Fan {
public:
  const int pin = 7;
  bool byHeater = false;
  bool bySchedule = false;
  bool byHeat = false;
  bool byHumidity = false;

  void updateOutput() {
    if (byHeater || bySchedule || byHeat || byHumidity) digitalWrite(pin, HIGH);
    else digitalWrite(pin, LOW);
  }
};

class Heater {
public:
  const int pin = 4;
  bool activeByTemp = false;

  void updateOutput() {
    digitalWrite(pin, activeByTemp ? HIGH : LOW);
  }
};

class Lamp {
public:
  const int pin = 6;
  bool enabled = false;

  void updateOutput() {
    analogWrite(pin, enabled ? 255 : 0);
  }
};

class Pump {
public:
  const int pin = 5;
  bool enabled = false;

  void updateOutput() {
    if (enabled) analogWrite(pin, 255);
    else digitalWrite(pin, LOW);
  }
};

// датчики
class Thermometer {
public:
  int temperature = 0; // C
  void read() {
    temperature = dht.readTemperature();
    if (temperature == DHT11::ERROR_CHECKSUM || temperature == DHT11::ERROR_TIMEOUT) {
      Serial.println(DHT11::getErrorString(temperature));
    }
  }
};

class ThermometerRandom {
public:
  int temperature = 0;
  void read() {
    temperature = random(0, 50);
  }
};

class ThermometerMedian : public Thermometer {
  public:
    int *t1 = nullptr;
    int *t2 = nullptr;
    int *t3 = nullptr;

  void calcMedian() {
    int a = *t1;
    int b = *t2;
    int c = *t3;
    int tmp;
    if (a > b) { tmp = a; a = b; b = tmp; }
    if (b > c) { tmp = b; b = c; c = tmp; }
    if (a > b) { tmp = a; a = b; b = tmp; }
    temperature = b;
  }
};

class LightSensor {
public:
  const int pin = A0;
  int value = 0; // Большое значение (около 500) — темно, малое — светло.
  void read() {
    value = analogRead(pin);
  }
};

class AirHumidity {
public:
  int value = 0;
  void read() {
    value = dht.readHumidity();
    if (value == DHT11::ERROR_CHECKSUM || value == DHT11::ERROR_TIMEOUT) {
      Serial.println(DHT11::getErrorString(value));
    }
  }
};

class SoilHumidity {
public:
  const int pin = A3;
  int value = 0;
  void read() { value = analogRead(pin); }
};

//
// логика контроля
//

void regulateTemperature(Thermometer &therm, Fan &fan, Heater &heater) {
  if (therm.temperature > limits.tempMax) {
    fan.byHeat = true;
    heater.activeByTemp = false;
  } else if (therm.temperature < limits.tempMin) {
    fan.byHeat = false;
    fan.byHeater = true;
    heater.activeByTemp = true;
  } else {
    heater.activeByTemp = false;
  }
}

void scheduledVentilation(Fan &fan, unsigned long now) {
  static unsigned long lastStart = 0;
  static unsigned long ventStart = 0;
  static bool isVenting = false;

  if (!isVenting && now - lastStart >= VENT_PERIOD) {
    isVenting = true;
    ventStart = now;
  }

  if (isVenting) {
    if (now - ventStart < VENT_TIME) fan.bySchedule = true;
    else {
      fan.bySchedule = false;
      isVenting = false;
      lastStart = now;
    }
  } else {
    fan.bySchedule = false;
  }
}

void regulateLight(const LightSensor &ls, Lamp &lamp, unsigned long now) {
  unsigned long hour = now / HOUR_MS;
  bool withinDay = (hour % DAY_HOURS > LIGHT_ON_HOUR) && (hour % DAY_HOURS < LIGHT_OFF_HOUR);
  if (withinDay && ls.value > limits.minLight) lamp.enabled = true;
  else lamp.enabled = false;
}

void regulateAirHumidity(Fan &fan, const AirHumidity &ah) {
  fan.byHumidity = (ah.value > limits.maxAirHum);
}

void regulateSoilMoisture(Pump &pump, const SoilHumidity &sh, unsigned long now) {
  static unsigned long lastWater = 0;
  static unsigned long waterStart = 0;
  static bool watering = false;

  if (!watering && sh.value < limits.minSoilMoisture && now - lastWater >= WATER_PERIOD) {
    watering = true;
    waterStart = now;
  }

  if (watering) {
    if (now - waterStart < WATER_TIME) pump.enabled = true;
    else {
      pump.enabled = false;
      watering = false;
      lastWater = now;
    }
  } else {
    pump.enabled = false;
  }
}

void printStatus(const Thermometer &t, const LightSensor &ls, const AirHumidity &ah, const SoilHumidity &sh, unsigned long now) {
  Serial.print("Temperature: ");
  Serial.print(t.temperature);
  Serial.print(" Light: ");
  Serial.print(ls.value);
  Serial.print(" Air_hum: ");
  Serial.print(ah.value);
  Serial.print(" Dirt_hum: ");
  Serial.print(sh.value);
  Serial.print(" Time (hour): ");
  Serial.println(now / HOUR_MS);
}

// экземпляры
Fan fan;
Heater heater;
Thermometer t1;
Thermometer t2;
ThermometerRandom t3;
ThermometerMedian tMerged;
Lamp lamp;
Pump pump;
LightSensor lightSensor;
AirHumidity airHum;
SoilHumidity soilHum;

void setup() {
  pinMode(lightSensor.pin, INPUT);
  pinMode(soilHum.pin, INPUT);

  pinMode(fan.pin, OUTPUT);
  pinMode(lamp.pin, OUTPUT);
  pinMode(pump.pin, OUTPUT);
  pinMode(heater.pin, OUTPUT);

  tMerged.t1 = &t1.temperature;
  tMerged.t2 = &t2.temperature;
  tMerged.t3 = &t3.temperature;

  Serial.begin(9600);
}

void loop() {
  const int FLA = 1200; // ускорение
  unsigned long now = millis() * FLA;

  t1.read();
  t2.read();
  t3.read();
  tMerged.calcMedian();

  lightSensor.read();
  airHum.read();
  soilHum.read();

  regulateLight(lightSensor, lamp, now);
  regulateAirHumidity(fan, airHum);
  scheduledVentilation(fan, now);
  regulateTemperature(tMerged, fan, heater);
  regulateSoilMoisture(pump, soilHum, now);

  fan.updateOutput();
  heater.updateOutput();
  lamp.updateOutput();
  pump.updateOutput();

  delay(10);

  printStatus(tMerged, lightSensor, airHum, soilHum, now);
}
