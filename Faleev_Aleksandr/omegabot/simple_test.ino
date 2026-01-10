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
const int BASE_FOLLOW_SPEED = 250;
const int BASE_TURN_SPEED = 250;
const int BASE_SEARCH_SPEED = 190;
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
    state.search_iter = state.search_iter + 1;
    state.current_speed = BASE_INITIAL_SPEED;
  }
  else {
    state.is_following = true;
    state.is_in_search = false;
    state.search_iter = 0;
  }
}

void follow_line() {
  if (state.left_sensor == 1000 && state.right_sensor == 1000) {
    if (state.current_speed >= BASE_FOLLOW_SPEED){
      controll.move_foreward(BASE_FOLLOW_SPEED);
    }
    else {
      state.current_speed = state.current_speed + 1;
      controll.move_foreward(state.current_speed);
    };
  }
  else if (state.left_sensor == 1000 && state.right_sensor == 0) {
    if (state.current_speed >= BASE_TURN_SPEED){
      controll.turn_left(BASE_TURN_SPEED);
    }
    else {
      state.current_speed = state.current_speed + 1;
      controll.turn_left(state.current_speed);
    };
  }
  else if (state.left_sensor == 0 && state.right_sensor == 1000) {
    if (state.current_speed >= BASE_TURN_SPEED){
      controll.turn_right(BASE_TURN_SPEED);
    }
    else {
      state.current_speed = state.current_speed + 1;
      controll.turn_right(state.current_speed);
    };
  }
}

void search_line() {
  controll.move_foreward(BASE_SEARCH_SPEED);
  delay(10 + state.search_iter * 0.01);
  controll.turn_left(BASE_SEARCH_SPEED);
  delay(10);
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
