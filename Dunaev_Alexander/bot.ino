#define LM_PWM_PIN  6
#define LM_DIR_PIN  7
#define RM_PWM_PIN  5
#define RM_DIR_PIN  4

#define BUTTON_PIN  12
#define LS_PIN  A0
#define RS_PIN  A1 


#define SENSOR_RANGE 100
#define base_speed 150 // базовая скорость 
float gain_p = 3.0;   // коэф. Р
float gain_d = 1.1;   // коэф. D
int search_speed = 70; // скорость при поиске линии
int light_threshold = 50; //Значение, при котором линия считается потерянной
int spiral_increase = 30; // инкремент шага спирали
int spiral_interval = 1000; // интервал повышения шага
int search_timeout = 10000; // время на поиск линии

// служебные переменные, будут меняться по ходу
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

// статусы робота
enum RobotState { IDLE, FOLLOW_LINE, SEARCH_LINE };
RobotState state = IDLE;

// инициализация моторов
void InitMotors(){
  pinMode(LM_PWM_PIN, OUTPUT);
  pinMode(RM_PWM_PIN, OUTPUT);
  pinMode(LM_DIR_PIN, OUTPUT);
  pinMode(RM_DIR_PIN, OUTPUT);
}

// установка скорости
void SetSpeed(int pwm, uint8_t dir_pin, uint8_t pwm_pin){
  bool dir = 0;
  if (pwm > 0) dir = 1;
  if (abs(pwm) > 255) pwm = 255;

  digitalWrite(dir_pin, dir);
  analogWrite(pwm_pin, abs(pwm));
}

// установка скорости на левом моторе
void SetLSpeed(int speed){
  SetSpeed(speed, LM_DIR_PIN, LM_PWM_PIN);
}

// установка скорости на правом моторе
void SetRSpeed(int speed){
  SetSpeed(speed, RM_DIR_PIN, RM_PWM_PIN);
}

// движение робота
void SetTankSpeed(int L_speed, int R_speed){
  SetLSpeed(L_speed);
  SetRSpeed(R_speed);
}

// остановка робота
void StopRobot() {
  SetTankSpeed(0, 0);
  state = IDLE;
}

//среднее арифметическое, для калибровки
int MidArifm(int pin) {
  long sum = 0;
  for (int i = 0; i < 20; i++)
    sum += analogRead(pin);
  return ((float)sum / 20);
}

//калибровка сенсоров
void SensorCalibration(){
  //инициализация кнопки и сенсоров
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(LS_PIN, INPUT);
  pinMode(RS_PIN, INPUT);

  //ожидание нажатия кнопки
  while (digitalRead(BUTTON_PIN)) {delay(50);}
  while (!digitalRead(BUTTON_PIN)) {delay(50);}

  //запись белого цвета
  white_L = MidArifm(LS_PIN);
  white_R = MidArifm(RS_PIN);

  //ожидание нажатия кнопки
  while (digitalRead(BUTTON_PIN)) {delay(50);}
  while (!digitalRead(BUTTON_PIN)) {delay(50);}

  //запись черного цвета
  black_L = MidArifm(LS_PIN);
  black_R = MidArifm(RS_PIN);

  //ожидание нажатия кнопки
  while (digitalRead(BUTTON_PIN)) {delay(50);}

  Serial.print(white_L);
  Serial.print(" ");
  Serial.print(white_R);

  Serial.print(black_L);
  Serial.print(" ");
  Serial.print(black_R);
  // ожидание отпусскания кнопки
  while (!digitalRead(BUTTON_PIN)) {delay(50);}
  state = FOLLOW_LINE;
}


void UpdateSensors(){
  sensor_left = map (analogRead(LS_PIN), black_R, white_R, 0, SENSOR_RANGE);
  sensor_right = map (analogRead(RS_PIN), black_L, white_L, 0, SENSOR_RANGE);
}
 

// следование по линии, PD регулятор
void FollowLine() {
  double error = sensor_left - sensor_right;
  double correction = error * gain_p + (error - last_error) * gain_d;
  int l_speed = base_speed + correction;
  int r_speed = base_speed - correction;
  SetTankSpeed(l_speed, r_speed);
  last_error = error;

  if (sensor_left > light_threshold && sensor_right > light_threshold) {
    spiral_step = 0;
    spiral_timer = millis();
    spiral_direction = (last_direction == 0);
    search_start_timer = millis();
    state = SEARCH_LINE;  
  }
}

//поиск линии
void SearchLine(){
  if (millis() - spiral_timer > spiral_interval) {
    spiral_step += spiral_increase;
    spiral_timer = millis();
  }

  int l_speed = -search_speed - spiral_step;
  int r_speed = -search_speed - spiral_step;

  Serial.print(l_speed);
  Serial.print("  ");
  Serial.println(r_speed);

  if (spiral_direction) SetTankSpeed(l_speed, search_speed);
  else SetTankSpeed(search_speed, r_speed);

  if (sensor_left < light_threshold){
    last_direction = 1; 
    // SetTankSpeed(-100,100);
    // delay(500);
    state = FOLLOW_LINE;
  } else if(sensor_right < light_threshold) {
    last_direction = 0;
    // SetTankSpeed(100,-100);
    // delay(500);
    state = FOLLOW_LINE;
  }

  if (millis() - search_start_timer > search_timeout) {
      StopRobot();
  }
}


void setup() {
  Serial.begin(9600);
  InitMotors();
  SensorCalibration();
}


void loop() {
  UpdateSensors();
  switch (state) {
    case IDLE:
      SetTankSpeed(0,0);
      break;
    case FOLLOW_LINE:
      FollowLine();
      break;
    case SEARCH_LINE:
      SearchLine();
      break;
  }
}
