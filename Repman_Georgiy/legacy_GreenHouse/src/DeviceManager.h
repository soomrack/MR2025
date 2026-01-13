#ifndef DEVICE_MANAGER_H
#define DEVICE_MANAGER_H

#include <Arduino.h>
#include <Wire.h>
class DeviceManager
{
private:
    // Пины устройств
    uint8_t lightPin;
    uint8_t fanPin;
    uint8_t pumpPin;

    // Состояния устройств
    bool lightState;
    bool fanState;
    bool pumpState;

    // Управление насосом
    uint32_t pumpStartTime;
    uint32_t pumpDuration;
    bool pumpAutoStop;
    uint16_t pumpFlowRate; // мл в минуту

    // Приватные методы
    void updatePump();
    uint32_t calculatePumpTime(uint16_t ml) const;

public:
    // Конструктор с настройкой пинов
    DeviceManager(uint8_t lightPin, uint8_t fanPin, uint8_t pumpPin);

    // Инициализация
    void init() const;

    // Управление устройствами
    void setLight(bool state);
    void setFan(bool state);
    void startPump(uint16_t ml); // Полив заданного объема в мл
    void stopPump();

    // Статус
    bool isLightOn() const { return lightState; }
    bool isFanOn() const { return fanState; }
    bool isPumpOn() const { return pumpState; }

    // Обновление состояния (вызывать в loop для управления насосом)
    void update();

    // Настройка производительности насоса (мл/мин)
    void setPumpFlowRate(uint16_t mlPerMin) { pumpFlowRate = mlPerMin; }
};

#endif