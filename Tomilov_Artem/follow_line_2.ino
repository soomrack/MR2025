#define left_pwm 6
#define left_dir 7
#define right_pwm 5
#define right_dir 4
#define sound 9
#define button A2
#define left_sensor A0
#define right_sensor A1

#define gain_p 8.0
#define gain_d 6.0
#define base_speed 90
#define search_speed 110

#define light_threshold 50
#define spiral_increase 3
#define spiral_interval 50

long long int time_sound = 0;
int left_min = 1023;
int left_max = 0;
int right_min = 1023;
int right_max = 0;

int last_direction = 0;
int errold = 0;

bool is_active = false; 
bool button_old = 1;
bool searching = false;   
bool spiral_direction = 0;
int spiral_step = 0;        
unsigned long spiral_timer = 0;

void drive(int left, int right) {
  digitalWrite(left_dir, left > 0);
  digitalWrite(right_dir, right > 0);
  analogWrite(left_pwm, abs(left));
  analogWrite(right_pwm, abs(right));
}

void calibrate() {
  drive(120, -120);
  delay(4000);
  drive(0, 0);
}

void start_search() {
  searching = true;
  spiral_step = 0;
  spiral_timer = millis();
  spiral_direction = (last_direction == 0) ? 1 : 0;
}

void spiral_search() {
  if (millis() - spiral_timer > spiral_interval) {
    spiral_step += spiral_increase;
    spiral_timer = millis();
  }

  int left_speed = search_speed - spiral_step;
  int right_speed = search_speed - spiral_step;

  if (spiral_direction) {
    drive(left_speed, search_speed);
  } else {
    drive(search_speed, right_speed);
  }
}

void line_following(int s1, int s2) {
  double err = (s1 - s2);
  double u = err * gain_p + (err - errold) * gain_d;
  drive(constrain(base_speed + u, -250, 250), 
        constrain(base_speed - u, -250, 250));
  errold = err;
}

void check_sensors() {
  int s1 = map(analogRead(left_sensor), left_min, left_max, 0, 100);
  int s2 = map(analogRead(right_sensor), right_min, right_max, 0, 100);

  if (searching) {
    if (s1 > light_threshold || s2 > light_threshold) {
      searching = false;
      last_direction = (s1 > light_threshold) ? 0 : 1;
    } else {
      spiral_search();
    }
  } else {
    if (s1 < light_threshold && s2 < light_threshold) {
      start_search();
    } else {
      line_following(s1, s2);
    }
  }
}

void setup() {
  pinMode(left_pwm, OUTPUT);
  pinMode(left_dir, OUTPUT);
  pinMode(right_pwm, OUTPUT);
  pinMode(right_dir, OUTPUT);
  pinMode(sound, OUTPUT);
  
  pinMode(button, INPUT_PULLUP);
  
  int tim = millis();
  while (millis() - tim < 4000) {
    drive(120, -120);
    int left = analogRead(left_sensor);
    int right = analogRead(right_sensor);
    
    if (left < left_min) left_min = left;
    if (left > left_max) left_max = left;
    if (right < right_min) right_min = right;
    if (right > right_max) right_max = right;
  }
  drive(0, 0);
  
  while (true) {
    if (digitalRead(button) == LOW) { 
      delay(50);
      if (digitalRead(button) == LOW) {
        break;
      }
    }
  }
  
  tone(sound, 1000, 200);
  delay(200);
}

void loop() {
  if (is_active) {
    check_sensors();
  }
  
  if (digitalRead(button) == HIGH && button_old == LOW) {
    is_active = !is_active;
    searching = false;
  }
  button_old = digitalRead(button);
}
