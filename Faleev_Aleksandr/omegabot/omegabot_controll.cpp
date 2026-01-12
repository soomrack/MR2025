#include <Arduino.h>
#include "omegabot_controll.h"

// Движение вперед
void OmegaBotControll::move_foreward(int speed) {
  digitalWrite(LEFT_MOTOR_DIR, HIGH);   // HIGH = 1
  digitalWrite(RIGHT_MOTOR_DIR, HIGH);  // HIGH = 1

  analogWrite(LEFT_MOTOR_SPEED, speed);
  analogWrite(RIGHT_MOTOR_SPEED, speed);
}

// Движение назад
void OmegaBotControll::move_backward(int speed) {
  digitalWrite(LEFT_MOTOR_DIR, LOW);   // LOW = 0
  digitalWrite(RIGHT_MOTOR_DIR, LOW);  // LOW = 0

  analogWrite(LEFT_MOTOR_SPEED, speed);
  analogWrite(RIGHT_MOTOR_SPEED, speed);
}

// Остановка
void OmegaBotControll::stop_moving() {
  analogWrite(LEFT_MOTOR_SPEED, 0);
  analogWrite(RIGHT_MOTOR_SPEED, 0);
}

// Поворот влево на месте
void OmegaBotControll::turn_left(int speed) {
  digitalWrite(LEFT_MOTOR_DIR, HIGH);   // LOW = 0
  digitalWrite(RIGHT_MOTOR_DIR, LOW);  // HIGH = 1

  analogWrite(LEFT_MOTOR_SPEED, speed);
  analogWrite(RIGHT_MOTOR_SPEED, speed);
}

// Поврот вправо на месте
void OmegaBotControll::turn_right(int speed) {
  digitalWrite(LEFT_MOTOR_DIR, LOW);   // HIGH = 1
  digitalWrite(RIGHT_MOTOR_DIR, HIGH);  // LOW = 0

  analogWrite(LEFT_MOTOR_SPEED, speed);
  analogWrite(RIGHT_MOTOR_SPEED, speed);
}
