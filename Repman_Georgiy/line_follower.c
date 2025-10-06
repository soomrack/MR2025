#define LEFT_MOTOR_PWM 5
#define LEFT_MOTOR_IN1 4
#define LEFT_MOTOR_IN2 3
#define RIGHT_MOTOR_PWM 6
#define RIGHT_MOTOR_IN1 7
#define RIGHT_MOTOR_IN2 8
#define LEFT_IR_SENSOR A0
#define RIGHT_IR_SENSOR A1

// Параметры PID-регулятора
float Kp = 25.0;    
float Ki = 0.01;    
float Kd = 15.0;    

// Переменные PID
float previousError = 0;
float integral = 0;

// Скорости
int baseSpeed = 150;
int maxSpeed = 255;
int minSpeed = 100;

// Переменная таймера
uint32_t timer = 0;

void setup() {
  // Настройка пинов
  pinMode(LEFT_MOTOR_PWM, OUTPUT);
  pinMode(LEFT_MOTOR_IN1, OUTPUT);
  pinMode(LEFT_MOTOR_IN2, OUTPUT);
  pinMode(RIGHT_MOTOR_PWM, OUTPUT);
  pinMode(RIGHT_MOTOR_IN1, OUTPUT);
  pinMode(RIGHT_MOTOR_IN2, OUTPUT);
  pinMode(LEFT_IR_SENSOR, INPUT);
  pinMode(RIGHT_IR_SENSOR, INPUT);
  
  Serial.begin(9600);
  stopMotors();
  delay(1000);
}

void loop() {
  // Чтение значений датчиков
  if(millis() - timer >= 10){
  int leftValue = analogRead(LEFT_IR_SENSOR);
  int rightValue = analogRead(RIGHT_IR_SENSOR);
  
  // Расчет ошибки для PID
  // -100: полностью слева, 0: по центру, +100: полностью справа
  int error = calculateError(leftValue, rightValue);
  
  // Вычисление коррекции с помощью PID
  int correction = computePID(error);
  
  // Применение коррекции к моторам
  motorControl(correction);
  // Обновление таймера
  timer = millis(); 
  }
  
}

int calculateError(int left, int right) {
  // Нормализованные значения (0-100)
  int normLeft = map(left, 0, 1023, 0, 100);
  int normRight = map(right, 0, 1023, 0, 100);
  
  // Расчет ошибки: положительная - смещение вправо, отрицательная - влево
  return normRight - normLeft;
}

int computePID(int error) {
  // Пропорциональная составляющая
  float proportional = Kp * error;
  
  // Интегральная составляющая
  integral += error;
  float integralTerm = Ki * integral;
  
  // Дифференциальная составляющая
  float derivative = Kd * (error - previousError);
  previousError = error;
  
  // Суммарная коррекция
  int correction = (int)(proportional + integralTerm + derivative);
  
  // Ограничение коррекции
  correction = constrain(correction, -baseSpeed, baseSpeed);
  
  return correction;
}

void motorControl(int correction) {
  int leftSpeed = baseSpeed;
  int rightSpeed = baseSpeed;
  
  if (correction > 0) {
    // Смещение вправо - уменьшаем скорость правых моторов
    rightSpeed -= correction;
  } else {
    // Смещение влево - уменьшаем скорость левых моторов  
    leftSpeed += correction; // correction отрицательный
  }
  
  // Ограничение скоростей
  leftSpeed = constrain(leftSpeed, minSpeed, maxSpeed);
  rightSpeed = constrain(rightSpeed, minSpeed, maxSpeed);
  
  // Управление моторами
  digitalWrite(LEFT_MOTOR_IN1, HIGH);
  digitalWrite(LEFT_MOTOR_IN2, LOW);
  analogWrite(LEFT_MOTOR_PWM, leftSpeed);
  
  digitalWrite(RIGHT_MOTOR_IN1, HIGH);
  digitalWrite(RIGHT_MOTOR_IN2, LOW);
  analogWrite(RIGHT_MOTOR_PWM, rightSpeed);
}

void stopMotors() {
  digitalWrite(LEFT_MOTOR_IN1, LOW);
  digitalWrite(LEFT_MOTOR_IN2, LOW);
  analogWrite(LEFT_MOTOR_PWM, 0);
  digitalWrite(RIGHT_MOTOR_IN1, LOW);
  digitalWrite(RIGHT_MOTOR_IN2, LOW);
  analogWrite(RIGHT_MOTOR_PWM, 0);
}