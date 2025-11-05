 const int Left_Sensor_Pin = A3;
  const int Right_Sensor_Pin = A2;
  const int Left_Motor_PWM_Pin = 6;
  const int Left_Motor_Dir_Pin = 7;
  const int Right_Motor_PWM_Pin = 5;
  const int Right_Motor_Dir_Pin = 4;
  const int Buzzer_Pin = 8;

  const int Black_Threshold = 700;
  const int Motor_Speed = 150;
  const int dMotor_Delta_Speed = 40;


  unsigned long Lost_Time = 0;
  const unsigned long Search_Time = 4000;
  bool Is_Searching = false;
  bool Is_waiting = false;

  void setup() {
    Serial.begin(9600);
    pinMode(Left_Sensor_Pin, INPUT);
    pinMode(Right_Sensor_Pin, INPUT);
    pinMode(Buzzer_Pin, OUTPUT);
    pinMode(Left_Motor_PWM_Pin, OUTPUT);
    pinMode(Left_Motor_Dir_Pin, OUTPUT);
    pinMode(Right_Motor_PWM_Pin, OUTPUT);
    pinMode(Right_Motor_Dir_Pin, OUTPUT);
  }

  void beep(int duration) {
    tone(Buzzer_Pin, 500, duration);
  }

  void follow_line(bool is_Left_Line_Black, bool is_Right_Line_Black) {
    if (is_Left_Line_Black && is_Right_Line_Black) {
      move_Forward();
    } else if (!is_Left_Line_Black && is_Right_Line_Black) {
      turn_Left();
    } else if (is_Left_Line_Black && !is_Right_Line_Black) {
      turn_Right();
    } else {
      if (!Is_Searching) {
        Is_Searching = true;
        Lost_Time = millis();
        beep(200);
      }
    }
  }

  void search_line(bool is_Left_Line_Black, bool is_Right_Line_Black) {
    if (is_Left_Line_Black || is_Right_Line_Black) {
      Is_Searching = false;
      beep(100);
      return;
    }

    unsigned long current_Time = millis() - Lost_Time;
    
    if (current_Time % 2000 < 2000) {
      move_Find_Right();
    } else {
      move_Find_Left();
    }

    if (millis() - Lost_Time > Search_Time) {
      Is_waiting = true;
    }
  }

  void move_Forward() {
    analogWrite(Left_Motor_PWM_Pin, Motor_Speed);
    digitalWrite(Left_Motor_Dir_Pin, HIGH);
    analogWrite(Right_Motor_PWM_Pin, Motor_Speed);
    digitalWrite(Right_Motor_Dir_Pin, HIGH);
  }

  void turn_Left() {
    analogWrite(Left_Motor_PWM_Pin, Motor_Speed);
    digitalWrite(Left_Motor_Dir_Pin, LOW);
    analogWrite(Right_Motor_PWM_Pin, Motor_Speed + dMotor_Delta_Speed);
    digitalWrite(Right_Motor_Dir_Pin, HIGH);
  }

  void turn_Right() {
    analogWrite(Left_Motor_PWM_Pin, Motor_Speed + dMotor_Delta_Speed);
    digitalWrite(Left_Motor_Dir_Pin, HIGH);
    analogWrite(Right_Motor_PWM_Pin, Motor_Speed);
    digitalWrite(Right_Motor_Dir_Pin, LOW);
  }

  void Stop_Moving() {
    digitalWrite(Left_Motor_PWM_Pin, LOW);
    digitalWrite(Left_Motor_Dir_Pin, LOW);
    digitalWrite(Right_Motor_PWM_Pin, LOW);
    digitalWrite(Right_Motor_Dir_Pin, LOW);
  }

  void move_Find_Right() {
    analogWrite(Left_Motor_PWM_Pin, 200);
    digitalWrite(Left_Motor_Dir_Pin, HIGH);
    analogWrite(Right_Motor_PWM_Pin, 0);
    digitalWrite(Right_Motor_Dir_Pin, LOW);
  }

  void move_Find_Left() {
    analogWrite(Left_Motor_PWM_Pin, 0);
    digitalWrite(Left_Motor_Dir_Pin, LOW);
    analogWrite(Right_Motor_PWM_Pin, 200);
    digitalWrite(Right_Motor_Dir_Pin, HIGH);
  }

  void loop() {
    int left_Sensor_Value = analogRead(Left_Sensor_Pin);
    int right_Sensor_Value = analogRead(Right_Sensor_Pin);
    bool is_Left_Line_Black = left_Sensor_Value > Black_Threshold;
    bool is_Right_Line_Black = right_Sensor_Value > Black_Threshold;

    if (Is_Searching) {
      search_line(is_Left_Line_Black, is_Right_Line_Black);
    } else {
      follow_line(is_Left_Line_Black, is_Right_Line_Black);
    }
    if (Is_waiting) {
    Stop_Moving();
    beep(Search_Time);
    }
  }
