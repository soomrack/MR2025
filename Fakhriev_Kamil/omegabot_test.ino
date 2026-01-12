// ===========================================
// ПИНЫ ПОДКЛЮЧЕНИЯ КОМПОНЕНТОВ
// ===========================================
#define POWER_LEFT_MOTOR_PIN 6
#define DIRECTION_LEFT_MOTOR_PIN 7
#define POWER_RIGHT_MOTOR_PIN 5
#define DIRECTION_RIGHT_MOTOR_PIN 4
#define SOUND_PIN 9
#define CALIBRATE_BUTTON_PIN A2
#define LEFT_SENSOR_PIN A0
#define RIGHT_SENSOR_PIN A1

// ===========================================
// НАСТРОЙКИ ПД-РЕГУЛЯТОРА
// ===========================================
#define PID_GAIN_P 8.0
#define PID_GAIN_D 6.0
#define BASE_SPEED 90
#define SEARCH_SPEED 140
#define MAX_MOTOR_SPEED 250

// ===========================================
// НАСТРОЙКИ ДАТЧИКОВ И ПОИСКА
// ===========================================
#define LIGHT_THRESHOLD 50
#define SPIRAL_INCREASE 3
#define SPIRAL_INTERVAL 50
#define SEARCH_TIMEOUT 20000
#define DEBOUNCE_DELAY 50
#define CALIBRATION_TIME 4000

// ===========================================
// ПЕРЕЧИСЛЕНИЕ СОСТОЯНИЙ РОБОТА
// ===========================================
enum RobotState {
  STATE_STOPPED,
  STATE_CALIBRATING,
  STATE_FOLLOWING,
  STATE_SEARCHING
};

// ===========================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ===========================================
int left_min = 1023;
int left_max = 0;
int right_min = 1023;
int right_max = 0;

int last_error = 0;
int last_direction = 0;
int spiral_step = 0;
bool spiral_direction = 0;

unsigned long spiral_timer = 0;
unsigned long search_start_time = 0;
unsigned long last_button_check = 0;

RobotState current_state = STATE_STOPPED;
bool last_button_state = HIGH;

int left_sensor_value = 0;
int right_sensor_value = 0;

// Флаг для звукового сигнала при таймауте
bool timeout_signal = false;

