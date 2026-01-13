void startup(){
  pinMode(PIN_D_TEMP_ACTUATOR, OUTPUT);
  pinMode(PIN_D_FAN, OUTPUT);
  pinMode(PIN_D_LED, OUTPUT);
  pinMode(PIN_D_WATERING, OUTPUT);

  Serial.begin(115200);
}
