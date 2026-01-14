#define M1_PWM 5
#define M1_DIR 4
#define M2_PWM 6
#define M2_DIR 7  
#define R_Sensor A0
#define L_Sensor A1

int ButtonRead(int ButtonPin) // не понял как реализовать запуск сценария по кнопке, поэтому подсмотрел как это можно сделать
{
  int Data = digitalRead(ButtonPin);
  if(ButtonPin == 12) Data = !Data;
  return Data;
}
void initA()
{
  pinMode(M1_PWM, OUTPUT);
  pinMode(M1_DIR, OUTPUT);
  pinMode(M2_PWM, OUTPUT);
  pinMode(M2_DIR, OUTPUT);
  pinMode(R_Sensor, INPUT);
  pinMode(L_Sensor, INPUT);
}

void Motors(int speed1, int speed2)
{
  if (speed1 > 255) speed1 = 255;      // защита от дебила
  if (speed1 < -255) speed1 = -255;   // защита от дебила
  if (speed2 > 255) speed2 = 255;    // защита от дебила
  if (speed2 < -255) speed2 = -255; // защита от дебила

  if (speed1 > 0)                          //объяснить мотору куда крутить колесо
  {
    digitalWrite(M1_DIR, 1);
    analogWrite(M1_PWM, speed1);
  }

  else                                     //объяснить мотору куда крутить колесо
  {
    digitalWrite(M1_DIR, 0);
    analogWrite(M1_PWM, -speed1);
  }

  if (speed2 > 0)                          //объяснить мотору куда крутить колесо
  {
    digitalWrite(M2_DIR, 1);
    analogWrite(M2_PWM, speed2);
  }

  else                                     //объяснить мотору куда крутить колесо
  {
    digitalWrite(M2_DIR, 0);
    analogWrite(M2_PWM, -speed2);
  }
}

void moveForward(int speed)                                            // задать движение вперед
{
  Motors(speed, speed);
}

void stop()                                                            // остановка
{
  Motors(0, 0);
}

void moveBack(int speed)                                               //движение назад
{
  Motors(-speed, -speed);
}

void moveRight(int speed)                                              //движение вправо
{
  Motors(speed, -speed);
}

void moveLeft(int speed)                                               //движение влево
{
  Motors(-speed, speed);
}

void moveHide(int speed)
{
  Motors(speed, -0.3 * speed);
}


void moveForwardDelay(int speed, int time)                           // полноценная команда для сценария
{
  moveForward(speed);
  delay(time);
  stop();
}

void moveBackDelay(int speed, int time)                              // полноценная команда для сценария
{
  moveBack(speed);
  delay(time);
  stop();
}

void moveRightDelay(int speed, int time)                             // полноценная команда для сценария
{
  moveRight(speed);
  delay(time);
  stop();
}

void moveLeftDelay(int speed, int time)                              // полноценная команда для сценария
{
  moveLeft(speed);
  delay(time);
  stop();
}


void IKsensor()
{
  int R;
  int L; 
  do
  {
  L = analogRead(L_Sensor) * 100 / 1023;               // считываем значения с ик датчика и переводим значиния в шкалу от 0 до 10
  R = analogRead(R_Sensor) * 100 / 1023;              // считываем значения с ик датчика и переводим значиния в шкалу от 0 до 10
  Serial.print(L);
  Serial.print("\t");
  Serial.println(R);
  moveForward(150);                                 // старт начинается сразу на черной линии
  if ((L <= 60) && (R >= 60)) moveLeft(255);         // подруливание влево
  else if ((L >= 60) && (R <= 60)) moveRight(255);  // подруливание вправо
  } while ((L >= 60) && (R >= 60));

  do
  {
  L = analogRead(L_Sensor) * 100 / 1023;             // считываем значения с ик датчика и переводим значиния в шкалу от 0 до 10
  R = analogRead(R_Sensor) * 100 / 1023;
  moveHide(255);
  } while((L < 60) && (R < 60));

}   


void setup()          // обозначаем моторы и модули бутерброду на колесах
{
  initA();
  Serial.begin(9600);
}

void loop()           // сценаррий, камера, мотор, поехали
{
  IKsensor(); 

}