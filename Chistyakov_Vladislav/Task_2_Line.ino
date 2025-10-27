#define Sensor_L A0
#define Sensor_R A1
#define Button 12
#define Motor_L_dir 7
#define Motor_L_pow 6
#define Motor_R_dir 4
#define Motor_R_pow 5

float factor_a = 1.0;
float factor_b = 1.0;
int speed = 100;

int sensor_L_min, sensor_L_max, sensor_R_min, sensor_R_max;

int last_error = 0;
bool is_bot_active = false;
bool button_last = HIGT;

void motors (int leftspeed, int rightspeed) {
  digitalWrite (Motor_L_dir, leftspeed > 0);
  digitalWrite (Motor_R_dir, rightspeed > 0);
  analogWrite (Motor_L_pow, abs(leftspeed));
  analogWrite (Motor_R_pow, abs(rightspeed));
}

int avarage(int Pin) {
  long sum = 0;
  for (int i = 0; i < 10; i++)
    sum += analogRead(Pin);
  return (sum / 10);
}

void sensor_calibration() {
  Serial.println("Calibration started");

  while (digitalRead(Button)) {delay(10);}
  while (!digitalRead(Button)) {delay(10);}

  sensor_L_min = avarage(Sensor_L);
  sensor_R_min = avarage(Sensor_R);
    
  while (digitalRead(Button)) {delay(10);}
  while (!digitalRead(Button)) {delay(10);}

  sensor_L_max = avarage(Sensor_L);
  sensor_R_max = avarage(Sensor_R);

  Serial.print("L: "); Serial.print(sensor_L_min); Serial.print(" - "); Serial.println(sensor_L_max);
  Serial.print("R: "); Serial.print(sensor_R_min); Serial.print(" - "); Serial.println(sensor_R_max);

  
}

void line_following() {
  int value_left = map(analogRead(Sensor_L), sensor_L_min, sensor_L_max, 0, 100);
  int value_right = map(analogRead(Sensor_R), sensor_R_min, sensor_R_max, 0, 100);

  float error = value_left - value_right;
  float correct = error * factor_a + (error - last_error) * factor_b;

  move_motors(constrain(speed + correct, -250, 250), constrain(speed - correct, -250, 250));

  last_error = error;
}

void line_lost () {

}

void setup() {
  Serial.begin(9600);

  pinMode(Motor_L_pow, OUTPUT);
  pinMode(Motor_L_dir, OUTPUT);
  pinMode(Motor_R_pow, OUTPUT);
  pinMode(Motor_R_dir, OUTPUT);

  pinMode(Button, INPUT_PULLUP);
  pinMode(Sensor_L, INPUT);
  pinMode(Sensor_R, INPUT);

  sensor_calibration();

  while (digitalRead(Button) == HIGH) {}

  is_bot_active = true;
}

void loop() {
  bool button_state = digitalRead(Button);

  if (button_state == LOW && button_old == HIGH) {
    is_bot_active = !is_bot_active;
    delay(30);
  }

  button_last = button_state;

  if (is_bot_active) {
    line_following();
  }
}