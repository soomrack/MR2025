// =============================================================
// Пины моторов
// =============================================================
#define MOTOR_LEFT_PWM_PIN 6
#define MOTOR_LEFT_DIR_PIN 7
#define MOTOR_RIGHT_PWM_PIN 5
#define MOTOR_RIGHT_DIR_PIN 4

#define LEFT_SENSOR_PIN A0
#define RIGHT_SENSOR_PIN A1
#define BUTTON_PIN 12
#define SOUND_PIN 8

// =============================================================
// Параметры управления
// =============================================================
float gain_p = 8.0;
float gain_d = 5.0;
int base_speed = 90;
int search_speed = 110;
int light_threshold = 50;
int spiral_increase = 3;
int spiral_interval = 50;
int search_timeout = 10000;

// =============================================================
// Служебные переменные
// =============================================================
int left_min = 1023, left_max = 0;
int right_min = 1023, right_max = 0;
double last_error = 0;
int last_direction = 0;

bool is_active = false;
bool button_old = HIGH;
bool searching = false;
bool spiral_direction = false;
int spiral_step = 0;
unsigned long spiral_timer = 0;
unsigned long search_start_time = 0;

// Сенсоры
int sensor_left = 0;
int sensor_right = 0;

// Таймер для Serial
unsigned long last_serial_time = 0;
const unsigned long serial_interval = 200;

// =============================================================
// Состояния робота
// =============================================================
enum RobotState { IDLE, FOLLOW_LINE, SEARCH_LINE };
RobotState state = IDLE;

// =============================================================
// Управление моторами
// =============================================================
void drive(int left_speed, int right_speed) {
  digitalWrite(MOTOR_LEFT_DIR_PIN, left_speed > 0);
  digitalWrite(MOTOR_RIGHT_DIR_PIN, right_speed > 0);
  analogWrite(MOTOR_LEFT_PWM_PIN, abs(left_speed));
  analogWrite(MOTOR_RIGHT_PWM_PIN, abs(right_speed));
}

// =============================================================
// Остановка робота
// =============================================================
void stop_robot() {
  drive(0, 0);
  is_active = false;
  searching = false;
  state = IDLE;
  tone(SOUND_PIN, 500, 500);
}

// =============================================================
// Калибровка сенсоров
// =============================================================
void calibrate_sensors() {
  Serial.println("Калибровка сенсоров...");
  unsigned long start_time = millis();
  while (millis() - start_time < 4000) {
    drive(150, -150);
    int left = analogRead(LEFT_SENSOR_PIN);
    int right = analogRead(RIGHT_SENSOR_PIN);
    if (left < left_min) left_min = left;
    if (left > left_max) left_max = left;
    if (right < right_min) right_min = right;
    if (right > right_max) right_max = right;
  }
  drive(0, 0);
  Serial.println("Калибровка завершена");
}

// =============================================================
// Обновление сенсоров
// =============================================================
void update_sensors() {
  sensor_left = map(analogRead(LEFT_SENSOR_PIN), left_min, left_max, 0, 100);
  sensor_right = map(analogRead(RIGHT_SENSOR_PIN), right_min, right_max, 0, 100);
}

// =============================================================
// Управление кнопкой
// =============================================================
void handle_button() {
  bool button_state = digitalRead(BUTTON_PIN);
  if (button_state == LOW && button_old == HIGH) {
    is_active = !is_active;
    state = is_active ? FOLLOW_LINE : IDLE;
    searching = false;
  }
  button_old = button_state;
}

// =============================================================
// Следование за линией
// =============================================================
void follow_line() {
  double error = sensor_left - sensor_right;
  double correction = error * gain_p + (error - last_error) * gain_d;
  drive(constrain(base_speed + correction, -250, 250),
        constrain(base_speed - correction, -250, 250));
  last_error = error;

  if (sensor_left < light_threshold && sensor_right < light_threshold) {
    searching = true;
    spiral_step = 0;
    spiral_timer = millis();
    spiral_direction = (last_direction == 0);
    search_start_time = millis();
    state = SEARCH_LINE;
  }
}

// =============================================================
// Поиск линии по спирали
// =============================================================
void search_line() {
  if (millis() - spiral_timer > spiral_interval) {
    spiral_step += spiral_increase;
    spiral_timer = millis();
  }

  int left_speed = search_speed - spiral_step;
  int right_speed = search_speed - spiral_step;

  if (spiral_direction) drive(left_speed, search_speed);
  else drive(search_speed, right_speed);

  if (sensor_left > light_threshold || sensor_right > light_threshold) {
    searching = false;
    last_direction = (sensor_left > light_threshold) ? 0 : 1;
    state = FOLLOW_LINE;
  }

  if (millis() - search_start_time > search_timeout) {
    stop_robot();
  }
}

// =============================================================
// Менеджер состояния робота
// =============================================================
void update_state() {
  handle_button();
  update_sensors();

  switch (state) {
    case IDLE:
      drive(0, 0);
      break;
    case FOLLOW_LINE:
      follow_line();
      break;
    case SEARCH_LINE:
      search_line();
      break;
  }
}

// =============================================================
// Отладка
// =============================================================
void serial_debug() {
  if (millis() - last_serial_time >= serial_interval) {
    Serial.print("Left: "); Serial.print(sensor_left);
    Serial.print(" | Right: "); Serial.print(sensor_right);
    Serial.print(" | Last Error: "); Serial.print(last_error);
    Serial.print(" | State: ");
    switch(state) {
      case IDLE: Serial.println("IDLE"); break;
      case FOLLOW_LINE: Serial.println("FOLLOW_LINE"); break;
      case SEARCH_LINE: Serial.println("SEARCH_LINE"); break;
    }
    last_serial_time = millis();
  }
}


void setup() {
  Serial.begin(9600);
  pinMode(MOTOR_LEFT_PWM_PIN, OUTPUT);
  pinMode(MOTOR_LEFT_DIR_PIN, OUTPUT);
  pinMode(MOTOR_RIGHT_PWM_PIN, OUTPUT);
  pinMode(MOTOR_RIGHT_DIR_PIN, OUTPUT);
  pinMode(LEFT_SENSOR_PIN, INPUT);
  pinMode(RIGHT_SENSOR_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SOUND_PIN, OUTPUT);

  calibrate_sensors();
  Serial.println("Нажмите кнопку для старта");
}


void loop() {
  update_state();   // менеджер состояний
  serial_debug();   // вывод отладочной информации
}
