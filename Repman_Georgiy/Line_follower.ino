#include <Arduino.h>

// ===== КЛАСС МОТОРА =====
class Motor {
private:
    int pinForward;
    int pinBackward;
    int pinPWM;
    int currentSpeed;
    bool initialized;

public:
    Motor() : pinForward(-1), pinBackward(-1), pinPWM(-1), currentSpeed(0), initialized(false) {}

    void init(int forwardPin, int backwardPin, int pwmPin) {
        pinForward = forwardPin;
        pinBackward = backwardPin;
        pinPWM = pwmPin;
        
        pinMode(pinForward, OUTPUT);
        pinMode(pinBackward, OUTPUT);
        pinMode(pinPWM, OUTPUT);
        
        digitalWrite(pinForward, LOW);
        digitalWrite(pinBackward, LOW);
        analogWrite(pinPWM, 0);
        
        initialized = true;
    }

    void setSpeed(int speed) {
        if (!initialized) return;
        
        speed = constrain(speed, 0, 255);
        currentSpeed = speed;
        
        if (speed > 0) {
            digitalWrite(pinForward, HIGH);
            digitalWrite(pinBackward, LOW);
            analogWrite(pinPWM, speed);
        } else {
            digitalWrite(pinForward, LOW);
            digitalWrite(pinBackward, LOW);
            analogWrite(pinPWM, 0);
        }
    }

    void setBackward(int speed) {
        if (!initialized) return;
        
        speed = constrain(speed, 0, 255);
        currentSpeed = -speed;
        
        if (speed > 0) {
            digitalWrite(pinForward, LOW);
            digitalWrite(pinBackward, HIGH);
            analogWrite(pinPWM, speed);
        } else {
            digitalWrite(pinForward, LOW);
            digitalWrite(pinBackward, LOW);
            analogWrite(pinPWM, 0);
        }
    }

    void stop() {
        if (!initialized) return;
        
        digitalWrite(pinForward, LOW);
        digitalWrite(pinBackward, LOW);
        analogWrite(pinPWM, 0);
        currentSpeed = 0;
    }

    void brake() {
        if (!initialized) return;
        
        digitalWrite(pinForward, HIGH);
        digitalWrite(pinBackward, HIGH);
        analogWrite(pinPWM, 255);
        currentSpeed = 0;
    }

    int getSpeed() const {
        return currentSpeed;
    }

    bool isInitialized() const {
        return initialized;
    }
};

// ===== КЛАСС ПИД-РЕГУЛЯТОРА =====
class PIDController {
private:
    float kp, ki, kd;
    float integral;
    float previousError;
    unsigned long lastTime;
    float outputMin, outputMax;

public:
    PIDController() : kp(0), ki(0), kd(0), integral(0), previousError(0), lastTime(0), outputMin(0), outputMax(255) {}

    void init(float p, float i, float d, float minOutput = 0, float maxOutput = 255) {
        kp = p;
        ki = i;
        kd = d;
        outputMin = minOutput;
        outputMax = maxOutput;
        reset();
    }

    float compute(float error) {
        unsigned long now = micros();
        float dt = (now - lastTime) / 1000000.0;
        if (dt <= 0 || dt > 1.0) dt = 0.01;
        
        float proportional = kp * error;
        
        integral += error * dt;
        integral = constrain(integral, -100, 100);
        float integralTerm = ki * integral;
        
        float derivative = (error - previousError) / dt;
        float derivativeTerm = kd * derivative;
        
        float output = proportional + integralTerm + derivativeTerm;
        output = constrain(output, outputMin, outputMax);
        
        previousError = error;
        lastTime = now;
        
        return output;
    }

    void reset() {
        integral = 0;
        previousError = 0;
        lastTime = micros();
    }

    void setTunings(float p, float i, float d) {
        kp = p;
        ki = i;
        kd = d;
        reset();
    }
};

// ===== КЛАСС МАССИВА ИК ДАТЧИКОВ =====
class IRSensorArray {
private:
    int sensorPins[5];
    bool sensorStates[5];
    int sensorWeights[5];
    
    bool finishLinePattern[5] = {0, 0, 0, 0, 0};
    float previousError = 0;
    bool initialized;

public:
    IRSensorArray() : initialized(false) {
        for (int i = 0; i < 5; i++) {
            sensorPins[i] = -1;
            sensorStates[i] = false;
        }
    }

    void init(int pin1, int pin2, int pin3, int pin4, int pin5) {
        sensorPins[0] = pin1;
        sensorPins[1] = pin2;
        sensorPins[2] = pin3;
        sensorPins[3] = pin4;
        sensorPins[4] = pin5;
        
        sensorWeights[0] = -2;
        sensorWeights[1] = -1;
        sensorWeights[2] = 0;
        sensorWeights[3] = 1;
        sensorWeights[4] = 2;
        
        for (int i = 0; i < 5; i++) {
            pinMode(sensorPins[i], INPUT_PULLUP);
        }
        
        initialized = true;
    }

