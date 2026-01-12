// Пины подключения
#define MOTOR_LEFT_PWM_PIN 6        // Скорость левого мотора
#define MOTOR_LEFT_DIR_PIN 7        // Направление левого мотора  
#define MOTOR_RIGHT_PWM_PIN 5       // Скорость правого мотора
#define MOTOR_RIGHT_DIR_PIN 4       // Направление правого мотора
#define BUZZER_PIN 9                // Динамик для звуков
#define BUTTON_PIN A2               // Кнопка включения
#define SENSOR_LEFT_PIN A0          // Левый датчик линии
#define SENSOR_RIGHT_PIN A1         // Правый датчик линии

// Настройки движения
#define PID_P_GAIN 8.0              // Чувствительность к отклонению от линии
#define PID_D_GAIN 6.0              // Плавность поворотов
#define BASE_SPEED 90               // Обычная скорость
#define SEARCH_SPEED 110            // Скорость при поиске линии

// Настройки датчиков
#define LINE_THRESHOLD 50           // Порог срабатывания датчиков
#define SPIRAL_STEP 3               // Шаг увеличения спирали
#define SPIRAL_DELAY 50             // Пауза между шагами спирали
#define SEARCH_TIMEOUT 10000        // Максимальное время поиска (10 сек)

// Настройки звуков
#define BEEP_INTERVAL 2000          // Как часто пищать при движении
#define NOTE_DURATION 150           // Длительность ноты

// Состояния робота
enum RobotState {
  STATE_IDLE,           // Ожидание - робот выключен
  STATE_CALIBRATING,    // Калибровка - настраиваем датчики
  STATE_FOLLOW_LINE,    // Движение по линии - основной режим
  STATE_SEARCH_LINE     // Поиск линии - потеряли траекторию
};

// Переменные для калибровки датчиков
unsigned long last_beep_time = 0;
int left_sensor_min = 1023;        // Минимум левого датчика (черная линия)
int left_sensor_max = 0;           // Максимум левого датчика (белый фон)
int right_sensor_min = 1023;       // Минимум правого датчика
int right_sensor_max = 0;          // Максимум правого датчика

// Переменные управления
RobotState current_state = STATE_IDLE; // Текущее состояние робота
int last_turn_side = 0;             // Куда поворачивали в прошлый раз
int prev_pid_error = 0;             // Предыдущая ошибка для плавности
bool prev_button_state = true;      // Состояние кнопки в прошлый раз
bool spiral_direction = false;      // Направление спирали
int spiral_radius = 0;              // Текущий радиус спирали
unsigned long spiral_timer = 0;     // Таймер для спирали
unsigned long search_start_time = 0; // Когда начали поиск
unsigned long last_note_time = 0;   // Когда играли последнюю ноту
int current_note_index = 0;         // Какую ноту играть следующей

//--------------------------------------------------
// Режим ожидания
void state_idle() {
  set_motors(0, 0);  // Стоит на месте
  
}

//--------------------------------------------------
// Режим калибровки датчиков
void state_calibrating() {
  // Крутимся и запоминаем значения датчиков
  unsigned long cal_start = millis();
  while (millis() - cal_start < 4000) {
    set_motors(120, -120); // Вращение на месте
    
    int left_val = analogRead(SENSOR_LEFT_PIN);
    int right_val = analogRead(SENSOR_RIGHT_PIN);
    
    if (left_val < left_sensor_min) left_sensor_min = left_val;
    if (left_val > left_sensor_max) left_sensor_max = left_val;
    if (right_val < right_sensor_min) right_sensor_min = right_val;
    if (right_val > right_sensor_max) right_sensor_max = right_val;
  }
  
  set_motors(0, 0); // Останавливаемся после калибровки
  tone(BUZZER_PIN, 1500, 500); // Сигнал завершения калибровки
  
  // Переходим к движению
  current_state = STATE_FOLLOW_LINE;
}

//--------------------------------------------------
// Движение по линии
void state_follow_line() {
  int left_sensor, right_sensor;
  read_sensors(left_sensor, right_sensor);
  
  // Поиск при потере
  if (left_sensor < LINE_THRESHOLD && right_sensor < LINE_THRESHOLD) {
    current_state = STATE_SEARCH_LINE;
    start_line_search();
    return;
  }
  
  // Двигаемся по линии с PID-регулятором
  follow_line(left_sensor, right_sensor);
  periodic_beep();
}

//--------------------------------------------------
// Режим поиска
//--------------------------------------------------
void state_search_line() {
  int left_sensor, right_sensor;
  read_sensors(left_sensor, right_sensor);
  
  // Проверяем таймаут поиска
  if (millis() - search_start_time > SEARCH_TIMEOUT) {
    stop_robot();
    return;
  }
  
  // Если нашли линию - возвращаемся к движению
  if (left_sensor > LINE_THRESHOLD || right_sensor > LINE_THRESHOLD) {
    last_turn_side = (left_sensor > LINE_THRESHOLD) ? 0 : 1;
    tone(BUZZER_PIN, 1000, 300); // Сигнал "нашел линию"
    current_state = STATE_FOLLOW_LINE;
    return;
  }
  
  // Продолжаем поиск по спирали
  spiral_search();
  periodic_beep();
}

