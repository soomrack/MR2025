#include <Servo.h>

bool flag= false;

const int leftSensorPin = A3;
const int rightSensorPin = A2;
const int leftMotorPin1 = 6;
const int leftMotorPin2 = 7;
const int rightMotorPin1 = 5;
const int rightMotorPin2 = 4;



long duration, cm;

// Пороговые значения для определения черной и белой линии
const int blackThreshold = 700;
const int whiteThreshold = 200;

// Скорость движения моторов (от 0 до 255)
const int motorSpeed = 140;
const int deltaSpeed = 40;

void setup() {

  Serial.begin(9600);

// Настройка пинов датчиков линии как входов
 pinMode(leftSensorPin, INPUT);
 pinMode(rightSensorPin, INPUT);
 
// Настройка пинов моторов как выходов
 pinMode(leftMotorPin1, OUTPUT);
 pinMode(leftMotorPin2, OUTPUT);
 pinMode(rightMotorPin1, OUTPUT);2
 pinMode(rightMotorPin2, OUTPUT);
}


void loop() {

 int leftSensorValue = analogRead(leftSensorPin);
 int rightSensorValue = analogRead(rightSensorPin);
 
 bool isLeftLineBlack = leftSensorValue > blackThreshold;
 bool isRightLineBlack = rightSensorValue > blackThreshold;
 
 if (isLeftLineBlack && isRightLineBlack) {
  moveForward();
 } else if (!isLeftLineBlack && isRightLineBlack) {
   turnLeft();
   } else if (isLeftLineBlack && !isRightLineBlack) {
 turnRight();
 } else {
  stopMoving();
 }
}

void moveForward() {
 analogWrite(leftMotorPin1, motorSpeed);
 digitalWrite(leftMotorPin2, HIGH);
 analogWrite(rightMotorPin1, motorSpeed);
 digitalWrite(rightMotorPin2, HIGH);
}

void turnLeft() {
 analogWrite(leftMotorPin1, motorSpeed);
 digitalWrite(leftMotorPin2, LOW);
 analogWrite(rightMotorPin1, motorSpeed+deltaSpeed);
 digitalWrite(rightMotorPin2, HIGH);
}

void turnRight() {
 analogWrite(leftMotorPin1, motorSpeed+deltaSpeed);
 digitalWrite(leftMotorPin2, HIGH);
 analogWrite(rightMotorPin1, motorSpeed);
 digitalWrite(rightMotorPin2, LOW);
}

void stopMoving() {
 digitalWrite(leftMotorPin1, LOW);
 digitalWrite(leftMotorPin2, LOW);
 digitalWrite(rightMotorPin1, LOW);
 digitalWrite(rightMotorPin2, LOW);
}
