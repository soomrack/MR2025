#define LEFT_MOTOR_PWM 6
#define LEFT_MOTOR_DIR 7
#define RIGHT_MOTOR_PWM 5
#define RIGHT_MOTOR_DIR 4

#define LEFT_SENSOR A0
#define RIGHT_SENSOR A1
#define BUTTON_PIN 12
#define SOUND_PIN 8

float Kp = 8.0;
float Kd = 5.0;
int base_speed = 150;
int search_speed = 110;
int light_threshold = 50;
int spiral_increase = 3;
int spiral_interval = 50;
int search_timeout = 10000;

int white_L; 
int black_L;
int white_R;
int black_R;
int last_error = 0;
int last_direction = 0;

bool is_active = false;
bool button_old = HIGH;
bool searching = false;
bool spiral_direction = false;
int spiral_step = 0;
unsigned long spiral_timer = 0;
unsigned long search_start_time = 0;

int sensor_left = 0;
int sensor_right = 0;

unsigned long last_serial_time = 0;
const unsigned long serial_interval = 200;

enum RobotState { IDLE, FOLLOW_LINE, SEARCH_LINE };
RobotState state = IDLE;


void init_motors() {
  pinMode(LEFT_MOTOR_PWM, OUTPUT);
  pinMode(LEFT_MOTOR_DIR, OUTPUT);
  pinMode(RIGHT_MOTOR_PWM, OUTPUT);
  pinMode(RIGHT_MOTOR_DIR, OUTPUT);
  pinMode(LEFT_SENSOR, INPUT);
  pinMode(RIGHT_SENSOR, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SOUND_PIN, OUTPUT);
}


void set_speed(int left_speed, int right_speed) {
  digitalWrite(LEFT_MOTOR_DIR, left_speed > 0);
  digitalWrite(RIGHT_MOTOR_DIR, right_speed > 0);
  analogWrite(LEFT_MOTOR_PWM, abs(left_speed));
  analogWrite(RIGHT_MOTOR_PWM, abs(right_speed));
}


void stop_robot() {
  set_speed(0, 0);
  is_active = false;
  searching = false;
  state = IDLE;
  tone(SOUND_PIN, 500, 500);
}


int mid_arifm(int pin) {
  long sum = 0;                       
  for (int i = 0; i < 20; i++) {
    sum += analogRead(pin);       
  }           
  return (sum / 20.0);
}


void sensor_calibration(){ // Калибровка датчиков на белое и чёрное через нажатие кнопок
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LEFT_SENSOR, INPUT);
  pinMode(RIGHT_SENSOR, INPUT);

  while (digitalRead(BUTTON_PIN)) {delay(50);}  // Ожидание первого нажатия клавиши
  while (!digitalRead(BUTTON_PIN)) {delay(50);} // Ожидание когда кнопку отпустят

  white_L = mid_arifm(LEFT_SENSOR); // Запись значения белого с левого датчика
  white_R = mid_arifm(RIGHT_SENSOR); // Запись значения белого с правого датчика
  
  while (digitalRead(BUTTON_PIN)) {delay(50);}  // Ожидание второго нажатия клавиши
  while (!digitalRead(BUTTON_PIN)) {delay(50);} // Ожидание когда кнопку отпустят

  black_L = mid_arifm(LEFT_SENSOR); // Запись значения чёрного с левого датчика
  black_R = mid_arifm(RIGHT_SENSOR); // Запись значения чёрного с правого датчика

  while (digitalRead(BUTTON_PIN)) {delay(50);}  // Ожидание третьего нажатия клавиши

  Serial.print(white_L);
  Serial.println(" ");
  Serial.println(white_R);

  Serial.print(black_L);
  Serial.println(" ");
  Serial.println(black_R);

  while (!digitalRead(BUTTON_PIN)) {delay(50);} // Ожидание когда кнопку отпустят
}


void update_sensors() {
  sensor_left = map(analogRead(LEFT_SENSOR), white_L, black_L, 0, 100);
  sensor_right = map(analogRead(RIGHT_SENSOR), white_R, black_R, 0, 100);
}


void handle_button() {
  bool button_state = digitalRead(BUTTON_PIN);
  if (button_state == LOW && button_old == HIGH) {
    is_active = !is_active;
    state = is_active ? FOLLOW_LINE : IDLE;
    searching = false;
  }
  button_old = button_state;
}


void set_searching_state() {
  searching = true;
  spiral_step = 0;
  spiral_timer = millis();
  spiral_direction = (last_direction == 0);
  search_start_time = millis();
  state = SEARCH_LINE;
}


void set_line_following_state(){
  searching = false;
  last_direction = (sensor_left > light_threshold) ? 0 : 1;
  state = FOLLOW_LINE;
}


void follow_line() {
  int error = sensor_left - sensor_right;
  int correction = error * Kp + (error - last_error) * Kd;
  set_speed(constrain(base_speed + correction, -250, 250),
        constrain(base_speed - correction, -250, 250));
  last_error = error;

  if (sensor_left < light_threshold && sensor_right < light_threshold) {
    set_searching_state();
  }
}


void search_line() {
  if (millis() - spiral_timer > spiral_interval) {
    spiral_step += spiral_increase;
    spiral_timer = millis();
  }

  int left_speed = search_speed - spiral_step;
  int right_speed = search_speed - spiral_step;

  if (spiral_direction) set_speed(left_speed, search_speed);
  else set_speed(search_speed, right_speed);

  if (sensor_left > light_threshold || sensor_right > light_threshold) {
    set_line_following_state();
  }

  if (millis() - search_start_time > search_timeout) {
    stop_robot();
  }
}


void setup() {
  Serial.begin(9600);
  init_motors();
  sensor_calibration();
}


void loop() {
  handle_button();
  update_sensors();

  switch (state) {
    case IDLE:
      set_speed(0, 0);
      break;
    case FOLLOW_LINE:
      follow_line();
      break;
    case SEARCH_LINE:
      search_line();
      break;
  }
}