    void update() {
        if (!initialized) return;
        
        for (int i = 0; i < 5; i++) {
            sensorStates[i] = digitalRead(sensorPins[i]);
        }
    }

    float getLinePosition() {
        if (!initialized) return 0;
        
        int sum = 0;
        int count = 0;
        
        for (int i = 0; i < 5; i++) {
            if (!sensorStates[i]) {
                sum += sensorWeights[i];
                count++;
            }
        }
        
        if (count == 0) {
            return (previousError > 0) ? 2.5 : -2.5;
        }
        
        float position = (float)sum / count;
        previousError = position;
        return position;
    }

    bool* getSensorStates() {
        return sensorStates;
    }

    bool isFinishLine() {
        if (!initialized) return false;
        
        for (int i = 0; i < 5; i++) {
            if (sensorStates[i] != finishLinePattern[i]) {
                return false;
            }
        }
        return true;
    }

    bool isLineLost() {
        if (!initialized) return true;
        
        for (int i = 0; i < 5; i++) {
            if (!sensorStates[i]) {
                return false;
            }
        }
        return true;
    }

    bool isOnLine() {
        if (!initialized) return false;
        
        for (int i = 0; i < 5; i++) {
            if (!sensorStates[i]) {
                return true;
            }
        }
        return false;
    }

    void printSensorStates() {
        if (!initialized) return;
        
        Serial.print("Sensors: ");
        for (int i = 0; i < 5; i++) {
            Serial.print(sensorStates[i] ? "░" : "█");
        }
        Serial.print(" | Position: ");
        Serial.print(getLinePosition(), 2);
        Serial.print(" | Finish: ");
        Serial.print(isFinishLine() ? "YES" : "NO");
        Serial.print(" | Lost: ");
        Serial.print(isLineLost() ? "YES" : "NO");
        Serial.println();
    }

    bool isInitialized() const {
        return initialized;
    }
};

// ===== КЛАСС УПРАВЛЕНИЯ РОБОТОМ =====
class LineFollowerRobot {
public:
    enum State {
        STATE_FOLLOWING_LINE,
        STATE_AT_FINISH_LINE,
        STATE_LOST_LINE,
        STATE_STOPPED
    };

private:
    Motor leftMotor;
    Motor rightMotor;
    IRSensorArray sensorArray;
    PIDController pidController;
    
    int baseSpeed;
    unsigned long lastControlTime;
    const unsigned long CONTROL_INTERVAL = 2;
    
    int ledPin;
    unsigned long lastLedTime;
    bool ledState;
    
    State currentState;
    unsigned long stateStartTime;
    int searchDirection;
    bool initialized;

    // Переменные для поиска линии
    unsigned long searchStartTime;
    bool searchStarted;
    unsigned long lastDirectionChangeTime;

public:
    LineFollowerRobot() 
        : baseSpeed(0),
          lastControlTime(0),
          ledPin(-1),
          lastLedTime(0),
          ledState(false),
          currentState(STATE_STOPPED),
          stateStartTime(0),
          searchDirection(1),
          initialized(false),
          searchStartTime(0),
          searchStarted(false),
          lastDirectionChangeTime(0) {
    }

    void init(int leftMotorForward, int leftMotorBackward, int leftMotorPWM,
              int rightMotorForward, int rightMotorBackward, int rightMotorPWM,
              int irSensor1, int irSensor2, int irSensor3, int irSensor4, int irSensor5,
              int ledPin, int baseSpeed = 180) {
        
        leftMotor.init(leftMotorForward, leftMotorBackward, leftMotorPWM);
        rightMotor.init(rightMotorForward, rightMotorBackward, rightMotorPWM);
        
        sensorArray.init(irSensor1, irSensor2, irSensor3, irSensor4, irSensor5);
        
        pidController.init(30.0, 0.8, 20.0, 0, 255);
        
        this->ledPin = ledPin;
        pinMode(ledPin, OUTPUT);
        digitalWrite(ledPin, LOW);
        
        this->baseSpeed = baseSpeed;
        lastControlTime = micros();
        stateStartTime = millis();
        
        setState(STATE_FOLLOWING_LINE);
        initialized = true;
        
        Serial.println("Robot initialized successfully");
    }

