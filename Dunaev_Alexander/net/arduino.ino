const int LED_PIN = 13; // Встроенный светодиод

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    
    if (command == '1') {
      digitalWrite(LED_PIN, HIGH);
      Serial.println("LED ON");
    } 
    else if (command == '0') {
      digitalWrite(LED_PIN, LOW);
      Serial.println("LED OFF");
    }
  }
}
