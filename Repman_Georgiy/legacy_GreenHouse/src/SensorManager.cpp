#include "SensorManager.h"

SensorManager::SensorManager() : hc(TRIG_PIN, ECHO_PIN) , ens160(ENS160_I2CADDR_1), aht20(AHTXX_ADDRESS_X38, AHT2x_SENSOR)
{
  float light_lux = 0;
  float air_temp = 0;
  float air_qual = 0;
  float air_hum = 0;
  float water_dist_cm = 0;
  float water_volume_ml = 0;
  uint16_t soil_moist_1 = 0;
  uint16_t soil_moist_2 = 0;
  uint8_t hour = 0;
  uint8_t minute = 0;
  uint8_t second = 0;
  uint8_t day = 1;
  uint8_t month = 1;
  uint16_t year = 2026;
  //TODO: Add error handler, which process the following data
  /*
  bool light_sensor_ok = false;
  bool air_temp_sensor_ok = false;
  bool air_qual_sensor_ok = false;
  bool soil_sensor_1_ok = false;
  bool soil_sensor_2_ok = false;
  bool rtc_ok = false;
  bool water_sensor_ok = false;
  */
}

bool SensorManager::init()
{

  Wire.begin();

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);
  pinMode(SOIL_1_PIN, INPUT);
  pinMode(SOIL_2_PIN, INPUT);

  bool all_ok = true;

  if (!init_air_qual_sensor())
  {
    all_ok = false;
  }

  delay(10);

  if (!init_air_temp_hum_sensor())
  {
    all_ok = false;
  }

  delay(10);

  if (!init_light_sensor())
  {
    all_ok = false;
  }

  delay(10);

  readings.soil_sensor_1_ok = check_soil_sensor(SOIL_1_PIN);
  readings.soil_sensor_2_ok = check_soil_sensor(SOIL_2_PIN);

  if (!readings.soil_sensor_1_ok)
  {
    all_ok = false;
  }

  if (!readings.soil_sensor_2_ok)
  {
    all_ok = false;
  }

  delay(10);

  if (!init_rtc())
  {
    all_ok = false;
  }
  return all_ok;
}

void SensorManager::update_all()
{

  if (readings.light_sensor_ok)
  {
    readings.light_lux = read_light_sensor();
  }

  if (readings.air_temp_sensor_ok)
  {
    readings.air_temp = read_air_temp_sensor();
    readings.air_hum = read_air_hum_sensor();
  }

  if (readings.air_qual_sensor_ok)
  {
    readings.air_qual = read_air_quality_sensor();
  }

  if (readings.soil_sensor_1_ok)
  {
    readings.soil_moist_1 = read_soil_sensor(SOIL_1_PIN);
  }

  if (readings.soil_sensor_2_ok)
  {
    readings.soil_moist_2 = read_soil_sensor(SOIL_2_PIN);
  }

  if (readings.water_sensor_ok)
  {
    readings.water_dist_cm = read_water_distance_sensor();
    readings.water_volume_ml = calculate_water_volume(readings.water_dist_cm);
  }

  if (readings.rtc_ok)
  {
    read_rtc_time();
  }
}

bool SensorManager::init_light_sensor()
{
  if (!veml.begin())
  {
    Serial.println("VEML7700 FAIL");
    return false;
  }
  Serial.println("VEML7700 OK");
  veml.setLowThreshold(LIGHT_LOW_THRESHOLD);
  veml.setHighThreshold(LIGHT_HIGH_THRESHOLD);
  veml.interruptEnable(false);
  readings.light_sensor_ok = true;
  return true;
}

float SensorManager::read_light_sensor()
{
  Serial.print("Light lux: ");
  Serial.println(veml.readLux());
  return veml.readLux();
}

bool SensorManager::init_air_temp_hum_sensor()
{
  if (aht20.getStatus() == AHTXX_NO_ERROR && (aht20.readHumidity() < 110) && (aht20.readTemperature() < 70) )
  {
      Serial.println("AHT20 OK");
      readings.air_temp_sensor_ok = true;
      return true;
  }
  Serial.println("AHT20 FAIL");
  return false;
}

float SensorManager::read_air_temp_sensor()
{
  Serial.print("Air temperature: ");
  Serial.println(aht20.readTemperature());
  return aht20.readTemperature();
}

float SensorManager::read_air_hum_sensor()
{
  Serial.print("Air humidity: ");
  Serial.println(aht20.readHumidity());
  return aht20.readHumidity();
}

bool SensorManager::init_air_qual_sensor()
{
  ens160.begin();
  if (!ens160.available())
  {
    Serial.println("ens160 FAIL");
    return false;
  }
  Serial.println("ens160 OK");
  readings.air_qual_sensor_ok = true;
  return true;
}

float SensorManager::read_air_quality_sensor()
{
  //ens160.getCO2();
  Serial.print("AQI: ");
  Serial.println(ens160.getAQI());
  return ens160.getAQI();
}


bool SensorManager::check_soil_sensor(uint8_t pin)
{
  int value = analogRead(pin);
  //TODO: Add some more robust logic to test sensor (this logic if not working)
  if (value < 320) {
    Serial.print("Soil sensor on pin ");
    Serial.print(pin);
    Serial.println(" FAIL");
    return false;
  } else {
    Serial.print("Soil sensor on pin ");
    Serial.print(pin);
    Serial.println(" OK");
    return true;
  }
}

uint8_t SensorManager::read_soil_sensor(uint8_t pin)
{
  int raw = analogRead(pin);
  Serial.print("Soil sensor on pin ");
  Serial.print(pin);
  Serial.print(" raw value: ");
  Serial.println(raw);
  return convert_soil_reading(raw);
}

uint8_t SensorManager::convert_soil_reading(int raw_value)
{
  int percentage = map(raw_value, SOIL_DRY_VALUE, SOIL_WET_VALUE, 0, 100);
  return constrain(percentage, 0, 100);
}


bool SensorManager::init_rtc()
{
  if (!DS1307RTC::read(tm))
  {
      Serial.println("RTC FAIL");
      return false;
  }
  Serial.println("RTC OK");
  readings.rtc_ok = true;
  return true;
}

void SensorManager::read_rtc_time()
{
  if (DS1307RTC::read(tm))
  {
    Serial.print(tm.Hour);
    Serial.print(":");
    Serial.print(tm.Minute);
    Serial.print(":");
    Serial.println(tm.Second);
    readings.second = tm.Second;
    readings.minute = tm.Minute;
    readings.hour = tm.Hour;
    readings.day = tm.Day;
    readings.month = tm.Month;
    readings.year = tmYearToCalendar(tm.Year);
  }
  else
  {
    readings.rtc_ok = false;
  }
}



float SensorManager::calculate_water_volume(float distance_cm)
{

  float water_height_cm = TANK_HEIGHT_CM - distance_cm;

  if (water_height_cm < 0)
  {
    water_height_cm = 0;
  }
  if (water_height_cm > TANK_HEIGHT_CM)
  {
    water_height_cm = TANK_HEIGHT_CM;
  }
  float radius_cm = TANK_DIAMETER_CM / 2.0;
  float volume_cm3 = 3.14 * radius_cm * radius_cm * water_height_cm;

  return volume_cm3;
}

