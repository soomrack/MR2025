#define LEFT_MOTOR_PWM_PIN 6
#define LEFT_MOTOR_DIR_PIN 7
#define RIGHT_MOTOR_PWM_PIN 5
#define RIGHT_MOTOR_DIR_PIN 4

#define LEFT_SENSOR_PIN A1
#define RIGHT_SENSOR_PIN A0
#define BUTTON_PIN 12
#define SOUND_PIN 8

float Kp = 8.0;
float Kd = 5.0;
int base_speed = 150;
int max_speed = 250;
int search_speed = 150;
int light_threshold = 50;
int max_light_value = 100;
int spiral_increase = 6;
int spiral_interval = 30;
int search_timeout = 20000;

int white_L; 
int black_L;
int white_R;
int black_R;
int last_error = 0;
int last_direction = 0;

bool is_active = false;
bool button_old = HIGH;
bool searching = false;
bool spiral_direction = false;
int spiral_step = 0;
unsigned long spiral_timer = 0;
unsigned long search_start_time = 0;

int sensor_left = 0;
int sensor_right = 0;

enum RobotState { IDLE, FOLLOW_LINE, SEARCH_LINE };
RobotState state = IDLE;


void init_motors() {
  pinMode(LEFT_MOTOR_PWM_PIN, OUTPUT);
  pinMode(LEFT_MOTOR_DIR_PIN, OUTPUT);
  pinMode(RIGHT_MOTOR_PWM_PIN, OUTPUT);
  pinMode(RIGHT_MOTOR_DIR_PIN, OUTPUT);
  pinMode(LEFT_SENSOR_PIN, INPUT);
  pinMode(RIGHT_SENSOR_PIN, INPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SOUND_PIN, OUTPUT);
}


void set_speed(int left_speed, int right_speed) {
  digitalWrite(LEFT_MOTOR_DIR_PIN, left_speed > 0);
  digitalWrite(RIGHT_MOTOR_DIR_PIN, right_speed > 0);
  analogWrite(LEFT_MOTOR_PWM_PIN, abs(left_speed));
  analogWrite(RIGHT_MOTOR_PWM_PIN, abs(right_speed));
}


void stop_robot() {
  set_speed(0, 0);
  is_active = false;
  searching = false;
  state = IDLE;
  tone(SOUND_PIN, 500, 500);
}


int mid_arifm(int pin) {
  long sum = 0;                       
  for (int i = 0; i < 20; i++) {
    sum += analogRead(pin);       
  }           
  return (sum / 20.0);
}


void sensor_calibration(){ 
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LEFT_SENSOR_PIN, INPUT);
  pinMode(RIGHT_SENSOR_PIN, INPUT);

  while (digitalRead(BUTTON_PIN)) {delay(50);}  
  while (!digitalRead(BUTTON_PIN)) {delay(50);} 

  white_L = mid_arifm(LEFT_SENSOR_PIN); 
  white_R = mid_arifm(RIGHT_SENSOR_PIN);
  
  while (digitalRead(BUTTON_PIN)) {delay(50);}
  while (!digitalRead(BUTTON_PIN)) {delay(50);}

  black_L = mid_arifm(LEFT_SENSOR_PIN);
  black_R = mid_arifm(RIGHT_SENSOR_PIN);

  while (digitalRead(BUTTON_PIN)) {delay(50);} 

  Serial.print(white_L);
  Serial.println(" ");
  Serial.println(white_R);

  Serial.print(black_L);
  Serial.println(" ");
  Serial.println(black_R);

  while (!digitalRead(BUTTON_PIN)) {delay(50);} 
}


void update_sensors() {
  sensor_left = map(analogRead(LEFT_SENSOR_PIN), white_L, black_L, 0, max_light_value);
  sensor_right = map(analogRead(RIGHT_SENSOR_PIN), white_R, black_R, 0, max_light_value);
}


void handle_button() {
  bool button_state = digitalRead(BUTTON_PIN);
  if (button_state == LOW && button_old == HIGH) {
    is_active = !is_active;
    state = is_active ? FOLLOW_LINE : IDLE;
    searching = false;
  }
  button_old = button_state;
}


void set_searching_state() {
  searching = true;
  spiral_step = 0;
  spiral_timer = millis();
  spiral_direction = (last_direction == 0);
  search_start_time = millis();
  state = SEARCH_LINE;
}


void set_line_following_state(){
  if (sensor_left < light_threshold) set_speed(155, -155);
  else set_speed(-155, 155);
  delay(200);
  searching = false;
  last_direction = (sensor_left > light_threshold) ? 0 : 1;
  state = FOLLOW_LINE;
}


void follow_line() {
  int error = sensor_left - sensor_right;
  int correction = error * Kp + (error - last_error) * Kd;
  set_speed(constrain(base_speed + correction, -max_speed, max_speed),
        constrain(base_speed - correction, -max_speed, max_speed));
  last_error = error;

  if (sensor_left < light_threshold && sensor_right < light_threshold) {
    set_searching_state();
  }
}


void search_line() {
  if (millis() - spiral_timer > spiral_interval) {
    spiral_step += spiral_increase;
    spiral_timer = millis();
  }

  int left_speed = search_speed - spiral_step;
  int right_speed = search_speed - spiral_step;

  set_speed(constrain(left_speed, 0, search_speed), search_speed);


  if (sensor_left > light_threshold || sensor_right > light_threshold) {
    set_line_following_state();
  }

  if (millis() - search_start_time > search_timeout) {
    stop_robot();
  }
}


void setup() {
  Serial.begin(9600);
  init_motors();
  sensor_calibration();
}


void loop() {
  handle_button();
  update_sensors();

  switch (state) {
    case IDLE:
      stop_robot();
      break;
    case FOLLOW_LINE:
      follow_line();
      break;
    case SEARCH_LINE:
      search_line();
      break;
  }
}
