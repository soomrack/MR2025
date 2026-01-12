void temp_check(){
  // Проверка температуры
  if (high_temp) {
    is_cold = false;
  }
  else {
    is_cold = true;
  }
}

void soil_moisture_check(){
  // Проверка влажности почвы
  if (dry_soil_moisture && !is_soil_dry){
    is_soil_dry = true;
  }
  if (!dry_soil_moisture) {
    is_soil_dry = false;
    is_pause = false;
  }
}

void air_moisture_check(){
  // Проверка влажности воздуха
  if (!dry_air_moisture){
    is_air_wet = true;
  }
  if (dry_air_moisture){
    is_air_wet = false;
  }
}

void light_level_check(){
  // Проверка освещённости
  if (low_light_value) {
    is_dark = true;
  }
  if (!low_light_value){
    is_dark = false;
  }
}

void time_shedule_check(){
  
}
