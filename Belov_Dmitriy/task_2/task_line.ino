// Пины двигателей
#define MOTOR_LEFT_PWM 6
#define MOTOR_LEFT_DIR 7
#define MOTOR_RIGHT_PWM 5
#define MOTOR_RIGHT_DIR 4

// Пины сенсоров и кнопки
#define LEFT_SENSOR A0
#define RIGHT_SENSOR A1
#define BUTTON_PIN 12

// Параметры управления
float gain_p = 8.0;
float gain_d = 5.0;
int base_speed = 180;

// Калибровочные значения
int left_min = 1023, left_max = 0;
int right_min = 1023, right_max = 0;
int thresholdL, thresholdR;

// Служебные переменные
float last_error = 0;
bool is_active = false;
bool button_old = HIGH;
unsigned long last_serial_time = 0;
const unsigned long serial_interval = 2000; // раз в 2 секунды вывод


// =============================================================
// Считывание аналогового значения с усреднением
// =============================================================
int read_average(int pin) {
  long sum = 0;
  for (int i = 0; i < 20; i++) sum += analogRead(pin);
  return sum / 20;
}


// =============================================================
// Калибровка сенсоров
// =============================================================
void sensor_calibration() {
  Serial.println("=== Калибровка сенсоров ===");
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("1) Поставьте робота на СВЕТЛУЮ поверхность и нажмите кнопку");
  while (digitalRead(BUTTON_PIN)) delay(10);
  while (!digitalRead(BUTTON_PIN)) delay(10);
  left_max = read_average(LEFT_SENSOR);
  right_max = read_average(RIGHT_SENSOR);
  Serial.println("2) Поставьте робота на ЛИНИЮ и нажмите кнопку");
  while (digitalRead(BUTTON_PIN)) delay(10);
  while (!digitalRead(BUTTON_PIN)) delay(10);
  left_min = read_average(LEFT_SENSOR);
  right_min = read_average(RIGHT_SENSOR);
  thresholdL = left_min + (left_max - left_min) * 0.1;
  thresholdR = right_min + (right_max - right_min) * 0.1;
  Serial.println("Калибровка завершена!");
}


// =============================================================
// Управление моторами
// =============================================================
void move_motors(int left_speed, int right_speed) {
  digitalWrite(MOTOR_LEFT_DIR, left_speed > 0);
  digitalWrite(MOTOR_RIGHT_DIR, right_speed > 0);
  analogWrite(MOTOR_LEFT_PWM, abs(left_speed));
  analogWrite(MOTOR_RIGHT_PWM, abs(right_speed));
}


// =============================================================
// Проверка потери линии
// =============================================================
bool line_lost() {
  int left = analogRead(LEFT_SENSOR);
  int right = analogRead(RIGHT_SENSOR);
  return (left < thresholdL && right < thresholdR);
}


// =============================================================
// Следование по линии (PD-регулятор)
// =============================================================
void follow_line() {
  int left_value = map(analogRead(LEFT_SENSOR), left_min, left_max, 0, 100);
  int right_value = map(analogRead(RIGHT_SENSOR), right_min, right_max, 0, 100);
  float error = left_value - right_value;
  float correction = error * gain_p + (error - last_error) * gain_d;
  int left_motor = constrain(base_speed - correction, -255, 255);
  int right_motor = constrain(base_speed + correction, -255, 255);
  move_motors(left_motor, right_motor);
  last_error = error;
}


// =============================================================
// Поиск линии
// =============================================================
void find_line() {
  Serial.println("Поиск линии...");
  unsigned long start_time = millis();
  while (millis() - start_time < 20000) {
    if (last_error > 0) move_motors(100, -100);
    else move_motors(-100, 100);
    if (!line_lost()) {
      Serial.println("Линия найдена!");
      return;
    }
  }
  move_motors(0, 0);
  Serial.println("Не удалось найти линию!");
}


// =============================================================
// Вывод диагностической информации
// =============================================================
void serial_output() {
  Serial.print("Left: "); Serial.print(analogRead(LEFT_SENSOR));
  Serial.print(" | Right: "); Serial.print(analogRead(RIGHT_SENSOR));
  Serial.print(" | Error: "); Serial.println(last_error);
}


// =============================================================
// Периодический таймер вывода
// =============================================================
void serial_timer() {
  if (millis() - last_serial_time >= serial_interval) {
    serial_output();
    last_serial_time = millis();
  }
}


// =============================================================
// Управление кнопкой
// =============================================================
void button_control() {
  bool button_state = digitalRead(BUTTON_PIN);
  if (button_state == LOW && button_old == HIGH) {
    is_active = !is_active;
    delay(200);
  }
  button_old = button_state;
}


// =============================================================
// Главная логика управления
// =============================================================
void process_logic() {
  if (is_active) {
    if (line_lost()) find_line();
    else follow_line();
  } else {
    move_motors(0, 0);
  }
}


// =============================================================
// Инициализация
// =============================================================
void initialization() {
  sensor_calibration();
  Serial.println("Нажмите кнопку для старта");
  while (digitalRead(BUTTON_PIN)) delay(10);
  is_active = true;
}


// =============================================================
// Setup
// =============================================================
void setup() {
  Serial.begin(9600);
  pinMode(MOTOR_LEFT_PWM, OUTPUT);
  pinMode(MOTOR_LEFT_DIR, OUTPUT);
  pinMode(MOTOR_RIGHT_PWM, OUTPUT);
  pinMode(MOTOR_RIGHT_DIR, OUTPUT);
  pinMode(LEFT_SENSOR, INPUT);
  pinMode(RIGHT_SENSOR, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  initialization();
}


// =============================================================
// Loop
// =============================================================
void loop() {
  button_control();
  process_logic();
  serial_timer();
}
