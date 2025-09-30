#define SENSOR A0


unsigned long int TIME;
unsigned int DATA = 0;  


void setup()
{
    Serial.begin(9600);
    pinMode(SENSOR, INPUT);
}


void loop()
{
    if (millis() - TIME >= 50) {
        TIME = millis();
        DATA = analogRead(SENSOR);
        Serial.print("TIME = ");
        Serial.print(TIME);
        Serial.print(" SENSOR = ");
        Serial.println(DATA);
    }
}
