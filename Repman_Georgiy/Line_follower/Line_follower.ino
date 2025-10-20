#include <Arduino.h>

// ===== КОНСТАНТЫ И КОНФИГУРАЦИИ =====
struct MotorConfig {
    int forwardPin;
    int backwardPin;
    int pwmPin;
};

struct IRSensorConfig {
    int pins[5];
    float weights[5];
    bool invertLogic;
    int activationThreshold;
    int finishLineThreshold;
};

struct PIDConfig {
    float kp;
    float ki;
    float kd;
    float minOutput;
    float maxOutput;
};

struct RobotConfig {
    int baseSpeed;
    int maxSpeed;
    unsigned long controlInterval;
    int ledPin;
    
    unsigned long searchTimeout;
    unsigned long searchDirectionChangeTime;
    int searchSpeed;
    
    float turnAggression;
    bool useHardTurns;
    
    unsigned long finishConfirmationTime;  // Время подтверждения финишной линии (мс)
    unsigned long sharpTurnFilterTime;
    
    PIDConfig defaultPID;
};

namespace Config {
    // Конфигурация моторов
    const MotorConfig LEFT_MOTOR = {4, 3, 5};
    const MotorConfig RIGHT_MOTOR = {2, 7, 6};

    // Конфигурация датчиков
    const IRSensorConfig SENSORS = {
        .pins = {8, 9, 10, 11, 12},
        .weights = {-3.0f, -1.5f, 0.0f, 1.5f, 3.0f},
        .invertLogic = true,
        .activationThreshold = 1,
        .finishLineThreshold = 5  // Все 5 датчиков видят линию
    };

    // ПИД-коэффициенты
    const PIDConfig DEFAULT_PID = {
        .kp = 35.0f,
        .ki = 0.3f,
        .kd = 5.0f,
        .minOutput = -255.0f,
        .maxOutput = 255.0f
    };

    // Конфигурация робота
    const RobotConfig ROBOT = {
        .baseSpeed = 200U,
        .maxSpeed = 255U,
        .controlInterval = 2,
        .ledPin = 13,
        .searchTimeout = 3000,
        .searchDirectionChangeTime = 800,
        .searchSpeed = 180U,
        .turnAggression = 0.8f,
        .useHardTurns = true,
        .finishConfirmationTime = 1500,  // Финишная линия должна наблюдаться 1 секунду
        .sharpTurnFilterTime = 100,
        .defaultPID = DEFAULT_PID
    };
}

// ===== ВСПОМОГАТЕЛЬНЫЕ СТРУКТУРЫ =====
struct MotorSpeeds {
    int left;
    int right;
    
    void clamp(int minVal, int maxVal) {
        left = constrain(left, minVal, maxVal);
        right = constrain(right, minVal, maxVal);
    }
};

struct SensorReadings {
    bool states[5];
    int activeCount;
    float position;
    
    void update(const bool* newStates, const float* weights, int threshold) {
        activeCount = 0;
        float weightedSum = 0.0f;
        
        for (int i = 0; i < 5; i++) {
            states[i] = newStates[i];
            if (states[i]) {
                weightedSum += weights[i];
                activeCount++;
            }
        }
        
        position = (activeCount >= threshold) ? weightedSum / activeCount : 0.0f;
    }
};

// ===== КЛАСС МОТОРА =====
class Motor {
private:
    MotorConfig config_;
    int currentSpeed_;
    bool initialized_;

    void setupPins() const {
        pinMode(config_.forwardPin, OUTPUT);
        pinMode(config_.backwardPin, OUTPUT);
        pinMode(config_.pwmPin, OUTPUT);
        stop();
    }

    void writePins(bool forward, bool backward, int speed) const {
        digitalWrite(config_.forwardPin, forward);
        digitalWrite(config_.backwardPin, backward);
        analogWrite(config_.pwmPin, speed);
    }

public:
    Motor() : currentSpeed_(0), initialized_(false) {}

    void init(const MotorConfig& config) {
        config_ = config;
        setupPins();
        initialized_ = true;
    }

