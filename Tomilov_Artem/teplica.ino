#include <DHT11.h>

#define TEMP_HUM_SENSOR_PIN 12

DHT11 dht11(TEMP_HUM_SENSOR_PIN);

// =================== Config ===================
struct ClimateConfig {
    int minLight = 400;
    int minMoisture = 500;
    int tempMin = 20;
    int tempMax = 30;
    int maxHum = 60;
};

ClimateConfig climate;

namespace Timing {
    const unsigned long HOUR = 3600000UL;
    const unsigned long LIGHT_ON = 6;
    const unsigned long LIGHT_OFF = 22;
    const unsigned long VENT_PERIOD = 3600000UL;
    const unsigned long VENT_DURATION = 300000UL;
    const unsigned long WATER_PERIOD = 10000UL;
    const unsigned long WATER_DURATION = 5000UL;
}

// =================== Device Classes ===================
class Device {
protected:
    int pin;
public:
    Device(int p) : pin(p) { pinMode(pin, OUTPUT); }
    virtual void power(bool state) { digitalWrite(pin, state ? HIGH : LOW); }
};

class Fan : public Device {
public:
    bool on_heater = false, on_timetable = false, on_hot = false, on_humidity = false;
    Fan(int p) : Device(p) {}
    void updatePower() {
        power(on_heater || on_timetable || on_hot || on_humidity);
    }
};

class Heater : public Device {
public:
    bool on_temperature = false;
    Heater(int p) : Device(p) {}
    void updatePower() { power(on_temperature); }
};

class Lamp : public Device {
public:
    bool on_lamp = false;
    Lamp(int p) : Device(p) {}
    void updatePower() { analogWrite(pin, on_lamp ? 255 : 0); }
};

class Pump : public Device {
public:
    bool on_pump = false;
    Pump(int p) : Device(p) {}
    void updatePower() { analogWrite(pin, on_pump ? 255 : 0); }
};

// =================== Sensor Classes ===================
class Thermometer {
public:
    int temperature = 0;
    virtual void read() { temperature = dht11.readTemperature(); }
};

class RandomThermometer : public Thermometer {
public:
    void read() override { temperature = random(0,50); }
};

class ThermometerMerge : public Thermometer {
public:
    int* t1; int* t2; int* t3;
    void calculate() {
        int a = *t1, b = *t2, c = *t3, tmp;
        if(a>b){ tmp=a;a=b;b=tmp; }
        if(b>c){ tmp=b;b=c;c=tmp; }
        if(a>b){ tmp=a;a=b;b=tmp; }
        temperature = b;
    }
};

class LightSensor {
public:
    const int pin;
    int value = 0;
    LightSensor(int p) : pin(p) { pinMode(pin, INPUT); }
    void read() { value = analogRead(pin); }
};

class AirHumidity {
public:
    int humidity = 0;
    void read() { humidity = dht11.readHumidity(); }
};

class SoilHumidity {
public:
    const int pin;
    int humidity = 0;
    SoilHumidity(int p) : pin(p) { pinMode(pin, INPUT); }
    void read() { humidity = analogRead(pin); }
};

// =================== Global Instances ===================
Fan fan(7);
Heater heater(4);
Lamp lamp(6);
Pump pump(5);

Thermometer thermometer1, thermometer2;
RandomThermometer thermometer3;
ThermometerMerge thermometer;

LightSensor lightSen(A0);
AirHumidity air_humidity;
SoilHumidity dirt_humidity(A3);

unsigned long TIME = 0;

// =================== Control Functions ===================
void controlTemperature() {
    if (thermometer.temperature > climate.tempMax) {
        fan.on_hot = true;
        heater.on_temperature = false;
    } else if (thermometer.temperature < climate.tempMin) {
        fan.on_hot = false;
        fan.on_heater = true;
        heater.on_temperature = true;
    } else {
        heater.on_temperature = false;
    }
}

void ventilationByTime() {
    static unsigned long lastVent = 0;
    static unsigned long startVent = 0;
    static bool venting = false;

    if (!venting && TIME - lastVent >= Timing::VENT_PERIOD) {
        venting = true;
        startVent = TIME;
    }

    fan.on_timetable = venting && (TIME - startVent < Timing::VENT_DURATION);

    if (venting && TIME - startVent >= Timing::VENT_DURATION) {
        venting = false;
        lastVent = TIME;
    }
}

void controlLight() {
    int hour = TIME / Timing::HOUR;
    lamp.on_lamp = (hour >= Timing::LIGHT_ON && hour < Timing::LIGHT_OFF) && (lightSen.value < climate.minLight);
}

void controlAirHumidity() {
    fan.on_humidity = air_humidity.humidity > climate.maxHum;
}

void controlSoilHumidity() {
    static unsigned long lastWater = 0;
    static unsigned long startWater = 0;
    static bool watering = false;

    if (!watering && dirt_humidity.humidity < climate.minMoisture && TIME - lastWater >= Timing::WATER_PERIOD) {
        watering = true;
        startWater = TIME;
    }

    pump.on_pump = watering && (TIME - startWater < Timing::WATER_DURATION);

    if (watering && TIME - startWater >= Timing::WATER_DURATION) {
        watering = false;
        lastWater = TIME;
    }
}

void printStatus() {
    Serial.print("Temp: "); Serial.print(thermometer.temperature);
    Serial.print(" Light: "); Serial.print(lightSen.value);
    Serial.print(" AirHum: "); Serial.print(air_humidity.humidity);
    Serial.print(" SoilHum: "); Serial.print(dirt_humidity.humidity);
    Serial.print(" Hour: "); Serial.println(TIME/Timing::HOUR);

    // === Отладка состояний устройств ===
    /*
    Serial.print("Fan: "); Serial.print(fan.on_heater || fan.on_timetable || fan.on_hot || fan.on_humidity);
    Serial.print(" Heater: "); Serial.print(heater.on_temperature);
    Serial.print(" Lamp: "); Serial.print(lamp.on_lamp);
    Serial.print(" Pump: "); Serial.println(pump.on_pump);
    */
}

// =================== Setup & Loop ===================
void setup() {
    Serial.begin(9600);
    thermometer.t1 = &thermometer1.temperature;
    thermometer.t2 = &thermometer2.temperature;
    thermometer.t3 = &thermometer3.temperature;
}

void loop() {
    TIME = millis() * 1000;  // speeding up time

    thermometer1.read();
    thermometer2.read();
    thermometer3.read();
    thermometer.calculate();
    lightSen.read();
    air_humidity.read();
    dirt_humidity.read();

    controlLight();
    controlAirHumidity();
    ventilationByTime();
    controlTemperature();
    controlSoilHumidity();

    fan.updatePower();
    heater.updatePower();
    lamp.updatePower();
    pump.updatePower();

    printStatus();
    delay(10);
}
