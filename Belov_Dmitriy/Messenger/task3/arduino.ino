#include <DHT.h>

#define DHTPIN 2
#define DHTTYPE DHT11

#define LIGHT_PIN A0

DHT dht(DHTPIN, DHTTYPE);

const int SEND_INTERVAL = 5000;

void setup()
{
    Serial.begin(9600);
    dht.begin();

    delay(2000);
}

void loop()
{
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();

    int lightRaw = analogRead(LIGHT_PIN);

    // переводим в условные люксы (0-1500 примерно)
    int light = map(lightRaw, 0, 1023, 0, 1500);

    if (isnan(temperature) || isnan(humidity))
    {
        Serial.println("ERROR");
        delay(SEND_INTERVAL);
        return;
    }

    Serial.print("TEMP:");
    Serial.print(temperature,1);

    Serial.print(" HUM:");
    Serial.print(humidity,1);

    Serial.print(" LIGHT:");
    Serial.println(light);

    delay(SEND_INTERVAL);
}