    void setSpeed(int speed) {
        if (!initialized_) return;
        
        speed = constrain(speed, 0, 255);
        currentSpeed_ = speed;
        
        if (speed > 0) {
            writePins(HIGH, LOW, speed);
        } else {
            stop();
        }
    }

    void setBackward(int speed) {
        if (!initialized_) return;
        
        speed = constrain(speed, 0, 255);
        currentSpeed_ = -speed;
        
        if (speed > 0) {
            writePins(LOW, HIGH, speed);
        } else {
            stop();
        }
    }

    void stop() {
        if (!initialized_) return;
        writePins(LOW, LOW, 0);
        currentSpeed_ = 0;
    }

    void brake() {
        if (!initialized_) return;
        writePins(HIGH, HIGH, 255);
        currentSpeed_ = 0;
    }

    int getSpeed() const { return currentSpeed_; }
    bool isInitialized() const { return initialized_; }
};

// ===== КЛАСС ПИД-РЕГУЛЯТОРА =====
class PIDController {
private:
    PIDConfig config_;
    float integral_;
    float previousError_;
    unsigned long lastTime_;

    float calculateDt(unsigned long currentTime) {
        float dt = (currentTime - lastTime_) / 1000000.0f;
        return (dt > 0 && dt <= 1.0f) ? dt : 0.01f;
    }

public:
    PIDController() : integral_(0), previousError_(0), lastTime_(0) {}

    void init(const PIDConfig& config) {
        config_ = config;
        reset();
    }

    float compute(float error) {
        unsigned long now = micros();
        float dt = calculateDt(now);
        
        // Пропорциональная составляющая
        float proportional = config_.kp * error;
        
        // Интегральная составляющая
        integral_ += error * dt;
        integral_ = constrain(integral_, -50.0f, 50.0f);
        float integralTerm = config_.ki * integral_;
        
        // Дифференциальная составляющая
        float derivative = (error - previousError_) / dt;
        float derivativeTerm = config_.kd * derivative;
        
        // Суммирование и ограничение
        float output = proportional + integralTerm + derivativeTerm;
        output = constrain(output, config_.minOutput, config_.maxOutput);
        
        previousError_ = error;
        lastTime_ = now;
        
        return output;
    }

    void reset() {
        integral_ = 0;
        previousError_ = 0;
        lastTime_ = micros();
    }

    void setTunings(float kp, float ki, float kd) {
        config_.kp = kp;
        config_.ki = ki;
        config_.kd = kd;
        reset();
    }

    PIDConfig getConfig() const { return config_; }
};

// ===== КЛАСС МАССИВА ИК ДАТЧИКОВ =====
class IRSensorArray {
private:
    IRSensorConfig config_;
    SensorReadings readings_;
    float previousError_;
    bool initialized_;

    bool readSensor(int pin) const {
        bool state = digitalRead(pin);
        return config_.invertLogic ? !state : state;
    }

public:
    IRSensorArray() : previousError_(0), initialized_(false) {}

    void init(const IRSensorConfig& config) {
        config_ = config;
        for (int i = 0; i < 5; i++) {
            pinMode(config_.pins[i], INPUT_PULLUP);
        }
        initialized_ = true;
    }

    void update() {
        if (!initialized_) return;
        
        bool currentStates[5];
        for (int i = 0; i < 5; i++) {
            currentStates[i] = readSensor(config_.pins[i]);
        }
        
        readings_.update(currentStates, config_.weights, config_.activationThreshold);
        
        if (readings_.activeCount >= config_.activationThreshold) {
            previousError_ = readings_.position;
        }
    }

    float getLinePosition() const {
        return (readings_.activeCount >= config_.activationThreshold) ? 
               readings_.position : previousError_;
    }

    const bool* getSensorStates() const { return readings_.states; }

    bool isFinishLineDetected() const {
        if (!initialized_) return false;
        return (readings_.activeCount >= config_.finishLineThreshold);
    }

