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
#define pid_gain_p 8.0                
#define pid_gain_d 6.0                
#define base_speed 90                 
#define search_speed 140              

// Настройки поиска линии 
#define light_threshold 50            
#define spiral_increase 3             
#define spiral_interval 50            
#define search_timeout 20000          

int left_min = 1023;                  
int left_max = 0;                     
int right_min = 1023;                 
int right_max = 0;                    

int last_direction = 0;               // Последнее направление движения при потере линии (0 — влево, 1 — вправо)
int last_error = 0;                   // Предыдущее значение ошибки для D-компоненты PID

bool is_turned_on = false;             
bool is_line_found = false;
bool CALIBRATE_BOTTON_PIN_old = 1;    // Предыдущее состояние кнопки 
bool spiral_direction = 0;            // Направление вращения при поиске (0 — влево, 1 — вправо)
int spiral_step = 0;                  
unsigned long spiral_timer = 0;       // Таймер для увеличения шага спирали
unsigned long search_start_time = 0;  
int s1;
int s2;

// Управление моторами 
void move(int left, int right) {
  digitalWrite(DIRECTION_LEFT_MOTOR_PIN, left > 0);   
  digitalWrite(DIRECTION_RIGHT_MOTOR_PIN, right > 0); 
  analogWrite(POWER_LEFT_MOTOR_PIN, abs(left));       
  analogWrite(POWER_RIGHT_MOTOR_PIN, abs(right));     
}


//  Остановка робота 
void stop_robot() {
  move(0, 0);                    
  is_turned_on = false;             
  tone(SOUND_PIN, 500, 500);         
}


// Начало поиска линии
void begin_searching() {
  spiral_step = 0;                           
  spiral_timer = millis();                   
  spiral_direction = (last_direction == 0) ? 1 : 0; 
  search_start_time = millis();              
}


// Спиральный поиск линии. Робот вращается в одну сторону, постепенно увеличивая радиус поворота.
void spiral_search() {
  // Увеличиваем "амплитуду спирали" через заданные интервалы
  if (millis() - spiral_timer > spiral_interval) {
    spiral_step += spiral_increase;
    spiral_timer = millis();
  }

  int left_speed = search_speed - spiral_step;
  int right_speed = search_speed - spiral_step;

  // Определяем направление вращения
  if (spiral_direction) {
    move(left_speed, search_speed);    // Вращение вправо
  } else {
    move(search_speed, right_speed);   // Вращение влево
  }
}


// Следование по линии
// Используется PID-регулятор для корректировки скорости колес в зависимости от разницы показаний датчиков.
void line_moving(int s1, int s2) {
  double err = (s1 - s2);                  // Ошибка — разность яркости левого и правого датчиков
  double u = err * pid_gain_p + (err - last_error) * pid_gain_d; // Вычисляем управляющее воздействие
  move(constrain(base_speed + u, -250, 250),  
        constrain(base_speed - u, -250, 250)); 
  last_error = err;                            // Запоминаем ошибку для следующего шага
}


// Проверка состояния датчиков и принятие решения 
void check_sensors() {
  // Считываем данные с аналоговых датчиков и переводим их в шкалу 0–100
  s1 = map(analogRead(LEFT_SENSOR_PIN), left_min, left_max, 0, 100);
  s2 = map(analogRead(RIGHT_SENSOR_PIN), right_min, right_max, 0, 100);

  // Если робот находится в режиме поиска линии 
  if (!is_line_found) {
    // Проверка тайм-аута — если поиск слишком долгий
    if (millis() - search_start_time > search_timeout) {
      stop_robot();
      return;
    }

    // Проверка — найдена ли линия
    if (s1 > light_threshold || s2 > light_threshold) {
        is_line_found = true;                          
        last_direction = (s1 > light_threshold) ? 0 : 1;    // Запоминаем направление
    } else {
        is_line_found = false;  // Продолжаем поиск
    }
  } else {
    // Если не ищем, но потеряли линию (оба датчика не видят черного)
    if (s1 < light_threshold && s2 < light_threshold) {
        is_line_found = false; // Переходим в режим поиска
    } else {
        is_line_found = true; // Следуем по линии
    }
  }
}

void sens_calibration() {
  int time = millis();
  while (millis() - time < 4000) {          // В течение 4 секунд вращаемся и считываем данные
    move(140, -140);                        // Вращение на месте
    int left = analogRead(LEFT_SENSOR_PIN);
    int right = analogRead(RIGHT_SENSOR_PIN);
    
    // Обновляем минимумы и максимумы
    if (left < left_min) left_min = left;
    if (left > left_max) left_max = left;
    if (right < right_min) right_min = right;
    if (right > right_max) right_max = right;
  }
  move(0, 0); // Остановка после калибровки
}

// Начальная настройка 
void setup() {
  // Настраиваем пины ввода/вывода
  pinMode(POWER_LEFT_MOTOR_PIN, OUTPUT);
  pinMode(DIRECTION_LEFT_MOTOR_PIN, OUTPUT);
  pinMode(POWER_RIGHT_MOTOR_PIN, OUTPUT);
  pinMode(DIRECTION_RIGHT_MOTOR_PIN, OUTPUT);
  pinMode(SOUND_PIN, OUTPUT);
  pinMode(CALIBRATE_BOTTON_PIN, INPUT_PULLUP); // Кнопка подключена с подтяжкой

  sens_calibration();
  
  // Ожидание нажатия кнопки для старта
  while (true) {
    if (digitalRead(CALIBRATE_BOTTON_PIN) == LOW) { 
      delay(50); // Защита от дребезга кнопки
      if (digitalRead(CALIBRATE_BOTTON_PIN) == LOW) break; // Кнопка нажата — выходим
    }
  }
  
  // Звуковой сигнал готовности
  tone(SOUND_PIN, 1000, 100);
  delay(50);
  tone(SOUND_PIN, 1000, 100);
  delay(300);
}


void loop() {
  int calib_button = digitalRead(CALIBRATE_BOTTON_PIN);
  if (calib_button == HIGH && CALIBRATE_BOTTON_PIN_old == LOW) {
    is_turned_on = !is_turned_on;    // Переключаем состояние активности
    delay(80);
  }

  CALIBRATE_BOTTON_PIN_old = calib_button; // Обновляем состояние кнопки

  if (is_turned_on) {
    check_sensors(); // Пока робот активен — считываем датчики и управляем движением
    if (is_line_found) {
      line_moving(s1, s2);
    }
    if (!is_line_found) {
      begin_searching();
      spiral_search();
    }
  }
}