// Настройки пинов
#define MOTOR_LEFT_PWM_PIN 6
#define MOTOR_LEFT_DIR_PIN 7
#define MOTOR_RIGHT_PWM_PIN 5
#define MOTOR_RIGHT_DIR_PIN 4
#define BUTTON_PIN 2
#define SOUND_PIN 9
#define SENSOR_LEFT_PIN A0
#define SENSOR_RIGHT_PIN A1
#define SENSOR_BACK_PIN A2

// Коэффициенты ПД-регулятора
const float Kp = 5;
const float Kd = 2;
const float Ki = 0;
const int base_speed = 200;

// Переменные для калибровки 
int left_white, right_white, left_black, right_black, back_white, back_black;
int left_threshold, right_threshold, back_threshold;
int left_minVal = 1023, left_maxVal = 0;
int right_minVal = 1023, right_maxVal = 0;

// Счётчик потери линии
unsigned long line_seen_millis = 0;
const int line_seen_threshold_ms = 800;
const int line_seen_threshold_2_ms = 5000; // for 2nd stage of searching
const int line_search_stop_threshold_ms = 10 * 1000; // Сколько мс робот будет искать линию до остановки.

// Прочие переменные
const int sensor_mode = -1; // +1 или -1
float last_error = 0; float integral = 0;
bool system_active = false;
bool last_button_state = LOW; // чтобы при первом запуске нажимать только 1 раз

// Управление моторами 
void set_motors(int left, int right) {
    // Serial.print("Motor L:"); Serial.print(left); Serial.print(" R:"); Serial.println(right);
    digitalWrite(MOTOR_LEFT_DIR_PIN, left >= 0);
    digitalWrite(MOTOR_RIGHT_DIR_PIN, right >= 0);
    analogWrite(MOTOR_LEFT_PWM_PIN, constrain(abs(left), 0, 255));
    analogWrite(MOTOR_RIGHT_PWM_PIN, constrain(abs(right), 0, 255));
}

// Усреднённое считывание аналогового сигнала 
int read_smooth(int pin) {
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

    left_white = read_smooth(SENSOR_LEFT_PIN);
    right_white = read_smooth(SENSOR_RIGHT_PIN);

    Serial.println("Place on black and press button.");
    while (digitalRead(BUTTON_PIN)) delay(10);
    while (!digitalRead(BUTTON_PIN)) delay(10);

    left_black = read_smooth(SENSOR_LEFT_PIN);
    right_black = read_smooth(SENSOR_RIGHT_PIN);

    left_minVal = left_black;
    left_maxVal = left_white;
    right_minVal = right_black;
    right_maxVal = right_white;

    back_white = (left_white + right_white) / 2;
    back_black = (left_black + right_black) / 2;

    const float ratio = 0.1; // порог потери линии
    left_threshold = left_black + (left_white - left_black) * ratio;
    right_threshold = right_black + (right_white - right_black) * ratio;
    back_threshold = back_black + (back_white - back_black) * ratio;

    Serial.println("Calibration completed:");
    Serial.print("L: "); Serial.print(left_minVal); Serial.print(" - "); Serial.println(left_maxVal);
    Serial.print("R: "); Serial.print(right_minVal); Serial.print(" - "); Serial.println(right_maxVal);
    Serial.print("Thresholds: "); Serial.print(left_threshold); Serial.print(" / "); Serial.println(right_threshold);
}

// Проверка потери линии
bool line_lost() {
    int l_read = analogRead(SENSOR_LEFT_PIN);
    int r_read = analogRead(SENSOR_RIGHT_PIN);
    bool lost = (l_read < left_threshold && r_read < right_threshold);
    if (!lost) line_seen_millis=millis();
    return lost;
}

bool is_aligned() {
    int left_read = analogRead(SENSOR_LEFT_PIN);
    int right_read = analogRead(SENSOR_RIGHT_PIN);
    int back_read = analogRead(SENSOR_BACK_PIN);
    return ((left_read >= left_threshold || right_read >= right_threshold) && back_read >= back_threshold);
}

bool back_is_on_line() {
    int back = analogRead(SENSOR_BACK_PIN);
    return (back >= back_threshold);
}

double tanh(double x) {
    const double e2x=exp(2*x);
    return (e2x-1.0)/(e2x+1.0);
}

