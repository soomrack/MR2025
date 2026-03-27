#define LEFT_PWM_PIN 6
#define LEFT_DIR_PIN 7
#define RIGHT_PWM_PIN 5
#define RIGHT_DIR_PIN 4
#define SOUND_PIN 9

int currentSpeed = 150;
unsigned long lastCommandTime = 0;
const unsigned long TIMEOUT = 300;

void drive(int left, int right) {
  if (left > 0) digitalWrite(LEFT_DIR_PIN, HIGH);
  else digitalWrite(LEFT_DIR_PIN, LOW);
  
  if (right > 0) digitalWrite(RIGHT_DIR_PIN, HIGH);
  else digitalWrite(RIGHT_DIR_PIN, LOW);
  
  analogWrite(LEFT_PWM_PIN, min(abs(left), 255));
  analogWrite(RIGHT_PWM_PIN, min(abs(right), 255));
}

void stopMotors() {
  analogWrite(LEFT_PWM_PIN, 0);
  analogWrite(RIGHT_PWM_PIN, 0);
}

void setup() {
  Serial.begin(9600);
  
  pinMode(LEFT_PWM_PIN, OUTPUT);
  pinMode(LEFT_DIR_PIN, OUTPUT);
  pinMode(RIGHT_PWM_PIN, OUTPUT);
  pinMode(RIGHT_DIR_PIN, OUTPUT);
  pinMode(SOUND_PIN, OUTPUT);
  
  stopMotors();
  
  // Отправляем сигнал готовности
  Serial.println("ARD:READY");
}

void loop() {
  if (Serial.available() > 0) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    lastCommandTime = millis();
    
    if (command == "w") {
      drive(currentSpeed, currentSpeed);
      Serial.println("CMD:w OK");
    }
    else if (command == "s") {
      drive(-currentSpeed, -currentSpeed);
      Serial.println("CMD:s OK");
    }
    else if (command == "a") {
      drive(-currentSpeed, currentSpeed);
      Serial.println("CMD:a OK");
    }
    else if (command == "d") {
      drive(currentSpeed, -currentSpeed);
      Serial.println("CMD:d OK");
    }
    else if (command == "x") {
      stopMotors();
      Serial.println("CMD:x OK");
    }
    else if (command.startsWith("speed ")) {
      currentSpeed = command.substring(6).toInt();
      currentSpeed = constrain(currentSpeed, 0, 255);
      Serial.print("SPEED:");
      Serial.println(currentSpeed);
    }
  }
  
  if (millis() - lastCommandTime > TIMEOUT) {
    stopMotors();
  }
  
  delay(10);
}
