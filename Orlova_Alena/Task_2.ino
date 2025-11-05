const int Left_Sensor = A3;
const int Right_Sensor = A2;
const int Left_Motor_PWM = 6;
const int Left_Motor_Dir = 7;
const int Right_Motor_PWM = 5;
const int Right_Motor_Dir = 4;
const int Buzzer = 8;

const int Black_Threshold = 700;
const int Motor_Speed = 150;
const int dMotor_Delta_Speed = 40;

unsigned long Lost_Time = 0;
const unsigned long Search_Time = 4000;
bool Is_Searching = false;

void setup() {
  Serial.begin(9600);
  pinMode(Left_Sensor, INPUT);
  pinMode(Right_Sensor, INPUT);
  pinMode(Buzzer, OUTPUT);
  pinMode(Left_Motor_PWM, OUTPUT);
  pinMode(Left_Motor_Dir, OUTPUT);
  pinMode(Right_Motor_PWM, OUTPUT);
  pinMode(Right_Motor_Dir, OUTPUT);
}

void loop() {
  int leftSensorValue = analogRead(Left_Sensor);
  int rightSensorValue = analogRead(Right_Sensor);
  bool isLeftLineBlack = leftSensorValue > Black_Threshold;
  bool isRightLineBlack = rightSensorValue > Black_Threshold;

  if (Is_Searching) {
    search_line(isLeftLineBlack, isRightLineBlack);
  } else {
    follow_line(isLeftLineBlack, isRightLineBlack);
  }
}

void beep(int duration) {
  tone(Buzzer, 500, duration);
}

void follow_line(bool isLeftLineBlack, bool isRightLineBlack) {
  if (isLeftLineBlack && isRightLineBlack) {
    move_Forward();
  } else if (!isLeftLineBlack && isRightLineBlack) {
    turn_Left();
  } else if (isLeftLineBlack && !isRightLineBlack) {
    turn_Right();
  } else {
    if (!Is_Searching) {
      Is_Searching = true;
      Lost_Time = millis();
      beep(200);
    }
  }
}

void search_line(bool isLeftLineBlack, bool isRightLineBlack) {
  if (isLeftLineBlack || isRightLineBlack) {
    Is_Searching = false;
    beep(100);
    return;
  }

  if (millis() - Lost_Time > Search_Time) {
    Stop_Moving();
    while(true) {
      digitalWrite(Buzzer, HIGH);
    }
  }

  unsigned long currentTime = millis() - Lost_Time;
  
  if (currentTime % 2000 < 2000) {
    move_Find_Right();
  } else {
    move_Find_Left();
  }
}


void move_Forward() {
  analogWrite(Left_Motor_PWM, Motor_Speed);
  digitalWrite(Left_Motor_Dir, HIGH);
  analogWrite(Right_Motor_PWM, Motor_Speed);
  digitalWrite(Right_Motor_Dir, HIGH);
}

void turn_Left() {
  analogWrite(Left_Motor_PWM, Motor_Speed);
  digitalWrite(Left_Motor_Dir, LOW);
  analogWrite(Right_Motor_PWM, Motor_Speed + dMotor_Delta_Speed);
  digitalWrite(Right_Motor_Dir, HIGH);
}

void turn_Right() {
  analogWrite(Left_Motor_PWM, Motor_Speed + dMotor_Delta_Speed);
  digitalWrite(Left_Motor_Dir, HIGH);
  analogWrite(Right_Motor_PWM, Motor_Speed);
  digitalWrite(Right_Motor_Dir, LOW);
}

void Stop_Moving() {
  digitalWrite(Left_Motor_PWM, LOW);
  digitalWrite(Left_Motor_Dir, LOW);
  digitalWrite(Right_Motor_PWM, LOW);
  digitalWrite(Right_Motor_Dir, LOW);
}

void move_Find_Right() {
  analogWrite(Left_Motor_PWM, 200);
  digitalWrite(Left_Motor_Dir, HIGH);
  analogWrite(Right_Motor_PWM, 0);
  digitalWrite(Right_Motor_Dir, LOW);
}

void move_Find_Left() {
  analogWrite(Left_Motor_PWM, 0);
  digitalWrite(Left_Motor_Dir, LOW);
  analogWrite(Right_Motor_PWM, 200);
  digitalWrite(Right_Motor_Dir, HIGH);
}