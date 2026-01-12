// -- Пины моторов --
const int LEFT_MOTOR_SPEED = 6;  // Скорость ЛЕВОГО мотора
const int RIGHT_MOTOR_SPEED = 5; // Скорость ПРАВОГО мотора
const int LEFT_MOTOR_DIR = 7;    // Направление ЛЕВОГО мотора (1 - вперёд)
const int RIGHT_MOTOR_DIR = 4;   // Направление ПРАВОГО мотора (1 - вперёд)

// -- Датчики линии --
const int LEFT_SENSOR = A0;
const int RIGHT_SENSOR = A1;

// -- Константы --
const int BLACK_THRESHOLD = 650;  // Граница черного (больше 650 - черный)
const int BASE_SPEED = 90;        // Низкая скорость движения
const int TURN_SPEED = 180;       // Увеличенная скорость поворота
const unsigned long SEARCH_TIME = 15000; // Время поиска линии (15 секунд)

// -- Переменные состояния --
bool lineLost = false;
unsigned long lostTime = 0;

void setup() {
  // Настройка пинов моторов как выходы
  pinMode(LEFT_MOTOR_SPEED, OUTPUT);
  pinMode(RIGHT_MOTOR_SPEED, OUTPUT);
  pinMode(LEFT_MOTOR_DIR, OUTPUT);
  pinMode(RIGHT_MOTOR_DIR, OUTPUT);
  
  // Настройка пинов датчиков как входы
  pinMode(LEFT_SENSOR, INPUT);
  pinMode(RIGHT_SENSOR, INPUT);
  
  // Установка направления движения вперед
  digitalWrite(LEFT_MOTOR_DIR, HIGH);
  digitalWrite(RIGHT_MOTOR_DIR, HIGH);
  
  Serial.begin(9600); // Для отладки
}

void loop() {
  // Чтение значений с датчиков
  int leftValue = analogRead(LEFT_SENSOR);
  int rightValue = analogRead(RIGHT_SENSOR);
  
  // Определение состояния датчиков
  bool leftOnBlack = (leftValue > BLACK_THRESHOLD);
  bool rightOnBlack = (rightValue > BLACK_THRESHOLD);
  
  Serial.print("Left: ");
  Serial.print(leftValue);
  Serial.print(" Right: ");
  Serial.print(rightValue);
  Serial.print(" Lost: ");
  Serial.println(lineLost);
  
  // Проверка потери линии
  if (!leftOnBlack && !rightOnBlack) {
    if (!lineLost) {
      // Только что потеряли линию
      lineLost = true;
      lostTime = millis();
      Serial.println("Line lost! Starting search...");
    }
  } else {
    // Линия найдена
    lineLost = false;
  }
  
  // Логика движения
  if (lineLost) {
    searchLineSpiral();
  } else {
    followLine(leftOnBlack, rightOnBlack);
  }
}

void followLine(bool leftBlack, bool rightBlack) {
  // Оба датчика на черном - едем прямо
  if (leftBlack && rightBlack) {
    moveForward(BASE_SPEED);
  }
  // Левый датчик на черном, правый на белом - поворот направо
  else if (leftBlack && !rightBlack) {
    turnRight(TURN_SPEED);
  }
  // Правый датчик на черном, левый на белом - поворот налево
  else if (!leftBlack && rightBlack) {
    turnLeft(TURN_SPEED);
  }
  // Оба датчика на белом - продолжаем предыдущее движение
  else {
    moveForward(BASE_SPEED);
  }
}

void searchLineSpiral() {
  unsigned long currentTime = millis();
  unsigned long timeLost = currentTime - lostTime;
  
  // Проверка времени поиска
  if (timeLost > SEARCH_TIME) {
    // Время поиска истекло - остановка
    stopMotors();
    Serial.println("Search time expired. Stopping.");
    return;
  }
  
  // Спиральный поиск: постепенно увеличиваем радиус
  int spiralPhase = (timeLost / 800) % 4; // Меняем фазу каждые 0.5 s
  int speedMultiplier = min(100, 150 + (timeLost / 100)); // Постепенно увеличиваем скорость
  
  switch (spiralPhase) {
    case 0:
      // Резкий поворот
      turnLeft(TURN_SPEED + 20);
      break;
    case 1:
      // Плавная дуга
      analogWrite(LEFT_MOTOR_SPEED, BASE_SPEED + 10);
      analogWrite(RIGHT_MOTOR_SPEED, TURN_SPEED + speedMultiplier / 2);
      break;
    case 2:
      // Резкий поворот в другую сторону
      turnRight(TURN_SPEED + 20);
      break;
    case 3:
      // Плавная дуга в другую сторону
      analogWrite(LEFT_MOTOR_SPEED, TURN_SPEED + speedMultiplier / 2);
      analogWrite(RIGHT_MOTOR_SPEED, BASE_SPEED + 10);
      break;
  }
}

// -- Функции управления моторами --

void moveForward(int speed) {
  digitalWrite(LEFT_MOTOR_DIR, HIGH);
  digitalWrite(RIGHT_MOTOR_DIR, HIGH);
  analogWrite(LEFT_MOTOR_SPEED, speed);
  analogWrite(RIGHT_MOTOR_SPEED, speed);
}

void turnLeft(int speed) {
  digitalWrite(LEFT_MOTOR_DIR, LOW);   // Левый мотор назад
  digitalWrite(RIGHT_MOTOR_DIR, HIGH); // Правый мотор вперед
  analogWrite(LEFT_MOTOR_SPEED, speed);
  analogWrite(RIGHT_MOTOR_SPEED, speed);
}

void turnRight(int speed) {
  digitalWrite(LEFT_MOTOR_DIR, HIGH);  // Левый мотор вперед
  digitalWrite(RIGHT_MOTOR_DIR, LOW);  // Правый мотор назад
  analogWrite(LEFT_MOTOR_SPEED, speed);
  analogWrite(RIGHT_MOTOR_SPEED, speed);
}

void stopMotors() {
  analogWrite(LEFT_MOTOR_SPEED, 0);
  analogWrite(RIGHT_MOTOR_SPEED, 0);
}
