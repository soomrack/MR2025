#define SENSOR_LEFT A0
#define SENSOR_RIGHT A1

#define MOTOR_R_F  4 
#define MOTOR_R_B 5  
#define MOTOR_L_F 6  
#define MOTOR_L_B 7 

float Kp = 20; 
float Kd = 0.01;   
float Ki = 0.05;   // интегральный  


// перменныеPID
float previous_error = 0;
float integral = 0;

const int BLACK_LINE_THRESHOLD = 800;

const int BASE_SPEED = 120;

// время последнего вычисления для производной
unsigned long last_time = 0;

float calculateError(int left_val, int right_val) {

  if (left_val > BLACK_LINE_THRESHOLD && right_val > BLACK_LINE_THRESHOLD) {
    return 0;
  }
  
  else if (left_val > BLACK_LINE_THRESHOLD) {
    return -2;
  }

  else if (right_val > BLACK_LINE_THRESHOLD) {
    return 2;//можно пробовать 1.5
  }

  else {
    float error = (right_val - left_val) / 200;//можно поставить 600 какие нибудь в знаменатель
    return error;
  }
}


float calculatePID(float error, float dt) {
  // пропорциональная составляющая
  float proportional = Kp * error;
  
  // интегральная составляющая
  integral += error * dt;
  float integral_term = Ki * integral;
  
  // ограничение интегральной составляющей 
  if (integral_term > 100) integral_term = 100;
  if (integral_term < -100) integral_term = -100;
  
  // дифф составляющая
  float derivative = (error - previous_error) / dt;
  float derivative_term = Kd * derivative;
  
  previous_error = error;
  
  return proportional + integral_term + derivative_term;
}

void motorControl(float pid_output) {
  int left_speed = BASE_SPEED + pid_output;  
  int right_speed = BASE_SPEED - pid_output; 
  
  // ограничение скоростей
  left_speed = constrain(left_speed, 0, 255);
  right_speed = constrain(right_speed, 0, 255);
  
  analogWrite(MOTOR_R_F, left_speed);
  analogWrite(MOTOR_R_B, left_speed);
  
  analogWrite(MOTOR_L_F, right_speed);
  analogWrite(MOTOR_L_B, right_speed);
}

void setup() {

  pinMode(MOTOR_R_F, OUTPUT);
  pinMode(MOTOR_R_B, OUTPUT);
  pinMode(MOTOR_L_F, OUTPUT);
  pinMode(MOTOR_L_B, OUTPUT);
  
  Serial.begin(9600); 
  last_time = millis();
}

void loop() {

  int left_value = analogRead(SENSOR_LEFT);
  int right_value = analogRead(SENSOR_RIGHT);
  
  // вычисление ошибки
  float error = calculateError(left_value, right_value);
  
  // вычисление времени для производной
  unsigned long current_time = millis();
  float dt = (current_time - last_time) / 1000.0;
  last_time = current_time;
  
  // вычисление PID
  float pid_value = calculatePID(error, dt);
  
  // управление моторами
  motorControl(pid_value);
  
  delay(10); 
}