// Поиск линии, если она потеряна 
void recover_line() {
    Serial.println("Line lost.");
    tone(SOUND_PIN, 500, 500);
    const unsigned long recover_start_ms = millis();
    long i = 0;
    while (line_lost()) {
        const int search_speed = 120;
        const int turn_time_ms = 10;
        const float radius_coeff = 0.01; // linear coeff, from zero to infinity
        int go_forward_ms = (turn_time_ms/2+1) * radius_coeff * i;

        set_motors(-search_speed, search_speed); // turn left
        delay(turn_time_ms);
        set_motors(search_speed, search_speed); // go forward
        delay(go_forward_ms);
        set_motors(0, 0);

        // delay(1);
        if (millis() - recover_start_ms >= line_search_stop_threshold_ms) {
            // Остановка
            tone(SOUND_PIN, 500, 1000);
            set_motors(0, 0);
            system_active = false;
            return;
        }
        i+=1;
    }

    // Линия найдена. Останавливаемся и вращаемся.
    set_motors(0, 0);
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
        if (!line_lost()) { // Передний датчик на линии
            Serial.println("On line: Front");
            unsigned long started_going_ms = millis();
            line_seen_wiggle_millis = millis(); 
            set_motors(adjusting_speed, adjusting_speed);
            while( millis() - started_going_ms <= wiggle_timeout_ms ) {
                if (back_is_on_line()) break;
                if (line_lost()) break;
                delay(1); // go forward
            }
            
            started_going_ms = millis();
            set_motors(adjusting_speed, -adjusting_speed);
            while ( millis() - started_going_ms <= wiggle_timeout_ms ) {
                if (back_is_on_line()) break;
                if (!line_lost()) break;
                delay(1); // turn right
            }
        }
        else if (back_is_on_line()) { // Задний датчик на линии
            Serial.println("On line: Back");
            unsigned long started_going_ms = millis();
            line_seen_wiggle_millis = millis(); 
            set_motors(-adjusting_speed, -adjusting_speed);
            while( millis() - started_going_ms <= wiggle_timeout_ms ) {
                if (!back_is_on_line()) break;
                if (!line_lost()) break;
                delay(1); // go backward
            }
            
            started_going_ms = millis();
            set_motors(adjusting_speed, -adjusting_speed);
            while ( millis() - started_going_ms <= wiggle_timeout_ms ) {
                if (back_is_on_line()) break;
                if (!line_lost()) break;
                delay(1); // turn right
            }
        }
        else { // линия между датчиками
            Serial.println("On line: Between sensors");
            unsigned long started_going_ms = millis();
            set_motors(adjusting_speed, -adjusting_speed);
            while( millis() - started_going_ms <= wiggle_timeout_ms ) {
                if (back_is_on_line()) break;
                if (line_lost()) break;
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
    set_motors(0, 0);
    tone(SOUND_PIN, 500, 100);
    delay(200);
    tone(SOUND_PIN, 500, 100);
    delay(200);
    tone(SOUND_PIN, 500, 100);
    // Линия выровнена
}

// Следование по линии
void follow_track() {
    int l_val = map(analogRead(SENSOR_LEFT_PIN), left_minVal, left_maxVal, 0, 100);
    int r_val = map(analogRead(SENSOR_RIGHT_PIN), right_minVal, right_maxVal, 0, 100);

    float error =  sensor_mode * (l_val - r_val);
    integral += error;
    float correction = Kp * error + Kd * (error - last_error) + Ki * integral; // PID

    int leftPower = base_speed + correction;
    int rightPower = base_speed - correction;

    set_motors(leftPower, rightPower);
    last_error = error;
}

// Инициализация
void setup() {
    Serial.begin(9600);

    pinMode(MOTOR_LEFT_PWM_PIN, OUTPUT);
    pinMode(MOTOR_LEFT_DIR_PIN, OUTPUT);
    pinMode(MOTOR_RIGHT_PWM_PIN, OUTPUT);
    pinMode(MOTOR_RIGHT_DIR_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    calibrate();

    system_active = false;
    Serial.println("Press button to start.");
    while (digitalRead(BUTTON_PIN) == HIGH) delay(10);
}

// Основной цикл
void loop() {
    bool btnState = digitalRead(BUTTON_PIN);

    if (btnState == LOW && last_button_state == HIGH) {
        system_active = !system_active;
        delay(80);
    }
    last_button_state = btnState;

    if (system_active) {
        if (
            line_lost() 
            && (millis() - line_seen_millis) >= line_seen_threshold_ms
        ) recover_line();
        else follow_track();
    }
}
