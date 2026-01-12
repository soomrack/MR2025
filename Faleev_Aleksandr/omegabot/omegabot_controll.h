#ifndef OMEGABOT_CONTROLL_H
#define OMEGABOT_CONTROLL_H
class OmegaBotControll {
public:
// === Вход Arduino - задняя часть машины ===
// -- Пины моторов --
  const int LEFT_MOTOR_SPEED = 6;  // Скорость ЛЕВОГО мотора
  const int RIGHT_MOTOR_SPEED = 5; // Скорость ПРАВОГО мотора
  const int LEFT_MOTOR_DIR = 7;    // Направление ЛЕВОГО мотора (1 - вперёд)
  const int RIGHT_MOTOR_DIR = 4;   // Направление ПРАВОГО мотора (1 - вперёд)

public:
  void move_foreward(int speed);
  void move_backward(int speed);
  void stop_moving();
  void turn_left(int speed);
  void turn_right(int speed);
};
#endif
