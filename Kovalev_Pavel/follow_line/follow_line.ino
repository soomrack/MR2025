// Настройки пинов
#define MOTOR_L_PWM 6
#define MOTOR_L_DIR 7
#define MOTOR_R_PWM 5
#define MOTOR_R_DIR 4
#define BUTTON_PIN 12
#define SENSOR_L A0
#define SENSOR_R A1
#define SENSOR_BACK A2

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

// Проверка потери линии
bool lineLost() {
    int l_read = map(analogRead(SENSOR_L), l_minVal, l_maxVal, 0, 100);
    int r_read = map(analogRead(SENSOR_R), r_minVal, r_maxVal, 0, 100);
    bool lost = (l_read < l_threshold && r_read < r_threshold);
    if (!lost) line_seen_millis=millis();
    return lost;
}

bool is_aligned() {
    int l_read = map(analogRead(SENSOR_L), l_minVal, l_maxVal, 0, 100);
    int r_read = map(analogRead(SENSOR_R), r_minVal, r_maxVal, 0, 100);
    int back = map(analogRead(SENSOR_BACK), r_minVal, r_maxVal, 0, 100);
    return ((l_read >= l_threshold || r_read >= r_threshold) && back >= b_threshold);
}

bool back_is_on_line() {
    int back = map(analogRead(SENSOR_BACK), r_minVal, r_maxVal, 0, 100);
    return (back >= b_threshold);
}

float tanh(unsigned long x) {
    const float e2x=exp(2*x);
    return (e2x-1.0)/(e2x-1.0);
}

// Поиск линии, если она потеряна 
void recoverLine() {
    float radius_coeff; // Коэффциент кривизны поворота: от -1 (вращение на месте) до +1 (прямая линия)
    const int recover_start_ms = millis();
    while (!lineLost()) {
        radius_coeff = 2*tanh((millis()-recover_start_ms)/4)-1;
        const int left_speed = 80;
        const int right_speed = left_speed * radius_coeff;
        setMotors(left_speed, right_speed);
        delay(1);
    }

    // Линия найдена. Останавливаемся и вращаемся.
    setMotors(0, 0);
    while (!is_aligned()) {
        if (!lineLost()) { // Передний датчик на линии
            int started_going_ms = millis();
            setMotors(80, 80);
            while( millis() - started_going_ms <= 100 ) {
                if (back_is_on_line()) break;
                if (lineLost()) break;
                delay(1); // go forward
            }
            
            started_going_ms = millis();
            setMotors(80, -80);
            while ( millis() - started_going_ms <= 100 ) {
                if (back_is_on_line()) break;
                if (!lineLost()) break;
                delay(1); // turn right
            }
        }
        else if (back_is_on_line()) { // Задний датчик на линии
            int started_going_ms = millis();
            setMotors(-80, -80);
            while( millis() - started_going_ms <= 100 ) {
                if (!back_is_on_line()) break;
                if (!lineLost()) break;
                delay(1); // go backward
            }
            
            started_going_ms = millis();
            setMotors(80, -80);
            while ( millis() - started_going_ms <= 100 ) {
                if (back_is_on_line()) break;
                if (!lineLost()) break;
                delay(1); // turn right
            }
        }
        else { // линия между датчиками
            int started_going_ms = millis();
            setMotors(80, 80);
            while( millis() - started_going_ms <= 100 ) {
                if (back_is_on_line()) break;
                if (lineLost()) break;
                delay(1); // go forward
            }
        }
    }
    setMotors(0, 0);
    // Линия выровнена
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
        if (lineLost() && (millis() - line_seen_millis >= line_seen_threshold_ms)) recoverLine();
        else followTrack();
    }
}
