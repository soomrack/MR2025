// Определение пинов подключения
#define POWER_LEFT_MOTOR_PIN 6        // Пин для управления скоростью левого мотора
#define DIRECTION_LEFT_MOTOR_PIN 7    // Пин для управления направлением левого мотора  
#define POWER_RIGHT_MOTOR_PIN 5       // Пин для управления скоростью правого мотора
#define DIRECTION_RIGHT_MOTOR_PIN 4   // Пин для управления направлением правого мотора
#define SOUND_PIN 9                   // Пин для звуковых сигналов
#define CALIBRATE_BUTTON_PIN A2       // Пин для кнопки старта/остановки 
#define LEFT_SENSOR_PIN A0            // Пин левого датчика линии (считывает уровень отражения)
#define RIGHT_SENSOR_PIN A1           // Пин правого датчика линии

// Настройка параметров и поиска
#define PID_GAIN_P 8.0                // Коэффициент PID-регулятора (пропорциональный, влияет на резкость реакции)
#define PID_GAIN_D 6.0                // Коэффициент PID-регулятора (дифференциальный, сглаживает колебания)
#define BASE_SPEED 90                 // Базовая скорость движения по линии (от 0 до 255)
#define SEARCH_SPEED 110              // Скорость движения при поиске линии (выше базовой)
#define LIGHT_THRESHOLD 50            // Порог для датчиков линии (0-100)
#define SPIRAL_INCREASE 3             // Шаг увеличения амплитуды спирали при поиске 
#define SPIRAL_INTERVAL 50            // Интервал времени между увеличениями спирали (в миллисекундах)
#define SEARCH_TIMEOUT 20000          // Максимальное время поиска линии (20 секунд до остановки)
#define SOUND_INTERVAL 2000           // Интервал между звуковыми сигналами при движении (2 секунды)

// Состояния робота

enum RobotState {
  STATE_IDLE,           // Состояние ожидания - ждет команды
  STATE_CALIBRATING,    // Состояние калибровки - вращается и калибрует датчики
  STATE_FOLLOW_LINE,    // Состояние следования - движется по черной линии
  STATE_SEARCH_LINE     // Состояние поиска - потерял линию и ищет ее
};

// Переменные для хранения состояния
// Переменные для калибровки датчиков (определяют диапазон значений)
int left_min = 1023;                  // Минимальное значение с левого датчика (черная линия)
int left_max = 0;                     // Максимальное значение с левого датчика (белый фон)
int right_min = 1023;                 // Минимальное значение с правого датчика (черная линия)  
int right_max = 0;                    // Максимальное значение с правого датчика (белый фон)

// Переменные для управления состоянием
RobotState currentState = STATE_IDLE; // Текущее состояние робота (ожидание)
int last_direction = 0;               // Последнее направление движения при потере линии (0-влево, 1-вправо)
int error_old = 0;                    // Предыдущее значение ошибки для дифференциальной составляющей PID
bool button_old_state = HIGH;         // Предыдущее состояние кнопки (для обнаружения нажатия)
bool button_pressed = false;          // Флажок, что кнопка была нажата 
bool searching = false;               // Флажок, что робот находится в режиме поиска линии
bool spiral_direction = false;        // Направление вращения при спиральном поиске (false-влево, true-вправо)
int spiral_step = 0;                  // Текущий шаг спирали 
unsigned long spiral_timer = 0;       // Таймер для отсчета интервалов увеличения спирали
unsigned long search_start_time = 0;  // Время начала поиска линии 
unsigned long last_sound_time = 0;    // Время последнего звукового сигнала 

// Функция для управления скоростью и направлением моторов
void drive(int left_speed, int right_speed) {
  // Установка направления левого мотора
  digitalWrite(DIRECTION_LEFT_MOTOR_PIN, left_speed >= 0);
  // Установка направления правого мотора
  digitalWrite(DIRECTION_RIGHT_MOTOR_PIN, right_speed >= 0);
  // Установка скорости левого мотора
  analogWrite(POWER_LEFT_MOTOR_PIN, abs(left_speed));
  // Установка скорости правого мотора
  analogWrite(POWER_RIGHT_MOTOR_PIN, abs(right_speed));
}