// ===========================================
// ФУНКЦИИ УПРАВЛЕНИЯ МОТОРАМИ
// ===========================================
void drive(int left_speed, int right_speed) {
  left_speed = constrain(left_speed, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
  right_speed = constrain(right_speed, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
  
  digitalWrite(DIRECTION_LEFT_MOTOR_PIN, left_speed >= 0);
  analogWrite(POWER_LEFT_MOTOR_PIN, abs(left_speed));
  
  digitalWrite(DIRECTION_RIGHT_MOTOR_PIN, right_speed >= 0);
  analogWrite(POWER_RIGHT_MOTOR_PIN, abs(right_speed));
}

void stop_robot(bool timeout = false) {
  drive(0, 0);
  current_state = STATE_STOPPED;
  
  // Звуковой сигнал ТОЛЬКО при остановке по таймауту
  if (timeout) {
    tone(SOUND_PIN, 500, 500);  // Сигнал 500 Гц, 0.5 секунды
    timeout_signal = true;
  }
}

// ===========================================
// ФУНКЦИИ КАЛИБРОВКИ ДАТЧИКОВ
// ===========================================
void calibrate_sensors() {
  unsigned long calibration_start = millis();
  
  left_min = 1023; left_max = 0;
  right_min = 1023; right_max = 0;
  
  while (millis() - calibration_start < CALIBRATION_TIME) {
    drive(140, -140);
    
    int left_raw = analogRead(LEFT_SENSOR_PIN);
    int right_raw = analogRead(RIGHT_SENSOR_PIN);
    
    if (left_raw < left_min) left_min = left_raw;
    if (left_raw > left_max) left_max = left_raw;
    if (right_raw < right_min) right_min = right_raw;
    if (right_raw > right_max) right_max = right_raw;
  }
  
  drive(0, 0);
}

// ===========================================
// ФУНКЦИИ СЛЕДОВАНИЯ ПО ЛИНИИ
// ===========================================
void read_normalized_sensors() {
  int left_raw = analogRead(LEFT_SENSOR_PIN);
  int right_raw = analogRead(RIGHT_SENSOR_PIN);
  
  left_sensor_value = map(left_raw, left_min, left_max, 0, 100);
  right_sensor_value = map(right_raw, right_min, right_max, 0, 100);
  
  left_sensor_value = constrain(left_sensor_value, 0, 100);
  right_sensor_value = constrain(right_sensor_value, 0, 100);
}

void line_following() {
  int error = left_sensor_value - right_sensor_value;
  int control = (error * PID_GAIN_P) + ((error - last_error) * PID_GAIN_D);
  last_error = error;
  
  int left_speed = BASE_SPEED + control;
  int right_speed = BASE_SPEED - control;
  
  left_speed = constrain(left_speed, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
  right_speed = constrain(right_speed, -MAX_MOTOR_SPEED, MAX_MOTOR_SPEED);
  
  drive(left_speed, right_speed);
}

bool is_on_line() {
  return (left_sensor_value > LIGHT_THRESHOLD || 
          right_sensor_value > LIGHT_THRESHOLD);
}

// ===========================================
// ФУНКЦИИ ПОИСКА ЛИНИИ
// ===========================================
void start_line_search() {
  current_state = STATE_SEARCHING;
  search_start_time = millis();
  spiral_step = 0;
  spiral_timer = millis();
  spiral_direction = (last_direction == 0) ? 1 : 0;
}

void spiral_search() {
  if ((unsigned long)(millis() - search_start_time) > SEARCH_TIMEOUT) {
    // Остановка с звуковым сигналом по таймауту
    stop_robot(true);
    return;
  }
  
  if (millis() - spiral_timer > SPIRAL_INTERVAL) {
    spiral_step += SPIRAL_INCREASE;
    spiral_step = min(spiral_step, SEARCH_SPEED);
    spiral_timer = millis();
  }
  
  if (spiral_direction) {
    drive(SEARCH_SPEED - spiral_step, SEARCH_SPEED);
  } else {
    drive(SEARCH_SPEED, SEARCH_SPEED - spiral_step);
  }
}

// ===========================================
// ОСНОВНАЯ ЛОГИКА УПРАВЛЕНИЯ
// ===========================================
void robot_control() {
  read_normalized_sensors();
  bool on_line = is_on_line();
  
  switch (current_state) {
    case STATE_FOLLOWING:
      if (!on_line) {
        last_direction = (left_sensor_value > right_sensor_value) ? 0 : 1;
        start_line_search();
      } else {
        line_following();
      }
      break;
      
    case STATE_SEARCHING:
      if (on_line) {
        current_state = STATE_FOLLOWING;
        last_direction = (left_sensor_value > LIGHT_THRESHOLD) ? 0 : 1;
      } else {
        spiral_search();
      }
      break;
  }
}

bool check_button_press() {
  bool current_button_state = digitalRead(CALIBRATE_BUTTON_PIN);
  
  if (current_button_state != last_button_state) {
    last_button_check = millis();
  }
  
  if ((millis() - last_button_check) > DEBOUNCE_DELAY) {
    if (current_button_state == LOW && last_button_state == HIGH) {
      last_button_state = current_button_state;
      return true;
    }
  }
  
  last_button_state = current_button_state;
  return false;
}

// ===========================================
// ARDUINO STANDARD FUNCTIONS
// ===========================================
void setup() {
  pinMode(POWER_LEFT_MOTOR_PIN, OUTPUT);
  pinMode(DIRECTION_LEFT_MOTOR_PIN, OUTPUT);
  pinMode(POWER_RIGHT_MOTOR_PIN, OUTPUT);
  pinMode(DIRECTION_RIGHT_MOTOR_PIN, OUTPUT);
  pinMode(SOUND_PIN, OUTPUT);
  pinMode(CALIBRATE_BUTTON_PIN, INPUT_PULLUP);
  
  current_state = STATE_CALIBRATING;
  calibrate_sensors();
  current_state = STATE_STOPPED;
  
  while (true) {
    if (check_button_press()) {
      break;
    }
  }
  
  current_state = STATE_FOLLOWING;
}

void loop() {
  if (check_button_press()) {
    if (current_state == STATE_STOPPED) {
      current_state = STATE_FOLLOWING;
    } else {
      stop_robot(false);  // Остановка без звука
    }
    delay(300);
  }
  
  // Сброс флага таймаута, если робот был перезапущен
  if (current_state == STATE_FOLLOWING || current_state == STATE_SEARCHING) {
    timeout_signal = false;
  }
  
  if (current_state == STATE_FOLLOWING || current_state == STATE_SEARCHING) {
    robot_control();
  }
}
