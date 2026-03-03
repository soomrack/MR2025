// Serial1 (пины 18/19)
// + дублирование в Serial Monitor
// Команды: FORWARD, BACK, LEFT, RIGHT, STOP, SPEED:N

#define MOTOR_LEFT_PWM_PIN  6
#define MOTOR_LEFT_INA_PIN  7
#define MOTOR_LEFT_INB_PIN  30
#define MOTOR_LEFT_EN_PIN   31

#define MOTOR_RIGHT_PWM_PIN 5
#define MOTOR_RIGHT_INA_PIN 32
#define MOTOR_RIGHT_INB_PIN 4
#define MOTOR_RIGHT_EN_PIN  33

#define CS_LEFT_PIN          A0
#define CS_RIGHT_PIN         A1
#define CS_VOLTS_PER_AMP     0.1f
#define CURRENT_SEND_INTERVAL 1000

unsigned long last_current_send = 0;

int  motor_speed = 50;
char cmd_buf[32];
int  cmd_idx = 0;

// =============================================================
// Управление моторами
// =============================================================

void set_motor(int pwm_pin, int ina_pin, int inb_pin, int en_pin, int speed) {
    speed = constrain(speed, -255, 255);
    digitalWrite(en_pin, HIGH);

    if (speed > 0) {
        digitalWrite(ina_pin, HIGH);
        digitalWrite(inb_pin, LOW);
        analogWrite(pwm_pin, speed);
    } else if (speed < 0) {
        digitalWrite(ina_pin, LOW);
        digitalWrite(inb_pin, HIGH);
        analogWrite(pwm_pin, -speed);
    } else {
        analogWrite(pwm_pin, 0);
    }
}

void drive(int left_speed, int right_speed) {
    set_motor(MOTOR_LEFT_PWM_PIN,  MOTOR_LEFT_INA_PIN,
              MOTOR_LEFT_INB_PIN,  MOTOR_LEFT_EN_PIN,  left_speed);
    set_motor(MOTOR_RIGHT_PWM_PIN, MOTOR_RIGHT_INA_PIN,
              MOTOR_RIGHT_INB_PIN, MOTOR_RIGHT_EN_PIN, right_speed);
}

void do_forward() {
    int spd = map(motor_speed, 0, 100, 0, 255);
    drive(spd, spd);
    Serial.print("[OK] FORWARD speed=");
    Serial.println(motor_speed);
}

void do_back() {
    int spd = map(motor_speed, 0, 100, 0, 255);
    drive(-spd, -spd);
    Serial.print("[OK] BACK speed=");
    Serial.println(motor_speed);
}

void do_left() {
    int spd = map(motor_speed, 0, 100, 0, 255);
    drive(-spd, spd);
    Serial.print("[OK] LEFT speed=");
    Serial.println(motor_speed);
}

void do_right() {
    int spd = map(motor_speed, 0, 100, 0, 255);
    drive(spd, -spd);
    Serial.print("[OK] RIGHT speed=");
    Serial.println(motor_speed);
}

void do_stop() {
    drive(0, 0);
    Serial.println("[OK] STOP");
}

void send_current() {
    int raw_l = analogRead(CS_LEFT_PIN);
    int raw_r = analogRead(CS_RIGHT_PIN);
    float left  = (raw_l * 5.0f / 1023.0f) / CS_VOLTS_PER_AMP;
    float right = (raw_r * 5.0f / 1023.0f) / CS_VOLTS_PER_AMP;
    char sl[10], sr[10];
    dtostrf(left,  5, 2, sl);
    dtostrf(right, 5, 2, sr);
    char buf[64];
    snprintf(buf, sizeof(buf), "CURR:%s,%s\n", sl, sr);
    Serial1.print(buf);
}


void parse_command(const char *cmd) {
    Serial.print("[CMD] ");
    Serial.println(cmd);

    if (strcmp(cmd, "FORWARD") == 0) {
        do_forward();
    } else if (strcmp(cmd, "BACK") == 0) {
        do_back();
    } else if (strcmp(cmd, "LEFT") == 0) {
        do_left();
    } else if (strcmp(cmd, "RIGHT") == 0) {
        do_right();
    } else if (strcmp(cmd, "STOP") == 0) {
        do_stop();
    } else if (strncmp(cmd, "SPEED:", 6) == 0) {
        int s = atoi(cmd + 6);
        if (s >= 0 && s <= 100) {
            motor_speed = s;
            Serial.print("[OK] SPEED=");
            Serial.println(motor_speed);
        } else {
            Serial.println("[ERR] Speed must be 0-100");
        }
    } else {
        Serial.print("[ERR] Unknown: ");
        Serial.println(cmd);
    }
}


void serial_process() {
    while (Serial1.available()) {
        char b = Serial1.read();

        Serial.print("[RX] ");
        Serial.println(b);

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
                // Переполнение буфера — сбрасываем
                cmd_idx = 0;
            }
        }
    }
}

// =============================================================
// SETUP
// =============================================================

void setup() {
    Serial.begin(115200);
    Serial1.begin(115200);

    pinMode(MOTOR_LEFT_PWM_PIN,  OUTPUT);
    pinMode(MOTOR_LEFT_INA_PIN,  OUTPUT);
    pinMode(MOTOR_LEFT_INB_PIN,  OUTPUT);
    pinMode(MOTOR_LEFT_EN_PIN,   OUTPUT);

    pinMode(MOTOR_RIGHT_PWM_PIN, OUTPUT);
    pinMode(MOTOR_RIGHT_INA_PIN, OUTPUT);
    pinMode(MOTOR_RIGHT_INB_PIN, OUTPUT);
    pinMode(MOTOR_RIGHT_EN_PIN,  OUTPUT);

    pinMode(CS_LEFT_PIN, INPUT);
    pinMode(CS_RIGHT_PIN, INPUT);

    digitalWrite(MOTOR_LEFT_EN_PIN,  HIGH);
    digitalWrite(MOTOR_RIGHT_EN_PIN, HIGH);

    do_stop();

    Serial.println("========================================");
    Serial.println("  Waiting for commands from RPi...");
    Serial.println("  Serial1 (pins 18/19) at 115200");
    Serial.println("  Commands: FORWARD BACK LEFT RIGHT STOP SPEED:N");
    Serial.println("========================================");
}

// =============================================================
// LOOP
// =============================================================

void loop() {
    serial_process();
    unsigned long now = millis();
      if (now - last_current_send >= CURRENT_SEND_INTERVAL) {
        send_current();
        last_current_send = now;
      }
}
