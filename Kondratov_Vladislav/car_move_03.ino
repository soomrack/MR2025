#define LEFT_PWM_PIN 6
#define LEFT_DIR_PIN 7
#define RIGHT_PWM_PIN 5
#define RIGHT_DIR_PIN 4
#define SOUND_PIN 9
#define LINE_L A2
#define LINE_R A3

int current_speed = 150;
unsigned long last_command_time = 0;
const unsigned long TIMEOUT = 300;

// ========== ПЕРЕМЕННЫЕ ДЛЯ СТАТИСТИКИ ==========
unsigned long total_commands = 0;
unsigned long command_counts[6] = {0}; // w, a, s, d, x, speed
const char* command_names[6] = {"w", "a", "s", "d", "x", "speed"};
unsigned long start_time = 0;
float min_temp = 99.9;
float max_temp = 0.0;

// ========== ПЕРЕМЕННЫЕ ДЛЯ РАСЧЁТА ПУТИ ==========
unsigned long command_start_time = 0;
String current_command = "";
float total_distance = 0.0;
const float WHEEL_DIAMETER = 8.0; // см
const float WHEEL_CIRCUMFERENCE = 3.14159 * WHEEL_DIAMETER; // 25.13 см
const float SPEED_FACTOR = 0.1; // Коэффициент пересчёта скорости в см/сек (калибровка)

void drive(int left, int right) {
  if (left > 0) {
    digitalWrite(LEFT_DIR_PIN, HIGH);
  } else {
    digitalWrite(LEFT_DIR_PIN, LOW);
  }
  
  if (right > 0) {
    digitalWrite(RIGHT_DIR_PIN, HIGH);
  } else {
    digitalWrite(RIGHT_DIR_PIN, LOW);
  }
  
  analogWrite(LEFT_PWM_PIN, min(abs(left), 255));
  analogWrite(RIGHT_PWM_PIN, min(abs(right), 255));
}

void stopMotors() {
  analogWrite(LEFT_PWM_PIN, 0);
  analogWrite(RIGHT_PWM_PIN, 0);
}

void ready_sound() {
  tone(SOUND_PIN, 1000, 200);
  delay(200);
  tone(SOUND_PIN, 1500, 200);
  delay(200);
}

// ========== РАСЧЁТ ПРОЙДЕННОГО ПУТИ ==========
void calculate_distance() {
  if (command_start_time > 0 && current_command != "") {
    unsigned long press_duration = millis() - command_start_time;
    float time_seconds = press_duration / 1000.0;
    
    // Расчёт пути: скорость * время * коэффициент
    float speed_cm_per_sec = current_speed * SPEED_FACTOR;
    float distance = speed_cm_per_sec * time_seconds;
    
    total_distance += distance;
    
    // Отправляем данные на Raspberry Pi для логирования
    Serial.print("DISTANCE:");
    Serial.print(distance);
    Serial.print(",");
    Serial.println(total_distance);
  }
}

// ========== НАЧАЛО ДВИЖЕНИЯ ==========
void start_movement(String cmd) {
  calculate_distance(); // Считаем путь для предыдущей команды
  command_start_time = millis();
  current_command = cmd;
}

// ========== ВЫВОД СТАТИСТИКИ ==========
void printStats() {
  Serial.println("=== СТАТИСТИКА ДВИЖЕНИЙ ===");
  
  unsigned long runtime = (millis() - start_time) / 1000;
  Serial.print("Время работы: ");
  Serial.print(runtime / 60);
  Serial.print(" мин ");
  Serial.print(runtime % 60);
  Serial.println(" сек");
  
  Serial.print("Всего команд: ");
  Serial.println(total_commands);
  
  for (int i = 0; i < 6; i++) {
    if (command_counts[i] > 0) {
      Serial.print("  ");
      Serial.print(command_names[i]);
      Serial.print(": ");
      Serial.println(command_counts[i]);
    }
  }
  
  Serial.print("Пройденный путь: ");
  Serial.print(total_distance);
  Serial.println(" см");
  
  Serial.println("TEMP_REQUEST");
}

