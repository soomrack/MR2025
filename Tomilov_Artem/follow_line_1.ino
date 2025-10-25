#define motor_left_pwm 6
#define motor_left_dir 7
#define motor_right_pwm 5
#define motor_right_dir 4
#define sound_pin 9
#define button_pin A2           // кнопка для старта/паузы
#define sensor_left_pin A0      // левый датчик линии
#define sensor_right_pin A1     // правый датчик линии

#define gain_p 8.0
#define gain_d 5.0
#define base_speed 70
#define search_speed 90

int sensorL_min = 1023, sensorL_max = 0;
int sensorR_min = 1023, sensorR_max = 0;

int last_error = 0;
int last_dir = 0;

bool is_bot_active = false;
bool button_old = HIGH;
bool is_searching = false;
bool spiral_dir = false;

int light_threshold_L = 50;
int light_threshold_R = 50;


void move_motors(int leftSpeed, int rightSpeed) {
  digitalWrite(motor_left_dir, leftSpeed > 0);
  digitalWrite(motor_right_dir, rightSpeed > 0);
  analogWrite(motor_left_pwm, abs(leftSpeed));
  analogWrite(motor_right_pwm, abs(rightSpeed));

  Serial.print("Motors -> L:");
  Serial.print(leftSpeed);
  Serial.print(" R:");
  Serial.println(rightSpeed);
}


void sensor_calibration() {
  Serial.println("Calibration");

  // Калибровка фона
  Serial.println("Place robot on background");
  delay(2000);
  sensorL_max = analogRead(sensor_left_pin);
  sensorR_max = analogRead(sensor_right_pin);
  Serial.print("Background L:");
  Serial.print(sensorL_max);
  Serial.print(" R:");
  Serial.println(sensorR_max);

  // Калибровка линии
  Serial.println("Place robot on line");
  delay(2000);
  sensorL_min = analogRead(sensor_left_pin);
  sensorR_min = analogRead(sensor_right_pin);
  Serial.print("Line L:");
  Serial.print(sensorL_min);
  Serial.print(" R:");
  Serial.println(sensorR_min);

  // Автоматический порог
  light_threshold_L = (sensorL_min + sensorL_max) / 2;
  light_threshold_R = (sensorR_min + sensorR_max) / 2;

  Serial.print("Threshold L:");
  Serial.print(light_threshold_L);
  Serial.print(" R:");
  Serial.println(light_threshold_R);
}


void start_search() {
  is_searching = true;
  spiral_dir = (last_dir == 0) ? 1 : 0;
  Serial.print("Search start ");
  Serial.println(spiral_dir ? "R" : "L");
}


void spiral_search() {
  int t = (millis() / 200) % 100; // плавный поворот
  int bias = t * 2;               // больше bias → больший радиус

  int left = constrain(search_speed - bias, 0, search_speed);
  int right = search_speed;

  if (spiral_dir) move_motors(left, right);
  else move_motors(right, left);

  Serial.print("Spiral bias:");
  Serial.println(bias);
}


void line_following(int sL, int sR) {
  float err = sL - sR;
  float corr = err * gain_p + (err - last_error) * gain_d;
  move_motors(constrain(base_speed + corr, -250, 250),
              constrain(base_speed - corr, -250, 250));
  last_error = err;

  Serial.print("Follow L:");
  Serial.print(sL);
  Serial.print(" R:");
  Serial.print(sR);
  Serial.print(" Err:");
  Serial.print(err);
  Serial.print(" Corr:");
  Serial.println(corr);
}


void check_sensors() {
  int valL = map(analogRead(sensor_left_pin), sensorL_min, sensorL_max, 0, 100);
  int valR = map(analogRead(sensor_right_pin), sensorR_min, sensorR_max, 0, 100);

  Serial.print("Sensors L:");
  Serial.print(valL);
  Serial.print(" R:");
  Serial.println(valR);

  if (is_searching) {
    if (valL > light_threshold_L || valR > light_threshold_R) {
      is_searching = false;
      last_dir = (valL > light_threshold_L) ? 0 : 1;
      Serial.println("Line found");
    } else spiral_search();
  } else {
    if (valL < light_threshold_L && valR < light_threshold_R) {
      Serial.println("Line lost");
      start_search();
    } else line_following(valL, valR);
  }
}


void setup() {
  Serial.begin(9600);
  Serial.println("Setup");

  pinMode(motor_left_pwm, OUTPUT);
  pinMode(motor_left_dir, OUTPUT);
  pinMode(motor_right_pwm, OUTPUT);
  pinMode(motor_right_dir, OUTPUT);
  pinMode(sound_pin, OUTPUT);
  pinMode(button_pin, INPUT_PULLUP);

  sensor_calibration();

  Serial.println("Wait button");
  while (digitalRead(button_pin) == HIGH) {}

  tone(sound_pin, 1000, 200);
  delay(200);
  Serial.println("Ready");

  is_bot_active = true;
}


void loop() {
  bool button_state = digitalRead(button_pin);

  if (button_state == LOW && button_old == HIGH) {
    is_bot_active = !is_bot_active;
    is_searching = false;
    delay(30);
    Serial.print("Button ");
    Serial.println(is_bot_active ? "ON" : "OFF");
  }

  button_old = button_state;

  if (is_bot_active) check_sensors();
}