    bool isSharpTurn() const {
        if (!initialized_) return false;
        
        int leftGroup = 0, rightGroup = 0;
        for (int i = 0; i < 3; i++) if (readings_.states[i]) leftGroup++;
        for (int i = 2; i < 5; i++) if (readings_.states[i]) rightGroup++;
        
        return ((leftGroup >= 2 && rightGroup <= 1) || 
                (rightGroup >= 2 && leftGroup <= 1));
    }

    bool isLineLost() const { return readings_.activeCount == 0; }
    bool isOnLine() const { return readings_.activeCount > 0; }

    void printSensorStates() const {
        if (!initialized_) return;
        
        Serial.print("Sensors: ");
        for (int i = 0; i < 5; i++) {
            Serial.print(readings_.states[i] ? "█" : "░");
        }
        Serial.print(" | Active: ");
        Serial.print(readings_.activeCount);
        Serial.print(" | Position: ");
        Serial.print(getLinePosition(), 2);
        Serial.println();
    }

    bool isInitialized() const { return initialized_; }
};

// ===== КЛАСС УПРАВЛЕНИЯ РОБОТОМ =====
class LineFollowerRobot {
public:
    enum State {
        STATE_FOLLOWING_LINE,
        STATE_AT_FINISH_LINE,
        STATE_LOST_LINE,
        STATE_STOPPED,
        STATE_CALIBRATING
    };

private:
    // Компоненты
    Motor leftMotor_;
    Motor rightMotor_;
    IRSensorArray sensors_;
    PIDController pid_;
    
    // Конфигурация и состояние
    RobotConfig config_;
    State currentState_;
    
    // Тайминги
    unsigned long lastControlTime_;
    unsigned long lastLedTime_;
    unsigned long stateStartTime_;
    unsigned long searchStartTime_;
    unsigned long lastDirectionChangeTime_;
    unsigned long lastDebugTime_;
    unsigned long lastSharpTurnTime_;
    
    // Флаги и счетчики
    bool ledState_;
    int searchDirection_;
    bool initialized_;
    bool searchStarted_;
    bool sharpTurnFilterActive_;

    // Обработка финишной линии
    unsigned long finishLineFirstDetection_;
    bool finishLineConfirmed_;
    bool finishLineProcessing_;

    // Вспомогательные методы
    void updateLED() {
        if (config_.ledPin == -1) return;
        
        unsigned long now = millis();
        int blinkInterval = getLEDBlinkInterval();
        
        if (currentState_ == STATE_AT_FINISH_LINE) {
            digitalWrite(config_.ledPin, HIGH);
            return;
        }
        
        if (currentState_ != STATE_FOLLOWING_LINE) {
            if (now - lastLedTime_ >= blinkInterval) {
                ledState_ = !ledState_;
                digitalWrite(config_.ledPin, ledState_);
                lastLedTime_ = now;
            }
        } else {
            digitalWrite(config_.ledPin, LOW);
        }
    }

    int getLEDBlinkInterval() const {
        switch (currentState_) {
            case STATE_STOPPED: return 500;
            case STATE_LOST_LINE: return 100;
            case STATE_CALIBRATING: return 200;
            default: return 500;
        }
    }

    void handleSharpTurnDetection() {
        if (sensors_.isSharpTurn()) {
            lastSharpTurnTime_ = millis();
            sharpTurnFilterActive_ = true;
        } else if (sharpTurnFilterActive_ && 
                  (millis() - lastSharpTurnTime_ > config_.sharpTurnFilterTime)) {
            sharpTurnFilterActive_ = false;
        }
    }

