#include <DHT11.h>

#define PUMP_PIN 5 
#define LIGHT_PIN 6
#define HEAT_PIN 4
#define FAN_PIN 7
#define TEMP_HUMID_SENSOR_PIN 12
#define SOIL_MOISTURE_SENSOR_PIN 3

#define LIGHT_SENSOR_1_PIN 0
#define LIGHT_SENSOR_2_PIN 1
#define LIGHT_SENSOR_3_PIN 2
#define LIGHT_SENSOR_4_PIN 8
#define LIGHT_SENSOR_5_PIN 9

DHT11 dht11(TEMP_HUMID_SENSOR_PIN);


const unsigned long day_duration = 12 * 1000;
const unsigned long night_duration = 12 * 1000;
const unsigned long serial_interval = 1 * 1000;

const unsigned long watering_duration = 2 * 1000;
const unsigned long watering_delay = 2 * 1000;

unsigned long time = 0;


struct Climate_conditions {
    int min_temperature;
    int max_temperature;
    int min_air_humidity;
    int max_air_humidity;
    int min_soil_moisture;  // Чем выше значение, тем суше почва
    int max_soil_moisture;  // Чем ниже значение, тем влажнее почва
    int min_light;
    int max_light;
};


Climate_conditions climate = { 29, 30, 20, 40, 900, 10, 400, 20 };


class Fan {
public:
    const int pin = FAN_PIN;
    bool on_heater;
    bool on_humidity;
    bool on_hot;
public:
    void power();
};


class Heater {
public:
    const int pin = HEAT_PIN;
    bool on_temperature;
public:
    void power();
};


class Lamp {
public:
    const int pin = LIGHT_PIN;
    bool on_light;
    bool on_night;
public:
    void power();
};


class Pump {
public:
    const int pin = PUMP_PIN;
    bool on_pump;
    unsigned long start_watering_time = 0;
    unsigned long stop_watering_time = 0;
    bool watering_in_progress = false;
    bool pump_currently_active = false;
public:
    void power();
};


class Thermometer {
public:
    int temperature;
public:
    void get_data();
};


class Light_sensor {
public:
    int pin;
    int light_level;
public:
    void get_data();
};


class Light_sensor_merge : public Light_sensor {
public:
    Light_sensor sensors[5];
    int average_light_level;
    int light_levels[5];
    bool sensor_active[5] = {true, true, true, true, true};
public:
    void calculate();
    void init_pins();
    void read_all_sensors();
};


class Air_humidity_sensor {
public:
    int humidity;
public:
    void get_data();
};


class Soil_moisture_sensor {
public:
    const int pin = SOIL_MOISTURE_SENSOR_PIN;
    int moisture;
public:
    void get_data();
};


class Day_night_cycle {
public:
    bool is_night;
    unsigned long day_duration;
    unsigned long night_duration;
    unsigned long cycle_start_time;
    Day_night_cycle(unsigned long day, unsigned long night) : day_duration(day), night_duration(night), is_night(false), cycle_start_time(0) {}
    void update();
};


void Fan::power()
{
    if (on_heater || on_humidity || on_hot) {
        digitalWrite(pin, HIGH);
    }
    else {
        digitalWrite(pin, LOW);
    }
}


void Heater::power()
{
    if (on_temperature) {
        digitalWrite(pin, HIGH);
    }
    else {
        digitalWrite(pin, LOW);
    }
}


void Lamp::power()
{
    if (on_light || on_night) {
        digitalWrite(pin, HIGH);
    }
    else {
        digitalWrite(pin, LOW);
    }
}

void Pump::power()
{
    if (on_pump) {
        digitalWrite(pin, HIGH);
    }
    else {
        digitalWrite(pin, LOW);
    }
}


void Thermometer::get_data()
{
    temperature = dht11.readTemperature();
}


void Light_sensor::get_data()
{
    light_level = analogRead(pin);
}


void Air_humidity_sensor::get_data()
{
    humidity = dht11.readHumidity();
}


void Soil_moisture_sensor::get_data()
{
    moisture = analogRead(pin);
}


void Day_night_cycle::update()
{
    unsigned long cycle_time = time - cycle_start_time;

    if (!is_night && cycle_time >= day_duration) {
        is_night = true;
        cycle_start_time = time;
    }
    else if (is_night && cycle_time >= night_duration) {
        is_night = false;
        cycle_start_time = time;
    }
}


void Light_sensor_merge::init_pins() {
    sensors[0].pin = LIGHT_SENSOR_1_PIN;
    sensors[1].pin = LIGHT_SENSOR_2_PIN;
    sensors[2].pin = LIGHT_SENSOR_3_PIN;
    sensors[3].pin = LIGHT_SENSOR_4_PIN;
    sensors[4].pin = LIGHT_SENSOR_5_PIN;

    for (int i = 0; i < 5; i++) {
        pinMode(sensors[i].pin, INPUT);
    }
}


