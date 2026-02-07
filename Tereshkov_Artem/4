#include <stdint.h>

const uint8_t PIN_PWM_LEFT = 6;
const uint8_t PIN_DIR_LEFT = 7;
const uint8_t PIN_PWM_RIGHT = 5;
const uint8_t PIN_DIR_RIGHT = 4;
const uint8_t PIN_BUTTON = A2;
const uint8_t PIN_BUZZER = 9;

const uint8_t ANALOG_PINS[] = {A0, A1};
const uint8_t NUM_SENSORS = sizeof(ANALOG_PINS);

const unsigned long CALIBRATION_DURATION_MS = 4000;и
const unsigned long BUZZER_DURATION_MS = 5100;
const unsigned long BUZZER_TOGGLE_INTERVAL_MS = 2000;
const unsigned long SEEK_DURATION_MS = 4000;
const unsigned long LOOP_UPDATE_INTERVAL_MS = 1;
const int DEBOUNCE_DELAY_MS = 5;

const float KP = 7.0;
const float KD = 30.0;
const float KI = 0.01;
const float INTEGRAL_LIMIT = 50.0;

const int BASE_SPEED = 150;
const int MAX_SPEED = 250;
const int MIN_SPEED = -100;
const int SEEK_LINE_CONTROL = 190;
const int BLACK_THRESHOLD = 70;

const float SMOOTHING_FACTOR = 0.2;

struct Limit {
    int minVal;
    int maxVal;

    Limit() : minVal(1024), maxVal(0) {}
};

Limit calibrationData[NUM_SENSORS];
int rawSensorValues[NUM_SENSORS];
float normalizedSensorValues[NUM_SENSORS];
float smoothedSensorValues[NUM_SENSORS];

void initPins() {
    pinMode(PIN_PWM_LEFT, OUTPUT);
    pinMode(PIN_DIR_LEFT, OUTPUT);
    pinMode(PIN_PWM_RIGHT, OUTPUT);
    pinMode(PIN_DIR_RIGHT, OUTPUT);
    pinMode(PIN_BUTTON, INPUT);
    pinMode(PIN_BUZZER, OUTPUT);
}

void setMotorSpeeds(int leftSpeed, int rightSpeed) {
    leftSpeed = constrain(leftSpeed, -(int)UINT8_MAX, (int)UINT8_MAX);
    rightSpeed = constrain(rightSpeed, -(int)UINT8_MAX, (int)UINT8_MAX);

    digitalWrite(PIN_DIR_LEFT, leftSpeed < 0);
    digitalWrite(PIN_DIR_RIGHT, rightSpeed < 0);

    analogWrite(PIN_PWM_LEFT, abs(leftSpeed));
    analogWrite(PIN_PWM_RIGHT, abs(rightSpeed));
}

bool isButtonPressed() {
    static bool oldState = false;
    static bool isPressed = false;
    static unsigned long timeoutEnd = 0;

    bool state = digitalRead(PIN_BUTTON);

    if (!isPressed) {
        if (state && !oldState) {
            isPressed = true;
            timeoutEnd = millis() + DEBOUNCE_DELAY_MS;
        }
        oldState = state;
    } else {
        if (millis() > timeoutEnd) {
            isPressed = false;
            if (state) return true;
        }
    }

    return false;
}

void readRawSensors() {
    for (uint8_t idx = 0; idx < NUM_SENSORS; idx++) {
        rawSensorValues[idx] = analogRead(ANALOG_PINS[idx]);
    }
}

void calibrateSensors() {
    readRawSensors();

    for (uint8_t idx = 0; idx < NUM_SENSORS; idx++) {
        if (rawSensorValues[idx] < calibrationData[idx].minVal) {
            calibrationData[idx].minVal = rawSensorValues[idx];
        }
        if (rawSensorValues[idx] > calibrationData[idx].maxVal) {
            calibrationData[idx].maxVal = rawSensorValues[idx];
        }
    }
}

