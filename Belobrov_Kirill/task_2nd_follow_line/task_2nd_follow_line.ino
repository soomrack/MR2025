// Определение пинов подключения компонентов 
#define POWER_LEFT_MOTOR_PIN 6        
#define DIRECTION_LEFT_MOTOR_PIN 7   
#define POWER_RIGHT_MOTOR_PIN 5       
#define DIRECTION_RIGHT_MOTOR_PIN 4                   
#define CALIBRATE_BOTTON_PIN A2      
#define LEFT_SENSOR_PIN A0            
#define RIGHT_SENSOR_PIN A1           

// Параметры PID-регулятора и движения
#define PidGainP 8.0                
#define PidGainD 6.0                
#define BaseSpeed 90                 
#define SearchSpeed 140              

// Настройки поиска линии 
#define LightThreshold 50            
#define SpiralIncrease 10             
#define SpiralInterval 15            
#define SearchTimeout 20000          

int LeftMin = 1023;                  
int LeftMax = 0;                     
int RightMin = 1023;                 
int RightMax = 0;                    

int LastDirection = 0;               // Последнее направление при потере линии (0 — влево, 1 — вправо)
int LastError = 0;                   // Предыдущее значение ошибки для D-компоненты PID

bool IsTurnedOn = false;             
bool IsLineFound = false;
bool CALIBRATE_BOTTON_PIN_old = 1;    // Предыдущее состояние кнопки 
bool SpiralDirection = 0;            // Направление вращения при поиске (0 — влево, 1 — вправо)
int SpiralStep = 0;                  
unsigned long SpiralTimer = 0;       // Таймер для увеличения шага спирали
unsigned long SearchStartTime = 0;  
int s1;
int s2;

//Контроль моторов 
void Move(int left, int right) {
  digitalWrite(DIRECTION_LEFT_MOTOR_PIN, left > 0);   
  digitalWrite(DIRECTION_RIGHT_MOTOR_PIN, right > 0); 
  analogWrite(POWER_LEFT_MOTOR_PIN, abs(left));       
  analogWrite(POWER_RIGHT_MOTOR_PIN, abs(right));     
}


//  Остановка робота 
void RobotStop() {
  Move(0, 0);                    
  IsTurnedOn = false;                    
}


// Начало поиска линии
void StartSearching() {
  SpiralStep = 0;                           
  SpiralTimer = millis();                   
  SpiralDirection = (LastDirection == 0) ? 1 : 0; 
  SearchStartTime = millis();              
}


// Поиск линии по спирали. Робот вращается в одну сторону, увеличивая со временем радиус поворота.
void SearchSpiral() {
  // Увеличение "амплитуды спирали" через заданные интервалы
  if (millis() - SpiralTimer > SpiralInterval) {
    SpiralStep += SpiralIncrease;
    SpiralTimer = millis();
  }

  int LeftSpeed = constrain(SearchSpeed, 0, SpiralStep);
  int RightSpeed = constrain(SearchSpeed, 0, SpiralStep);

  // Определение направления вращения
  if (SpiralDirection) {
    Move(LeftSpeed, SearchSpeed/3);    // Вращение вправо
  } else {
    Move(SearchSpeed /3, RightSpeed);   // Вращение влево
  }
}


// Следование по линии. Применение PID-регулятор для регулировки скорости колес в зависимости от разницы показаний датчиков.
void MovingLine(int s1, int s2) {
  double err = (s1 - s2);                  // Ошибка — разность яркости левого и правого датчиков
  double u = err * PidGainP + (err - LastError) * PidGainD; // Вычисляем управляющее воздействие
  Move(constrain(BaseSpeed + u, -250, 250),  
        constrain(BaseSpeed - u, -250, 250)); 
  LastError = err;                            // Запоминаем ошибку для следующего шага
}


// Проверка состояния датчиков, принятие решения 
void CheckSensors() {
  // Считывание данных с датчиков и перевод показаний в шкалу 0–100
  s1 = map(analogRead(LEFT_SENSOR_PIN), LeftMin, LeftMax, 0, 100);
  s2 = map(analogRead(RIGHT_SENSOR_PIN), RightMin, RightMax, 0, 100);

  // Если робот находится в режиме поиска линии 
  if (!IsLineFound) {
    // Проверка тайм-аута — если поиск слишком долгий
    if (millis() - SearchStartTime > SearchTimeout) {
      RobotStop();
      return;
    }

    // Проверка — найдена ли линия
    if (s1 > LightThreshold || s2 > LightThreshold) {
        IsLineFound = true;                          
        LastDirection = (s1 > LightThreshold) ? 0 : 1;    // Запоминаем направление
    } else {
        IsLineFound = false;  // Продолжаем поиск
    }
  } else {
    // Если не ищем, но потеряли линию (оба датчика не видят черного)
    if (s1 < LightThreshold && s2 < LightThreshold) {
        IsLineFound = false; // Переходим в режим поиска
    } else {
        IsLineFound = true; // Двигаемся по линии
    }
  }
}

void SensCalibration() {
  int time = millis();
  while (millis() - time < 4000) {          // Вращение в течение 4 секунд и считывание данных
    Move(140, -140);                        // Вращение на месте
    int left = analogRead(LEFT_SENSOR_PIN);
    int right = analogRead(RIGHT_SENSOR_PIN);
    
    // Обновление минимумов и максимумов
    if (left < LeftMin) LeftMin = left;
    if (left > LeftMax) LeftMax = left;
    if (right < RightMin) RightMin = right;
    if (right > RightMax) RightMax = right;
  }
  Move(0, 0); // Остановка после калибровки
}
void SwitchButton() {
  int CalibButton = digitalRead(CALIBRATE_BOTTON_PIN);
  if (CalibButton == HIGH && CALIBRATE_BOTTON_PIN_old == LOW) {
    IsTurnedOn = !IsTurnedOn;    // Переключение состояние активности
    delay(80);
  }

  CALIBRATE_BOTTON_PIN_old = CalibButton; // Обновление состояние кнопки
}
// Начальная настройка 
void setup() {
  // Настройка пинов
  pinMode(POWER_LEFT_MOTOR_PIN, OUTPUT);
  pinMode(DIRECTION_LEFT_MOTOR_PIN, OUTPUT);
  pinMode(POWER_RIGHT_MOTOR_PIN, OUTPUT);
  pinMode(DIRECTION_RIGHT_MOTOR_PIN, OUTPUT);
  pinMode(CALIBRATE_BOTTON_PIN, INPUT_PULLUP); // Кнопка подключена с подтяжкой

  SensCalibration();
  
  // Ожидание нажатия кнопки для старта
  while (true) {
    if (digitalRead(CALIBRATE_BOTTON_PIN) == LOW) { 
      delay(50); // Защита от дребезжания кнопки
      if (digitalRead(CALIBRATE_BOTTON_PIN) == LOW) break; // Выход при нажатии кнопки
    }
  }
}


void loop() {
  SwitchButton();

  if (IsTurnedOn) {
    CheckSensors(); // Пока робот активен — считываем датчики и управляем движением
    if (IsLineFound) {
      MovingLine(s1, s2);
    }
    if (!IsLineFound) {
      StartSearching();
      SearchSpiral();
    }
  }
}
