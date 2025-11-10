#include "DHT.h"

#define PUMP_PIN 5 
#define LIGHT_PIN 6
#define HEAT_PIN 4
#define VENT_PIN 7

#define TEMP_HUMID_SENSOR_PIN 12
#define SOIL_MOISTURE_SENSOR_PIN 3

const int Light_Sensors_Count = 3;
const int LIGHT_SENSORS_PINS[Light_Sensors_Count] = {0, 1, 2}; 

int Air_Humidity_Value;
int Temperature_Value;
int Soil_Moisture_Value;
int Lighting_Values[Light_Sensors_Count];

bool Light_Status;
bool Heat_Status;
bool Vent_Status;

bool Low_Moisture_Flag;
bool Pump_Status;

unsigned long Previous_Time_Marker = 0;
unsigned long Current_Time_Value; 
const unsigned long Day_Period = 10*1000;
const unsigned long Night_Period = 10*1000;
bool Night_Flag;

unsigned long Last_Serial_Time = 0;
const unsigned long Serial_Output_Interval = 5000;

unsigned long Watering_Start_Time = 0;
unsigned long Watering_Stop_Time = 0;   
const unsigned long Watering_Duration = 2000;
const unsigned long Watering_Pause = 2000;
bool Watering_In_Progress = false;
bool Pump_Active_State;

struct Thresholds{
  int Temperature;
  int Air_Humidity;
  int Soil_Moisture;
  int Light;
};

struct Thresholds Min_Values;
struct Thresholds Max_Values;

DHT dht(TEMP_HUMID_SENSOR_PIN, DHT11);

void setup() {
  Serial.begin(9600);
  dht.begin();
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(LIGHT_PIN, OUTPUT);
  pinMode(HEAT_PIN, OUTPUT);
  pinMode(VENT_PIN, OUTPUT);
  Initialize_Thresholds();
}

void Time_Cycle_Manager(){
  Current_Time_Value = millis();
  if (Current_Time_Value - Previous_Time_Marker < Day_Period){
    Night_Flag = false;
  }
  else if (Current_Time_Value - Previous_Time_Marker > Day_Period && Current_Time_Value - Previous_Time_Marker < Night_Period + Day_Period){
    Night_Flag = true;
  }
  else if (Current_Time_Value - Previous_Time_Marker > Night_Period){
    Night_Flag = false;
    Previous_Time_Marker = Current_Time_Value;
  }
}

void Read_Light_Sensors() {
  for (int i = 0; i < Light_Sensors_Count; i++) {
    Lighting_Values[i] = analogRead(LIGHT_SENSORS_PINS[i]);
  }
}

bool Determine_Light_Control() { 
  int Low_Light_Counter = 0;
  for (int i = 0; i < Light_Sensors_Count; i++) {
    if (Lighting_Values[i] >= Min_Values.Light) {
      Low_Light_Counter = Low_Light_Counter + 1;
    }
  }

  int Majority_Threshold = Light_Sensors_Count / 2 + 1;
  return (Low_Light_Counter >= Majority_Threshold && Night_Flag == false);
}

void Check_Light_Condition(){
  Light_Status = Determine_Light_Control();
}

void Check_Temperature_Condition(const int Temp_Value){
  if (Temp_Value < Min_Values.Temperature){
    Heat_Status = true;
  }
  else if (Temp_Value >= Max_Values.Temperature){
    Heat_Status = false;
  }
}

void Check_Air_Humidity_Condition(const int Humidity_Value){
  if (Humidity_Value > Max_Values.Air_Humidity){
    Vent_Status = true;
  }
  else if (Humidity_Value <= Min_Values.Air_Humidity){
    Vent_Status = false;
  }
}

void Check_Soil_Moisture_Condition(const int Moisture_Value){
  if (Soil_Moisture_Value < Min_Values.Soil_Moisture){
    Low_Moisture_Flag = true;
  }
  else if (Soil_Moisture_Value >= Max_Values.Soil_Moisture){
    Low_Moisture_Flag = false;
  }
}

