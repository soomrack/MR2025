// Определение пинов подключения компонентов 
#define POWER_LEFT_MOTOR_PIN 6        
#define DIRECTION_LEFT_MOTOR_PIN 7   
#define POWER_RIGHT_MOTOR_PIN 5       
#define DIRECTION_RIGHT_MOTOR_PIN 4   
#define SOUND_PIN 9                   
#define CALIBRATE_BOTTON_PIN A2      
#define LEFT_SENSOR_PIN A0            
#define RIGHT_SENSOR_PIN A1           

// Параметры PID-регулятора и движения
#define pid_gain_p 8.0                // Пропорциональный коэффициент PID
#define pid_gain_d 6.0                // Дифференциальный коэффициент PID
#define base_speed 90                 // Базовая скорость движения при следовании по линии
#define search_speed 140              // Скорость при поиске линии

// Настройки поиска линии 
#define light_threshold 50            // Порог срабатывания датчика света (0–100)
#define spiral_increase 3             // Увеличение амплитуды "спирали" при поиске
#define spiral_interval 50            // Интервал увеличения спирали (мс)
#define search_timeout 20000          // Максимальное время поиска линии (20 секунд)

// Настройки мелодии
#define NOTE_C4  262   // Частота ноты До 4-й октавы
#define NOTE_D4  294   // Ре
#define NOTE_E4  330   // Ми
#define NOTE_F4  349   // Фа
#define NOTE_G4  392   // Соль
#define NOTE_A4  440   // Ля
#define NOTE_B4  494   // Си
#define NOTE_C5  523   // До 5-й октавы

// Мелодия - упрощенная версия "Оды к радости" Бетховена
int melody[] = {
  NOTE_E4, 0 , NOTE_F4, 0, NOTE_G4, 0, NOTE_E4, 0,
  NOTE_C4, 0, NOTE_D4, 0, NOTE_E4, 0, NOTE_D4, 0
};

// Длительности нот (в мс)
int noteDurations[] = {
  400, 400, 400, 400, 400, 400, 400, 400,
  800, 800, 800, 800, 800, 800, 800, 800
};

// Переменные для управления мелодией
int melodyLength = sizeof(melody) / sizeof(melody[0]);
int currentNote = 0;
unsigned long previousNoteTime = 0;
int noteDuration = 200;

// Переменные для калибровки датчиков
int left_min = 1023;
int left_max = 0;
int right_min = 1023;
int right_max = 0;

// Переменные для управления роботом
int last_direction = 0;
int errold = 0;
bool is_active = false;
bool CALIBRATE_BOTTON_PIN_old = 1;

// Переменные для поиска линии
bool spiral_direction = 0;
int spiral_step = 0;
unsigned long spiral_timer = 0;
unsigned long search_start_time = 0;

// Переменные для датчиков
int s1, s2;

// Состояния робота
enum RobotState { STOPPED, FOLLOWING_LINE, SEARCHING_LINE };
RobotState robotState = STOPPED;

// Управление моторами 
void drive(int left, int right) {
  digitalWrite(DIRECTION_LEFT_MOTOR_PIN, left > 0);
  digitalWrite(DIRECTION_RIGHT_MOTOR_PIN, right > 0);
  analogWrite(POWER_LEFT_MOTOR_PIN, abs(left));
  analogWrite(POWER_RIGHT_MOTOR_PIN, abs(right));
}

// Остановка робота 
void stop_robot() {
  drive(0, 0);
  noTone(SOUND_PIN);
  is_active = false;
  robotState = STOPPED;
  tone(SOUND_PIN, 500, 500);
}

// Воспроизведение мелодии - ВСЕГДА когда робот активен
void playMelody() {
  unsigned long currentTime = millis();
  
  if (currentTime - previousNoteTime >= noteDuration) {
    // Воспроизводим текущую ноту (если не пауза)
    if (melody[currentNote] != 0) {
      tone(SOUND_PIN, melody[currentNote], noteDuration);
    } else {
      noTone(SOUND_PIN);
    }
    
    // Переходим к следующей ноте
    currentNote++;
    if (currentNote >= melodyLength) {
      currentNote = 0; // Начинаем мелодию заново
    }
    
    noteDuration = noteDurations[currentNote];
    previousNoteTime = currentTime;
  }
}

