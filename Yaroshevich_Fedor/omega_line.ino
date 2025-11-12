#define PWM_Pin_Left_Motor 5
#define Dir_Pin_Left_Motor 4
#define PWM_Pin_Right_Motor 6
#define Dir_Pin_Right_Motor 7
#define Pin_Left_Sensor A0
#define Pin_Right_Sensor A1

const int black_threshold_value = 700;
const int motor_speed = 150;
const int delta_motor_turn_speed = 40;

bool searching = false;
bool aligning = false;
unsigned long time_of_lost = 0;
unsigned long time_of_cross_line_check = 0; 
bool is_checking_cross_line = false;        
const unsigned long time_for_searching = 5000;


void move_forward() {
    digitalWrite(Dir_Pin_Left_Motor, HIGH);
    analogWrite(PWM_Pin_Left_Motor, motor_speed);
    digitalWrite(Dir_Pin_Right_Motor, HIGH);
    analogWrite(PWM_Pin_Right_Motor, motor_speed);
}

void turn_left() {
    digitalWrite(Dir_Pin_Left_Motor, LOW);
    analogWrite(PWM_Pin_Left_Motor, motor_speed);
    digitalWrite(Dir_Pin_Right_Motor, HIGH);
    analogWrite(PWM_Pin_Right_Motor, motor_speed + delta_motor_turn_speed);
}

void turn_right() {
    digitalWrite(Dir_Pin_Left_Motor, HIGH);
    analogWrite(PWM_Pin_Left_Motor, motor_speed + delta_motor_turn_speed);
    digitalWrite(Dir_Pin_Right_Motor, LOW);
    analogWrite(PWM_Pin_Right_Motor, motor_speed);
}


void searching_right() {
    digitalWrite(Dir_Pin_Left_Motor, HIGH);
    analogWrite(PWM_Pin_Left_Motor, 150);
    digitalWrite(Dir_Pin_Right_Motor, LOW);
    analogWrite(PWM_Pin_Right_Motor, 0);
}

void searching_left() {
    digitalWrite(Dir_Pin_Left_Motor, LOW);
    analogWrite(PWM_Pin_Left_Motor, 0);
    digitalWrite(Dir_Pin_Right_Motor, HIGH);
    analogWrite(PWM_Pin_Right_Motor, 150);
}


void rotate_on_spot_right() {
    digitalWrite(Dir_Pin_Left_Motor, HIGH);
    analogWrite(PWM_Pin_Left_Motor, 170);
    digitalWrite(Dir_Pin_Right_Motor, LOW);
    analogWrite(PWM_Pin_Right_Motor, 170);
}

void rotate_on_spot_left() {
    digitalWrite(Dir_Pin_Left_Motor, LOW);
    analogWrite(PWM_Pin_Left_Motor, 170);
    digitalWrite(Dir_Pin_Right_Motor, HIGH);
    analogWrite(PWM_Pin_Right_Motor, 170);
}

void stop_all_motors() {
    analogWrite(PWM_Pin_Left_Motor, 0);
    analogWrite(PWM_Pin_Right_Motor, 0);
}


bool isLeftBlack() { return analogRead(Pin_Left_Sensor) > black_threshold_value; }
bool isRightBlack() { return analogRead(Pin_Right_Sensor) > black_threshold_value; }


void search_the_black_line(bool left_sensor_black, bool right_sensor_black) {
    if (is_checking_cross_line) {
        if (millis() - time_of_cross_line_check > 100) {
            bool still_both_black = isLeftBlack() && isRightBlack();
            if (still_both_black) {
                is_checking_cross_line = false;
                searching = false;
                aligning = false;
            } else {
                is_checking_cross_line = false;
                aligning = true;
            }
        } else {
            stop_all_motors();
            return;
        }
    }

    if (aligning) {
        bool l = isLeftBlack();
        bool r = isRightBlack();
        if (l && r) {
            aligning = false;
            searching = false;
            return;
        }
        if (l && !r) {
            rotate_on_spot_left();
            delay(60);
            stop_all_motors();
        } else if (!l && r) {
            rotate_on_spot_right();
            delay(60);
            stop_all_motors();
        } else {
            searching = true;
            aligning = false;
            time_of_lost = millis();
        }
        return;
    }

    if (left_sensor_black || right_sensor_black) {
        if (left_sensor_black && right_sensor_black) {
            stop_all_motors();
            is_checking_cross_line = true;
            time_of_cross_line_check = millis();
        } else {
            stop_all_motors();
            aligning = true;
        }
        return;
    }

    unsigned long passed_time = millis() - time_of_lost;
    if ((passed_time / 2000) % 2 == 0) {
        searching_right();
    } else {
        searching_left();
    }
}


void follow_the_black_line(bool left_sensor_black, bool right_sensor_black) {
    if (left_sensor_black && right_sensor_black) {
        move_forward();
    }
    else if (left_sensor_black && !right_sensor_black) {
        turn_left();
    }
    else if (!left_sensor_black && right_sensor_black) {
        turn_right();
    }
    else {
        if (!searching) {
            searching = true;
            time_of_lost = millis();
        }
    }
}


void setup() {
    Serial.begin(9600);
    pinMode(PWM_Pin_Left_Motor, OUTPUT);
    pinMode(Dir_Pin_Left_Motor, OUTPUT);
    pinMode(PWM_Pin_Right_Motor, OUTPUT);
    pinMode(Dir_Pin_Right_Motor, OUTPUT);
}

void loop() {
    bool left_sensor_Black = isLeftBlack();
    bool right_sensor_Black = isRightBlack();

    if (searching || aligning || is_checking_cross_line) {
        search_the_black_line(left_sensor_Black, right_sensor_Black);
        
        if (millis() - time_of_lost > time_for_searching) {
        stop_all_motors();
        return;
    }
    } else {
        follow_the_black_line(left_sensor_Black, right_sensor_Black);
    }
}