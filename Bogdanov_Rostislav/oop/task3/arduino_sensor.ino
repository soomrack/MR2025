// arduino_serial_sensor.ino
const int WARNING_THRESHOLD = 680;   // аналоговое значение выше которого Warning

int measurementCount = 0;            // счётчик измерений

void setup() {
  Serial.begin(9600);
  pinMode(A0, INPUT);
}

void loop() {
  measurementCount++;
  
  int sensorValue = analogRead(A0);
  bool boosted = false;              // флаг, было ли значение увеличено

  // Каждое 10-е или 15-е значение увеличиваем в 10 раз
  if (measurementCount % 10 == 0 || measurementCount % 15 == 0) {
    sensorValue = sensorValue * 10;
    boosted = true;
  }
  
  float voltage = sensorValue * (5.0 / 1023.0);
  
  // Обычный лог
  Serial.print("LOG: analog=");
  Serial.print(sensorValue);
  if (boosted) {
    Serial.print(" (boosted x10)");
  }
  Serial.print(", voltage=");
  Serial.print(voltage);
  Serial.println(" V");
  
  // Предупреждение при превышении порога
  if (sensorValue > WARNING_THRESHOLD) {
    Serial.print("WARNING: High value! analog=");
    Serial.print(sensorValue);
    if (boosted) {
      Serial.print(" (artificially increased)");
    }
    Serial.print(", voltage=");
    Serial.print(voltage);
    Serial.println(" V");
  }
  
  delay(2000);
}