// Настройки пинов
#define MOTOR_L_PWM 6
#define MOTOR_L_DIR 7
#define MOTOR_R_PWM 5
#define MOTOR_R_DIR 4
#define BUTTON_PIN 2
#define SOUND_PIN 9
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
const int line_seen_threshold_ms = 800;
const int line_seen_threshold_2_ms = 5000; // for 2nd stage of searching
const int line_search_stop_threshold_ms = 10 * 1000; // Сколько мс робот будет искать линию до остановки.

// Прочие переменные
const int sensor_mode = -1; // +1 или -1
float lastError = 0; float integral = 0;
bool systemActive = false;
bool lastButtonState = LOW; // чтобы при первом запуске нажимать только 1 раз

// Управление моторами 
void setMotors(int left, int right) {
    // Serial.print("Motor L:"); Serial.print(left); Serial.print(" R:"); Serial.println(right);
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
    int l_read = analogRead(SENSOR_L);
    int r_read = analogRead(SENSOR_R);
    bool lost = (l_read < l_threshold && r_read < r_threshold);
    if (!lost) line_seen_millis=millis();
    return lost;
}

bool is_aligned() {
    int l_read = analogRead(SENSOR_L);
    int r_read = analogRead(SENSOR_R);
    int back = analogRead(SENSOR_BACK);
    return ((l_read >= l_threshold || r_read >= r_threshold) && back >= b_threshold);
}

bool back_is_on_line() {
    int back = analogRead(SENSOR_BACK);
    return (back >= b_threshold);
}

double tanh(double x) {
    const double e2x=exp(2*x);
    return (e2x-1.0)/(e2x+1.0);
}

// Поиск линии, если она потеряна 
void recoverLine() {
    Serial.println("Line lost.");
    tone(SOUND_PIN, 500, 500);
    const unsigned long recover_start_ms = millis();
    long i = 0;
    while (lineLost()) {
        const int search_speed = 120;
        const int turn_time_ms = 10;
        const float radius_coeff = 0.01; // linear coeff, from zero to infinity
        int go_forward_ms = (turn_time_ms/2+1) * radius_coeff * i;

        setMotors(-search_speed, search_speed); // turn left
        delay(turn_time_ms);
        setMotors(search_speed, search_speed); // go forward
        delay(go_forward_ms);
        setMotors(0, 0);

        // delay(1);
        if (millis() - recover_start_ms >= line_search_stop_threshold_ms) {
            // Остановка
            tone(SOUND_PIN, 500, 1000);
            setMotors(0, 0);
            systemActive = false;
            return;
        }
        i+=1;
    }

    // Линия найдена. Останавливаемся и вращаемся.
    setMotors(0, 0);
    Serial.println("Line found. Stop and rotate.");
    tone(SOUND_PIN, 500, 100);
    delay(200);
    tone(SOUND_PIN, 500, 100);
    const int adjusting_speed = 120;
    const int wiggle_timeout_ms = 500; // Сколько мс робот будет ехать вперёд/назад до разворота
    unsigned long line_seen_wiggle_millis = millis(); 
    // Аналогично line_seen_millis, но для этого этапа.
    // Предполагаем, что линия рядом, поэтому ставим время сейчас.
    while (!is_aligned()) {
        if (!lineLost()) { // Передний датчик на линии
            Serial.println("On line: Front");
            unsigned long started_going_ms = millis();
            line_seen_wiggle_millis = millis(); 
            setMotors(adjusting_speed, adjusting_speed);
            while( millis() - started_going_ms <= wiggle_timeout_ms ) {
                if (back_is_on_line()) break;
                if (lineLost()) break;
                delay(1); // go forward
            }
            
            started_going_ms = millis();
            setMotors(adjusting_speed, -adjusting_speed);
            while ( millis() - started_going_ms <= wiggle_timeout_ms ) {
                if (back_is_on_line()) break;
                if (!lineLost()) break;
                delay(1); // turn right
            }
        }
        else if (back_is_on_line()) { // Задний датчик на линии
            Serial.println("On line: Back");
            unsigned long started_going_ms = millis();
            line_seen_wiggle_millis = millis(); 
            setMotors(-adjusting_speed, -adjusting_speed);
            while( millis() - started_going_ms <= wiggle_timeout_ms ) {
                if (!back_is_on_line()) break;
                if (!lineLost()) break;
                delay(1); // go backward
            }
            
            started_going_ms = millis();
            setMotors(adjusting_speed, -adjusting_speed);
            while ( millis() - started_going_ms <= wiggle_timeout_ms ) {
                if (back_is_on_line()) break;
                if (!lineLost()) break;
                delay(1); // turn right
            }
        }
        else { // линия между датчиками
            Serial.println("On line: Between sensors");
            unsigned long started_going_ms = millis();
            setMotors(adjusting_speed, -adjusting_speed);
            while( millis() - started_going_ms <= wiggle_timeout_ms ) {
                if (back_is_on_line()) break;
                if (lineLost()) break;
                delay(1); // turn right
            }
        }
        if (millis() - line_seen_wiggle_millis >= line_seen_threshold_2_ms ) {
            // Линия потеряна снова
            Serial.println("Line lost while aligning");
            break;
        }
    }
    Serial.println("Alligned.");
    setMotors(0, 0);
    tone(SOUND_PIN, 500, 100);
    delay(200);
    tone(SOUND_PIN, 500, 100);
    delay(200);
    tone(SOUND_PIN, 500, 100);
    // Линия выровнена
}

// Следование по линии
void followTrack() {
    int l_val = map(analogRead(SENSOR_L), l_minVal, l_maxVal, 0, 100);
    int r_val = map(analogRead(SENSOR_R), r_minVal, r_maxVal, 0, 100);

    float error =  sensor_mode * (l_val - r_val);
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

    systemActive = false;
    Serial.println("Press button to start.");
    while (digitalRead(BUTTON_PIN) == HIGH) delay(10);
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
        if (
            lineLost() 
            && (millis() - line_seen_millis) >= line_seen_threshold_ms
        ) recoverLine();
        else followTrack();
    }
}
