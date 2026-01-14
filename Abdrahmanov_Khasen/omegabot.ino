#define PIN_SENS_L A1
#define PIN_SENS_R A0
#define PIN_MOTOR_L_DIR 7
#define PIN_MOTOR_R_DIR 4
#define PIN_MOTOR_L_SPEED 6
#define PIN_MOTOR_R_SPEED 5
#define PIN_PIEZO A2

// Состояния поиска линии
enum SearchState {
  SEARCH_TURN_LEFT,   
  SEARCH_TURN_RIGHT,
  SEARCH_FORWARD,
  SEARCH_STOP
};

// Состояния звука
enum SoundState {
  SOUND_IDLE,
  SOUND_SOS_DOT1,
  SOUND_SOS_PAUSE1,
  SOUND_SOS_DASH,
  SOUND_SOS_PAUSE2,
  SOUND_SOS_DOT2,
  SOUND_SOS_END,

  SOUND_HAPPY_NOTE1,
  SOUND_HAPPY_PAUSE1,
  SOUND_HAPPY_NOTE2,
  SOUND_HAPPY_PAUSE2,
  SOUND_HAPPY_NOTE3,
  SOUND_HAPPY_PAUSE3,
  SOUND_HAPPY_NOTE4,
  SOUND_HAPPY_END
};

SearchState searchState = SEARCH_TURN_LEFT;
SoundState soundState = SOUND_IDLE;

unsigned long lastActionTime = 0;
unsigned long searchStartTime = 0;
unsigned long lastHappyTime = 0;

bool inSearchMode = false;
bool sosRequested = false;
bool happyRequested = false;

// Константы для настройки
const int LINE_THRESHOLD = 800;    // Порог обнаружения линии
const int SEARCH_TIMEOUT = 5000;   // Таймаут поиска (5 сек)
const int HAPPY_INTERVAL = 3000;   // Интервал "радостного" звука (3 сек)

// Настройки моторов
const int MOTOR_FORWARD_SPEED = 155;
const int MOTOR_TURN_SPEED_HIGH = 200;
const int MOTOR_TURN_SPEED_LOW = 130;
const int MOTOR_SEARCH_SPEED = 100;

// Настройки звука SOS
const int SOS_FREQUENCY = 880;     // Ля 5-й октавы (было 1000)
const int SOS_DOT_DURATION = 200;  // Увеличенная длительность точек
const int SOS_DASH_DURATION = 600; // Увеличенная длительность тире
const int SOS_PAUSE_DURATION = 100; // Увеличенные паузы

// Новые звуки для "радостной" мелодии - мажорное трезвучие с арпеджио
const int happyNotes[] = {262, 330, 392, 523, 659, 784}; // C4, E4, G4, C5, E5, G5
const int happyDurations[] = {150, 120, 130, 140, 130, 200};
const int happyPause = 40;

void setup() {
  Serial.begin(9600);
  pinMode(PIN_MOTOR_L_SPEED, OUTPUT);
  pinMode(PIN_MOTOR_L_DIR, OUTPUT);
  pinMode(PIN_MOTOR_R_SPEED, OUTPUT);
  pinMode(PIN_MOTOR_R_DIR, OUTPUT);
  pinMode(PIN_PIEZO, OUTPUT);

  // Инициализация моторов - остановка
  stopMotors();
}

void forward(int speed) {
  digitalWrite(PIN_MOTOR_L_DIR, HIGH);
  digitalWrite(PIN_MOTOR_R_DIR, HIGH);
  analogWrite(PIN_MOTOR_L_SPEED, speed);
  analogWrite(PIN_MOTOR_R_SPEED, speed);
}

void left(int speed_left, int speed_right) {
  digitalWrite(PIN_MOTOR_L_DIR, LOW);
  digitalWrite(PIN_MOTOR_R_DIR, HIGH);
  analogWrite(PIN_MOTOR_L_SPEED, speed_left);
  analogWrite(PIN_MOTOR_R_SPEED, speed_right);
}

void right(int speed_left, int speed_right) {
  digitalWrite(PIN_MOTOR_L_DIR, HIGH);
  digitalWrite(PIN_MOTOR_R_DIR, LOW);
  analogWrite(PIN_MOTOR_L_SPEED, speed_left);
  analogWrite(PIN_MOTOR_R_SPEED, speed_right);
}

