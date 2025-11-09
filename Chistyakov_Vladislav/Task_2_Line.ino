#define SENSOR_L_PIN A1
#define SENSOR_R_PIN A0
#define BUTTON_PIN 12
#define MOTOR_L_DIR 7
#define MOTOR_L_POW 6
#define MOTOR_R_DIR 4
#define MOTOR_R_POW 5
#define SOUND_PIN 11

// Значения PD контроллера
float gain_P = 7.5;
float gain_D = 6.0;

int speed = 125;                // Скорость на линии
int max_speed = 250;            // Максимальная скорость
int search_speed = 30;         // Скорость поиска линии
int spiral_strength = 5;        // Сила раскручивания при поиске линии
int sensor_threshold = 60;      // Чувствительность к потере линии в %
int search_timeout = 20000;     // Максимальное время поиска линии
int spiral_interval = 200;

struct Sensor_parametrs {
  int current;
  int min;
  int max;
};

struct Sensor_parametrs sensor_L;
struct Sensor_parametrs sensor_R;
int last_error = 0;
int spiral_speed = 0;
int spiral_timer = 0;
bool is_bot_active = false;
bool is_bot_on_line = true;

void move_motors (int leftspeed, int rightspeed) {
  digitalWrite (MOTOR_L_DIR, leftspeed > 0);
  digitalWrite (MOTOR_R_DIR, rightspeed > 0);
  analogWrite (MOTOR_L_POW, abs(leftspeed));
  analogWrite (MOTOR_R_POW, abs(rightspeed));
}


void stop_robot() {
  move_motors(0, 0);
  is_bot_active = false;
}


// Усреднение значений при колибровке сенсоров
int avarage(int Pin) {
  long sum = 0;
  for (int i = 0; i < 10; i++)
    sum += analogRead(Pin);
  return (sum / 10);
}


void sensor_calibration() {
  while (digitalRead(BUTTON_PIN)) delay(10);
  while (!digitalRead(BUTTON_PIN)) delay(10);

  sensor_L.min = avarage(SENSOR_L_PIN);
  sensor_R.min = avarage(SENSOR_R_PIN);
    
  while (digitalRead(BUTTON_PIN)) delay(10);
  while (!digitalRead(BUTTON_PIN)) delay(10);

  sensor_L.max = avarage(SENSOR_L_PIN);
  sensor_R.max = avarage(SENSOR_R_PIN);
}


void sensor_read() {
  sensor_L.current = map(analogRead(SENSOR_L_PIN), sensor_L.min, sensor_L.max, 0, 100);
  sensor_R.current = map(analogRead(SENSOR_R_PIN), sensor_R.min, sensor_R.max, 0, 100);

  if (!is_bot_on_line) {
    // Проверка — найдена ли линия
    if (sensor_L.current < sensor_threshold || sensor_R.current < sensor_threshold) {
        is_bot_on_line = true;
        spiral_speed = 0;
    }
    else is_bot_on_line = false;
  }
    // Проверка — потеряна ли линия
  else {
    if (sensor_L.current < sensor_threshold && sensor_R.current < sensor_threshold) {
        is_bot_on_line = false;
    }
    else is_bot_on_line = true;
  }
}


void line_following() {
  float error = sensor_L.current - sensor_R.current;
  float P = error * gain_P;
  float D = (error - last_error) * gain_D;
  float correct = P + D;

  move_motors(constrain(speed - correct, -max_speed, max_speed), constrain(speed + correct, - max_speed, max_speed));

  last_error = error;
}


void line_search() {
  if (millis() - spiral_timer > spiral_interval) {
    spiral_step += spiral_increase;
    spiral_timer = millis();
  }

  int left_speed = search_speed - spiral_step;
  int right_speed = search_speed - spiral_step;

  if (spiral_direction) drive(left_speed, search_speed);
  else drive(search_speed, right_speed);

  if (sensor_left > light_threshold || sensor_right > light_threshold) {
    searching = false;
    last_direction = (sensor_left > light_threshold) ? 0 : 1;
    state = FOLLOW_LINE;
  }

  if (millis() - search_start_time > search_timeout) {
    stop_robot();
  }
}


void setup() {
  Serial.begin(9600);

  pinMode(MOTOR_L_POW, OUTPUT);
  pinMode(MOTOR_L_DIR, OUTPUT);
  pinMode(MOTOR_R_POW, OUTPUT);
  pinMode(MOTOR_R_DIR, OUTPUT);

  pinMode(SOUND_PIN, OUTPUT);

  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(SENSOR_L_PIN, INPUT);
  pinMode(SENSOR_R_PIN, INPUT);

  sensor_calibration();

  while (digitalRead(BUTTON_PIN)) {}
  while (!digitalRead(BUTTON_PIN)) {}

  is_bot_active = true;
}


void loop() {
  if (is_bot_active) {
    sensor_read();
    if (is_bot_on_line) {
    line_following();
    tone(SOUND_PIN, 10, 10);
    }
    else {
    line_search();
    tone(SOUND_PIN, 500, 500);
    }
  }
}