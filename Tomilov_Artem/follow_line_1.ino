int motorL_pwm = 6;
int motorL_dir = 7;
int motorR_pwm = 5;
int motorR_dir = 4;

float gain_p = 8.0;
float gain_d = 5.0;
int speed = 150;

int sensorL_min = 1023, sensorL_max = 0;
int sensorR_min = 1023, sensorR_max = 0;

int last_error = 0;
bool active = false;
bool button_old = HIGH;

void moveMotors(int leftSpeed, int rightSpeed) {
  digitalWrite(motorL_dir, leftSpeed > 0);
  digitalWrite(motorR_dir, rightSpeed > 0);
  analogWrite(motorL_pwm, abs(leftSpeed));
  analogWrite(motorR_pwm, abs(rightSpeed));
}

void calibrate() {
  unsigned long time_start = millis();
  while (millis() - time_start < 6000) {
    moveMotors(120, -120);  // вращение на месте
    int left = analogRead(A0);
    int right = analogRead(A1);

    if (left < sensorL_min) sensorL_min = left;
    if (left > sensorL_max) sensorL_max = left;
    if (right < sensorR_min) sensorR_min = right;
    if (right > sensorR_max) sensorR_max = right;
  }
  moveMotors(0, 0);
}

void line_following() {
  int value_left = map(analogRead(A0), sensorL_min, sensorL_max, 0, 100);
  int value_right = map(analogRead(A1), sensorR_min, sensorR_max, 0, 100);

  float error = value_left - value_right;
  float correct = error * gain_p + (error - last_error) * gain_d;

  moveMotors(constrain(speed + correct, -250, 250),
             constrain(speed - correct, -250, 250));

  last_error = error;
}

void setup() {
  pinMode(motorL_pwm, OUTPUT);
  pinMode(motorL_dir, OUTPUT);
  pinMode(motorR_pwm, OUTPUT);
  pinMode(motorR_dir, OUTPUT);
  pinMode(A2, INPUT_PULLUP);  // пинмоды на каждый, на A2 подаем 1

  calibrate();

  while (digitalRead(A2) == HIGH) {
    active = false;
  }

  active = true;
}

void loop() {
  if (active) {
    line_following();
  }

  if (digitalRead(A2) == HIGH && button_old == LOW) {
    active = !active;
  }
  button_old = digitalRead(A2);
}
