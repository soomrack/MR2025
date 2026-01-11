#include <DHT11.h>

// аппаратные настройки
constexpr int SENSOR_PIN = 12; // DHT11
DHT11 dht(SENSOR_PIN);

// постоянные врмени (в миллисекундах)
constexpr unsigned long HOUR_MS      = 3600000UL; // час
constexpr unsigned long VENT_PERIOD  = 3600000UL; // период вентиляции (час)
constexpr unsigned long VENT_TIME    = 300000UL;  // время вентиляции (5 минут)
constexpr unsigned long WATER_PERIOD = 10000UL;   // период полива (10 секунд) 
constexpr unsigned long WATER_TIME   = 5000UL;    // время полива (5 секунд)

// время включения - выключения света (в часах)
constexpr int LIGHT_ON_HOUR  = 6;
constexpr int LIGHT_OFF_HOUR = 22;

// климатические параметры
struct EnvLimits {
  int MinLight;
  int MinSoilMoisture;
  int MinTemp;
  int MaxTemp;
  int MaxAirHum;
};

EnvLimits limits{ 400, 800, 20, 30, 60 };

// компоненты
class Heater {
public:
  const int pin = 4;
  bool StartByTemp = false;

  void UpdateOutput() const {
    digitalWrite(pin, StartByTemp ? HIGH : LOW);
  }
};

class Fan {
public:
  const int pin = 7;
  bool ByHeater = false;
  bool BySchedule = false;
  bool ByHeat = false;
  bool ByHumidity = false;

  void UpdateOutput() const {
    if (ByHeater || BySchedule || ByHeat || ByHumidity) digitalWrite(pin, HIGH);
    else digitalWrite(pin, LOW);
  }
};

class Pump {
public:
  const int pin = 5;
  bool enabled = false;

  void UpdateOutput() const {
    if (enabled) analogWrite(pin, 255);
    else digitalWrite(pin, LOW);
  }
};

class Lamp {
public:
  const int pin = 6;
  bool enabled = false;

  void UpdateOutput() const {
    analogWrite(pin, enabled ? 255 : 0);
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

class ThermometerRandom : public Thermometer {
public:
  void read() {
    temperature = random(0, 50);
  }
};

class ThermometerMedian : public Thermometer {
public:


  void CalcMedian(Thermometer &t1, Thermometer &t2, Thermometer &t3) {
    int a = t1.temperature;
    int b = t2.temperature;
    int c = t3.temperature;
    int tmp;
    if (a > b) { tmp = a; a = b; b = tmp; }
    if (b > c) { tmp = b; b = c; c = tmp; }
    if (a > b) { tmp = a; a = b; b = tmp; }
    temperature = b;
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

class LightSensor {
public:
  const int pin = A0;
  int value = 0;
  void read() { value = analogRead(pin); }
};

class SoilHumidity {
public:
  const int pin = A3;
  int value = 0;
  void read() { value = analogRead(pin); }
};

// логика контроля
void ControlTemperature(Thermometer &therm, Fan &fan, Heater &heater) {
  if (therm.temperature > limits.MaxTemp) {
    fan.ByHeat = true;
    heater.StartByTemp = false;
  } else if (therm.temperature < limits.MinTemp) {
    fan.ByHeat = false;
    fan.ByHeater = true;
    heater.StartByTemp = true;
  } else {
    heater.StartByTemp = false;
  }
}

void PlanedVentilation(Fan &fan, unsigned long now) {
  static unsigned long LastStart = 0;
  static unsigned long VentStart = 0;
  static bool IsVenting = false;

  if (!IsVenting && now - LastStart >= VENT_PERIOD) {
    IsVenting = true;
    VentStart = now;
  }

  if (IsVenting) {
    if (now - VentStart < VENT_TIME) fan.BySchedule = true;
    else {
      fan.BySchedule = false;
      IsVenting = false;
      LastStart = now;
    }
  } else {
    fan.BySchedule = false;
  }
}

void ControlLight(const LightSensor &ls, Lamp &lamp, unsigned long now) {
  unsigned long hour = now / HOUR_MS;
  bool WithinDay = (hour > LIGHT_ON_HOUR) && (hour < LIGHT_OFF_HOUR);
  if (WithinDay && ls.value > limits.MinLight) lamp.enabled = true;
  else lamp.enabled = false;
}

void ControlAirHumidity(Fan &fan, const AirHumidity &ah) {
  fan.ByHumidity = (ah.value > limits.MaxAirHum);
}

void ControlSoilMoisture(Pump &pump, const SoilHumidity &sh, unsigned long now) {
  static unsigned long LastWater = 0;
  static unsigned long WaterStart = 0;
  static bool watering = false;

  if (!watering && sh.value > limits.MinSoilMoisture && now - LastWater >= WATER_PERIOD) {
    watering = true;
    WaterStart = now;
  }

  if (watering) {
    if (now - WaterStart < WATER_TIME) pump.enabled = true;
    else {
      pump.enabled = false;
      watering = false;
      LastWater = now;
    }
  } else {
    pump.enabled = false;
  }
}

void PrintStatus(const Thermometer &t, const LightSensor &ls, const AirHumidity &ah, const SoilHumidity &sh, unsigned long now) {
  Serial.print("Temperature: ");
  Serial.print(t.temperature);
  Serial.print(" Light: ");
  Serial.print(ls.value);
  Serial.print(" Air Hum: ");
  Serial.print(ah.value);
  Serial.print(" Dirt Hum: ");
  Serial.print(sh.value);
  Serial.print(" Time (hour): ");
  Serial.println(now / HOUR_MS);
}

// экземпляры
Heater heater;
Fan fan;
Lamp lamp;
Pump pump;
Thermometer t01;
Thermometer t02;
ThermometerRandom t03;
ThermometerMedian tMerged;
LightSensor LightSen;
AirHumidity AirHum;
SoilHumidity SoilHum;

void setup() {
  pinMode(LightSen.pin, INPUT);
  pinMode(SoilHum.pin, INPUT);

  pinMode(fan.pin, OUTPUT);
  pinMode(lamp.pin, OUTPUT);
  pinMode(pump.pin, OUTPUT);
  pinMode(heater.pin, OUTPUT);


  Serial.begin(9600);
}

void loop() {
  unsigned long now = millis() * 900UL; // ускорение

  t01.read();
  t02.read();
  t03.read();
  tMerged.CalcMedian(t01, t02, t03);

  LightSen.read();
  AirHum.read();
  SoilHum.read();

  ControlLight(LightSen, lamp, now);
  ControlAirHumidity(fan, AirHum);
  PlanedVentilation(fan, now);
  ControlTemperature(tMerged, fan, heater);
  ControlSoilMoisture(pump, SoilHum, now);

  fan.UpdateOutput();
  heater.UpdateOutput();
  lamp.UpdateOutput();
  pump.UpdateOutput();

  delay(10);

  PrintStatus(tMerged, LightSen, AirHum, SoilHum, now);
}
