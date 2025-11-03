#define MOTOR_L_PWM 6
#define MOTOR_L_DIR 7
#define MOTOR_R_PWM 5
#define MOTOR_R_DIR 4
#define BUTTON_PIN 12
#define SENSOR_L A0
#define SENSOR_R A1

#define spiralInc 10             
#define spiralInt 100           


// Коэффициенты ПД-регулятора
float Kp = 3;
float Kd = 1;
int baseSpeed = 140;
int searchSpeed = 120;

// Переменные (для калибровки и другие)
int r_white, r_black, r_threshold;
int l_white, l_black, l_threshold;
int l_min = 1023, l_max = 0;
int r_min = 1023, r_max = 0;

float lastError = 0; 
bool systemActive = false;
bool buttonState = LOW; 
  
unsigned long spiralTimer = 0;  
int spiralStep = 0; 
int lastDirect = 0;    
bool spiralDirect = 0;
bool lineFound = false;

// Управление моторами 
void setMotors(int left, int right) {
    digitalWrite(MOTOR_L_DIR, left >= 0);
    digitalWrite(MOTOR_R_DIR, right >= 0);
    analogWrite(MOTOR_L_PWM, constrain(abs(left), 0, 255));
    analogWrite(MOTOR_R_PWM, constrain(abs(right), 0, 255));
}

// Считывание аналогового сигнала с усреднением
int readAnalog(int pin) {
    long total = 0;
    const char attempts = 17;
    for (int i = 0; i < attempts; i++) {
      total += analogRead(pin);}
    return total / attempts;
}

// Калибровка датчиков 
void calibrateDat() {
    Serial.println("Place on white and press button");
    while (digitalRead(BUTTON_PIN)) delay(10);
    while (!digitalRead(BUTTON_PIN)) delay(10);

    l_white = readAnalog(SENSOR_L);
    r_white = readAnalog(SENSOR_R);

    Serial.println("Place on black and press button");
    while (digitalRead(BUTTON_PIN)) delay(10);
    while (!digitalRead(BUTTON_PIN)) delay(10);

    l_black = readAnalog(SENSOR_L);
    r_black = readAnalog(SENSOR_R);

    l_min = l_black;
    l_max = l_white;
    r_min = r_black;
    r_max = r_white;

    const float corect = 0.1; 
    l_threshold = l_black + (l_white - l_black) * corect;
    r_threshold = r_black + (r_white - r_black) * corect;
}

// Следование по линии
void followLine() {
    int l_val = map(analogRead(SENSOR_L), l_min, l_max, 0, 100);
    int r_val = map(analogRead(SENSOR_R), r_min, r_max, 0, 100);

    float error =  l_val - r_val;
    float correction = Kp * error + Kd * (error - lastError);
    lastError = error;

    int leftPower = baseSpeed + correction;
    int rightPower = baseSpeed - correction;

    setMotors(leftPower, rightPower);
}

// Поиск линии при потере по спирали
void findLine() {
    bool isSearching = true;
    unsigned long startTime = millis();
    spiralStep = 0;

    while (isSearching && millis() - startTime < 10000) {
        if (millis() - spiralTimer > spiralInt) {
            spiralStep += spiralInc;
            spiralTimer = millis();
        }

        int l_val = constrain(searchSpeed - spiralStep, 0, 255);
        int r_val = searchSpeed;

        setMotors(l_val, r_val);
    }

    if (lineLost()) {
        systemActive = false;
    }

    setMotors(0, 0);
    delay(10);
}

// Проверка потряна ли линия
bool lineLost() {
    int l_read = readAnalog(SENSOR_L);
    int r_read = readAnalog(SENSOR_R);

    return (r_read < r_threshold && l_read < l_threshold);
}

// Инициализация
void setup() {
    Serial.begin(9600);

    pinMode(MOTOR_L_PWM, OUTPUT);
    pinMode(MOTOR_L_DIR, OUTPUT);
    pinMode(MOTOR_R_PWM, OUTPUT);
    pinMode(MOTOR_R_DIR, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    calibrateDat();

    Serial.println("Press button to start");
    while (digitalRead(BUTTON_PIN) == HIGH) delay(10);
    systemActive = true;
}

// Основа
void loop() {
    bool btnState = digitalRead(BUTTON_PIN);

    if (btnState == LOW && buttonState == HIGH) {
        systemActive = !systemActive;
        delay(80);
    }

    buttonState = btnState;

    if (systemActive) {
        if (lineLost()){ 
            findLine();
         }
         else 
        followLine();
    }
}
