#define LEFT_BRIGHTNESS_PIN A0  //change
#define RIGHT_BRIGHTNESS_PIN A1 //change
#define LEFT_MOTOR_POWER_PIN 6      
#define RIGHT_MOTOR_POWER_PIN 5     
#define RIGHT_MOTOR_DIRECTION_PIN 4     
#define LEFT_MOTOR_DIRECTION_PIN 7      
#define BUZZER_PIN 11

#define DT 20
#define AVG_SPEED 100

enum class OmegaBotState{
  START,
  IS_SEARCHING,
  IS_FOLLOWING,
  STOP
};

int brightness_threshold = 313;
unsigned long int TIME = 0;
OmegaBotState state;

class LightSensor{
private:
  int brightness_pin;
public:
  LightSensor(int new_brightness_pin);
  int get_brightness();
};

LightSensor::LightSensor(int new_brightness_pin){
  brightness_pin = new_brightness_pin;
}

int LightSensor::get_brightness(){
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

LightSensor left_LightSensor = LightSensor(LEFT_BRIGHTNESS_PIN);
LightSensor right_LightSensor = LightSensor(RIGHT_BRIGHTNESS_PIN);

Motor left_motor = Motor(LEFT_MOTOR_POWER_PIN, LEFT_MOTOR_DIRECTION_PIN);
Motor right_motor = Motor(RIGHT_MOTOR_POWER_PIN, RIGHT_MOTOR_DIRECTION_PIN);

void change_state(OmegaBotState new_state)
{
  state = new_state;
  TIME = millis();
}

int compute_PID(int input, int setpoint, float kp, float ki, float kd)
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
  static int white_value = 0;
  int dark_value = 0;
  static bool is_white_value_got = false;
  static bool is_dark_value_got = false;

  if(!is_white_value_got) {
    white_value = (left_LightSensor.get_brightness()+right_LightSensor.get_brightness())/2;
    tone(BUZZER_PIN, 2000, 100);
    Serial.print("White value: ");
    Serial.println(white_value);

    is_white_value_got = true;
  }
  if(millis() - TIME > 4000 && !is_dark_value_got) {
    dark_value = (left_LightSensor.get_brightness()+right_LightSensor.get_brightness())/2;
    tone(BUZZER_PIN, 4000, 100);
    Serial.print("Dark value: ");
    Serial.println(dark_value);

    brightness_threshold = (white_value+dark_value)/2;
    Serial.print("Threshold: ");
    Serial.println(brightness_threshold);

    is_dark_value_got = true;
  }
  
  if (millis()-TIME > 6000){
    change_state(OmegaBotState::IS_FOLLOWING);
  }
}

void follow_line(){
  int left_brightness = left_LightSensor.get_brightness();
  int right_brightness = right_LightSensor.get_brightness();

  // Serial.print("Left brightness: ");
  // Serial.print(left_brightness);
  // Serial.print(" Right brightness: ");
  // Serial.print(right_brightness);

  int left_speed;
  int right_speed;

  int error = right_brightness-left_brightness;
  int control = compute_PID(error, 0, 0.4,0,16); //0.3, 10
  
  if (left_brightness > brightness_threshold && right_brightness > brightness_threshold){
    change_state(OmegaBotState::IS_SEARCHING);
  }

  left_speed = constrain(AVG_SPEED + control, -250, 250);
  right_speed = constrain(AVG_SPEED - control, -250, 250);
  
  Serial.print(" Left speed: ");
  Serial.print(left_speed);
  Serial.print(" Right speed: ");
  Serial.println(right_speed);

  left_motor.drive(left_speed);
  right_motor.drive(right_speed);
}

void search_line(){
  int left_brightness = left_LightSensor.get_brightness();
  int right_brightness = right_LightSensor.get_brightness();
  
  left_motor.drive(200);
  right_motor.drive(-30);

  Serial.print("Left brightness: ");
  Serial.print(left_brightness);
  Serial.print(" Right brightness: ");
  Serial.print(right_brightness);
  Serial.print(" Threshold: ");
  Serial.println(brightness_threshold);

  if(left_LightSensor.get_brightness() < brightness_threshold || right_LightSensor.get_brightness() < brightness_threshold){
    change_state(OmegaBotState::IS_FOLLOWING);
  }
  if(millis()-TIME > 10000){
    change_state(OmegaBotState::STOP);
  }
}

void stop(){
  left_motor.drive(0);
  right_motor.drive(0);
}


void setup() {
  Serial.begin(9600);
  change_state(OmegaBotState::START);
}

void loop() {
  if (state == OmegaBotState::START){
    start();
  }
  if (state == OmegaBotState::IS_SEARCHING){
    search_line();
  }
  if (state == OmegaBotState::IS_FOLLOWING){
    follow_line();
  }
  if (state == OmegaBotState::STOP){
    stop();
  }
  delay(DT);
}
