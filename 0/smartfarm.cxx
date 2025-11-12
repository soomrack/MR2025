

class Fan {
public:
    bool on_heater;
    bool on_timetable;
    bool on_hot;
    bool on_humidity;
public:
    void power();
};


class Heater {
};


class Thermometer {
public:
    int temperature;  // Celsius
};


class ThermometerMerge : Thermometer {
public:
    void get_data_merge(Thermometer therm01, Thermometer therm02) {};
};


void Fan::power()
{
    if( on_heater
        || on_timetable
        || on_hot
        || on_humidity ) {
        // digitalWrite(...);
    }
    else {
        // digitalWrite(...);
    }
}



void control_temperature(Thermometer& thermometer, Fan& fan, Heater& heater)
{
    if (thermometer.temperature > 40) {
        fan.on_hot = true;
    }
    if (thermometer.temperature < 20) {
        fan.on_hot = false;
        fan.on_heater = true;
        // heater.on_temperature = true;
    }
}


Fan fan;

void loop()
{
    // thermometer.get_data();
    // light.get_data();
    // air_humidity.get_data();
    // dirt_humidity.get_data();
    // thermometer_merge.get_data_merge(thermometer_bottom, thermometer_top);

    // control_light();
    // control_air_humidity();
    // ventilation_by_time();
    control_temperature(thermometer_merge, fan, heater);
    // control_dirt_humidity();

    // fan01.power();
    // fan_array.power(fan01, fan02)
    // heater.power();
    // light.power();
    // pump.power();
    
    sleep(10);
}


int main()
{
    while(1) {
        loop();
    }
    return 0;
}