    bool checkFinishLine() {
        // Если финиш уже подтвержден или обрабатывается, не проверяем снова
        if (finishLineConfirmed_ || finishLineProcessing_) {
            return false;
        }
        
        // Проверяем, видим ли мы финишную линию в данный момент
        if (sensors_.isFinishLineDetected()) {
            unsigned long currentTime = millis();
            
            // Если это первое обнаружение, запоминаем время
            if (finishLineFirstDetection_ == 0) {
                finishLineFirstDetection_ = currentTime;
                Serial.println(F("=== FINISH LINE DETECTED - STARTING CONFIRMATION ==="));
            } 
            // Проверяем, наблюдаем ли мы линию достаточно долго
            else if (currentTime - finishLineFirstDetection_ >= config_.finishConfirmationTime) {
                Serial.println(F("=== FINISH LINE CONFIRMED ==="));
                finishLineConfirmed_ = true;
                finishLineFirstDetection_ = 0; // Сбрасываем для следующего использования
                return true;
            }
            
            // Выводим прогресс подтверждения
            if (currentTime - lastDebugTime_ > 200) {
                unsigned long elapsed = currentTime - finishLineFirstDetection_;
                unsigned long remaining = config_.finishConfirmationTime - elapsed;
                Serial.print(F("Finish confirmation: "));
                Serial.print(elapsed);
                Serial.print(F("ms / "));
                Serial.print(config_.finishConfirmationTime);
                Serial.print(F("ms ("));
                Serial.print((elapsed * 100) / config_.finishConfirmationTime);
                Serial.println(F("%)"));
                lastDebugTime_ = currentTime;
            }
        } else {
            // Если линия пропала, сбрасываем таймер подтверждения
            if (finishLineFirstDetection_ != 0) {
                Serial.println(F("=== FINISH LINE LOST - RESETTING CONFIRMATION ==="));
                finishLineFirstDetection_ = 0;
            }
        }
        
        return false;
    }

    MotorSpeeds calculateMotorSpeeds(float linePosition) {
        float correction = pid_.compute(linePosition);
        MotorSpeeds speeds;
        
        if (config_.useHardTurns && abs(correction) > 50) {
            if (correction < -50) {
                // Резкий поворот налево
                speeds.left = 0;
                speeds.right = config_.baseSpeed + abs(correction) * config_.turnAggression;
            } else {
                // Резкий поворот направо
                speeds.left = config_.baseSpeed + abs(correction) * config_.turnAggression;
                speeds.right = 0;
            }
        } else {
            // Плавное регулирование
            speeds.left = config_.baseSpeed + correction;
            speeds.right = config_.baseSpeed - correction;
        }
        
        speeds.clamp(0, config_.maxSpeed);
        return speeds;
    }

    void applyMotorSpeeds(const MotorSpeeds& speeds) {
        leftMotor_.setSpeed(speeds.left);
        rightMotor_.setSpeed(speeds.right);
    }

    void stopMotors() {
        leftMotor_.stop();
        rightMotor_.stop();
    }

public:
    LineFollowerRobot() 
        : lastControlTime_(0),
          lastLedTime_(0),
          stateStartTime_(0),
          searchStartTime_(0),
          lastDirectionChangeTime_(0),
          lastDebugTime_(0),
          lastSharpTurnTime_(0),
          ledState_(false),
          searchDirection_(1),
          initialized_(false),
          searchStarted_(false),
          sharpTurnFilterActive_(false),
          finishLineFirstDetection_(0),
          finishLineConfirmed_(false),
          finishLineProcessing_(false) {
    }

    void init(const RobotConfig& robotConfig) {
        config_ = robotConfig;
        
        leftMotor_.init(Config::LEFT_MOTOR);
        rightMotor_.init(Config::RIGHT_MOTOR);
        sensors_.init(Config::SENSORS);
        pid_.init(config_.defaultPID);
        
        pinMode(config_.ledPin, OUTPUT);
        digitalWrite(config_.ledPin, LOW);
        
        lastControlTime_ = micros();
        stateStartTime_ = millis();
        
        setState(STATE_FOLLOWING_LINE);
        initialized_ = true;
        
        Serial.println(F("=== LINE FOLLOWER ROBOT ==="));
        Serial.println(F("Commands: s=stop, f=follow, t=test sensors"));
        Serial.println(F("+/-=speed, 1/2/3=PID tuning"));
        Serial.println(F("================================="));
    }

