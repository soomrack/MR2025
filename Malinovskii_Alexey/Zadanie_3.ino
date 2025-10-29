#include <dht.h>

dht envSensor;

#define DHT_PIN 8
#define LIGHT_SENSOR_PIN A0
#define MOISTURE_SENSOR_PIN A1
#define LED_GROW_PIN 6
#define WATER_PUMP_PIN 5
#define HEATER_PIN 3
#define FAN_PIN 7

const int OPT_TEMP            = 25;
const int OPT_HUMIDITY        = 50;
const int OPT_MOISTURE        = 400;
const int OPT_LIGHT           = 500;

const int TEMP_TOLERANCE      = 2;
const int HUMID_TOLERANCE     = 2;
const int MOIST_TOLERANCE     = 2;
const int LIGHT_TOLERANCE     = 2;

const long PUMP_RUN_DURATION   = 5000;
const long PUMP_PAUSE_DURATION = 30000;

bool pumpIsOn = false;
unsigned long pumpStartTime = 0;
unsigned long pumpPauseTime = 0;

int smoothRead(int pin, int samples = 5) {
  long sum = 0;
  for (int i = 0; i < samples; i++) {
    sum += analogRead(pin);
    delay(2);
  }
  return sum / samples;
}

void setup() {
  Serial.begin(115200);

  pinMode(LED_GROW_PIN, OUTPUT);
  pinMode(WATER_PUMP_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  pinMode(FAN_PIN, OUTPUT);

  digitalWrite(LED_GROW_PIN, LOW);
  digitalWrite(WATER_PUMP_PIN, HIGH);
  digitalWrite(HEATER_PIN, LOW);
  digitalWrite(FAN_PIN, LOW);
}

void readSensors(int* temp, int* humidity, int* light, int* moisture) {
  int err = envSensor.read11(DHT_PIN);
  if (err != DHTLIB_OK) {
    *temp = -1;
    *humidity = -1;
  } else {
    *temp = envSensor.temperature;
    *humidity = envSensor.humidity;
  }

  *light = smoothRead(LIGHT_SENSOR_PIN);
  *moisture = smoothRead(MOISTURE_SENSOR_PIN);
}

void controlLight(int light) {
  if (light < OPT_LIGHT - LIGHT_TOLERANCE) {
    digitalWrite(LED_GROW_PIN, HIGH);
  } else if (light > OPT_LIGHT + LIGHT_TOLERANCE) {
    digitalWrite(LED_GROW_PIN, LOW);
  }
}

void controlWater(int moisture) {
  if (moisture < OPT_MOISTURE - MOIST_TOLERANCE) {
    if (!pumpIsOn && (millis() - pumpPauseTime >= PUMP_PAUSE_DURATION)) {
      digitalWrite(WATER_PUMP_PIN, LOW);
      pumpIsOn = true;
      pumpStartTime = millis();
      Serial.println("Pump ON");
    }
  }

  if (pumpIsOn && (millis() - pumpStartTime >= PUMP_RUN_DURATION)) {
    digitalWrite(WATER_PUMP_PIN, HIGH);
    pumpIsOn = false;
    pumpPauseTime = millis();
    Serial.println("Pump OFF");
  }

  if (moisture >= OPT_MOISTURE) {
    digitalWrite(WATER_PUMP_PIN, HIGH);
    pumpIsOn = false;
    pumpStartTime = 0;
    pumpPauseTime = 0;
  }
}

void controlTemperature(int temp) {
  if (temp < OPT_TEMP - TEMP_TOLERANCE) {
    digitalWrite(HEATER_PIN, HIGH);
  } else if (temp >= OPT_TEMP) {
    digitalWrite(HEATER_PIN, LOW);
  }
}

void controlVentilation(int temp, int humidity) {
  if (humidity > OPT_HUMIDITY + HUMID_TOLERANCE || temp > OPT_TEMP + TEMP_TOLERANCE) {
    digitalWrite(FAN_PIN, HIGH);
  } else {
    digitalWrite(FAN_PIN, LOW);
  }
}

void logData(int temp, int humidity, int light, int moisture) {
  Serial.print("T: ");
  Serial.print(temp);
  Serial.print("C, H: ");
  Serial.print(humidity);
  Serial.print("%, Light: ");
  Serial.print(light);
  Serial.print(", Moisture: ");
  Serial.println(moisture);
}

void loop() {
  static unsigned long lastMillis = 0;
  const long interval = 1000;

  if (millis() - lastMillis >= interval) {
    lastMillis = millis();

    int temp, humidity, light, moisture;
    readSensors(&temp, &humidity, &light, &moisture);

    controlLight(light);
    controlWater(moisture);
    controlTemperature(temp);
    controlVentilation(temp, humidity);

    logData(temp, humidity, light, moisture);
  }
}
