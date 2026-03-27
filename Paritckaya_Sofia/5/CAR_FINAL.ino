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
bool IS_TURNED = false;
bool BUTTON_OLD = 1;
bool RECOVERING = false;

// Переменные для управления восстановлением линии
unsigned long recoveryStartTime = 0;
int recoveryStep = 0;
int recoveryDir = 0;
const int RECOVERY_STEPS = 16; 
const int RECOVERY_STEP_DELAY = 50;

// Функция управления движением
void drive(int left, int right) {
  digitalWrite(LEFT_DIR_PIN, left > 0);
  digitalWrite(RIGHT_DIR_PIN, right > 0);
  analogWrite(LEFT_PWM_PIN, min(abs(left), 180));
  analogWrite(RIGHT_PWM_PIN, min(abs(right), 180));
}

void end_music() {
  tone(SOUND_PIN, 1000, 2000);
}

// Функция для запуска восстановления линии
void startRecovery(int dir) {
  RECOVERING = true;
  recoveryStartTime = millis();
  recoveryStep = 0;
  recoveryDir = dir;
}

// Функция обработки восстановления линии (вызывается в loop)
void processRecovery() {
  if (!RECOVERING) return;
  
  if (millis() - recoveryStartTime >= recoveryStep * RECOVERY_STEP_DELAY) {
    float spiral_factor = (float)recoveryStep / RECOVERY_STEPS;
    
    if (recoveryDir == 0) { // Толкнули влево - спираль вправо
      int left_speed = -80 + (int)(260 * spiral_factor);
      int right_speed = 180 - (int)(20 * spiral_factor);
      drive(left_speed, right_speed);
    } else { // Толкнули вправо - спираль влево
      int right_speed = -80 + (int)(260 * spiral_factor);
      int left_speed = 180 - (int)(20 * spiral_factor);
      drive(left_speed, right_speed);
    }
    
    recoveryStep++;
    
    // Проверяем датчики во время движения
    int sensor1 = map(analogRead(A0), LEFT_MIN, LEFT_MAX, 0, 100);
    int sensor2 = map(analogRead(A1), RIGHT_MIN, RIGHT_MAX, 0, 100);
    
    // Если нашли линию или завершили все шаги, прекращаем восстановление
    if (sensor1 > 60 || sensor2 > 60 || recoveryStep >= RECOVERY_STEPS) {
      RECOVERING = false;
    }
  }
}

// Функция обработки кнопки (вызывается в loop)
void processButton() {
  if (digitalRead(A2) == 1 && BUTTON_OLD == 0) {
    delay(5); // Антидребезг
    if (digitalRead(A2) == 1) {
      IS_TURNED = !IS_TURNED;
      if (IS_TURNED) {
        tone(SOUND_PIN, 1000, 100);
      } else {
        tone(SOUND_PIN, 500, 100);
        drive(0, 0); // Остановка при выключении
      }
    }
  }
  BUTTON_OLD = digitalRead(A2);
}

// Функция следования по линии (вызывается в loop)
void processLineFollowing() {
  if (!IS_TURNED || RECOVERING) return;
  
  // Чтение и нормализация значений сенсоров
  int sensor1 = map(analogRead(A0), LEFT_MIN, LEFT_MAX, 0, 100);
  int sensor2 = map(analogRead(A1), RIGHT_MIN, RIGHT_MAX, 0, 100);
  
  // Если оба датчика на белом (потеряли линию)
  if (sensor1 < 60 && sensor2 < 60) {
    if (millis() - TIME_SOUND > 30 * 1000) {
      end_music();
      drive(0, 0);
      exit(0);
    }
    startRecovery(LAST_DIRECTION);
    return;
  }
  
  // Обновляем таймер когда видим линию
  TIME_SOUND = millis();
  
  // Вычисление ошибки для ПД-регулятора
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
  processButton();      // Обработка кнопки
  processRecovery();    // Обработка восстановления линии
  processLineFollowing(); // Следование по линии
  
  delay(10); // Небольшая задержка для стабильности
  }