// ========== ОБНОВЛЕНИЕ ТЕМПЕРАТУРЫ ==========
void updateTemperature(float temp) {
  if (temp < min_temp) min_temp = temp;
  if (temp > max_temp) max_temp = temp;
  
  Serial.print("TEMP_STATS:");
  Serial.print(min_temp);
  Serial.print(",");
  Serial.println(max_temp);
}

void setup() {
  Serial.begin(9600);
  
  // Настройка пинов моторов
  pinMode(LEFT_PWM_PIN, OUTPUT);
  pinMode(LEFT_DIR_PIN, OUTPUT);
  pinMode(RIGHT_PWM_PIN, OUTPUT);
  pinMode(RIGHT_DIR_PIN, OUTPUT);
  pinMode(SOUND_PIN, OUTPUT);
  
  // Настройка датчиков линии
  pinMode(LINE_L, INPUT);
  pinMode(LINE_R, INPUT);

  // Остановка моторов при старте
  stopMotors();
  
  // Звуковой сигнал готовности
  ready_sound();
  
  // Запоминаем время старта
  start_time = millis();
  
  // Сообщение о готовности
  Serial.println("ARD:READY with STATS");
}

void loop() {
  // --- 1. МОНИТОРИНГ ДАТЧИКОВ ЛИНИИ ---
  static unsigned long last_line_report = 0;
  if (millis() - last_line_report >= 500) {
    int lineL = analogRead(LINE_L);
    int lineR = analogRead(LINE_R);
    
    // Отправка данных на сервер
    Serial.print("LINE:");
    Serial.print(lineL);
    Serial.print(",");
    Serial.println(lineR);
    
    last_line_report = millis();
  }

  // --- 2. ОБРАБОТКА КОМАНД ---
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    // Обновляем время последней команды
    last_command_time = millis();

    // Обработка данных о температуре от сервера
    if (command.startsWith("TEMP:")) {
      float temp = command.substring(5).toFloat();
      updateTemperature(temp);
    } 
    // Движение вперёд
    else if (command == "w") {
      total_commands++;
      command_counts[0]++;
      start_movement("w");
      drive(current_speed, current_speed);
      tone(SOUND_PIN, 1200, 20);
      Serial.println("CMD:w OK");
    }
    // Движение назад
    else if (command == "s") {
      total_commands++;
      command_counts[2]++;
      start_movement("s");
      drive(-current_speed, -current_speed);
      tone(SOUND_PIN, 800, 20);
      Serial.println("CMD:s OK");
    }
    // Поворот влево
    else if (command == "a") {
      total_commands++;
      command_counts[1]++;
      start_movement("a");
      drive(-current_speed, current_speed);
      tone(SOUND_PIN, 600, 20);
      Serial.println("CMD:a OK");
    }
    // Поворот вправо
    else if (command == "d") {
      total_commands++;
      command_counts[3]++;
      start_movement("d");
      drive(current_speed, -current_speed);
      tone(SOUND_PIN, 600, 20);
      Serial.println("CMD:d OK");
    }
    // Стоп
    else if (command == "x") {
      total_commands++;
      command_counts[4]++;
      calculate_distance();
      command_start_time = 0;
      current_command = "";
      stopMotors();
      tone(SOUND_PIN, 400, 50);
      Serial.println("CMD:x OK");
    }
    // Установка скорости
    else if (command.startsWith("speed ")) {
      calculate_distance();
      int new_speed = command.substring(6).toInt();
      current_speed = constrain(new_speed, 0, 255);
      command_counts[5]++;
      Serial.print("SPEED:");
      Serial.println(current_speed);
    }
    // Запрос статистики
    else if (command == "GET_STATS") {
      printStats();
    }
    // Неизвестная команда
    else if (command.length() > 0) {
      Serial.print("UNKNOWN_CMD:");
      Serial.println(command);
    }
  }

  // --- 3. АВАРИЙНАЯ ОСТАНОВКА ПРИ ПОТЕРЕ СВЯЗИ ---
  if (millis() - last_command_time > TIMEOUT) {
    if (current_command != "") {
      calculate_distance();
      stopMotors();
      current_command = "";
      command_start_time = 0;
    }
  }
}