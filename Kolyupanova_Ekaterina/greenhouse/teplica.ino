#include <DHT.h>

// -- PINs --
#define DATA_SENSOR A5 // время
#define PIN_A_TEMP_SENS 12
#define PIN_D_TEMP_ACTUATOR 4
#define PIN_D_FAN 7
#define PIN_A_SOIL_MOISTURE 17
#define PIN_A_AIR_MOISTURE 12
#define PIN_D_LED 6
#define PIN_A_LIGHT_SENS 14
#define PIN_D_WATERING 5
#define DHT_AIR_HUMIDITY_SENSOR 12 // #define AIR_HUMIDITY_SENSOR 13 
#define DHT_TEMPERATURE_SENSOR DHT11  // #define TEMPERATURE_SENSOR A1 

// -- HIGH and LOW levels on sensors --
// Датчик температуры
#define HIGH_TEMP_SENS 1023
#define LOW_TEMP_SENS 0
// Датчик влажности почвы
#define DRY_SOIL_MOISTURE 1023
#define WET_SOIL_MOISTURE 0
// Датчик влажнlow_levelости воздуха
#define DRY_AIR_MOISTURE 512
#define WET_AIR_MOISTURE 0
// Датчик света
#define HIGH_LIGHT_SENS 450
#define LOW_LIGHT_SENS 600

#define SENSOR_READ_INTERVAL 500

DHT dht(DHT_AIR_HUMIDITY_SENSOR, DHT_TEMPERATURE_SENSOR);

// -- Flags --
bool is_cold = false;
bool is_soil_dry = false;
bool is_air_wet = false;
bool is_dark = false;
bool is_pause = false;
bool high_temp = false;
bool dry_soil_moisture = false;
bool dry_air_moisture = false;
bool low_light_value = false;

// millis
unsigned long time_start_watering = 0;
unsigned long time_pause_watering = 0;
unsigned long last_sensor_read_time = 0;

// время
int HOURS = 8;
unsigned long int TIME;
unsigned int DATA = 0;

// value of sensors
int temperature = 0;
int soil_moisture = 0;
int air_moisture = 0;
int light_value = 0;


class Sensor {
public:
    virtual int read_value() = 0;
};

class Sensor_GPIO: public Sensor {
public:
    unsigned char pin = 0;
    int value = 0;
    
    int min_trigger_value = 0;
    int max_trigger_value = 1023;

    int read_value() override {
        value = analogRead(pin); // закомментировано для примера
        return get_value();
    }

    int get_value() const {
        return value;
    }

    Sensor_GPIO(unsigned char s_pin, int s_min_trigger_value = 0, int s_max_trigger_value = 1023) {
        pin = s_pin;
        min_trigger_value = s_min_trigger_value;
        max_trigger_value = s_max_trigger_value;
        pinMode(pin, INPUT); // закомментировано для примера
    }
};

bool calculate_average(Sensor_GPIO* sensors, unsigned char numSensors = 1) {
    unsigned char count_low = 0;
    unsigned char count_high = 0;

    for (unsigned char i = 0; i < numSensors; ++i) {
        int val = sensors[i].get_value();

        if (val <= sensors[i].min_trigger_value)
            count_low++;

        if (val >= sensors[i].max_trigger_value)
            count_high++;
    }

    return count_high > count_low;
}

bool calculate_average(const Sensor_GPIO &sensor) {
    uint16_t val = sensor.get_value();
    return (val >= sensor.max_trigger_value && !(val <= sensor.min_trigger_value));
}

void night_mode() {
    digitalWrite(PIN_D_TEMP_ACTUATOR, LOW);
    digitalWrite(PIN_D_FAN, LOW);
    digitalWrite(PIN_D_LED, LOW);
    digitalWrite(PIN_D_WATERING, LOW);
}

void hours(unsigned long current_time) {
    if (current_time % 60 * 60 * 1000 == 0) {
        HOURS += 1;
    }
    if (HOURS == 24) HOURS = 0;
}


void time(unsigned long current_time) {
    if (current_time - TIME >= 50) {
        TIME = millis();
        DATA = analogRead(DATA_SENSOR);
        Serial.print("TIME = ");
        Serial.print(TIME);
        Serial.print("DATA_SENSOR = ");
        Serial.println(DATA);
    }

}

Sensor_GPIO temp_sens_1(PIN_A_TEMP_SENS, LOW_TEMP_SENS, HIGH_TEMP_SENS);
Sensor_GPIO moisture_soil_sens_1(PIN_A_SOIL_MOISTURE, WET_SOIL_MOISTURE, DRY_SOIL_MOISTURE);
Sensor_GPIO moisture_air_sens_1(PIN_A_AIR_MOISTURE, WET_AIR_MOISTURE, WET_AIR_MOISTURE);
Sensor_GPIO light_sens_1(PIN_A_LIGHT_SENS, HIGH_LIGHT_SENS, LOW_LIGHT_SENS);

void setup() {
  // put your setup code here, to run once:
  pinMode(PIN_D_TEMP_ACTUATOR, OUTPUT);
  pinMode(PIN_D_FAN, OUTPUT);
  pinMode(PIN_D_LED, OUTPUT);
  pinMode(PIN_D_WATERING, OUTPUT);

  Serial.begin(115200);
}

void loop() {
  // put your main code here, to run repeatedly:

  unsigned long current_time = millis();

  // подсчет  и обнуление часов
  hours(current_time);


  // Night check
  if (HOURS >= 21 || HOURS <= 7) {
      night_mode();
  }
  else {
      // Чтение датчиков с интервалом
      if (current_time - last_sensor_read_time >= SENSOR_READ_INTERVAL) {
          // Считываем значения
          temp_sens_1.read_value();
          moisture_soil_sens_1.read_value();
          moisture_air_sens_1.read_value();
          light_sens_1.read_value();
        

//          Sensor_GPIO sensors[] = {sensor1, sensor2, sensor3};  // Массив датчиков
//          bool result = calculate_average(sensors, sizeof(sensors)/sizeof(sensors[0]));
          high_temp = calculate_average(temp_sens_1);
          dry_soil_moisture = calculate_average(moisture_soil_sens_1);
          dry_air_moisture = calculate_average(moisture_air_sens_1);
          low_light_value = calculate_average(light_sens_1);
        
          
          temp_check();
          soil_moisture_check();
          air_moisture_check();
          light_level_check();
          
          // Актуаторы
          fan_actuator();
          temp_actuator();
          light_actuator();
          watering_actuator();
      }
  }

  time(current_time); // время
  
  
}
