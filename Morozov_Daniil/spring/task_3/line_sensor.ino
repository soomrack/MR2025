const int lineSensorPin = A0;
const int threshold = 500;

void setup() {
  pinMode(lineSensorPin, INPUT);
  Serial.begin(9600);
}

void loop() {
  int sensorValue = analogRead(lineSensorPin);
  String status = (sensorValue > threshold) ? "BLACK (Line)" : "WHITE (Floor)";
  
  // Отправляем обычный текст! Никаких бинарных данных.
  Serial.print("Line sensor: ");
  Serial.print(sensorValue);
  Serial.print(" [");
  Serial.print(status);
  Serial.println("]");
  
  delay(1000);
}
