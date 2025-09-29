#define LED 13


unsigned long TIME = 0;
boolean STATE = 0;  


void setup()
{
    Serial.begin(9600);
    pinMode(LED, OUTPUT);
}


void loop()
{
    if (millis() - TIME > 500) {
        TIME = millis();
        STATE = !STATE;
        Serial.print("State changed. TIME = ");
        Serial.println(TIME);
    }
    digitalWrite(LED, STATE);
}
