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

const char recover_status_spiral = 1;
const char recover_status_align = 2;
char recover_status = 0;

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

bool line_lost_with_time_threshold() {
    return
        line_lost() 
        && (millis() - line_seen_millis) >= line_seen_threshold_ms;
}

// Поиск линии по спирали, если она потеряна
void spiral_search() {
    const char spiral_status_turn = 1;
    const char spiral_status_forward = 2;
    static char spiral_status = 0;
    static unsigned long go_until = 0;

    Serial.print("spiral_status: "); Serial.println(spiral_status, DEC); 

    static unsigned long recover_start_ms = 0;
    static long i = 0;
    i += 1;
    if (!line_lost()) {
        // Линия найдена, сбрасываем статус и переходим к выравниванию
        Serial.println("Line noted, aligning");
        recover_status = recover_status_align;
        spiral_status = 0;
        recover_start_ms = 0;
        go_until = 0;
        return;
    } else if ((recover_start_ms != 0) and (millis() - recover_start_ms >= line_search_stop_threshold_ms)) {
        // Остановка
        Serial.println("Line not found, stopping bot");
        tone(SOUND_PIN, 500, 1000);
        set_motors(0, 0);
        system_active = false;
        recover_status = 0;
        spiral_status = 0;
        recover_start_ms = 0;
        return;
    }

    const int search_speed = 120;
    const int turn_time_ms = 10;
    const float radius_coeff = 0.01;  // linear coeff, from zero to infinity
    int go_forward_ms = (turn_time_ms / 2 + 1) * radius_coeff * i;

    // Serial.println("here");

    // чередуем вращение и движение вперёд
    switch (spiral_status) {
        default:
            // только что начали искать линию
            Serial.println("Line lost.");
            tone(SOUND_PIN, 500, 500);
            i = 0;
            recover_start_ms = millis();
            spiral_status = spiral_status_turn;
            go_until = millis() + turn_time_ms;
            return;
        case spiral_status_turn:
            set_motors(-search_speed, search_speed);  // turn left
            if (millis() > go_until) {
                // finished turning, переходим к движению вперёд
                // set_motors(0, 0);  // мб можно убрать, если другие процессы происходят быстро
                spiral_status = spiral_status_forward;
                go_until = millis() + go_forward_ms;
            }
            return;
        case spiral_status_forward:
            set_motors(search_speed, search_speed);  // go forward
            if (millis() > go_until) {
                // finished going forward, переходим к вращению
                // set_motors(0, 0);  // мб можно убрать, если другие процессы происходят быстро
                spiral_status = spiral_status_turn;
                go_until = millis() + turn_time_ms;
            }
            return;
    }
}

// Вращение бота для выравнивания с линией
void align_line() {
    const int adjusting_speed = 120;
    const int wiggle_timeout_ms = 500; // Сколько мс робот будет ехать вперёд/назад до разворота

    static char align_status = 0;
    const char align_status_check = 1;
    const char align_status_forward = 2;
    const char align_status_turn = 3;
    const char align_status_backward = 4;

    static unsigned long recover_start_ms;
    static unsigned long line_seen_wiggle_millis; 
    // Аналогично line_seen_millis, но для этого этапа.
    // Предполагаем, что линия рядом, поэтому в начале ставим текущее время.
    static unsigned long started_going_ms;

    Serial.print("align_status: "); Serial.println(align_status, DEC); 

    switch (align_status) {
        default:
            // Только что заметили линию
            set_motors(0, 0);
            Serial.println("Line found. Stop and rotate.");

            // tone-delay
            tone(SOUND_PIN, 500, 100);
            delay(200);
            tone(SOUND_PIN, 500, 100);

            line_seen_wiggle_millis = millis(); 
            recover_start_ms = millis();

            align_status = align_status_check;
            return;
        case align_status_check:
            if (is_aligned()) {
                // Линия выровнена
                Serial.println("Aligned.");
                set_motors(0, 0);
                // Сбрасываем статусы
                recover_status = 0;
                align_status = 0;

                // tone-delay
                tone(SOUND_PIN, 500, 100);
                delay(200);
                tone(SOUND_PIN, 500, 100);
                delay(200);
                tone(SOUND_PIN, 500, 100);

                return;
            }
            else if (!line_lost()) { // Передний датчик на линии
                started_going_ms = millis();
                line_seen_wiggle_millis = millis(); 
                align_status = align_status_forward;
            }
            else if (back_is_on_line()) { // Задний датчик на линии
                align_status = align_status_backward;
            }
            else { // линия между датчиками
                align_status = align_status_turn;
            }

            if (millis() - line_seen_wiggle_millis >= line_seen_threshold_2_ms ) {
                // Линия потеряна снова
                Serial.println("Line lost while aligning");
                tone(SOUND_PIN, 500, 500);
                delay(1000);

                // tone-delay
                // debug: "print" state
                for (int i=0; i<align_status; i++) {
                    tone(SOUND_PIN, 500, 100);
                    delay(500);
                }
                delay(1000);

                align_status = 0;
                recover_status = 0;

                return;
            }
            break;

        case align_status_forward:
            started_going_ms = millis();
            set_motors(adjusting_speed, adjusting_speed);
            if (millis() >= started_going_ms + wiggle_timeout_ms ) {
                align_status = align_status_turn;
                set_motors(0, 0);
            }
            else if (back_is_on_line()) {
                align_status = align_status_turn;
                set_motors(0, 0);
            }
            else if (line_lost()) {
                align_status = align_status_turn;
                set_motors(0, 0);
            }
            return;
        case align_status_backward:
            started_going_ms = millis();
            set_motors(-adjusting_speed, -adjusting_speed); // go backward
            if (millis() >= started_going_ms + wiggle_timeout_ms ) {
                align_status = align_status_turn;
                set_motors(0, 0);
                // tone(SOUND_PIN, 200, 20); // debug: "print"
            }
            else if (!back_is_on_line()) {
                align_status = align_status_turn;
                set_motors(0, 0);
            }
            else if (!line_lost()) { // front on line
                align_status = align_status_turn;
                set_motors(0, 0);
            }
            return;
        case align_status_turn:
            started_going_ms = millis();
            set_motors(adjusting_speed, -adjusting_speed); // turn right
            if (millis() >= started_going_ms + wiggle_timeout_ms ) {
                align_status = align_status_check;
                set_motors(0, 0);
            }
            else if (back_is_on_line()) {
                align_status = align_status_check;
                set_motors(0, 0);
            }
            else if (!line_lost()) { // front on line
                align_status = align_status_check;
                set_motors(0, 0);
            }
            return;
    }
}

void recover_line() {
    // static char recover_status = 0; // moved to global

    Serial.print("recover_status: "); Serial.println(recover_status, DEC);

    switch (recover_status) {
        case recover_status_spiral:
        default:
            spiral_search();
            break;
        case recover_status_align:
            align_line();
            break;
    }
}

// Следование по линии с использованием PID-регулятора
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

void toggle_system_active_on_button() {
    bool btnState = digitalRead(BUTTON_PIN);
    if (btnState == LOW && last_button_state == HIGH) {
        system_active = !system_active;
    }
    last_button_state = btnState;
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
    toggle_system_active_on_button();
    if (system_active) {
        if ( line_lost_with_time_threshold() || (recover_status==recover_status_align) ) recover_line();
        else follow_track();
    }
}
