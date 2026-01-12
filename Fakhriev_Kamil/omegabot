#define POWER_LEFT_MOTOR_PIN 6        
#define DIRECTION_LEFT_MOTOR_PIN 7   
#define POWER_RIGHT_MOTOR_PIN 5       
#define DIRECTION_RIGHT_MOTOR_PIN 4   
#define SOUND_PIN 9                   
#define CALIBRATE_BOTTON_PIN A2      
#define LEFT_SENSOR_PIN A0            
#define RIGHT_SENSOR_PIN A1           

// PD-регулятор
#define pid_gain_p 8.0                // Пропорциональный коэффициент P
#define pid_gain_d 6.0                // Дифференциальный коэффициент D
#define base_speed 90                 // Базовая скорость движения при следовании по линии
#define search_speed 140              // Скорость при поиске линии

// Настройки поиска линии 
#define light_threshold 50            // Порог срабатывания датчика света (0–100)
#define spiral_increase 3             // Увеличение амплитуды "спирали" при поиске
#define spiral_interval 50            // Интервал увеличения спирали (мс)
#define search_timeout 20000          // Максимальное время поиска линии (20 секунд)

int left_min = 1023;                  // Минимальное значение с левого датчика (для калибровки)
int left_max = 0;                     // Максимальное значение с левого датчика
int right_min = 1023;                 // Минимальное значение с правого датчика
int right_max = 0;                    // Максимальное значение с правого датчика

int last_direction = 0;               // Последнее направление движения при потере линии (0 — влево, 1 — вправо)
int errold = 0;                       // Предыдущее значение ошибки для D-компоненты PID

bool is_active = false;               // Флаг активности робота 
bool is_line_following = false;
bool is_line_searching = false;
bool CALIBRATE_BOTTON_PIN_old = 1;    // Предыдущее состояние кнопки
bool searching = false;               // Флаг поиска линии
bool spiral_direction = 0;            // Направление вращения при поиске (0 — влево, 1 — вправо)
int spiral_step = 0;                  // Текущее значение "расширения спирали"
unsigned long spiral_timer = 0;       // Таймер для увеличения шага спирали
unsigned long search_start_time = 0;  // Время начала поиска линии
int s1;
int s2;

// Управление моторами 
void drive(int left, int right) {
  digitalWrite(DIRECTION_LEFT_MOTOR_PIN, left > 0);   // Задаем направление вращения левого мотора
  digitalWrite(DIRECTION_RIGHT_MOTOR_PIN, right > 0); // Задаем направление вращения правого мотора
  analogWrite(POWER_LEFT_MOTOR_PIN, abs(left));       // Управляем мощностью (скоростью) левого мотора
  analogWrite(POWER_RIGHT_MOTOR_PIN, abs(right));     // Управляем мощностью правого мотора
}


//  Остановка робота 
void stop_robot() {
  drive(0, 0);                       // Полная остановка моторов
  is_active = false;                 // Отключаем активный режим
  searching = false;                 // Прерываем поиск
  tone(SOUND_PIN, 500, 500);         // Издаем звуковой сигнал (500 Гц, 0.5 сек)
}


// Начало поиска линии
void start_search() {
  searching = true;                          // Переход в режим поиска
  spiral_step = 0;                           // Сброс шага спирали
  spiral_timer = millis();                   // Сбрасываем таймер
  spiral_direction = (last_direction == 0) ? 1 : 0; // Меняем направление поиска относительно предыдущего
  search_start_time = millis();              // Фиксируем момент начала поиска
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
    drive(left_speed, search_speed);    // Вращение вправо
  } else {
    drive(search_speed, right_speed);   // Вращение влево
  }
}


// Следование по линии
// Используется PID-регулятор для корректировки скорости колес в зависимости от разницы показаний датчиков.
void line_following(int s1, int s2) {
  double err = (s1 - s2);                     // Ошибка — разность яркости левого и правого датчиков
  double u = err * pid_gain_p + (err - errold) * pid_gain_d; // Вычисляем управляющее воздействие
  drive(constrain(base_speed + u, -250, 250),  // Левое колесо — увеличиваем или уменьшаем скорость
        constrain(base_speed - u, -250, 250)); // Правое колесо — противоположная корректировка
  errold = err;                               // Запоминаем ошибку для следующего шага
}


// Проверка состояния датчиков и принятие решения 
void check_sensors() {
  // Считываем данные с аналоговых датчиков и переводим их в шкалу 0–100
  s1 = map(analogRead(LEFT_SENSOR_PIN), left_min, left_max, 0, 100);
  s2 = map(analogRead(RIGHT_SENSOR_PIN), right_min, right_max, 0, 100);

  // Если робот находится в режиме поиска линии 
  if (is_line_searching == true) {
    // Проверка тайм-аута — если поиск слишком долгий
    if (millis() - search_start_time > search_timeout) {
      stop_robot(); // Прекращаем работу
      return;
    }

    // Проверка — найдена ли линия
    if (s1 > light_threshold || s2 > light_threshold) {
        is_line_searching = false;
        is_line_following = true;                               // Линия найдена, прекращаем поиск
        last_direction = (s1 > light_threshold) ? 0 : 1;  // Запоминаем направление
    } else {
        is_line_searching = true;
        is_line_following = false;  // Продолжаем поиск
    }
  } else {
    // Если не ищем, но потеряли линию (оба датчика не видят черного)
    if (s1 < light_threshold && s2 < light_threshold) {
        is_line_searching = true;
        is_line_following = false; // Переходим в режим поиска
    } else {
        is_line_searching = false;
        is_line_following = true; // Следуем по линии
    }
  }
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

  // ЭТАП КАЛИБРОВКИ ДАТЧИКОВ 
  int time = millis();
  while (millis() - time < 4000) {         // В течение 4 секунд вращаемся и считываем данные
    drive(140, -140);                      // Вращение на месте
    int left = analogRead(LEFT_SENSOR_PIN);
    int right = analogRead(RIGHT_SENSOR_PIN);
    
    // Обновляем минимумы и максимумы
    if (left < left_min) left_min = left;
    if (left > left_max) left_max = left;
    if (right < right_min) right_min = right;
    if (right > right_max) right_max = right;
  }
  drive(0, 0); // Остановка после калибровки
  
  // Ожидание нажатия кнопки для старта
  while (true) {
    if (digitalRead(CALIBRATE_BOTTON_PIN) == LOW) { 
      delay(50); // Защита от дребезга кнопки
      if (digitalRead(CALIBRATE_BOTTON_PIN) == LOW) break; // Кнопка нажата — выходим
    }
  }
  
  // Звуковой сигнал готовности
  tone(SOUND_PIN, 1000, 200);
  delay(200);
}


void loop() {
  if (is_active) {
    check_sensors(); // Пока робот активен — считываем датчики и управляем движением
    if (is_line_following) {
      line_following(s1, s2);
    }
    if (is_line_searching) {
      start_search();
      spiral_search();
    }
  }
  
  if (digitalRead(CALIBRATE_BOTTON_PIN) == HIGH && CALIBRATE_BOTTON_PIN_old == LOW) {
    is_active = !is_active;    // Переключаем состояние активности
    searching = false;         // При остановке сбрасываем поиск
  }
  CALIBRATE_BOTTON_PIN_old = digitalRead(CALIBRATE_BOTTON_PIN); // Обновляем состояние кнопки

}