    void update() {
        if (!initialized_) return;
        
        unsigned long now = micros();
        if (now - lastControlTime_ >= config_.controlInterval * 1000) {
            lastControlTime_ = now;
            
            sensors_.update();
            updateLED();
            
            if (currentState_ == STATE_STOPPED || currentState_ == STATE_CALIBRATING) {
                return;
            }
            
            if (checkFinishLine()) {
                setState(STATE_AT_FINISH_LINE);
            }
            
            // Обработка состояний
            switch (currentState_) {
                case STATE_FOLLOWING_LINE:
                    handleFollowingState();
                    break;
                case STATE_AT_FINISH_LINE:
                    handleFinishLine();
                    break;
                case STATE_LOST_LINE:
                    handleSearchState();
                    break;
                default:
                    break;
            }
        }
    }

private:
    void handleFollowingState() {
        handleSharpTurnDetection();
        
        if (sensors_.isLineLost() && !sharpTurnFilterActive_) {
            setState(STATE_LOST_LINE);
            return;
        }
        
        float linePosition = sensors_.getLinePosition();
        MotorSpeeds speeds = calculateMotorSpeeds(linePosition);
        applyMotorSpeeds(speeds);
        
        printDebugInfo(linePosition, speeds);
    }

    void handleSearchState() {
        unsigned long currentTime = millis();
        
        if (!searchStarted_) {
            initializeSearch();
        }
        
        sensors_.update();
        
        // Проверка условий выхода из поиска
        if (sensors_.isOnLine()) {
            endSearch("=== LINE FOUND ===", STATE_FOLLOWING_LINE);
            return;
        }
        
        // Проверка таймаута поиска
        if (currentTime - searchStartTime_ > config_.searchTimeout) {
            Serial.print(F("Search timeout: "));
            Serial.print(currentTime - searchStartTime_);
            Serial.println(F(" ms"));
            endSearch("=== SEARCH TIMEOUT ===", STATE_STOPPED);
            return;
        }
        
        // Поиск линии с изменением направления
        performSearch();
        
        if (currentTime - lastDirectionChangeTime_ > config_.searchDirectionChangeTime) {
            changeSearchDirection();
        }
    }

    void initializeSearch() {
        searchStartTime_ = millis();
        lastDirectionChangeTime_ = searchStartTime_;
        
        // Определяем направление поиска на основе последней известной позиции линии
        float lastPosition = sensors_.getLinePosition();
        searchDirection_ = (lastPosition > 0) ? 1 : -1;
        if (lastPosition == 0) {
            // Если позиция неизвестна, выбираем случайное направление
            searchDirection_ = (random(2) == 0) ? 1 : -1;
        }
        
        searchStarted_ = true;
        Serial.println(F("=== SEARCH STARTED ==="));
        Serial.print(F("Initial search direction: "));
        Serial.println(searchDirection_ > 0 ? "RIGHT" : "LEFT");
    }

    void performSearch() {
        if (searchDirection_ > 0) {
            // Поиск вправо
            leftMotor_.setSpeed(config_.searchSpeed);
            rightMotor_.setBackward(config_.searchSpeed);
        } else {
            // Поиск влево
            leftMotor_.setBackward(config_.searchSpeed);
            rightMotor_.setSpeed(config_.searchSpeed);
        }
    }

    void changeSearchDirection() {
        searchDirection_ = -searchDirection_;
        lastDirectionChangeTime_ = millis();
        Serial.print(F("=== DIRECTION CHANGED to: "));
        Serial.println(searchDirection_ > 0 ? "RIGHT" : "LEFT");
    }

    void endSearch(const char* message, State nextState) {
        Serial.println(message);
        stopMotors();
        searchStarted_ = false;
        setState(nextState);
    }

    void handleFinishLine() {
        if (!finishLineProcessing_) {
            Serial.println(F("=== PROCESSING FINISH LINE ==="));
            finishLineProcessing_ = true;
            
            // Плавная остановка
            /*for (int speed = config_.baseSpeed; speed >= 0; speed -= 20) {
                leftMotor_.setSpeed(speed);
                rightMotor_.setSpeed(speed);
                delay(30);
            }*/
            
            leftMotor_.brake();
            rightMotor_.brake();
            
            Serial.println(F("=== ROBOT STOPPED AT FINISH LINE ==="));
            setState(STATE_STOPPED);
        }
    }

