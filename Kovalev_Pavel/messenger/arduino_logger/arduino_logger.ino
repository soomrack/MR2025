long i = 0;

void setup() {
  Serial.begin(9600);
}

void loop() {
  Serial.println("iteration #" + String(i));
  i++;
  delay(2000);
}