    void update() {
        if (!initialized) return;
        
        unsigned long now = micros();
        if (now - lastControlTime >= CONTROL_INTERVAL * 1000) {
            lastControlTime = now;
            
            sensorArray.update();
            
            updateLED();
            
            if (currentState == STATE_STOPPED) {
                return;
            }
            
            if (sensorArray.isFinishLine()) {
                setState(STATE_AT_FINISH_LINE);
            } else if (sensorArray.isLineLost() && currentState == STATE_FOLLOWING_LINE) {
                setState(STATE_LOST_LINE);
            }
            
            switch (currentState) {
                case STATE_FOLLOWING_LINE:
                    followLine();
                    break;
                case STATE_AT_FINISH_LINE:
                    handleFinishLine();
                    break;
                case STATE_LOST_LINE:
                    searchLine();
                    break;
                case STATE_STOPPED:
                    break;
            }
        }
    }

    void followLine() {
        float linePosition = sensorArray.getLinePosition();
        
        float correction = pidController.compute(linePosition);
        
        // ИСПРАВЛЕНА ЛОГИКА ПИД-РЕГУЛЯТОРА
        // Если линия слева (отрицательная позиция), нужно увеличить скорость правого мотора
        // Если линия справа (положительная позиция), нужно увеличить скорость левого мотора
        int leftSpeed = baseSpeed + correction;   // Было: baseSpeed - correction
        int rightSpeed = baseSpeed - correction;  // Было: baseSpeed + correction
        
        leftSpeed = constrain(leftSpeed, 0, 255);
        rightSpeed = constrain(rightSpeed, 0, 255);
        
        leftMotor.setSpeed(leftSpeed);
        rightMotor.setSpeed(rightSpeed);

        // Отладочная информация для ПИД
        static unsigned long lastPidDebug = 0;
        if (millis() - lastPidDebug > 500) {
            Serial.print("PID Debug - Position: ");
            Serial.print(linePosition, 2);
            Serial.print(" | Correction: ");
            Serial.print(correction, 2);
            Serial.print(" | L: ");
            Serial.print(leftSpeed);
            Serial.print(" | R: ");
            Serial.println(rightSpeed);
            lastPidDebug = millis();
        }
    }

    void handleFinishLine() {
        for (int speed = baseSpeed; speed >= 0; speed -= 10) {
            leftMotor.setSpeed(speed);
            rightMotor.setSpeed(speed);
            delay(50);
        }
        leftMotor.brake();
        rightMotor.brake();
        
        Serial.println("=== FINISH LINE DETECTED ===");
        setState(STATE_STOPPED);
    }

    void searchLine() {
        unsigned long currentTime = millis();
        
        if (!searchStarted) {
            searchStartTime = currentTime;
            lastDirectionChangeTime = currentTime;
            // Исправлено направление поиска на основе позиции линии
            searchDirection = (sensorArray.getLinePosition() > 0) ? 1 : -1;
            searchStarted = true;
            Serial.println("=== SEARCH STARTED ===");
        }
        
        // Вращение для поиска линии
        if (searchDirection > 0) {
            // Вращение вправо
            leftMotor.setSpeed(baseSpeed / 2);
            rightMotor.setBackward(baseSpeed / 2);
        } else {
            // Вращение влево
            leftMotor.setBackward(baseSpeed / 2);
            rightMotor.setSpeed(baseSpeed / 2);
        }
        
        sensorArray.update();
        if (sensorArray.isOnLine()) {
            Serial.println("=== LINE FOUND ===");
            searchStarted = false;
            setState(STATE_FOLLOWING_LINE);
            return;
        }
        
        unsigned long searchDuration = currentTime - searchStartTime;
        if (searchDuration > 3000) {
            stopMotors();
            setState(STATE_STOPPED);
            searchStarted = false;
            Serial.println("=== LINE LOST - STOPPED AFTER 3 SECONDS ===");
            return;
        }
        
        unsigned long directionDuration = currentTime - lastDirectionChangeTime;
        if (directionDuration > 1500) {
            searchDirection = -searchDirection;
            lastDirectionChangeTime = currentTime;
            Serial.println("=== SEARCH DIRECTION CHANGED ===");
        }
        
        static unsigned long lastSearchDebug = 0;
        if (currentTime - lastSearchDebug > 500) {
            Serial.print("Searching... Time: ");
            Serial.print(searchDuration);
            Serial.print(" ms | Direction: ");
            Serial.println(searchDirection > 0 ? "RIGHT" : "LEFT");
            lastSearchDebug = currentTime;
        }
    }

    void updateLED() {
        if (ledPin == -1) return;
        
        unsigned long now = millis();
        
        switch (currentState) {
            case STATE_STOPPED:
                if (now - lastLedTime >= 500) {
                    ledState = !ledState;
                    digitalWrite(ledPin, ledState);
                    lastLedTime = now;
                }
                break;
                
            case STATE_LOST_LINE:
                if (now - lastLedTime >= 100) {
                    ledState = !ledState;
                    digitalWrite(ledPin, ledState);
                    lastLedTime = now;
                }
                break;
                
            case STATE_AT_FINISH_LINE:
                digitalWrite(ledPin, HIGH);
                break;
                
            default:
                digitalWrite(ledPin, LOW);
                break;
        }
    }