// Функция для остановки робота и сброса флагов
void stopRobot() {
  drive(0, 0);                        // Подает нулевую скорость на оба мотора
  searching = false;                  // Сбрасывает флаг поиска линии
  tone(SOUND_PIN, 500, 500);          // Издает звуковой сигнал остановки (500 Гц, 500 мс)
}

// Функция для запуска поиска линии
void startSearch() {
  searching = true;                   // Устанавливает флаг поиска
  spiral_step = 0;                    // Сбрасывает шаг спирали к начальному значению
  spiral_timer = millis();            // Запоминает текущее время для таймера спирали
  // Меняет направление поиска на основе последнего
  spiral_direction = (last_direction == 0) ? 1 : 0;
  search_start_time = millis();       // Запоминает время начала поиска для контроля таймаута
  tone(SOUND_PIN, 300, 200);          // Издает звуковой сигнал начала поиска (300 Гц, 200 мс)
}

// Алгоритм поиска линии по спирали, происходит вращение робота с увеличением радиуса поворота
void spiralSearch() {
  // Проверяет не превышено ли максимальное время поиска
  if (millis() - search_start_time > SEARCH_TIMEOUT) {
    currentState = STATE_IDLE;        // Переходит в состояние ожидания при таймауте
    stopRobot();                      // Останавливает робота
    return;                           // Выходит из функции досрочно
  }
  
  // Проверяет пришло ли время увеличить шаг спирали
  if (millis() - spiral_timer > SPIRAL_INTERVAL) {
    spiral_step += SPIRAL_INCREASE;   // Увеличивает шаг спирали
    spiral_timer = millis();          // Сбрасывает таймер для следующего увеличения
  }
  
  // Вычисляет скорости моторов для спирального движения
  int left_speed = SEARCH_SPEED - spiral_step;  // Левый мотор замедляется на величину шага
  int right_speed = SEARCH_SPEED - spiral_step; // Правый мотор замедляется на величину шага
  
  // Применяет спиральное движение в зависимости от направления
  if (spiral_direction) {
  // Движение по спирали вправо: левый мотор медленнее
    drive(left_speed, SEARCH_SPEED);
  } else {
  // Движение по спирали влево: правый мотор медленнее
    drive(SEARCH_SPEED, right_speed);
  }
}

// PID-регулятор для плавного движения по линии
void lineFollowing(int left_sensor, int right_sensor) {
  // Вычисляет ошибку как разность показаний датчиков
  // Положительная ошибка - смещение вправо, отрицательная - влево
  int error = (left_sensor - right_sensor);
  
  // Вычисляет управляющий сигнал PID-регулятора:
  // P-составляющая: пропорциональна текущей ошибке
  // D-составляющая: пропорциональна скорости изменения ошибки
  double control_signal = error * PID_GAIN_P + (error - error_old) * PID_GAIN_D;
  
  // Применяет управляющий сигнал к моторам:
  // Левый мотор: базовая скорость + корректировка
  // Правый мотор: базовая скорость - корректировка
  int left_motor_speed = constrain(BASE_SPEED + control_signal, -255, 255);
  int right_motor_speed = constrain(BASE_SPEED - control_signal, -255, 255);
  
  // Передает вычисленные скорости моторам
  drive(left_motor_speed, right_motor_speed);
  error_old = error;  // Сохраняет текущую ошибку для следующей итерации D-составляющей
  
  // Проверяет нужен ли звуковой сигнал
  if (millis() - last_sound_time > SOUND_INTERVAL) {
    tone(SOUND_PIN, 800, 100);       // Издает короткий сигнал (800 Гц, 100 мс)
    last_sound_time = millis();      // Обновляет время последнего сигнала
  }
}

  // Функция считывает значения датчиков и нормализует их
