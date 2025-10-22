#define SENS_L A0
#define SENS_R A2
#define MOTOR_L_DIR 7
#define MOTOR_R_DIR 4
#define MOTOR_L_SPEED 6
#define MOTOR_R_SPEED 5



void forward(int speed) {
  digitalWrite(MOTOR_L_DIR, HIGH);
  digitalWrite(MOTOR_R_DIR, HIGH);
  analogWrite(MOTOR_L_SPEED, speed);
  analogWrite(MOTOR_R_SPEED, speed);
}


void left(int speed_1, int speed_2) {
  digitalWrite(MOTOR_L_DIR, LOW);
  digitalWrite(MOTOR_R_DIR, HIGH);
  analogWrite(MOTOR_L_SPEED, speed_2);
  analogWrite(MOTOR_R_SPEED, speed_1);
}

void right(int speed_1, int speed_2) {
  digitalWrite(MOTOR_L_DIR, HIGH);
  digitalWrite(MOTOR_R_DIR, LOW);
  analogWrite(MOTOR_L_SPEED, speed_1);
  analogWrite(MOTOR_R_SPEED, speed_2);
}


void back(int speed) {
  digitalWrite(MOTOR_L_DIR, LOW);
  digitalWrite(MOTOR_R_DIR, LOW);
  analogWrite(MOTOR_L_SPEED, speed);
  analogWrite(MOTOR_R_SPEED, speed);
}

void forward_look(int speed) {
  digitalWrite(MOTOR_L_DIR, HIGH);
  digitalWrite(MOTOR_R_DIR, HIGH);
  analogWrite(MOTOR_L_SPEED, speed);
  analogWrite(MOTOR_R_SPEED, speed);
}


void stop() {
  digitalWrite(MOTOR_L_DIR, HIGH);
  digitalWrite(MOTOR_R_DIR, HIGH);
  analogWrite(MOTOR_L_SPEED, 0);
  analogWrite(MOTOR_R_SPEED, 0);
}


void path_finding() {
  int speed = 100;
  unsigned long time = millis();

  while (analogRead(SENS_L) < 700 && analogRead(SENS_R) < 700) {
  left(130, 100);
  delay(100);
  forward_look(speed);
  delay(60);

  speed = speed + 2;

  if (speed >= 140) {speed = 140;}
  delay(100);

  if (millis() - time > 5000)
  {
    stop();
    delay(7000);
  }
  }

}

void ride (){
  int sens_l = analogRead(SENS_L);
  int sens_r = analogRead(SENS_R);
  if (sens_l > 800 && sens_r > 800) {forward(155);}
  else if (sens_l > 800 && sens_r < 800) {left(200, 130);}
  else if (sens_r > 800 && sens_l < 800) {right(200, 130);}
}

void setup() {
  Serial.begin(9600);
  pinMode(MOTOR_L_SPEED, OUTPUT);
  pinMode(MOTOR_L_DIR, OUTPUT);
  pinMode(MOTOR_R_SPEED, OUTPUT);
  pinMode(MOTOR_R_DIR, OUTPUT);
}

void loop() {
  ride();
  path_finding();