void performCalibration() {
    unsigned long timeEnd = millis() + CALIBRATION_DURATION_MS;

    setMotorSpeeds(120, -120);

    while (millis() < timeEnd) {
        calibrateSensors();
    }

    setMotorSpeeds(0, 0);
}

void readSensors() {
    readRawSensors();

    for (uint8_t idx = 0; idx < NUM_SENSORS; idx++) {
        normalizedSensorValues[idx] = map(rawSensorValues[idx], 
                                         calibrationData[idx].minVal, 
                                         calibrationData[idx].maxVal, 
                                         0, 
                                         100);

        smoothedSensorValues[idx] = (SMOOTHING_FACTOR * normalizedSensorValues[idx]) + 
                                   ((1.0 - SMOOTHING_FACTOR) * smoothedSensorValues[idx]);
    }
}

bool isOnLine() {
    for (uint8_t idx = 0; idx < NUM_SENSORS; idx++) {
        if (smoothedSensorValues[idx] > BLACK_THRESHOLD) return true;
    }
    return false;
}

float pidRegulator() {
    static float previousError = 0.0;
    static float integralSum = 0.0;

    float error = smoothedSensorValues[0] - smoothedSensorValues[1];
    
    integralSum += error;
    integralSum = constrain(integralSum, -INTEGRAL_LIMIT, INTEGRAL_LIMIT);

    float controlSignal = (KP * error) + 
                         (KI * integralSum) + 
                         (KD * (error - previousError));

    previousError = error;

    return controlSignal;
}

bool seekLine(int* controlSignal) {
    static unsigned long seekStartTime = 0;
    static int previousControlSign = 0;

    if (!isOnLine()) {
        if (seekStartTime == 0) {
            seekStartTime = millis();
        }

        *controlSignal = (float)previousControlSign * 
                        (float)SEEK_LINE_CONTROL * 
                        exp(-(float)(millis() - seekStartTime) / (float)SEEK_DURATION_MS);

        if ((millis() - seekStartTime) > SEEK_DURATION_MS) return false;
    } else {
        seekStartTime = 0;
    }

    previousControlSign = (*controlSignal > 0) ? 1 : -1;
    return true;
}

bool followLine() {
    readSensors();

    int controlSignal = pidRegulator();

    if (!seekLine(&controlSignal)) return false;

    int leftSpeed = constrain(BASE_SPEED + controlSignal, MIN_SPEED, MAX_SPEED);
    int rightSpeed = constrain(BASE_SPEED - controlSignal, MIN_SPEED, MAX_SPEED);
    setMotorSpeeds(leftSpeed, rightSpeed);

    return true;
}

bool activateBuzzer() {
    static unsigned long startTime = 0;

    if (startTime == 0) {
        startTime = millis();
    }

    if ((millis() - startTime) > BUZZER_DURATION_MS) {
        startTime = 0;
        return false;
    }

    if ((millis() - startTime) % BUZZER_TOGGLE_INTERVAL_MS < BUZZER_TOGGLE_INTERVAL_MS / 2) {
        analogWrite(PIN_BUZZER, 1);
    } else {
        digitalWrite(PIN_BUZZER, 0);
    }

    return true;
}

void setup() {
    initPins();
    performCalibration();

    for (uint8_t idx = 0; idx < NUM_SENSORS; idx++) {
        smoothedSensorValues[idx] = 0.0;
    }
}

void loop() {
    static bool isRunning = false;
    static bool isBuzzerActive = false;
    static unsigned long lastUpdateTime = 0;

    if (millis() - lastUpdateTime >= LOOP_UPDATE_INTERVAL_MS) {
        lastUpdateTime = millis();

        if (isRunning) {
            if (!followLine()) {
                isRunning = false;
                isBuzzerActive = true;
            }
        } else {
            setMotorSpeeds(0, 0);
        }

        if (isBuzzerActive) {
            isBuzzerActive = activateBuzzer();
        }

        if (isButtonPressed()) {
            isRunning = !isRunning;
        }
    }
}
