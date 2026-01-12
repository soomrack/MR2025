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
bool calibrated = false;   // Флаг завершённой калибровки
bool searching  = false;    // Флаг потери линии

// НАСТРОЙКИ ДВИЖЕНИЯ 
int baseSpeed = 100;

// ==== PID ====
float Kp = 1.5;
float Ki = 0;
float Kd = 1;
float error = 0, lastError = 0, integral = 0;

// Счётчик потери линии
unsigned long line_seen_millis = 0;
const int line_seen_threshold_ms = 800;
const int line_search_stop_threshold_ms = 8 * 1000; // Сколько мс робот будет искать линию до остановки.

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

  Serial.println("Робот готов для калибровки.");

  calibrateSensors();
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

void wait(unsigned long ms) {
    const unsigned long end_ms = millis() + ms;    
    while (millis() <= end_ms);
    {

    }
}


void stopMotors() {
  analogWrite(IN1, 0);
  analogWrite(IN2, 0);
  analogWrite(IN3, 0);
  analogWrite(IN4, 0);
}

//  КАЛИБРОВКА 
void calibrateSensors() {

  calibrated = false; // блокируем движение
  
  Serial.println("\n КАЛИБРОВКА");
  Serial.println("Установите робота на белое:");

  
    while (digitalRead(BTN)) delay(10);
    while (!digitalRead(BTN)) delay(10);

    for (int i = 0; i < 2; i++) {
      sensorWhite[i] = analogRead(lineSensors[i]);
    }
    
    wait(20);

   Serial.println("Установите робота на чёрное:");

    while (digitalRead(BTN)) delay(10);
    while (!digitalRead(BTN)) delay(10);

  while (digitalRead(BTN) != LOW)
  {
    for (int i = 0; i < 2; i++) {
      sensorBlack[i] = analogRead(lineSensors[i]);
    }
    
    wait(20);
  }

  // Вычисляем пороги
  const float ratio = 1.1; // коэффицент порога потери линии

  for (int i = 0; i < 2; i++) {
    threshold[i] = (sensorBlack[i] + sensorWhite[i]) / 2;
    lost_threshold[i] = threshold[i] * ratio; //значение потери линии
  }

  
  
  for (int i = 0; i < 2; i++) {
    Serial.print("Датчик "); Serial.print(i);
    Serial.print(": min="); Serial.print(sensorBlack[i]);
    Serial.print(" max="); Serial.print(sensorWhite[i]);
    Serial.print(" threshold="); Serial.println(threshold[i]);
  }

  while (digitalRead(BTN)) delay(10);
  while (!digitalRead(BTN)) delay(10);

  Serial.print("На старт");
  calibrated = true; // теперь можно ехать
  Serial.println("\nКалибровка завершена");

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
void lineLost() {
    if ((analogRead(lineSensors[0]) < lost_threshold[0]) && (analogRead(lineSensors[1]) < lost_threshold[1]) && (searching == false)) 
    {
      line_seen_millis = millis();
      searching = true;
    }
}
// Поиск линии
void lineSerch() {
  
   long int i = 0;

  while((analogRead(lineSensors[0]) < lost_threshold[0]) || (analogRead(lineSensors[1]) < lost_threshold[1]))
      {

        if(millis() > (line_seen_millis + line_search_stop_threshold_ms))
        {
          return;
        }
        
        Motors(map(i, 0, 10000, 0, 200), 200);

        wait(10);
        //delay(10);
        i ++;
      }
    Motors(0, 0);
    wait(500);
    Motors(100, 100);
    wait(100);
     while((analogRead(lineSensors[0]) < lost_threshold[0]) || (analogRead(lineSensors[1]) < lost_threshold[1]))
      {        
        Motors(-200, 200);

      
        wait(10);
      }
    
    searching = false;
}

// ОСНОВНОЙ ЦИКЛ
void loop()
{
  // Если не откалибровано — моторы не работают
  if (!calibrated == true)
  {
    delay(50);
    return;
  }
  
  lineLost();

  if (searching == true)
  {
    lineSerch();
    delay(50);
    return;
  }
  else
  {
    line_following();
  }

  delay(20);
}