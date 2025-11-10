#define LM_PWM_PIN  6
#define LM_DIR_PIN  7
#define RM_PWM_PIN  5
#define RM_DIR_IN  4

#define BUTTON_PIN  12
#define LS_PIN  A0
#define RS_PIN  A1 


float gain_p = 7.0;
float gain_d = 4.0;
int base_speed = 255;
int search_speed = 110;
int light_threshold = 50;
int spiral_increase = 3;
int spiral_interval = 50;
int search_timeout = 100000;


#define SENSOR_RANGE 100
int white_L, black_L;
int white_R, black_R;
double last_error = 0;
int last_direction = 0;


bool spiral_direction = false;
int spiral_step = 0;
unsigned long spiral_timer = 0;
unsigned long search_start_timer = 0;


int sensor_left = 0;
int sensor_right = 0;


unsigned long last_serial_time = 0;
const unsigned long serial_interval = 200;


enum RobotState { IDLE, FOLLOW_LINE, SEARCH_LINE };
RobotState state = IDLE;


void InitMotors(){
  pinMode(LM_PWM_PIN, OUTPUT);
  pinMode(RM_PWM_PIN, OUTPUT);
  pinMode(LM_DIR_PIN, OUTPUT);
  pinMode(RM_DIR_PIN, OUTPUT);
}


void SetSpeed(int pwm, uint8_t dir_pin, uint8_t pwm_pin){
  bool dir = 0;
  if (pwm > 0) dir = 1;
  if (abs(pwm) > 255) pwm = 255;

  digitalWrite(dir_pin, dir);
  analogWrite(pwm_pin, abs(pwm));
}


void SetLSpeed(int speed){
  SetSpeed(speed, LM_DIR, LM_PWM);
}


void SetRSpeed(int speed){
  SetSpeed(speed, RM_DIR, RM_PWM);
}


void SetTankSpeed(int L_speed, int R_speed){
  SetLSpeed(L_speed);
  SetRSpeed(R_speed);
}


int midArifm(int pin) {
  long sum = 0;
  for (int i = 0; i < 20; i++)
    sum += analogRead(pin);
  return ((float)sum / 20);
}


void sensor_calibration(){
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LS_PIN, INPUT);
  pinMode(RS_PIN, INPUT);

  while (digitalRead(BUTTON_PIN)) {delay(50);}
  while (!digitalRead(BUTTON_PIN)) {delay(50);}

  white_L = midArifm(LS_PIN);
  white_R = midArifm(RS_PIN);

  while (digitalRead(BUTTON_PIN)) {delay(50);}
  while (!digitalRead(BUTTON_PIN)) {delay(50);}

  black_L = midArifm(LS_PIN);
  black_R = midArifm(RS_PIN);

  while (digitalRead(BUTTON_PIN)) {delay(50);}

  Serial.print(white_L);
  Serial.print(" ");
  Serial.print(white_R);

  Serial.print(black_L);
  Serial.print(" ");
  Serial.print(black_R);

  while (!digitalRead(BUTTON_PIN)) {delay(50);}
  state = FOLLOW_LINE;
}


double get_error(){
  sensor_left = analogRead(LS_PIN);
  sensor_right = analogRead(RS_PIN);

  double error = map (sensor_left, black_R, white_R, 0, SENSOR_RANGE)
               - map (sensor_right, black_L, white_L, 0, SENSOR_RANGE);

  Serial.println(error);

  return error;
}


void follow_line() {
  double error = get_error();
  double correction = error * gain_p + (error - last_error) * gain_d;
  int l_speed = base_speed + correction;
  int r_speed = base_speed - correction;
  SetTankSpeed(l_speed, r_speed);
  last_error = error;

  if (sensor_left < light_threshold && sensor_right < light_threshold) {
    spiral_step = 0;
    spiral_timer = millis();
    spiral_direction = (last_direction == 0);
    search_start_timer = millis();
    state = SEARCH_LINE;  
  }
}


void search_line() {
  if (millis() - spiral_timer > spiral_interval) {
    spiral_step += spiral_increase;
    spiral_timer = millis();
  }

  int l_speed = search_speed - spiral_step;
  int r_speed = search_speed - spiral_step;

  if (spiral_direction) SetTankSpeed(l_speed, search_speed);
  else SetTankSpeed(search_speed, r_speed);

  if (sensor_left > light_threshold || sensor_right > light_threshold) {
    last_direction = (sensor_left > light_threshold) ? 0 : 1;
    state = FOLLOW_LINE;
  }

  if (millis() - search_start_timer > search_timeout) {
    SetTankSpeed(0, 0);
    state = IDLE;
  }
}


void update_state(){
  switch (state) {
    case IDLE:
      SetTankSpeed(0,0);
      break;
    case FOLLOW_LINE:
      follow_line();
      break;
    case SEARCH_LINE:
      search_line();
      break;
  }
}


void setup() {
  Serial.begin(9600);
  InitMotors();
  sensor_calibration();
}


void loop() {
  update_state();
}