// Остановка мелодии
void stopMelody() {
  noTone(SOUND_PIN);
  currentNote = 0;
  previousNoteTime = millis();
  noteDuration = 200;
}

// Начало поиска линии
void start_search() {
  spiral_step = 0;
  spiral_timer = millis();
  spiral_direction = (last_direction == 0) ? 1 : 0;
  search_start_time = millis();
}

// Спиральный поиск линии
void spiral_search() {
  if (millis() - spiral_timer > spiral_interval) {
    spiral_step += spiral_increase;
    spiral_timer = millis();
    
    if (spiral_step > 100) {
      spiral_step = 100;
    }
  }

  int left_speed = search_speed - spiral_step;
  int right_speed = search_speed - spiral_step;
  
  left_speed = constrain(left_speed, 0, 255);
  right_speed = constrain(right_speed, 0, 255);

  if (spiral_direction) {
    drive(left_speed, search_speed);
  } else {
    drive(search_speed, right_speed);
  }
}

// Следование по линии с PID-регулятором
void line_following(int s1, int s2) {
  double err = (s1 - s2);
  double u = err * pid_gain_p + (err - errold) * pid_gain_d;
  drive(constrain(base_speed + u, -250, 250),
        constrain(base_speed - u, -250, 250));
  errold = err;
}

// Проверка состояния датчиков и принятие решения 
void check_sensors() {
  s1 = map(analogRead(LEFT_SENSOR_PIN), left_min, left_max, 0, 100);
  s2 = map(analogRead(RIGHT_SENSOR_PIN), right_min, right_max, 0, 100);

  bool see_line = (s1 > light_threshold || s2 > light_threshold);
  
  if (robotState == FOLLOWING_LINE) {
    if (!see_line) {
      robotState = SEARCHING_LINE;
      start_search();
    }
  } 
  else if (robotState == SEARCHING_LINE) {
    if (millis() - search_start_time > search_timeout) {
      stop_robot();
      return;
    }
    
    if (see_line) {
      robotState = FOLLOWING_LINE;
      last_direction = (s1 > light_threshold) ? 0 : 1;
    }
  }
}

// Начальная настройка 
void setup() {
  pinMode(POWER_LEFT_MOTOR_PIN, OUTPUT);
  pinMode(DIRECTION_LEFT_MOTOR_PIN, OUTPUT);
  pinMode(POWER_RIGHT_MOTOR_PIN, OUTPUT);
  pinMode(DIRECTION_RIGHT_MOTOR_PIN, OUTPUT);
  pinMode(SOUND_PIN, OUTPUT);
  pinMode(CALIBRATE_BOTTON_PIN, INPUT_PULLUP);

  // Калибровка датчиков
  int time = millis();
  while (millis() - time < 4000) {
    drive(140, -140);
    int left = analogRead(LEFT_SENSOR_PIN);
    int right = analogRead(RIGHT_SENSOR_PIN);
    
    if (left < left_min) left_min = left;
    if (left > left_max) left_max = left;
    if (right < right_min) right_min = right;
    if (right > right_max) right_max = right;
  }
  drive(0, 0);
  
  // Ожидание нажатия кнопки для старта
  while (true) {
    if (digitalRead(CALIBRATE_BOTTON_PIN) == LOW) { 
      delay(50);
      if (digitalRead(CALIBRATE_BOTTON_PIN) == LOW) break;
    }
  }
  
  tone(SOUND_PIN, 1000, 200);
  delay(200);
}

void loop() {
  if (is_active) {
    // ВСЕГДА играем мелодию когда робот активен
    playMelody();
    
    check_sensors();
    
    if (robotState == FOLLOWING_LINE) {
      line_following(s1, s2);
    }
    else if (robotState == SEARCHING_LINE) {
      spiral_search();
    }
  } else {
    stopMelody();
    robotState = STOPPED;
  }
  
  // Обработка кнопки старт/стоп
  if (digitalRead(CALIBRATE_BOTTON_PIN) == HIGH && CALIBRATE_BOTTON_PIN_old == LOW) {
    is_active = !is_active;
    if (is_active) {
      robotState = FOLLOWING_LINE;
    } else {
      robotState = STOPPED;
      stopMelody();
      drive(0, 0);
    }
  }
  CALIBRATE_BOTTON_PIN_old = digitalRead(CALIBRATE_BOTTON_PIN);
}