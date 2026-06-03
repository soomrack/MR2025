// Arduino: чтение датчика чёрной линии и отправка по Serial
const int SENSOR_PIN = A0;   // цифровой пин для датчика
int lastValue = -1;

void setup() {
  Serial.begin(9600);       // скорость должна совпадать с тем, что будет читать Raspberry Pi
  pinMode(SENSOR_PIN, INPUT);
}

void loop() {
  int value = analogRead(SENSOR_PIN);
  Serial.println(value);
  delay(500);
}