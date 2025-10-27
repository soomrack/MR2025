#define LEFT_PWM 6
#define LEFT_DIR 7
#define RIGHT_PWM 5
#define RIGHT_DIR 4
#define SOUND 9

#define K_P 8.0
#define K_D 5.0
#define BASE_SPEED 100
#define SEARCH_SPEED 150

#define LIGHT_THRESHOLD 60  // Порог обнаружения линии
#define SPIRAL_INCREASE 5   // Шаг увеличения радиуса спирали
#define SPIRAL_INTERVAL 100 // Интервал изменения спирали (мс)

long long int TIME_SOUND = 0;
int LEFT_MIN = 1023;
int LEFT_MAX = 0;
int RIGHT_MIN = 1023;
int RIGHT_MAX = 0;

int LAST_DIRECTION = 0;
int ERROLD = 0;

bool IS_ACTIVE = false;     // Флаг активности робота
bool BUTTON_OLD = 1;
bool SEARCHING = false;     // Флаг поиска линии
bool SPIRAL_DIRECTION = 0;  // Направление спирали (0-влево, 1-вправо)
int spiral_step = 0;        // Текущий шаг спирали
unsigned long spiral_timer = 0;

void drive(int left, int right) {
  digitalWrite(LEFT_DIR, left > 0);
  digitalWrite(RIGHT_DIR, right > 0);
  analogWrite(LEFT_PWM, abs(left));
  analogWrite(RIGHT_PWM, abs(right));
}

void calibrate() {
  drive(120, -120); // Вращение на месте
  delay(4000);
  drive(0, 0);
}

void startSearch() {
  SEARCHING = true;
  spiral_step = 0;
  spiral_timer = millis();
  SPIRAL_DIRECTION = (LAST_DIRECTION == 0) ? 1 : 0; // Направление спирали
}

void spiralSearch() {
  if (millis() - spiral_timer > SPIRAL_INTERVAL) {
    spiral_step += SPIRAL_INCREASE;
    spiral_timer = millis();
  }

  int left_speed = SEARCH_SPEED - spiral_step;
  int right_speed = SEARCH_SPEED - spiral_step;

  if (SPIRAL_DIRECTION) {
    drive(left_speed, SEARCH_SPEED);
  } else {
    drive(SEARCH_SPEED, right_speed);
  }
}

void lineFollowing(int s1, int s2) {
  double err = (s1 - s2);
  double u = err * K_P + (err - ERROLD) * K_D;
  drive(constrain(BASE_SPEED + u, -250, 250), 
        constrain(BASE_SPEED - u, -250, 250));
  ERROLD = err;
}

void checkSensors() {
  int s1 = map(analogRead(A0), LEFT_MIN, LEFT_MAX, 0, 100);
  int s2 = map(analogRead(A1), RIGHT_MIN, RIGHT_MAX, 0, 100);

  if (SEARCHING) {
    if (s1 > LIGHT_THRESHOLD || s2 > LIGHT_THRESHOLD) {
      // Обнаружена линия, завершаем поиск
      SEARCHING = false;
      LAST_DIRECTION = (s1 > LIGHT_THRESHOLD) ? 0 : 1;
    } else {
      spiralSearch();
    }
  } else {
    if (s1 < LIGHT_THRESHOLD && s2 < LIGHT_THRESHOLD) {
      startSearch();
    } else {
      lineFollowing(s1, s2);
    }
  }
}

void setup() {
  // Инициализация пинов управления моторами
  pinMode(LEFT_PWM, OUTPUT);
  pinMode(LEFT_DIR, OUTPUT);
  pinMode(RIGHT_PWM, OUTPUT);
  pinMode(RIGHT_DIR, OUTPUT);
  pinMode(SOUND, OUTPUT);
  
  // Инициализация пина кнопки (подтяжка к питанию)
  pinMode(A2, INPUT_PULLUP);
  
  // Калибровка датчиков (вращение на месте и запись min/max значений)
  int tim = millis();
  while (millis() - tim < 4000) {
    drive(120, -120);  // Вращение на месте
    int left = analogRead(A0);
    int right = analogRead(A1);
    
    // Обновление минимальных и максимальных значений
    if (left < LEFT_MIN) LEFT_MIN = left;
    if (left > LEFT_MAX) LEFT_MAX = left;
    if (right < RIGHT_MIN) RIGHT_MIN = right;
    if (right > RIGHT_MAX) RIGHT_MAX = right;
  }
  drive(0, 0);  // Остановка после калибровки
  
  // Ожидание нажатия кнопки для старта
  while (true) {
    if (digitalRead(A2) == LOW) {  // Кнопка нажата (используем INPUT_PULLUP)
      delay(50);  // Дебаунс
      if (digitalRead(A2) == LOW) {
        break;
      }
    }
  }
  
  // Короткий звуковой сигнал о готовности
  tone(SOUND, 1000, 200);
  delay(200);
}

void loop() {
  if (IS_ACTIVE) {
    checkSensors();
  }
  
  // Обработка кнопки
  if (digitalRead(A2) == HIGH && BUTTON_OLD == LOW) {
    IS_ACTIVE = !IS_ACTIVE;
    SEARCHING = false;
  }
  BUTTON_OLD = digitalRead(A2);
}
