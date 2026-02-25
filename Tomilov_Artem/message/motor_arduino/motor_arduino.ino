/*
 * motor.ino — управление ESC через Arduino Uno
 * Версия с аппаратным UART (Serial)
 */

#include <Servo.h>

// ============================================================================
// НАСТРОЙКИ
// ============================================================================

#define ESC_PIN         9
#define SERIAL_BAUD     115200

#define ESC_PULSE_MIN   1000
#define ESC_PULSE_STOP  1500
#define ESC_PULSE_MAX   2000

#define SPEED_DEFAULT   50

// ============================================================================
// ОБЪЕКТЫ
// ============================================================================

Servo esc;

// ============================================================================
// ПЕРЕМЕННЫЕ
// ============================================================================

int  motor_speed = SPEED_DEFAULT;
char cmd_buf[32];
int  cmd_idx = 0;

// ============================================================================
// УПРАВЛЕНИЕ ESC
// ============================================================================

void esc_set(int pulse_us) {
    if (pulse_us < ESC_PULSE_MIN) pulse_us = ESC_PULSE_MIN;
    if (pulse_us > ESC_PULSE_MAX) pulse_us = ESC_PULSE_MAX;
    esc.writeMicroseconds(pulse_us);
}

void do_forward() {
    int pulse = ESC_PULSE_STOP +
                (motor_speed * (ESC_PULSE_MAX - ESC_PULSE_STOP)) / 100;
    esc_set(pulse);
}

void do_back() {
    int pulse = ESC_PULSE_STOP -
                (motor_speed * (ESC_PULSE_STOP - ESC_PULSE_MIN)) / 100;
    esc_set(pulse);
}

void do_stop() {
    esc_set(ESC_PULSE_STOP);
}

// ============================================================================
// ПАРСИНГ
// ============================================================================

void parse_command(const char *cmd) {

    if (strcmp(cmd, "FORWARD") == 0) {
        do_forward();

    } else if (strcmp(cmd, "BACK") == 0) {
        do_back();

    } else if (strcmp(cmd, "STOP") == 0) {
        do_stop();

    } else if (strncmp(cmd, "SPEED:", 6) == 0) {
        int s = atoi(cmd + 6);
        if (s >= 0 && s <= 100) {
            motor_speed = s;
        }
    }

    // индикация приёма команды
    digitalWrite(LED_BUILTIN, HIGH);
    delay(30);
    digitalWrite(LED_BUILTIN, LOW);
}

// ============================================================================
// SETUP
// ============================================================================

void setup() {

    esc.attach(ESC_PIN);
    esc_set(ESC_PULSE_STOP);   // сразу нейтраль

    Serial.begin(SERIAL_BAUD);

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);
}

// ============================================================================
// LOOP
// ============================================================================

void loop() {

    while (Serial.available()) {

        char b = Serial.read();

        if (b == '\n' || b == '\r') {

            if (cmd_idx > 0) {
                cmd_buf[cmd_idx] = '\0';
                parse_command(cmd_buf);
                cmd_idx = 0;
            }

        } else {

            if (cmd_idx < (int)sizeof(cmd_buf) - 1) {
                cmd_buf[cmd_idx++] = b;
            } else {
                cmd_idx = 0;  // защита от переполнения
            }
        }
    }
}
