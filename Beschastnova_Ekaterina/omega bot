#define PIN_SENS_L A1
#define PIN_SENS_R A0
#define PIN_MOTOR_L_DIR 7
#define PIN_MOTOR_R_DIR 4
#define PIN_MOTOR_L_SPEED 6
#define PIN_MOTOR_R_SPEED 5

#define PIN_PIEZO A2

enum SearchState {
  SEARCH_TURN_LEFT,   // едет влево (дугой или поворотом)
  SEARCH_FORWARD      // едет вперёд
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

const int happyNotes[] = {523, 659, 784, 1047}; // C5, E5, G5, C6
const int happyDurations[] = {100, 100, 100, 150};
const int happyPause = 30;

void setup() {
  Serial.begin(9600);
  pinMode(PIN_MOTOR_L_SPEED, OUTPUT);
  pinMode(PIN_MOTOR_L_DIR, OUTPUT);
  pinMode(PIN_MOTOR_R_SPEED, OUTPUT);
  pinMode(PIN_MOTOR_R_DIR, OUTPUT);
  pinMode(PIN_PIEZO, OUTPUT);

  digitalWrite(PIN_MOTOR_L_DIR, HIGH);
  digitalWrite(PIN_MOTOR_R_DIR, HIGH);
}

void forward(int speed) {
  digitalWrite(PIN_MOTOR_L_DIR, HIGH);
  digitalWrite(PIN_MOTOR_R_DIR, HIGH);
  analogWrite(PIN_MOTOR_L_SPEED, speed);
  analogWrite(PIN_MOTOR_R_SPEED, speed);
}

void left(int speed_1, int speed_2) {
  digitalWrite(PIN_MOTOR_L_DIR, LOW);
  digitalWrite(PIN_MOTOR_R_DIR, HIGH);
  analogWrite(PIN_MOTOR_L_SPEED, speed_2);
  analogWrite(PIN_MOTOR_R_SPEED, speed_1);
}

void right(int speed_1, int speed_2) {
  digitalWrite(PIN_MOTOR_L_DIR, HIGH);
  digitalWrite(PIN_MOTOR_R_DIR, LOW);
  analogWrite(PIN_MOTOR_L_SPEED, speed_1);
  analogWrite(PIN_MOTOR_R_SPEED, speed_2);
}

void stopMotors() {
  digitalWrite(PIN_MOTOR_L_DIR, HIGH);
  digitalWrite(PIN_MOTOR_R_DIR, HIGH);
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

// Звук
void updateSound() {
  unsigned long now = millis();

  if (sosRequested) {
    soundState = SOUND_SOS_DOT1;
    sosRequested = false;
    tone(PIN_PIEZO, 1000);
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

  switch (soundState) {
    // --- SOS ---
    case SOUND_SOS_DOT1:
      if (now - lastActionTime >= 100) {
        noTone(PIN_PIEZO);
        soundState = SOUND_SOS_PAUSE1;
        lastActionTime = now;
      }
      break;

    case SOUND_SOS_PAUSE1:
      if (now - lastActionTime >= 50) {
        tone(PIN_PIEZO, 1000);
        soundState = SOUND_SOS_DASH;
        lastActionTime = now;
      }
      break;

    case SOUND_SOS_DASH:
      if (now - lastActionTime >= 400) {
        noTone(PIN_PIEZO);
        soundState = SOUND_SOS_PAUSE2;
        lastActionTime = now;
      }
      break;

    case SOUND_SOS_PAUSE2:
      if (now - lastActionTime >= 50) {
        tone(PIN_PIEZO, 1000);
        soundState = SOUND_SOS_DOT2;
        lastActionTime = now;
      }
      break;

    case SOUND_SOS_DOT2:
      if (now - lastActionTime >= 100) {
        noTone(PIN_PIEZO);
        soundState = SOUND_SOS_END;
      }
      break;

    case SOUND_SOS_END:
      // ничего не делаем, остаёмся в IDLE
      break;

    // Весёлая мелодия
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
        soundState = SOUND_HAPPY_END;
      }
      break;

    case SOUND_HAPPY_END:
      // завершено
      break;

    default:
      soundState = SOUND_IDLE;
      noTone(PIN_PIEZO);
      break;
  }

  if (soundState == SOUND_SOS_END || soundState == SOUND_HAPPY_END) {
    soundState = SOUND_IDLE;
  }
}

void lineFollower() {
  unsigned long now = millis();
  int sens_l = analogRead(PIN_SENS_L);
  int sens_r = analogRead(PIN_SENS_R);

  // Проверяем: линия найдена?
  bool lineDetected = (sens_l > 800 || sens_r > 800);

  if (lineDetected) {
    // Вышли из поиска
    inSearchMode = false;

    // Играем "радостный" звук раз в 3 сек
    if (now - lastHappyTime > 3000) {
      requestHappy();
      lastHappyTime = now;
    }

    // Следуем по линии
    if (sens_l > 800 && sens_r > 800) {
      forward(155);
    } else if (sens_l > 800 && sens_r < 800) {
      left(200, 130);
    } else if (sens_r > 800 && sens_l < 800) {
      right(200, 130);
    }
  } else {
    // Линия НЕ найдена → входим в режим поиска
    if (!inSearchMode) {
      inSearchMode = true;
      searchState = SEARCH_TURN_LEFT;
      searchStartTime = now;
      lastActionTime = now;
      requestSOS(); // один раз при потере
    }
    // Аварийная остановка через 5 сек
    if (now - searchStartTime > 5000) {
      stopMotors();
      return;
    }
    switch (searchState) {
    case SEARCH_TURN_LEFT:
      // Двигаемся влево (например, дугой или на месте)
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
}

void loop() {
  updateSound();    
  lineFollower();    
}