void readSensors(int& left_value, int& right_value) {
  // Считывает аналоговые значения с датчиков (0-1023)
  int left_raw = analogRead(LEFT_SENSOR_PIN);
  int right_raw = analogRead(RIGHT_SENSOR_PIN);
  
  // Преобразует значения в нормализованный диапазон 0-100
  // используя калибровочные значения (min/max)
  left_value = map(left_raw, left_min, left_max, 0, 100);
  right_value = map(right_raw, right_min, right_max, 0, 100);
  
  // Ограничивает значения диапазоном 0-100 на случай выхода за границы
  left_value = constrain(left_value, 0, 100);
  right_value = constrain(right_value, 0, 100);
}

// Калибровка датчиков линии путем вращения и записи min/max значений
void calibrateSensors() {
  // Сигнализирует о начале калибровки
  tone(SOUND_PIN, 800, 200);
  
  // Сбрасывает калибровочные значения к начальным
  left_min = 1023; right_min = 1023; // Устанавливает высокие минимальные значения
  left_max = 0; right_max = 0;       // Устанавливает низкие максимальные значения
  
  // Запоминает время начала калибровки
  unsigned long calibration_start = millis();
  // Выполняет калибровку в течение 4 секунд
  while (millis() - calibration_start < 4000) {
    // Вращает робота на месте для сканирования разных поверхностей
    drive(140, -140);
    
    // Считывает текущие значения датчиков
    int left_current = analogRead(LEFT_SENSOR_PIN);
    int right_current = analogRead(RIGHT_SENSOR_PIN);
    
    // Обновляет минимальные значения если текущие меньше
    if (left_current < left_min) left_min = left_current;
    if (right_current < right_min) right_min = right_current;
    // Обновляет максимальные значения если текущие больше  
    if (left_current > left_max) left_max = left_current;
    if (right_current > right_max) right_max = right_current;
    
    delay(10); // Задержка для стабильности измерений
  }
  
  // Останавливает робота после калибровки
  drive(0, 0);
  // Сигнализирует об окончании калибровки
  tone(SOUND_PIN, 1500, 500);
  delay(500); // Задержка для завершения звукового сигнала
}


// Обработка нажатия кнопки без дребезга контактов
void handleButton() {
  // Считывает текущее состояние кнопки (LOW - нажата, HIGH - отпущена)
  bool current_button_state = digitalRead(CALIBRATE_BUTTON_PIN);
  
  // Обнаружение фронта нажатия: переход из HIGH в LOW
  if (current_button_state == LOW && button_old_state == HIGH) {
    delay(50); // Задержка для защиты от дребезга контактов (50 мс)
    // Повторная проверка после задержки для подтверждения нажатия
    if (digitalRead(CALIBRATE_BUTTON_PIN) == LOW) {
      button_pressed = true; // Устанавливает флаг нажатия кнопки
    }
  }
  
  // Сохраняет текущее состояние кнопки для следующей итерации
  button_old_state = current_button_state;
}

