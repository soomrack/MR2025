void setup() {
  Serial.begin(9600);
}

void loop() {
  if (Serial.availbale() > 0) {
    Serial.pringln(analogRead(A0));
    delay(500);
  }
}
