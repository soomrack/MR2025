#define Sensor_L A0
#define Sensor_R A1
#define Button 12
#define Motor_L_dir 7
#define Motor_L_pow 6
#define Motor_R_dir 4
#define Motor_R_pow 5




bool sensigL, sensigR, act;
void lost(); 
act = false;



void setup() {
  pinMode(Motor_L_pow, OUTPUT);
  pinMode(Motor_L_dir, OUTPUT);
  pinMode(Motor_R_pow, OUTPUT);
  pinMode(Motor_R_dir, OUTPUT);
  pinMode(Sensor_L, INPUT);
  pinMode(Sensor_R, INPUT);
  pinMode(Motor_R_pow, OUTPUT);
  pinMode(Button, INPUT);
}



void loop() {
  act = digitalRead(Button);
  if (act == 1) {
    sensigL = digitalRead(Sensor_L);
    sensigR = digitalRead(Sensor_R);
    if (sensigL == false && sensigR == false) {
    anologWrite(Motor_L_pow, 150);
    digitalWrite(Motor_L_dir, 0);
    anologWrite(Motor_R_pow, 150);
    digitalWrite(Motor_R_dir, 0);
    }
    if (sensigL != false && sensigR == false) {
    anologWrite(Motor_L_pow, 90);
    digitalWrite(Motor_L_dir, 0);
    anologWrite(Motor_R_pow, 1);
    digitalWrite(Motor_R_dir, 90);
    }
    if (sensigL == false && sensigR != false) {
    anologWrite(Motor_L_pow, 90);
    digitalWrite(Motor_L_dir, 1);
    anologWrite(Motor_R_pow, 90);
    digitalWrite(Motor_R_dir, 0);
    }
    if (sensigL != false && sensigR != false) {
    lost();
    }
  }
  else {
      Serial.begin(9600);
      Serial.print("Der Bot ist nicht aktiv");
    }

  }



}
  
void lost() {
  int time = millis()/1000;
  while (time <= 60) {

    sensigL = digitalRead(Sensor_L);
    sensigR = digitalRead(Sensor_R);
    anologWrite(Motor_L_pow, 130);
    digitalWrite(Motor_L_dir, 0);
    anologWrite(Motor_R_pow, 30 + time);
    digitalWrite(Motor_R_dir, 0);
    if (sensigL != false && sensigR == false) {
      break;
    }
    if (sensigL == false && sensigR == false) {
      break;
    }
    if (sensigL == false && sensigR != false) {
      break;
    }
  }
}

 

  




  