//--------------------------------------------------
// Управление моторами
void set_motors(int left_speed, int right_speed) {
  digitalWrite(MOTOR_LEFT_DIR_PIN, left_speed > 0);
  digitalWrite(MOTOR_RIGHT_DIR_PIN, right_speed > 0);
  analogWrite(MOTOR_LEFT_PWM_PIN, abs(left_speed));
  analogWrite(MOTOR_RIGHT_PWM_PIN, abs(right_speed));
}

//--------------------------------------------------
// Полная остановка 
void stop_robot() {
  set_motors(0, 0);
  current_state = STATE_IDLE;
  tone(BUZZER_PIN, 500, 500);
  Serial.println("Робот остановлен!");
}

//--------------------------------------------------
// Начать поиск
void start_line_search() {
  spiral_radius = 0;
  spiral_timer = millis();
  spiral_direction = (last_turn_side == 0) ? 1 : 0;
  search_start_time = millis();
  tone(BUZZER_PIN, 300, 200);
  Serial.println("Начат поиск линии...");
}

//--------------------------------------------------
// Поиск по спирали
void spiral_search() {
  if (millis() - spiral_timer > SPIRAL_DELAY) {
    spiral_radius += SPIRAL_STEP;
    spiral_timer = millis();
  }

  int left_speed = SEARCH_SPEED - spiral_radius;
  int right_speed = SEARCH_SPEED - spiral_radius;

  if (spiral_direction) {
    set_motors(left_speed, SEARCH_SPEED);
  } else {
    set_motors(SEARCH_SPEED, right_speed);
  }
}

//--------------------------------------------------
// Играем мелодию
void play_melody() {
  if (millis() - last_note_time > NOTE_DURATION) {
    tone(BUZZER_PIN, melody_notes[current_note_index], NOTE_DURATION - 20);
    current_note_index = (current_note_index + 1) % melody_length;
    last_note_time = millis();
  }
}

//--------------------------------------------------
// Движение по линии с PID
void follow_line(int left_value, int right_value) {
  int error = (left_value - right_value);
  double adjustment = error * PID_P_GAIN + (error - prev_pid_error) * PID_D_GAIN;
  
  set_motors(constrain(BASE_SPEED + adjustment, -250, 250), 
             constrain(BASE_SPEED - adjustment, -250, 250));
  prev_pid_error = error;
  play_melody();
}

//--------------------------------------------------
// Чтение датчиков
void read_sensors(int& left_out, int& right_out) {
  left_out = map(analogRead(SENSOR_LEFT_PIN), left_sensor_min, left_sensor_max, 0, 100);
  right_out = map(analogRead(SENSOR_RIGHT_PIN), right_sensor_min, right_sensor_max, 0, 100);
  left_out = constrain(left_out, 0, 100);
  right_out = constrain(right_out, 0, 100);
}

//--------------------------------------------------
// Обработка кнопки
void handle_button() {
  bool current_button = digitalRead(BUTTON_PIN);
  
  if (current_button == HIGH && prev_button_state == LOW) {
    // Если были в ожидании - запускаем калибровку
    if (current_state == STATE_IDLE) {
      current_state = STATE_CALIBRATING;
      tone(BUZZER_PIN, 1200, 300);
      Serial.println("Запуск робота!");
    } 
    // Если в любом другом состоянии - останавливаем
    else {
      stop_robot();
    }
  }
  prev_button_state = current_button;
}

//--------------------------------------------------
// Периодический звуковой сигнал
void periodic_beep() {
  if (millis() - last_beep_time > BEEP_INTERVAL) {
    tone(BUZZER_PIN, 800, 100);
    last_beep_time = millis();
  }
}

//--------------------------------------------------
// Состояния
void run_state_robot() {
  switch (current_state) {
    case STATE_IDLE:
      state_idle();
      break;
    case STATE_CALIBRATING:
      state_calibrating();
      break;
    case STATE_FOLLOW_LINE:
      state_follow_line();
      break;
    case STATE_SEARCH_LINE:
      state_search_line();
      break;
  }
}

void setup() {
  pinMode(MOTOR_LEFT_PWM_PIN, OUTPUT);
  pinMode(MOTOR_LEFT_DIR_PIN, OUTPUT);
  pinMode(MOTOR_RIGHT_PWM_PIN, OUTPUT);
  pinMode(MOTOR_RIGHT_DIR_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  Serial.begin(9600);
  
  // Начальное состояние - ожидание
  current_state = STATE_IDLE;
}

void loop() {
  handle_button();       // Проверяем кнопку в любом состоянии
  run_state_robot();   // Cостояние с помощью "диспечера"
  delay(10);             // Стабилизируем работу
}
