#define LEFT_PWM_PIN 6
#define LEFT_DIR_PIN 7
#define RIGHT_PWM_PIN 5
#define RIGHT_DIR_PIN 4
#define SOUND_PIN 9

// Коэффициенты для плавного движения
#define K_P 5.0
#define K_D 3.0

// Базовая скорость
#define V 150

long long int TIME_SOUND = 0;
int LEFT_MIN = 1023;
int LEFT_MAX = 0;
int RIGHT_MIN = 1023;
int RIGHT_MAX = 0;

int LAST_DIRECTION = 0;
int ERROLD = 0;
bool IS_TURNED = false; // Флаг движения по линии
bool BUTTON_OLD = 1;
bool RECOVERING = false; // Флаг восстановления линии

// Функция управления движением
void drive(int left, int right) {
  digitalWrite(LEFT_DIR_PIN, left > 0);
  digitalWrite(RIGHT_DIR_PIN, right > 0);
  
  // Ограничение скорости для плавного движения
  analogWrite(LEFT_PWM_PIN, min(abs(left), 180));
  analogWrite(RIGHT_PWM_PIN, min(abs(right), 180));
}

// Функция восстановления линии с плавным дуговым движением
void recoverLine(int dir) {
  RECOVERING = true;
  
  // Параметры спирали
  int base_speed = 180;
  int spiral_duration = 800; // Общее время движения по спирали (мс)
  int step_delay = 50;       // Задержка между изменениями
  int steps = spiral_duration / step_delay;
  
  for (int step = 0; step < steps; step++) {
    // Вычисляем текущий коэффициент для спирали
    float spiral_factor = (float)step / steps;
    
    if (dir == 0) { // Толкнули влево - спираль вправо
      // Левый мотор: от заднего хода к переднему с увеличивающейся скоростью
      int left_speed = -80 + (int)(260 * spiral_factor); // -80 → 180
      // Правый мотор: постоянная скорость с небольшим уменьшением
      int right_speed = base_speed - (int)(20 * spiral_factor); // 180 → 160
      
      drive(left_speed, right_speed);
    } else { // Толкнули вправо - спираль влево
      // Правый мотор: от заднего хода к переднему с увеличивающейся скоростью
      int right_speed = -80 + (int)(260 * spiral_factor); // -80 → 180
      // Левый мотор: постоянная скорость с небольшим уменьшением
      int left_speed = base_speed - (int)(20 * spiral_factor); // 180 → 160
      
      drive(left_speed, right_speed);
    }
    
    // Проверяем датчики во время движения по спирали
    int sensor1 = map(analogRead(A0), LEFT_MIN, LEFT_MAX, 0, 100);
    int sensor2 = map(analogRead(A1), RIGHT_MIN, RIGHT_MAX, 0, 100);
    
    // Если нашли линию, прерываем спираль
    if (sensor1 > 60 || sensor2 > 60) {
      break;
    }
    
    delay(step_delay);
  }
  
  RECOVERING = false;
}

void end_music() {
  tone(SOUND_PIN, 1000, 2000);
}

void check_line(int sensor1, int sensor2) {
  // Если оба датчика на белом (потеряли линию)
  if (sensor1 < 60 && sensor2 < 60 && !RECOVERING) {
    if (millis() - TIME_SOUND > 30 * 1000) {
      end_music();
      drive (0,0);
      exit (0);
    }
    recoverLine(LAST_DIRECTION); // Вызов функции восстановления линии
    return;
  }
  
  // Если восстановление не требуется
  TIME_SOUND = millis();
  
  // Вычисление ошибки для обеспечения более плавного хода по линии
  double err = (sensor1 - sensor2);
  double u = err * K_P + (err - ERROLD) * K_D;
  
  // Ограниченные выходные значения для плавного движения
  int leftSpeed = constrain(V + u, -180, 180);
  int rightSpeed = constrain(V - u, -180, 180);
  
  drive(leftSpeed, rightSpeed);
  ERROLD = err;
  
  // Запоминаем последнее направление отклонения
  if (sensor2 > sensor1) {
    LAST_DIRECTION = 0; // Толкнули влево
  } else {
    LAST_DIRECTION = 1; // Толкнули вправо
  }
}

void start_button() {
  if (digitalRead(A2) == 1 && BUTTON_OLD == 0) {
    delay(5); // Антидребезг
    if (digitalRead(A2) == 1) {
      IS_TURNED = !IS_TURNED;
      if (IS_TURNED) {
        tone(SOUND_PIN, 1000, 100);
      } else {
        tone(SOUND_PIN, 500, 100);
      }
    }
  }
  BUTTON_OLD = digitalRead(A2);
}

void sensors()
{
   if (IS_TURNED) {
    // Чтение и нормализация значений сенсоров
    int sensor1 = map(analogRead(A0), LEFT_MIN, LEFT_MAX, 0, 100);
    int sensor2 = map(analogRead(A1), RIGHT_MIN, RIGHT_MAX, 0, 100);
    // Проверяем, потеряна ли линия
    check_line(sensor1, sensor2);
  } 
  else {
    drive(0, 0); // Остановка при выключенном режиме
  }
}

void setup() {
  pinMode(LEFT_PWM_PIN, OUTPUT);
  pinMode(LEFT_DIR_PIN, OUTPUT);
  pinMode(RIGHT_PWM_PIN, OUTPUT);
  pinMode(RIGHT_DIR_PIN, OUTPUT);
  pinMode(SOUND_PIN, OUTPUT);
  
  // Калибровка датчиков
  int tim = millis();
  while (millis() - tim < 4000) {
    drive(150, -150); // Медленное вращение для калибровки
    int left = analogRead(A0);
    int right = analogRead(A1);
    // Для каждого датчика определяет, что есть самое черное, а что есть самое белое
    LEFT_MIN = min(left, LEFT_MIN);
    LEFT_MAX = max(left, LEFT_MAX);
    RIGHT_MIN = min(right, RIGHT_MIN);
    RIGHT_MAX = max(right, RIGHT_MAX);
  }
  drive(0, 0);
  
  pinMode(A2, INPUT_PULLUP);
  
  // Ожидание нажатия кнопки для старта
  while (digitalRead(A2) == HIGH) {
    delay(10);
  }
  while (digitalRead(A2) == LOW) {
    delay(10);
  }
  
  tone(SOUND_PIN, 1000, 200);
  delay(200);
  IS_TURNED = true;
}

void loop() {
  start_button(); // Включает-выключает IS_TURNED
  sensors();  // Проверяет включена ли IS_TURNED. Если да, то вызывает функцию следования по линии, если не видит линию, то вызывает функцию поиска линии, если не находит линию 30 сек - останавливает код
  delay(10); // Небольшая задержка для стабильности
}
