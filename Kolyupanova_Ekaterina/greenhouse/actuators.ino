void fan_actuator(){
  digitalWrite(PIN_D_FAN, (is_cold || is_air_wet));
}


void temp_actuator(){
  digitalWrite(PIN_D_TEMP_ACTUATOR, is_cold);
}


void light_actuator(){
  digitalWrite(PIN_D_LED, is_dark);
}


void watering_actuator(){
  static bool is_first_time_run = true;
  if((is_soil_dry && (millis() - time_start_watering < 1000 || is_first_time_run)) || (is_pause && is_soil_dry && (millis() - time_pause_watering >= 1000))){
    digitalWrite(PIN_D_WATERING, HIGH);
    if (is_pause || is_first_time_run){
      time_start_watering = millis();
    }
    is_pause = false;
    is_first_time_run = false;
  }
  else {
    digitalWrite(PIN_D_WATERING, LOW);
    if (is_soil_dry && !is_pause){
      is_pause = true;
      time_pause_watering = millis();
    }
    if (is_soil_dry) is_first_time_run = true;
  }
}
