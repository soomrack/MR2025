#include "DeviceManager.h"

DeviceManager::DeviceManager(uint8_t lightPin, uint8_t fanPin, uint8_t pumpPin) : lightPin(lightPin), fanPin(fanPin), pumpPin(pumpPin)
{
    lightState = false;
    fanState = false;
    pumpState = false;

    pumpFlowRate = 100; // 100 мл/мин
    pumpAutoStop = false;
    pumpStartTime = 0;
    pumpDuration = 0;

}

void DeviceManager::init() const
{
    pinMode(lightPin, OUTPUT);
    pinMode(fanPin, OUTPUT);
    pinMode(pumpPin, OUTPUT);

    digitalWrite(lightPin, LOW);
    digitalWrite(fanPin, LOW);
    digitalWrite(pumpPin, LOW);
}

void DeviceManager::setLight(bool state)
{
    lightState = state;
    digitalWrite(lightPin, state ? HIGH : LOW);
}

void DeviceManager::setFan(bool state)
{
    fanState = state;
    digitalWrite(fanPin, state ? HIGH : LOW);
}

void DeviceManager::startPump(uint16_t ml)
{
    if (ml == 0)
    {
        return;
    }
    pumpDuration = calculatePumpTime(ml);
    pumpStartTime = millis();
    pumpAutoStop = true;
    pumpState = true;
    digitalWrite(pumpPin, HIGH);
}

void DeviceManager::stopPump()
{
    pumpState = false;
    digitalWrite(pumpPin, LOW);
    pumpAutoStop = false;
}

void DeviceManager::update()
{
    updatePump();
}

void DeviceManager::updatePump()
{
    if (!pumpState || !pumpAutoStop)
        return;

    uint32_t currentTime = millis();

    if (currentTime < pumpStartTime)
    {
        pumpStartTime = currentTime;
        return;
    }

    if (currentTime - pumpStartTime >= pumpDuration)
    {
        stopPump();
    }
}

uint32_t DeviceManager::calculatePumpTime(uint16_t ml) const
{
    return (static_cast<uint32_t>(ml) * 60000UL) / pumpFlowRate;
}