// Функция управления состояниями робота с использованием switch-case
void updateStateMachine() {
  // Сбор данных с датчиков для принятия решений
  int left_value, right_value;
  readSensors(left_value, right_value);
  
  // Вычисление условий для переходов между состояниями
  bool line_lost = (left_value > LIGHT_THRESHOLD && right_value > LIGHT_THRESHOLD);     // Оба датчика видят светлое - линия потеряна
  bool line_found = (left_value < LIGHT_THRESHOLD || right_value < LIGHT_THRESHOLD);    // Хотя бы один датчик видит темное - линия найдена
  bool search_timeout = (millis() - search_start_time > SEARCH_TIMEOUT);                // Превышено время поиска
  
  // Управление состояниями робота через switch-case
  switch (currentState) {
    
    case STATE_IDLE:
      // Условие перехода (кнопка на пине A2 нажата)
      if (button_pressed) {
        button_pressed = false;           // Сброс флага нажатия
        currentState = STATE_CALIBRATING; // Переход в состояние калибровки
        tone(SOUND_PIN, 1200, 200);       // Сигнал начала работы
      }
      drive(0, 0);  // Остановка в состоянии ожидания
      break;
      
    case STATE_CALIBRATING:
      // Автоматический переход после завершения калибровки
      calibrateSensors();              // Выполняем калибровку датчиков
      currentState = STATE_FOLLOW_LINE; // Переходим в состояние следования за линией
      tone(SOUND_PIN, 2000, 300);      // Сигнал готовности к работе
      break;
      
    case STATE_FOLLOW_LINE:
      // Условие перехода, что линия потеряна (оба датчика не видят черного)
      if (line_lost) {
        currentState = STATE_SEARCH_LINE; // Переходим в состояние поиска линии
        startSearch();                  // Инициализируем алгоритм поиска
      }
      // Переход (кнопка остановки нажата)
      else if (button_pressed) {
        button_pressed = false;        // Сбрасываем флаг нажатия
        currentState = STATE_IDLE;     // Переходим в состояние ожидания
        stopRobot();                   // Полностью останавливаем робота
      }
      // Если линия обнаружена, то продолжаем движение
      else {
        lineFollowing(left_value, right_value); // Выполняем PID-регулирование движения по линии
      }
      break;
      
    case STATE_SEARCH_LINE:
      // Если линия найдена (хотя бы один датчик видит черное)
      if (line_found) {
        searching = false;             // Сбрасываем флаг поиска
        // Запоминаем с какого датчика пришел сигнал для будущего поиска
        last_direction = (left_value < LIGHT_THRESHOLD) ? 0 : 1;
        tone(SOUND_PIN, 1000, 300);    // Сигнал нахождения линии
        currentState = STATE_FOLLOW_LINE; // Возвращаемся к следования по линии
      }
      // Если превышено время поиска
      else if (search_timeout) {
        currentState = STATE_IDLE;     // Переходим в состояние ожидания
        stopRobot();                   // Останавливаем робота
      }
      // Если кнопка остановки нажата
      else if (button_pressed) {
        button_pressed = false;        // Сбрасываем флаг нажатия
        currentState = STATE_IDLE;     // Переходим в состояние ожидания
        stopRobot();                   // Останавливаем робота
      }
      // Если продолжаем поиск линии
      else {
        spiralSearch();                // Выполняем спиральный алгоритм поиска
      }
      break;
  }
}

// Функция настройки Ардуино (разово)
void setup() {
  // Настройка пинов управления моторами как выходы
  pinMode(POWER_LEFT_MOTOR_PIN, OUTPUT);
  pinMode(DIRECTION_LEFT_MOTOR_PIN, OUTPUT);
  pinMode(POWER_RIGHT_MOTOR_PIN, OUTPUT);
  pinMode(DIRECTION_RIGHT_MOTOR_PIN, OUTPUT);
  
  // Настройка пина звука как выхода
  pinMode(SOUND_PIN, OUTPUT);
  
  // Настройка пина кнопки как входа с подтяжкой к питанию
  pinMode(CALIBRATE_BUTTON_PIN, INPUT_PULLUP);

  // Сигнал запуска системы: два коротких звука
  tone(SOUND_PIN, 1000, 100);  // Первый сигнал 1000 Гц, 100 мс
  delay(100);                  // Пауза между сигналами
  tone(SOUND_PIN, 1500, 100);  // Второй сигнал 1500 Гц, 100 мс
  
  // Устанавливает начальное состояние робота - ожидание
  currentState = STATE_IDLE;
}

// Главный цикл программы - только вызовы функций
// Выполняется бесконечно в цикле после setup()
void loop() {
  // Обработка ввода (состояние кнопки)
  handleButton();
  
  // Обновление состояний робота через state machine
  updateStateMachine();
}
