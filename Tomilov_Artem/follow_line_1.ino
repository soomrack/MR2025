#define motor_left_pwm 6
#define motor_left_dir 7
#define motor_right_pwm 5
#define motor_right_dir 4

#define BUTTON_PIN 12

#define LEFT_SENSOR_PIN A0
#define RIGHT_SENSOR_PIN A1

float gain_p = 8.0;
float gain_d = 5.0;
int speed = 170;

int sensorL_min = 1023, sensorL_max = 0;  // magic numbers
int sensorR_min = 1023, sensorR_max = 0;  // also

int last_error = 0;
bool is_bot_active = false;


void move_motors(int leftSpeed, int rightSpeed) {
  digitalWrite(motor_left_dir, leftSpeed > 0);
  digitalWrite(motor_right_dir, rightSpeed > 0);
  analogWrite(motor_left_pwm, abs(leftSpeed));
  analogWrite(motor_right_pwm, abs(rightSpeed));
}


void sensor_calibration() { // Калибровка датчиков на белое и чёрное через нажатие кнопок

    pinMode(BUTTON_PIN, INPUT_PULLUP);
    pinMode(LEFT_SENSOR_PIN, INPUT);
    pinMode(R_SENS_PIN, INPUT);

    while (digitalRead(BUTTON_PIN)) {delay(50);}  // Ожидание первого нажатия клавиши
    while (!digitalRead(BUTTON_PIN)) {delay(50);} // Ожидание когда кнопку отпустят

    white_L = midArifm(LEFT_SENSOR_PIN); // Запись значения белого с левого датчика
    white_R = midArifm(RIGHT_SENSOR_PIN); // Запись значения белого с правого датчика
    
    while (digitalRead(BUTTON_PIN)) {delay(50);}  // Ожидание второго нажатия клавиши
    while (!digitalRead(BUTTON_PIN)) {delay(50);} // Ожидание когда кнопку отпустят

    black_L = midArifm(LEFT_SENSOR_PIN); // Запись значения чёрного с левого датчика
    black_R = midArifm(RIGHT_SENSOR_PIN); // Запись значения чёрного с правого датчика

    while (digitalRead(BUTTON_PIN)) {delay(50);}  // Ожидание третьего нажатия клавиши

    Serial.print(white_L);
    Serial.println(" ");
    Serial.println(white_R);

    Serial.print(black_L);
    Serial.println(" ");
    Serial.println(black_R);

    while (!digitalRead(BUTTON_PIN)) {delay(50);} // Ожидание когда кнопку отпустят
}


void line_following() {
  int value_left = map(analogRead(LEFT_SENSOR_PIN), sensorL_min, sensorL_max, 0, 100);
  int value_right = map(analogRead(RIGHT_SENSOR_PIN), sensorR_min, sensorR_max, 0, 100);

  float error = value_left - value_right;
  float correct = error * gain_p + (error - last_error) * gain_d;

  move_motors(constrain(speed + correct, -250, 250),
             constrain(speed - correct, -250, 250));

  last_error = error;
}


void setup() {
  pinMode(motor_left_pwm, OUTPUT);
  pinMode(motor_left_dir, OUTPUT);
  pinMode(motor_right_pwm, OUTPUT);
  pinMode(motor_right_dir, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // пинмоды на каждый, на button подаем 1

  sensor_calibration()

  while (digitalRead(BUTTON_PIN) == HIGH) {
  }

  active = true;
}

void loop() {
  if (is_bot_active) {
    line_following();
  }

  if (digitalRead(BUTTON_PIN) == HIGH && button_old == LOW) {
    active = !active;
  }
  button_old = digitalRead(BUTTON_PIN);
}
