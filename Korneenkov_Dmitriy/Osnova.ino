#include <DHT.h>

// Пины датчиков
#define PHOTOSENSOR         A0
#define GROUND_SENSOR       A1
#define SOIL_SENSOR_PIN     A3
#define DHT_PIN             12

// Пины реле
#define RELAY_PIN_LED       6
#define RELAY_PIN_PUMP      5
#define RELAY_PIN_HEAT      4
#define RELAY_PIN_VENT      7

// Тип датчика DHT
#define DHT_TYPE            DHT11

// Пороговые значения
#define LIGHT_THRESHOLD     500
#define TEMP_THRESHOLD      16
#define MOISTURE_THRESHOLD  200
#define READ_INTERVAL       50


DHT dht(DHT_PIN, DHT_TYPE);

unsigned long previousTime = 0;
unsigned int lightData = 0;
unsigned int sensorValue = 0;
  unsigned int moisturePercent = 0;
  int endtime = 5000;
unsigned long previousMillis = 0;
bool PumpState = LOW;
const long onTime = 3000;
const long offTime  = 10000;
  

void setup() {
  Serial.begin(9600);
  dht.begin();
  
  // Настройка пинов датчиков
  pinMode(PHOTOSENSOR, INPUT);
  pinMode(GROUND_SENSOR, INPUT);
  pinMode(SOIL_SENSOR_PIN, INPUT);
  
  // Настройка пинов реле (HIGH - выключено, LOW - включено)
  pinMode(RELAY_PIN_LED, OUTPUT);
  pinMode(RELAY_PIN_PUMP, OUTPUT);
  pinMode(RELAY_PIN_VENT, OUTPUT);
  pinMode(RELAY_PIN_HEAT, OUTPUT);
  
  // Изначально все выключено
  digitalWrite(RELAY_PIN_LED, HIGH);
  digitalWrite(RELAY_PIN_PUMP, LOW);
  digitalWrite(RELAY_PIN_VENT, HIGH);
  digitalWrite(RELAY_PIN_HEAT, HIGH);
  
  Serial.println("Теплица запущена!");
}

void loop() {
  // Чтение температуры и влажности воздуха
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  
  // Управление освещением
  controlLighting();
  
  // Управление климатом
  controlClimate(temperature);
  
  // Управление поливом (только при низкой температуре)
  
    controlWatering();
  
  
  // Вывод данных в монитор порта
  printSensorData(temperature, humidity);
  
  delay(100); // Небольшая задержка для стабильности
}

void controlClimate(float temperature) {
  if (temperature > TEMP_THRESHOLD) {
    // Включить вентиляцию и выключить обогрев
    digitalWrite(RELAY_PIN_VENT, HIGH);   // Вентиляция включена
    digitalWrite(RELAY_PIN_HEAT, LOW);  // Обогрев выключен
  } else {
    // Выключить вентиляцию и включить обогрев
    digitalWrite(RELAY_PIN_VENT, HIGH);  // Вентиляция выключена
    digitalWrite(RELAY_PIN_HEAT, HIGH);   // Обогрев включен
  }
}

void controlLighting() {
  // Чтение данных с фотосенсора каждые 50 мс
  if (millis() - previousTime >= READ_INTERVAL) {
    previousTime = millis();
    lightData = analogRead(PHOTOSENSOR);
  }
  
  // Включение света при недостаточной освещенности
  if (lightData > LIGHT_THRESHOLD) {
    digitalWrite(RELAY_PIN_LED, HIGH); // Свет выключен
  } else {
    digitalWrite(RELAY_PIN_LED, LOW);  // Свет включен
  }
}


void controlWatering() {
   sensorValue = analogRead(SOIL_SENSOR_PIN);
   
  // Включение полива при сухой почве
  unsigned long currentMillis = millis();
  if ( (PumpState == HIGH) && (currentMillis - previousMillis >= onTime)) 
  {
    PumpState = LOW;
    previousMillis = currentMillis;
    digitalWrite(RELAY_PIN_PUMP, LOW);   // Насос включен
  } else if ((sensorValue > MOISTURE_THRESHOLD) && (PumpState == LOW) && (currentMillis - previousMillis >= offTime)) {
    PumpState = HIGH;
    previousMillis = currentMillis;

    digitalWrite(RELAY_PIN_PUMP, HIGH);
    
      // Насос выключен
  }
}

void printSensorData(float temperature, float humidity)
 {
  Serial.print("Температура: ");
  Serial.print(temperature);
  Serial.print("°C | Влажность воздуха: ");
  Serial.print(humidity);
  Serial.print("% | Освещенность: ");
  Serial.print(lightData);
  Serial.print(" | Влажность почвы:  ");
  Serial.println(sensorValue);

 
  
}