void stopMotors() {
  analogWrite(PIN_MOTOR_L_SPEED, 0);
  analogWrite(PIN_MOTOR_R_SPEED, 0);
}

void requestSOS() {
  if (soundState == SOUND_IDLE) {
    sosRequested = true;
  }
}

void requestHappy() {
  if (soundState == SOUND_IDLE) {
    happyRequested = true;
  }
}

// Обновление звуковой системы
void updateSound() {
  unsigned long now = millis();

  // Обработка запросов звуков
  if (sosRequested) {
    soundState = SOUND_SOS_DOT1;
    sosRequested = false;
    tone(PIN_PIEZO, SOS_FREQUENCY);
    lastActionTime = now;
    return;
  }

  if (happyRequested) {
    soundState = SOUND_HAPPY_NOTE1;
    happyRequested = false;
    tone(PIN_PIEZO, happyNotes[0], happyDurations[0]);
    lastActionTime = now;
    return;
  }

  // Обработка состояний звука
  switch (soundState) {
    // --- SOS (более медленный и мелодичный) ---
    case SOUND_SOS_DOT1:
      if (now - lastActionTime >= SOS_DOT_DURATION) {
        noTone(PIN_PIEZO);
        soundState = SOUND_SOS_PAUSE1;
        lastActionTime = now;
      }
      break;

    case SOUND_SOS_PAUSE1:
      if (now - lastActionTime >= SOS_PAUSE_DURATION) {
        tone(PIN_PIEZO, SOS_FREQUENCY);
        soundState = SOUND_SOS_DASH;
        lastActionTime = now;
      }
      break;

    case SOUND_SOS_DASH:
      if (now - lastActionTime >= SOS_DASH_DURATION) {
        noTone(PIN_PIEZO);
        soundState = SOUND_SOS_PAUSE2;
        lastActionTime = now;
      }
      break;

    case SOUND_SOS_PAUSE2:
      if (now - lastActionTime >= SOS_PAUSE_DURATION) {
        tone(PIN_PIEZO, SOS_FREQUENCY);
        soundState = SOUND_SOS_DOT2;
        lastActionTime = now;
      }
      break;

    case SOUND_SOS_DOT2:
      if (now - lastActionTime >= SOS_DOT_DURATION) {
        noTone(PIN_PIEZO);
        soundState = SOUND_SOS_END;
      }
      break;

    // --- Новая весёлая мелодия (арпеджио) ---
    case SOUND_HAPPY_NOTE1:
      if (now - lastActionTime >= happyDurations[0]) {
        noTone(PIN_PIEZO);
        soundState = SOUND_HAPPY_PAUSE1;
        lastActionTime = now;
      }
      break;

    case SOUND_HAPPY_PAUSE1:
      if (now - lastActionTime >= happyPause) {
        tone(PIN_PIEZO, happyNotes[1], happyDurations[1]);
        soundState = SOUND_HAPPY_NOTE2;
        lastActionTime = now;
      }
      break;

    case SOUND_HAPPY_NOTE2:
      if (now - lastActionTime >= happyDurations[1]) {
        noTone(PIN_PIEZO);
        soundState = SOUND_HAPPY_PAUSE2;
        lastActionTime = now;
      }
      break;

    case SOUND_HAPPY_PAUSE2:
      if (now - lastActionTime >= happyPause) {
        tone(PIN_PIEZO, happyNotes[2], happyDurations[2]);
        soundState = SOUND_HAPPY_NOTE3;
        lastActionTime = now;
      }
      break;

    case SOUND_HAPPY_NOTE3:
      if (now - lastActionTime >= happyDurations[2]) {
        noTone(PIN_PIEZO);
        soundState = SOUND_HAPPY_PAUSE3;
        lastActionTime = now;
      }
      break;

    case SOUND_HAPPY_PAUSE3:
      if (now - lastActionTime >= happyPause) {
        tone(PIN_PIEZO, happyNotes[3], happyDurations[3]);
        soundState = SOUND_HAPPY_NOTE4;
        lastActionTime = now;
      }
      break;

    case SOUND_HAPPY_NOTE4:
      if (now - lastActionTime >= happyDurations[3]) {
        noTone(PIN_PIEZO);
        // Добавляем дополнительные ноты для более богатой мелодии
        soundState = SOUND_HAPPY_PAUSE3;
        lastActionTime = now;
        // Временно используем то же состояние для продолжения мелодии
        static int extraNote = 4;
        if (extraNote < 6) {
          tone(PIN_PIEZO, happyNotes[extraNote], happyDurations[extraNote]);
          extraNote++;
        } else {
          soundState = SOUND_HAPPY_END;
          extraNote = 4; // Сброс для следующего раза
        }
      }
      break;

    // Завершающие состояния
    case SOUND_SOS_END:
    case SOUND_HAPPY_END:
      soundState = SOUND_IDLE;
      break;

    default:
      soundState = SOUND_IDLE;
      noTone(PIN_PIEZO);
      break;
  }
}

