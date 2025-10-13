// Настройки пинов
#define MOTOR_L_PWM 6
#define MOTOR_L_DIR 7
#define MOTOR_R_PWM 5
#define MOTOR_R_DIR 4
#define BUTTON_PIN 12
#define SENSOR_L A0
#define SENSOR_R A1

// Коэффициенты ПД-регулятора
float Kp = 3.0; 
float Kd = 1.2; 
int baseSpeed = 130;  

// Переменные калибровки
int l_white, r_white, l_black, r_black;
int l_threshold, r_threshold;
int l_minVal = 1023, l_maxVal = 0;
int r_minVal = 1023, r_maxVal = 0;

// Прочие переменные
float lastError = 0;
bool systemActive = false;
bool lastButtonState = HIGH;

// Управление моторами 
void setMotors(int left, int right) {
    digitalWrite(MOTOR_L_DIR, left >= 0);
    digitalWrite(MOTOR_R_DIR, right >= 0);
    analogWrite(MOTOR_L_PWM, constrain(abs(left), 0, 255));
    analogWrite(MOTOR_R_PWM, constrain(abs(right), 0, 255));
}

// Усреднённое считывание сигнала
int readSmooth(int pin) {
    long total = 0;
    for (int i = 0; i < 10; i++) total += analogRead(pin);
    return total / 10;
}

// Калибровка
void calibrate() {
    Serial.println("Calibration...");
    while (digitalRead(BUTTON_PIN)) delay(10);
    while (!digitalRead(BUTTON_PIN)) delay(10);

    l_white = readSmooth(SENSOR_L);
    r_white = readSmooth(SENSOR_R);

    while (digitalRead(BUTTON_PIN)) delay(10);
    while (!digitalRead(BUTTON_PIN)) delay(10);

    l_black = readSmooth(SENSOR_L);
    r_black = readSmooth(SENSOR_R);

    if (l_black < l_white) {
        l_minVal = l_black;
        l_maxVal = l_white;
    }
    else {
        l_minVal = l_white;
        l_maxVal = l_black;
    }

    if (r_black < r_white) {
        r_minVal = r_black;
        r_maxVal = r_white;
    }
    else {
        r_minVal = r_white;
        r_maxVal = r_black;
    }

    float ratio = 0.3; // 30% порог, более "мягкий"
    l_threshold = l_black + (l_white - l_black) * ratio;
    r_threshold = r_black + (r_white - r_black) * ratio;

    Serial.println("Calibration done");
}

// Проверка потери линии
bool lineLost() {
    int l_read = map(analogRead(SENSOR_L), l_minVal, l_maxVal, 0, 100);
    int r_read = map(analogRead(SENSOR_R), r_minVal, r_maxVal, 0, 100);
    return (l_read < l_threshold && r_read < r_threshold);
}

// Поиск линии
void recoverLine() {
    for (int k = 0; k < 3; k++) {
        if (lastError > 0)
            setMotors(70, -70);
        else if (lastError < 0)
            setMotors(-70, 70);
        else
            setMotors(-60, -60);

        delay(120);
        setMotors(0, 0);
        if (!lineLost()) break;
    }
}

// Следование по линии
void followTrack() {
    int l_val = map(readSmooth(SENSOR_L), l_minVal, l_maxVal, 0, 100);
    int r_val = map(readSmooth(SENSOR_R), r_minVal, r_maxVal, 0, 100);

    float error = r_val - l_val;
    float correction = Kp * error + Kd * (error - lastError);

    // Ограничим коррекцию, чтобы не крутился на месте
    correction = constrain(correction, -baseSpeed, baseSpeed);

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

    while (digitalRead(BUTTON_PIN) == HIGH) delay(10);
    systemActive = true;
}

// Основной цикл 
void loop() {
    bool btnState = digitalRead(BUTTON_PIN);

    if (btnState == LOW && lastButtonState == HIGH) {
        systemActive = !systemActive;
        delay(100);
    }
    lastButtonState = btnState;

    if (systemActive) {
        if (lineLost()) recoverLine();
        else followTrack();
    }
    else {
        setMotors(0, 0);
    }
}