void Light_sensor_merge::calculate() {
    int sum = 0;
    int count = 0;

    for (int i = 0; i < 5; i++) {
        sensors[i].get_data();
        if (sensor_active[i] && sensors[i].light_level > 0 && sensors[i].light_level < 1024) {
            sum += sensors[i].light_level;
            count++;
        }
    }
    if (count > 0) {
        average_light_level = sum / count;
    }
    else {
        average_light_level = 0;
    }
}


void Light_sensor_merge::read_all_sensors() {
    for (int i = 0; i < 5; i++) {
        sensors[i].get_data();
        light_levels[i] = sensors[i].light_level;
    }
}


void control_temperature(Thermometer& thermometer, Fan& fan, Heater& heater) {
    if (thermometer.temperature >= climate.max_temperature) {
        fan.on_hot = false;
        heater.on_temperature = false;
    }

    else if (thermometer.temperature < climate.min_temperature) {
        fan.on_hot = false;
        fan.on_heater = true;
        heater.on_temperature = true;
    }

    else {
        fan.on_hot = false;
        heater.on_temperature = false;
    }
}


void control_air_humidity(Fan& fan, Air_humidity_sensor& air_humidity_sensor) {
    if (air_humidity_sensor.humidity > climate.max_air_humidity) {
        fan.on_humidity = true;
    }
    else if (air_humidity_sensor.humidity <= climate.min_air_humidity) {
        fan.on_humidity = false;
    }
}


void control_light(Light_sensor_merge& light_sensor_merge, Lamp& lamp, Day_night_cycle& day_night) {
    if (day_night.is_night) {
        lamp.on_night = true;
        lamp.on_light = false;
    }
    else {
        if (light_sensor_merge.average_light_level >= climate.min_light) {// Чем выше значение, тем темнее
            lamp.on_light = true;
        }
        else {
            lamp.on_light = false;
        }

        lamp.on_night = false;
    }
}


void control_soil_moisture(Pump& pump, Soil_moisture_sensor& soil_moisture_sensor) {
    if (soil_moisture_sensor.moisture > climate.min_soil_moisture) {
        if (!pump.watering_in_progress) { // Начинаем новый цикл полива
            pump.watering_in_progress = true;
            pump.pump_currently_active = true;
            pump.start_watering_time = time;
            pump.on_pump = true;
        }

        else if (pump.watering_in_progress && pump.pump_currently_active) { // Полив в процессе, насос работает
            if (time - pump.start_watering_time >= watering_duration) { // Завершаем полив
                pump.on_pump = false;
                pump.pump_currently_active = false;
                pump.stop_watering_time = time;
            }
        }

        else if (pump.watering_in_progress && !pump.pump_currently_active) { // Пауза между поливами
            if (time - pump.stop_watering_time >= watering_delay) {
                pump.watering_in_progress = false;
            }
        }

    }
    else {
        pump.on_pump = false;
        pump.watering_in_progress = false;
        pump.pump_currently_active = false;
    }
}


void print_status(Thermometer& thermometer, Light_sensor_merge& light_sensor_merge,
    Air_humidity_sensor& air_humidity_sensor, Soil_moisture_sensor& soil_moisture_sensor,
    Day_night_cycle& day_night) {
    static unsigned long last_print_time = 0;

    if (time - last_print_time >= serial_interval) {
        Serial.print("Температура: ");
        Serial.print(thermometer.temperature);
        Serial.print("°C, Влажность воздуха: ");
        Serial.print(air_humidity_sensor.humidity);
        Serial.print("%, Влажность почвы: ");
        Serial.print(soil_moisture_sensor.moisture);
        Serial.print(", Освещенность: ");
        Serial.print(light_sensor_merge.average_light_level);
        last_print_time = time;
    }
}


Fan fan;
Heater heater;
Lamp lamp;
Pump pump;
Thermometer thermometer;
Light_sensor_merge light_sensor_merge;
Air_humidity_sensor air_humidity_sensor;
Soil_moisture_sensor soil_moisture_sensor;
Day_night_cycle day_night(day_duration, night_duration);


void setup() {
    Serial.begin(9600);
    dht11.begin();
    pinMode(fan.pin, OUTPUT);
    pinMode(heater.pin, OUTPUT);
    pinMode(lamp.pin, OUTPUT);
    pinMode(pump.pin, OUTPUT);
    light_sensor_merge.init_pins();
    Serial.println("Система умной теплицы инициализирована");
    Serial.println("======================================");
}

void loop() {
    time = millis();
    day_night.update();
    thermometer.get_data();
    light_sensor_merge.read_all_sensors();
    light_sensor_merge.calculate();
    air_humidity_sensor.get_data();
    soil_moisture_sensor.get_data();

    control_temperature(thermometer, fan, heater);
    control_air_humidity(fan, air_humidity_sensor);
    control_light(light_sensor_merge, lamp, day_night);
    control_soil_moisture(pump, soil_moisture_sensor);

    fan.power();
    heater.power();
    lamp.power();
    pump.power();

    print_status(thermometer, light_sensor_merge, air_humidity_sensor, soil_moisture_sensor, day_night);

    //Serial.print(light_sensor_merge.sensors[2].light_level);
    //Serial.print(light_sensor_merge.light_levels[2]);

    delay(100);
}