// Функция остановки робота
void stopRobotIfNeeded() {
  unsigned long now = millis();
  
  // Останавливаем робота если поиск длится дольше таймаута
  if (inSearchMode && (now - searchStartTime > SEARCH_TIMEOUT)) {
    stopMotors();
    searchState = SEARCH_STOP;
    Serial.println("Search timeout - robot stopped");
  }
}

// Функция следования по линии
void followLine() {
  // Если мы в режиме поиска, не выполняем следование по линии
  if (inSearchMode) {
    return;
  }
  
  unsigned long now = millis();
  int sens_l = analogRead(PIN_SENS_L);
  int sens_r = analogRead(PIN_SENS_R);

  // Проверяем: линия найдена?
  bool lineDetected = (sens_l > LINE_THRESHOLD || sens_r > LINE_THRESHOLD);

  if (lineDetected) {
    // Играем "радостный" звук с интервалом
    if (now - lastHappyTime > HAPPY_INTERVAL) {
      requestHappy();
      lastHappyTime = now;
    }

    // Следуем по линии
    if (sens_l > LINE_THRESHOLD && sens_r > LINE_THRESHOLD) {
      forward(MOTOR_FORWARD_SPEED);
    } else if (sens_l > LINE_THRESHOLD && sens_r < LINE_THRESHOLD) {
      left(MOTOR_TURN_SPEED_LOW, MOTOR_TURN_SPEED_HIGH);
    } else if (sens_r > LINE_THRESHOLD && sens_l < LINE_THRESHOLD) {
      right(MOTOR_TURN_SPEED_HIGH, MOTOR_TURN_SPEED_LOW);
    }
  } else {
    // Линия потеряна - переходим в режим поиска
    inSearchMode = true;
    searchState = SEARCH_TURN_LEFT;
    searchStartTime = now;
    lastActionTime = now;
    requestSOS();
    Serial.println("Line lost - entering search mode");
  }
}

// Функция поиска линии
void searchLine() {
  // Если не в режиме поиска, выходим
  if (!inSearchMode) {
    return;
  }
  
  unsigned long now = millis();
  int sens_l = analogRead(PIN_SENS_L);
  int sens_r = analogRead(PIN_SENS_R);

  // Проверяем: линия найдена во время поиска?
  bool lineDetected = (sens_l > LINE_THRESHOLD || sens_r > LINE_THRESHOLD);

  if (lineDetected) {
    // Линия найдена - выходим из режима поиска
    inSearchMode = false;
    Serial.println("Line found - exiting search mode");
    return;
  }

  // Алгоритм поиска линии
  switch (searchState) {
    case SEARCH_TURN_LEFT:
      left(180, 100); // левый мотор медленнее → поворот влево
      if (now - lastActionTime >= 1000) {
        lastActionTime = now;
        searchState = SEARCH_FORWARD;
      }
      break;

    case SEARCH_FORWARD:
      forward(100);
      if (now - lastActionTime >= 1000) {
        lastActionTime = now;
        searchState = SEARCH_TURN_LEFT;
      }
      break;
  }
}

void loop() {
  updateSound();    
  stopRobotIfNeeded();
  followLine();
  searchLine();   
}
