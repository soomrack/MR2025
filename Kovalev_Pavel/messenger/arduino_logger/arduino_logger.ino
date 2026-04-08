#define SENSOR_0_PIN A0
#define SENSOR_1_PIN A1

long i = 0;

void setup() {
  Serial.begin(9600);
}

void loop() {
//  Serial.println("iteration #" + String(i));
//  Serial.print(i);
//  Serial.print(", ");
  Serial.print(analogRead(SENSOR_0_PIN));
  Serial.print(", ");
  Serial.println(analogRead(SENSOR_1_PIN));
  i++;
  delay(500);
}