void Control_Light_System(const bool Light_State){
  if (Light_State == true){
    digitalWrite(LIGHT_PIN , HIGH);
  }
  else{
    digitalWrite(LIGHT_PIN , LOW);
  }
}

void Control_Heating_System(const bool Heat_State){
  if (Heat_State == true){
    digitalWrite(HEAT_PIN , HIGH);
  }
  else{
    digitalWrite(HEAT_PIN , LOW);
  }
}

void Control_Ventilation_System(const bool Vent_State, const bool Heat_State){
  if (Vent_State == true || Heat_State == true){
    digitalWrite(VENT_PIN , HIGH);
  }
  else{
    digitalWrite(VENT_PIN , LOW);
  }
}

void Determine_Pump_Operation(const bool Moisture_Low_Flag){
   if (Moisture_Low_Flag == false){  
    Pump_Status = false;
    Watering_In_Progress = false;
  }
  else {
    if (Watering_In_Progress == true && Pump_Active_State == true){
      if (Current_Time_Value - Watering_Start_Time < Watering_Duration){
        Pump_Status = true;
      }
      else{
        Pump_Status = false;
        Pump_Active_State = false;
        Watering_Stop_Time = Current_Time_Value;
      }
    }
    else if (Watering_In_Progress == true && Pump_Active_State == false){
      if (Current_Time_Value - Watering_Stop_Time < Watering_Pause){
          Pump_Status = false;
      }
      else{
          Watering_In_Progress = false;
      }
    }
    else if (Watering_In_Progress == false){
      Pump_Active_State = true;
      Watering_Start_Time = Current_Time_Value;
      Watering_In_Progress = true;
    }
  }
}

void Operate_Pump_System(const bool Pump_State){
    if (Pump_State == true){
    digitalWrite(PUMP_PIN , HIGH);
  }
  else{
    digitalWrite(PUMP_PIN , LOW);
  }
}

void Initialize_Thresholds(){
  Min_Values.Temperature = 29;
  Min_Values.Air_Humidity = 20;
  Min_Values.Soil_Moisture = 900;
  Min_Values.Light = 200;

  Max_Values.Temperature = 30;
  Max_Values.Air_Humidity = 40;
  Max_Values.Soil_Moisture = 10;
  Max_Values.Light = 20;
}

void Output_Serial_Data(){
  Serial.print("Температура: ");
  Serial.println(Temperature_Value);
  Serial.print("Влажность воздуха: ");
  Serial.println(Air_Humidity_Value);
  
  for (int i = 0; i < Light_Sensors_Count; i++) {
    Serial.print("Освещенность ");
    Serial.print(i + 1);
    Serial.print(": ");
    Serial.println(Lighting_Values[i]);
  }
  
  Serial.print("Влажность почвы: ");
  Serial.println(Soil_Moisture_Value);
  Serial.println();
}

void loop(){
  Air_Humidity_Value = dht.readHumidity();
  Temperature_Value = dht.readTemperature();
  Soil_Moisture_Value = analogRead(SOIL_MOISTURE_SENSOR_PIN);
  
  Read_Light_Sensors();

  Time_Cycle_Manager();
  Check_Light_Condition();
  Check_Temperature_Condition(Temperature_Value);
  Check_Air_Humidity_Condition(Air_Humidity_Value);
  Check_Soil_Moisture_Condition(Soil_Moisture_Value);

  Control_Light_System(Light_Status);
  Control_Heating_System(Heat_Status);
  Control_Ventilation_System(Vent_Status, Heat_Status);

  Determine_Pump_Operation(Low_Moisture_Flag);
  Operate_Pump_System(Pump_Status);

  if (Current_Time_Value - Last_Serial_Time >= Serial_Output_Interval) {
    Output_Serial_Data();
    Last_Serial_Time = Current_Time_Value;
  }
}