#include <Wire.h>

// КНОПКИ
#define BTN 12


// ПОДКЛЮЧЕНИЕ МОТОРОВ
int IN1 = 4;   // Левый мотор
int IN2 = 5;
int IN3 = 7;   // Правый мотор
int IN4 = 6;

// ==== ДАТЧИКИ ЛИНИИ ====
int lineSensors[2] = {A0, A1};
int sensorBlack[2];
int sensorWhite[2];
int threshold[2];
int lost_threshold[2];
int condition = 0;        // Текущее остояние 

// НАСТРОЙКИ ДВИЖЕНИЯ 
int baseSpeed = 100;

// ==== PID ====
float Kp = 1.5;
float Ki = 0;
float Kd = 1;
float error = 0, lastError = 0, integral = 0;

// Счётчик потери линии
unsigned long line_loss_time = 0;
const int time_to_stop = 8 * 100; // Сколько тиков loop робот будет искать линию до остановки.

void setup() {
  Serial.begin(9600);
  Wire.begin();

  // Моторы
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);

  // Кнопки
  pinMode(BTN, INPUT_PULLUP);

  // Инициализация датчиков
  for (int i = 0; i < 2; i++) {
    sensorBlack[i] = 1023;
    sensorWhite[i] = 0;
    threshold[i] = 512;
  }
}

void Motors(int leftspeed, int rightspeed)
{
  if (leftspeed > 255) leftspeed = 255;
  if (leftspeed < -255) leftspeed = -255;
  if (rightspeed > 255) rightspeed = 255;
  if (rightspeed < -255) rightspeed = -255;

  if (leftspeed > 0)
  {
    digitalWrite(IN3, 1);
    analogWrite(IN4, leftspeed);
  }
  else
  {
    digitalWrite(IN3, 0);
    analogWrite(IN4, -leftspeed);
  }

  if (rightspeed > 0)
  {
    digitalWrite(IN1, 1);
    analogWrite(IN2, rightspeed);
  }
  else
  {
    digitalWrite(IN1, 0);
    analogWrite(IN2, -rightspeed);
  }
}

//  КАЛИБРОВКА 
void calibrateSensors() {
 
  Serial.println("\n КАЛИБРОВКА");
  Serial.println("Установите робота на белое:");

    while (digitalRead(BTN)) delay(10);
    while (!digitalRead(BTN)) delay(10);

    for (int i = 0; i < 2; i++) {
      sensorWhite[i] = analogRead(lineSensors[i]);
    }

  Serial.println("Установите робота на чёрное:");

    while (digitalRead(BTN)) delay(10);
    while (!digitalRead(BTN)) delay(10);

  while (digitalRead(BTN) != LOW)
  {
    for (int i = 0; i < 2; i++) {
      sensorBlack[i] = analogRead(lineSensors[i]);
    }
  }

  // Вычисляем пороги
  const float ratio = 1.1; // коэффицент порога потери линии

  for (int i = 0; i < 2; i++) {
    threshold[i] = (sensorBlack[i] + sensorWhite[i]) / 2;
    lost_threshold[i] = threshold[i] * ratio; //значение потери линии
  }

  //отладочная информация
  for (int i = 0; i < 2; i++) {
    Serial.print("Датчик "); Serial.print(i);
    Serial.print(": min="); Serial.print(sensorBlack[i]);
    Serial.print(" max="); Serial.print(sensorWhite[i]);
    Serial.print(" threshold="); Serial.println(threshold[i]);
  }

Serial.println("\nКалибровка завершена");

  // while (digitalRead(BTN)) delay(10);
  // while (!digitalRead(BTN)) delay(10);

}

void line_following() {
  int value[2];
  for (int i = 0; i < 2; i++) {
  value[i] = map(analogRead(lineSensors[i]), sensorBlack[i], sensorWhite[i], 0, 100);
  }
  
  float error = value[1] - value[0];
  integral += error;
  float derivative = error - lastError;
  float correction = Kp * error + Ki * integral + Kd * derivative;
  lastError = error;

  Motors(constrain(baseSpeed + correction, -250, 250), constrain(baseSpeed - correction, -250, 250));
 
}

// Проверка потери линии
bool lineLost() {
  return ((analogRead(lineSensors[0]) < lost_threshold[0]) && (analogRead(lineSensors[1]) < lost_threshold[1]));
}

// Движение по спирали
void spiral(){ 
  Motors(map(line_loss_time, 0, 10000, 0, 200), 200);
}

// Остановка
void stop(){ 
  Motors(0, 0);
}

// Движение вперёд
void forward(){ 
  Motors(100, 100);
}

// Вращение
void spin(){ 
  Motors(-200, 200);
}


// ОСНОВНОЙ ЦИКЛ
void loop()
{
  switch(condition)
  {
    case 0:   // калибровка
      calibrateSensors();
      condition = 6;
      break;
    
    case 1:   // следование по линии
      line_following();
      Serial.println("\n Cледование по линии");
      if (lineLost() == true) 
      {
        condition = 2;
      }
      break;

    case 2:   // движение по сперали
      spiral();
      Serial.println("\n Движение по сперали");
      if (lineLost() == false) 
      {
        line_loss_time = 0;
        condition = 3;
      }
      else if (lineLost() == false && line_loss_time > time_to_stop)
      {
        line_loss_time = 0;
        condition = 6;
      }
      break;
    
    case 3:   // остановка
      stop();
      Serial.println("\n Остановка");
      delay(500);
      condition = 4;
      break;

    case 4:   // движение вперёд
      forward();
      Serial.println("\n Движение вперёд");
      delay(100);
      condition = 5;
      break;

    case 5:   // вращение вокруг своей оси
      spin();
      Serial.println("\n Вращение вокруг своей оси");
      if (lineLost() == false) 
      {
        line_loss_time = 0;
        condition = 1;
      }
      else if (lineLost() == false && line_loss_time > time_to_stop)
      {
        line_loss_time = 0;
        condition = 6;
      }
      break;

    case 6:   // ожидание нажатия
      Serial.println("\n Ожидание нажатия");
      if (digitalRead(BTN))
      {
        condition = 1;
      }
      break;

    default:
      Serial.println("НЕОБРАБОТАННОЕ ИСКЛЮЧЕНИЕ!");
      break;
  }

    line_loss_time++;
    delay(10);
}