    void printDebugInfo(float position, const MotorSpeeds& speeds) {
        unsigned long now = millis();
        if (now - lastDebugTime_ > 300) {
            Serial.print(F("PID | Pos:"));
            Serial.print(position, 2);
            Serial.print(F(" | L:"));
            Serial.print(speeds.left);
            Serial.print(F(" R:"));
            Serial.print(speeds.right);
            
            if (speeds.left == 0) Serial.print(F(" [LEFT_STOP]"));
            if (speeds.right == 0) Serial.print(F(" [RIGHT_STOP]"));
            
            Serial.println();
            lastDebugTime_ = now;
        }
    }

public:
    void setState(State newState) {
        if (currentState_ != newState) {
            // Останавливаем моторы при переходе в состояние STOPPED
            if (newState == STATE_STOPPED) {
                stopMotors();
            }
            
            // Сброс состояний финишной линии при переходе в режим следования
            if (newState == STATE_FOLLOWING_LINE) {
                finishLineFirstDetection_ = 0;
                finishLineConfirmed_ = false;
                finishLineProcessing_ = false;
            }
            
            currentState_ = newState;
            stateStartTime_ = millis();
            
            // Сброс состояний поиска
            if (newState != STATE_LOST_LINE) {
                searchStarted_ = false;
            }
            
            // Сброс ПИД при начале следования
            if (newState == STATE_FOLLOWING_LINE) {
                pid_.reset();
                sharpTurnFilterActive_ = false;
            }
            
            // Логирование
            const char* stateNames[] = {"FOLLOWING", "FINISH", "SEARCHING", "STOPPED", "CALIBRATING"};
            Serial.print(F("STATE: "));
            Serial.println(stateNames[newState]);
        }
    }

    void setBaseSpeed(int speed) {
        config_.baseSpeed = constrain(speed, 0, config_.maxSpeed);
        Serial.print(F("Base speed: "));
        Serial.println(config_.baseSpeed);
    }

    void setPIDTunings(float kp, float ki, float kd) {
        pid_.setTunings(kp, ki, kd);
        Serial.print(F("PID: P="));
        Serial.print(kp);
        Serial.print(F(" I="));
        Serial.print(ki);
        Serial.print(F(" D="));
        Serial.println(kd);
    }

    void testSensors() {
        sensors_.printSensorStates();
        setState(STATE_CALIBRATING);
    }

    // Геттеры
    State getState() const { return currentState_; }
    int getBaseSpeed() const { return config_.baseSpeed; }
    bool isInitialized() const { return initialized_; }
};

// ===== ГЛОБАЛЬНЫЙ ОБЪЕКТ И ОБРАБОТКА =====
LineFollowerRobot robot;

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println(F("=== LINE FOLLOWER ROBOT ==="));
    Serial.println(F("Initializing..."));
    
    randomSeed(analogRead(0)); // Для случайного выбора направления поиска
    
    robot.init(Config::ROBOT);
    
    Serial.println(F("Robot ready!"));
    delay(1000);
}

void loop() {
    robot.update();
    delay(10);
}

void serialEvent() {
    while (Serial.available()) {
        char command = Serial.read();
        
        switch (command) {
            case 's': robot.setState(LineFollowerRobot::STATE_STOPPED); break;
            case 'f': robot.setState(LineFollowerRobot::STATE_FOLLOWING_LINE); break;
            case 't': robot.testSensors(); break;
            case '+': robot.setBaseSpeed(min(robot.getBaseSpeed() + 10, 255)); break;
            case '-': robot.setBaseSpeed(max(robot.getBaseSpeed() - 10, 0)); break;
            case '1': robot.setPIDTunings(30.0f, 0.1f, 10.0f); break;
            case '2': robot.setPIDTunings(60.0f, 0.3f, 20.0f); break;
            case '3': robot.setPIDTunings(90.0f, 0.5f, 30.0f); break;
            case ' ': robot.setState(LineFollowerRobot::STATE_STOPPED); break;
        }
    }
}