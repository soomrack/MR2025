#define motor_left_pwm 6
#define motor_left_dir 7
#define motor_right_pwm 5
#define motor_right_dir 4
#define BUTTON_PIN 12
#define LEFT_SENSOR_PIN A0
#define RIGHT_SENSOR_PIN A1

float gain_p = 8.0;
float gain_d = 5.0;
int speed = 180;

int sensorL_min = 1023, sensorL_max = 0;  // magic numbers
int sensorR_min = 1023, sensorR_max = 0;  // also

int last_error = 0;
bool is_bot_active = false;
bool button_old = HIGH;

int white_L, white_R, black_L, black_R;
int tresholdL, tresholdR;


void move_motors(int leftSpeed, int rightSpeed) {
  digitalWrite(motor_left_dir, leftSpeed > 0);
  digitalWrite(motor_right_dir, rightSpeed > 0);
  analogWrite(motor_left_pwm, abs(leftSpeed));
  analogWrite(motor_right_pwm, abs(rightSpeed));
}


int midArifm(int pin) {
  long sum = 0;
  for (int i = 0; i < 20; i++)
    sum += analogRead(pin);
  return (sum / 20);
}


void sensor_calibration() {
  Serial.println("Calibration started");

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LEFT_SENSOR_PIN, INPUT);
  pinMode(RIGHT_SENSOR_PIN, INPUT);

  while (digitalRead(BUTTON_PIN)) {delay(10);}  // Ожидание первого нажатия клавиши
  while (!digitalRead(BUTTON_PIN)) {delay(10);} // Ожидание когда кнопку отпустят

  white_L = midArifm(LEFT_SENSOR_PIN); // Запись значения белого с левого датчика
  white_R = midArifm(RIGHT_SENSOR_PIN); // Запись значения белого с правого датчика
    
  while (digitalRead(BUTTON_PIN)) {delay(10);}  // Ожидание второго нажатия клавиши
  while (!digitalRead(BUTTON_PIN)) {delay(10);} // Ожидание когда кнопку отпустят

  black_L = midArifm(LEFT_SENSOR_PIN); // Запись значения чёрного с левого датчика
  black_R = midArifm(RIGHT_SENSOR_PIN); // Запись значения чёрного с правого датчика

  while (digitalRead(BUTTON_PIN)) {delay(10);}  // Ожидание третьего нажатия клавиши

  sensorL_min = black_L;  // минимальное значение (тёмная линия)
  sensorL_max = white_L;  // максимальное значение (светлый фон)
  sensorR_min = black_R;
  sensorR_max = white_R;

  float threshold_ratio = 0.05; // 5% от диапазона
  tresholdL = black_L + (white_L - black_L) * threshold_ratio;
  tresholdR = black_R + (white_R - black_R) * threshold_ratio;


  Serial.print("L: "); Serial.print(sensorL_min); Serial.print(" - "); Serial.println(sensorL_max);
  Serial.print("R: "); Serial.print(sensorR_min); Serial.print(" - "); Serial.println(sensorR_max);
  Serial.print("Threshold L: "); Serial.println(tresholdL);
  Serial.print("Threshold R: "); Serial.println(tresholdR);
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


bool is_line_lost() {
  int value_left = map(analogRead(LEFT_SENSOR_PIN), sensorL_min, sensorL_max, 0, 100);
  int value_right = map(analogRead(RIGHT_SENSOR_PIN), sensorR_min, sensorR_max, 0, 100);

  return (value_left < tresholdL && value_right < tresholdR);
}


void searching_line() {
  for (int i = 0; i < 3; i++) {
    if (last_error > 0) {
      move_motors(80, -80);
    }
    else if (last_error < 0) {
      move_motors(-80, 80);
    }
    else {
      move_motors(-80, -80);
    }

    delay(150);    // короткий поворот/движение
    move_motors(0,0); // остановка

    if (!is_line_lost()) break; // линия найдена, выход из цикла
  }
}


void setup() {
  Serial.begin(9600);

  pinMode(motor_left_pwm, OUTPUT);
  pinMode(motor_left_dir, OUTPUT);
  pinMode(motor_right_pwm, OUTPUT);
  pinMode(motor_right_dir, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);  // пинмоды на каждый, на button подаем 1

  sensor_calibration();

  while (digitalRead(BUTTON_PIN) == HIGH) {}

  is_bot_active = true;
}

void loop() {
  bool button_state = digitalRead(BUTTON_PIN);

  if (button_state == LOW && button_old == HIGH) {
    is_bot_active = !is_bot_active;
    delay(30);
  }

  button_old = button_state;

  if (is_bot_active) {
    if (is_line_lost()) searching_line();
    else line_following();
  }
}