    void stopMotors() {
        leftMotor.stop();
        rightMotor.stop();
    }

    void setState(State newState) {
        if (currentState != newState) {
            currentState = newState;
            stateStartTime = millis();
            
            if (newState != STATE_LOST_LINE) {
                searchStarted = false;
            }
            
            if (newState == STATE_FOLLOWING_LINE) {
                pidController.reset();
            }
            
            if (ledPin != -1 && newState != STATE_STOPPED && newState != STATE_LOST_LINE) {
                digitalWrite(ledPin, LOW);
            }
            
            Serial.print("State changed to: ");
            switch (newState) {
                case STATE_FOLLOWING_LINE: Serial.println("FOLLOWING_LINE"); break;
                case STATE_AT_FINISH_LINE: Serial.println("AT_FINISH_LINE"); break;
                case STATE_LOST_LINE: Serial.println("LOST_LINE"); break;
                case STATE_STOPPED: Serial.println("STOPPED"); break;
            }
        }
    }

    State getState() const {
        return currentState;
    }

    void setBaseSpeed(int speed) {
        baseSpeed = constrain(speed, 0, 255);
    }

    int getBaseSpeed() const {
        return baseSpeed;
    }

    void setPIDTunings(float kp, float ki, float kd) {
        pidController.setTunings(kp, ki, kd);
    }

    void printDebugInfo() {
        static unsigned long lastDebug = 0;
        unsigned long currentTime = millis();
        if (currentTime - lastDebug > 100) {
            Serial.print("State: ");
            switch (currentState) {
                case STATE_FOLLOWING_LINE: Serial.print("FOLLOWING"); break;
                case STATE_AT_FINISH_LINE: Serial.print("FINISH"); break;
                case STATE_LOST_LINE: Serial.print("SEARCHING"); break;
                case STATE_STOPPED: Serial.print("STOPPED"); break;
            }
            Serial.print(" | Base: ");
            Serial.print(baseSpeed);
            Serial.print(" | L:");
            Serial.print(leftMotor.getSpeed());
            Serial.print(" R:");
            Serial.print(rightMotor.getSpeed());
            sensorArray.printSensorStates();
            lastDebug = currentTime;
        }
    }

    bool isInitialized() const {
        return initialized;
    }
};

// ===== ГЛОБАЛЬНЫЙ ОБЪЕКТ =====
LineFollowerRobot robot;

// ===== НАСТРОЙКА =====
void setup() {
    Serial.begin(115200);
    Serial.println("2WD Ball Caster Line Follower Robot");
    Serial.println("Initializing...");
    
    int leftMotorForward = 4;
    int leftMotorBackward = 3;
    int leftMotorPWM = 5;
    int rightMotorForward = 2;
    int rightMotorBackward = 7;
    int rightMotorPWM = 6;
    
    int irSensor1 = 8;
    int irSensor2 = 9;
    int irSensor3 = 10;
    int irSensor4 = 11;
    int irSensor5 = 12;
    
    int ledPin = 13;
    
    int baseSpeed = 180;
    
    robot.init(
        leftMotorForward, leftMotorBackward, leftMotorPWM,
        rightMotorForward, rightMotorBackward, rightMotorPWM,
        irSensor1, irSensor2, irSensor3, irSensor4, irSensor5,
        ledPin, baseSpeed
    );
    
    Serial.println("Robot ready!");
    Serial.println("LED: OFF-normal, FAST-search, 1Hz-stop, ON-finish");
    Serial.println("======================================================");
    
    delay(2000);
}

// ===== ОСНОВНОЙ ЦИКЛ =====
void loop() {
    robot.update();
    robot.printDebugInfo();
    delay(1);
}

// ===== КОМАНДЫ ДЛЯ ОТЛАДКИ =====
void serialEvent() {
    while (Serial.available()) {
        char command = Serial.read();
        
        switch (command) {
            case 's': // stop
                robot.setState(LineFollowerRobot::STATE_STOPPED);
                break;
            case 'f': // follow line
                robot.setState(LineFollowerRobot::STATE_FOLLOWING_LINE);
                break;
            case '+': // increase speed
                robot.setBaseSpeed(min(robot.getBaseSpeed() + 10, 255));
                break;
            case '-': // decrease speed
                robot.setBaseSpeed(max(robot.getBaseSpeed() - 10, 0));
                break;
            case 'r': // reset
                robot.setPIDTunings(30.0, 0.8, 20.0);
                robot.setBaseSpeed(180);
                robot.setState(LineFollowerRobot::STATE_FOLLOWING_LINE);
                break;
        }
    }
}