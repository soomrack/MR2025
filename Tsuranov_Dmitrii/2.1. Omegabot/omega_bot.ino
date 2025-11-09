#define LEFT_BRIGHTNESS_PIN A0  //change
#define RIGHT_BRIGHTNESS_PIN A1 //change
#define LEFT_MOTOR_POWER_PIN 6      
#define RIGHT_MOTOR_POWER_PIN 5     
#define RIGHT_MOTOR_DIRECTION_PIN 4     
#define LEFT_MOTOR_DIRECTION_PIN 7      
#define BUZZER_PIN 11

#define DT 10
#define AVG_SPEED 80

enum class Omega_bot_state{
  START,
  IS_SEARCHING,
  IS_FOLLOWING,
  STOP
};

int brightness_threshold = 313;
unsigned long int TIME = 0;
Omega_bot_state state;

class Light_sensor{
private:
  int brightness_pin;
public:
  Light_sensor(int new_brightness_pin);
  int get_brightness();
};

Light_sensor::Light_sensor(int new_brightness_pin){
  brightness_pin = new_brightness_pin;
}

int Light_sensor::get_brightness(){
  return 1023-analogRead(brightness_pin);
}

class Motor{
    private:
      int power_pin;
      int direction_pin;
    public:
      Motor(int new_power_pin, int new_direction_pin);
      void drive(int speed);
};

Motor::Motor(int new_power_pin, int new_direction_pin){
  power_pin = new_power_pin;
  direction_pin = new_direction_pin;
}

void Motor::drive(int speed){
  if (speed >= 0){
    digitalWrite(direction_pin, HIGH);
    analogWrite(power_pin, speed);
  } else {
    digitalWrite(direction_pin, LOW);
    analogWrite(power_pin, -speed);
  }
  
}

Light_sensor left_light_sensor = Light_sensor(LEFT_BRIGHTNESS_PIN);
Light_sensor right_light_sensor = Light_sensor(RIGHT_BRIGHTNESS_PIN);

Motor left_motor = Motor(LEFT_MOTOR_POWER_PIN, LEFT_MOTOR_DIRECTION_PIN);
Motor right_motor = Motor(RIGHT_MOTOR_POWER_PIN, RIGHT_MOTOR_DIRECTION_PIN);

void change_state(Omega_bot_state new_state)
{
  state = new_state;
  TIME = millis();
}

int compute_PID(float input, float setpoint, float kp, float ki, float kd)
{
  float err = setpoint - input;
  //static float integral = 0;
  static float prev_err = 0;
  //integral += err*dt;
  float D = (err-prev_err)/DT;
  prev_err = err;
  return err*kp /*+ integral*ki*/ + D*kd;
}

void start(){
  int white_value = 0;
  int dark_value = 0;

  white_value = (left_light_sensor.get_brightness()+right_light_sensor.get_brightness())/2;
  tone(BUZZER_PIN, 2000, 100);
  Serial.print("White value: ");
  Serial.println(white_value);
  delay(4000);

  dark_value = (left_light_sensor.get_brightness()+right_light_sensor.get_brightness())/2;
  tone(BUZZER_PIN, 4000, 100);
  Serial.print("Dark value: ");
  Serial.println(dark_value);

  brightness_threshold = (white_value+dark_value)/2;
  Serial.print("Threshold: ");
  Serial.println(brightness_threshold);

  delay(2000);
  change_state(Omega_bot_state::IS_FOLLOWING);
}

void follow_line(){
  double left_brightness = left_light_sensor.get_brightness();
  double right_brightness = right_light_sensor.get_brightness();

  Serial.print("Left brightness: ");
  Serial.print(left_brightness);
  Serial.print(" Right brightness: ");
  Serial.print(right_brightness);

  int left_speed;
  int right_speed;

  double error = right_brightness-left_brightness;
  int control = compute_PID(error, 0, 5,0,2.3);
  static int max_control = 0;
  static int min_control = 0;
  max_control = max(max_control, control);
  min_control = min(min_control, control);
  Serial.print(" Control: ");
  Serial.print(control);
  Serial.print(" Max control: ");
  Serial.print(max_control);
  Serial.print(" Min control: ");
  Serial.print(min_control);
  if (left_brightness > brightness_threshold && right_brightness > brightness_threshold){
    //change_state(Omega_bot_state::IS_SEARCHING);
  }

  left_speed = AVG_SPEED + control;
  right_speed = AVG_SPEED - control;
  
  Serial.print(" Left speed: ");
  Serial.print(left_speed);
  Serial.print(" Right speed: ");
  Serial.println(right_speed);

  left_motor.drive(left_speed);
  right_motor.drive(right_speed);
}

void search_line(){
  left_motor.drive(150);
  right_motor.drive(100);
  if(left_light_sensor.get_brightness() < brightness_threshold || right_light_sensor.get_brightness() < brightness_threshold){
    change_state(Omega_bot_state::IS_FOLLOWING);
  }
  if(millis()-TIME > 5000){
    change_state(Omega_bot_state::STOP);
  }
}

void stop(){
  left_motor.drive(0);
  right_motor.drive(0);
}


void setup() {
  Serial.begin(9600);
  change_state(Omega_bot_state::IS_FOLLOWING);
}

void loop() {
  if (state == Omega_bot_state::START){
    start();
  }
  if (state == Omega_bot_state::IS_SEARCHING){
    search_line();
  }
  if (state == Omega_bot_state::IS_FOLLOWING){
    follow_line();
  }
  if (state == Omega_bot_state::STOP){
    stop();
  }
  delay(DT);
}
