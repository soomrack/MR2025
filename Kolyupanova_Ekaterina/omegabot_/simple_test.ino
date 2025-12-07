#include "omegabot_controll.h"

struct OmegaBotState {
  bool is_in_search;
  bool is_following;
  int left_sensor;
  int right_sensor;
  float search_iter;
  int current_speed;
};

OmegaBotState state;
OmegaBotControll controll;

// Датчики линии
const int LEFT_LINE_SENSOR = 0;
const int RIGHT_LINE_SENSOR = 1;

// Базовые скорости
const int BASE_FOLLOW_SPEED = 180;
const int BASE_TURN_SPEED = 180;
const int BASE_SEARCH_SPEED = 180;
const int BASE_INITIAL_SPEED = 0;

// Граница цвета линии
const int LINE_THRESHOLD = 650;

void setup() {
  // Инициализация моторов
  pinMode(controll.LEFT_MOTOR_SPEED, OUTPUT);
  pinMode(controll.RIGHT_MOTOR_SPEED, OUTPUT);
  pinMode(controll.LEFT_MOTOR_DIR, OUTPUT);
  pinMode(controll.RIGHT_MOTOR_DIR, OUTPUT);

  // Инициализация датчиков линии
  pinMode(LEFT_LINE_SENSOR, INPUT);
  pinMode(RIGHT_LINE_SENSOR, INPUT);

  // Инициализация состояния
  state.is_in_search = true;
  state.is_following = false;
  state.left_sensor = 0;
  state.right_sensor = 0;
  state.search_iter = 0;
  state.current_speed = BASE_INITIAL_SPEED;

  // Инициализация Serial для отладки
  Serial.begin(9600);
}

int threshold(int value) {
  if (value < LINE_THRESHOLD) {return 0;} // Белый
  else {return 1000;} // Черный
}

void renew_state() {
  state.left_sensor = threshold(analogRead(LEFT_LINE_SENSOR));
  state.right_sensor = threshold(analogRead(RIGHT_LINE_SENSOR));
  int sum = state.left_sensor + state.right_sensor;

  if (sum == 0) {
    state.is_following = false;
    state.is_in_search = true;
    state.current_speed = BASE_INITIAL_SPEED;
  }
  else {
    state.is_following = true;
    state.is_in_search = false;
  }
}

void follow_line() {
  if (state.left_sensor == 1000 && state.right_sensor == 1000) {
      controll.move_foreward(BASE_FOLLOW_SPEED);
  }
  else if (state.left_sensor == 1000 && state.right_sensor == 0) {
      controll.turn_left(BASE_TURN_SPEED);
  }
  else if (state.left_sensor == 0 && state.right_sensor == 1000) {
      controll.turn_right(BASE_TURN_SPEED);
  }
}

void search_line() {
  // Поиск по спирали
  int left_speed = BASE_SEARCH_SPEED;
  int right_speed = BASE_SEARCH_SPEED / (state.search_iter / 10 + 1);
  
  analogWrite(controll.LEFT_MOTOR_SPEED, left_speed);
  analogWrite(controll.RIGHT_MOTOR_SPEED, right_speed);
  
  state.search_iter += 0.1;
}

void loop() {
  renew_state();
  if (state.is_following) {
    follow_line();
  }
  else if (state.is_in_search) {
    search_line(); 
  }
  
}
