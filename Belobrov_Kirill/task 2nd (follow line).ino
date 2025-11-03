// Настройки пинов
#define MOTOR_L_PWM 6
#define MOTOR_L_DIR 7
#define MOTOR_R_PWM 5
#define MOTOR_R_DIR 4
#define BUTTON_PIN 12
#define SENSOR_L A0
#define SENSOR_R A1

// Коэффициенты ПД-регулятора
const float Kp = 5;
const float Kd = 2;
const float Ki = 0;
const int baseSpeed = 200;

// Переменные для калибровки 
int l_white, r_white, l_black, r_black, b_white, b_black;
int l_threshold, r_threshold, b_threshold;
int l_minVal = 1023, l_maxVal = 0;
int r_minVal = 1023, r_maxVal = 0;

// Счётчик потери линии
unsigned long line_seen_millis = 0;
const int line_seen_threshold_ms = 1500;
// const int line_seen_threshold_2_ms = 5000; // for 2nd stage of searching

// Прочие переменные
float lastError = 0; float integral = 0;
bool systemActive = false;
bool lastButtonState = LOW; // чтобы при первом запуске нажимать только 1 раз

// Управление моторами 
void setMotors(int left, int right) {
    Serial.print("Motor L:"); Serial.print(left); Serial.print(" R:"); Serial.println(right);
    digitalWrite(MOTOR_L_DIR, left >= 0);
    digitalWrite(MOTOR_R_DIR, right >= 0);
    analogWrite(MOTOR_L_PWM, constrain(abs(left), 0, 255));
    analogWrite(MOTOR_R_PWM, constrain(abs(right), 0, 255));
}

// Усреднённое считывание аналогового сигнала 
int readSmooth(int pin) {
    long total = 0;
    const char attempts = 15;
    for (int i = 0; i < attempts; i++) total += analogRead(pin);
    return total / attempts;
}

// Калибровка датчиков 
void calibrate() {
    Serial.println("Calibrating sensors. Place on white and press button.");
    while (digitalRead(BUTTON_PIN)) delay(10);
    while (!digitalRead(BUTTON_PIN)) delay(10);

    l_white = readSmooth(SENSOR_L);
    r_white = readSmooth(SENSOR_R);

    Serial.println("Place on black and press button.");
    while (digitalRead(BUTTON_PIN)) delay(10);
    while (!digitalRead(BUTTON_PIN)) delay(10);

    l_black = readSmooth(SENSOR_L);
    r_black = readSmooth(SENSOR_R);

    l_minVal = l_black;
    l_maxVal = l_white;
    r_minVal = r_black;
    r_maxVal = r_white;

    b_white = (l_white + r_white) / 2;
    b_black = (l_black + r_black) / 2;

    const float ratio = 0.1; // порог потери линии
    l_threshold = l_black + (l_white - l_black) * ratio;
    r_threshold = r_black + (r_white - r_black) * ratio;
    b_threshold = b_black + (b_white - b_black) * ratio;

    Serial.println("Calibration completed:");
    Serial.print("L: "); Serial.print(l_minVal); Serial.print(" - "); Serial.println(l_maxVal);
    Serial.print("R: "); Serial.print(r_minVal); Serial.print(" - "); Serial.println(r_maxVal);
    Serial.print("Thresholds: "); Serial.print(l_threshold); Serial.print(" / "); Serial.println(r_threshold);
}
    
  // Проверка потряна ли ниния
  bool lineLost() {
      int l_read = analogRead(SENSOR_L);
      int r_read = analogRead(SENSOR_R);
      return (l_read < l_threshold && r_read < r_threshold);
  }
  
  // Поиск линии при потере
  void findLine() {
    Serial.println("Поиск линии");
    bool isSearching = true;
    unsigned long startTime = millis();
  
    while (isSearching && millis() - startTime < 5000) {
      if (lastError > 0) setMotors(baseSpeed, -baseSpeed);
      else setMotors(-baseSpeed, baseSpeed);
  
      if (!lineLost()) {
        Serial.println("Line found");
        isSearching = false;
        return;
      }
    }
  
    setMotors(0, 0);
    Serial.println("Line lost");
    isSearching = false;
  }


// Следование по линии
void followTrack() {
    int l_val = map(analogRead(SENSOR_L), l_minVal, l_maxVal, 0, 100);
    int r_val = map(analogRead(SENSOR_R), r_minVal, r_maxVal, 0, 100);

    float error =  l_val - r_val;
    integral += error;
    float correction = Kp * error + Kd * (error - lastError) + Ki * integral; // PID

    int leftPower = baseSpeed + correction;
    int rightPower = baseSpeed - correction;

    setMotors(leftPower, rightPower);
    lastError = error;
}

// Инициализация
void setup() {
    Serial.begin(9600);

    pinMode(MOTOR_L_PWM, OUTPUT);
    pinMode(MOTOR_L_DIR, OUTPUT);
    pinMode(MOTOR_R_PWM, OUTPUT);
    pinMode(MOTOR_R_DIR, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    calibrate();

    Serial.println("Press button to start.");
    while (digitalRead(BUTTON_PIN) == HIGH) delay(10);
    systemActive = true;
}

// Основной цикл
void loop() {
    bool btnState = digitalRead(BUTTON_PIN);

    if (btnState == LOW && lastButtonState == HIGH) {
        systemActive = !systemActive;
        delay(80);
    }
    lastButtonState = btnState;

    if (systemActive) {
         if (lineLost()){ 
            findLine();
         }
        else followTrack();
    }
}
