# ДОКУМЕНТАЦИЯ — CHAT SENSOR SERVER 1.0

Многопользовательский чат с мониторингом системы и датчиками Arduino

Платформа сервера: Raspberry Pi / Linux

Клиенты (протестировано): Windows

Контроллер датчиков: Arduino + DHT11 + LDR

---

## Содержание

- [1. Описание](#1-описание)
- [2. Схема системы](#2-схема-системы)
- [3. Архитектура программы](#3-архитектура-программы)
- [4. Протокол сообщений](#4-протокол-сообщений)
- [5. Работа с логами](#5-работа-с-логами)
- [6. Мониторинг системы](#6-мониторинг-системы)
- [7. Датчики Arduino](#7-датчики-arduino)
- [8. Команды сервера](#8-команды-сервера)
- [9. Команды клиента](#9-команды-клиента)
- [10. Установка и запуск](#10-установка-и-запуск)

---

# 1. Описание

Chat Sensor Server — это многопользовательский TCP-чат, написанный на C++,  
который работает на Linux-сервере (например Raspberry Pi).

Основные возможности:

- текстовый чат между клиентами
- цветные сообщения пользователей
- система логирования
- мониторинг системы (CPU, RAM, uptime)
- интеграция с Arduino
- получение логов клиентом
- система предупреждений (WARNING)

Сервер способен одновременно:

- обслуживать несколько клиентов
- записывать события в лог
- мониторить систему
- получать данные с датчиков Arduino

---

# 2. Схема системы

## Общая архитектура
```
       TCP соединение
──────────────────────────┐
│ CLIENT                  │
│ (Windows)               │
│                         │
│ chat_client.exe         │
└─────────────┬───────────┘
│
│ TCP :54000
▼
┌─────────────────────────┐
│ SERVER                  │
│ Raspberry Pi            │
│                         │
│ Server.cpp              │
│                         │
│ - чат                   │
│ - логирование           │
│ - мониторинг            │
│ - датчики               │
└─────────────┬───────────┘
│
│ UART / USB
▼
┌─────────────────────────┐
│ ARDUINO                 │
│                         │
│ DHT11 + LDR             │
│                         │
│ TEMP / HUM / LIGHT      │
└─────────────────────────┘
```
---

# 3. Архитектура программы

## Файлы проекта
```
project/
│
├── Server.cpp
├── Client.cpp
├── arduino.ino
│
├── server.log
├── board.log
└── sensor.log
```
---

## Основные модули сервера
```
Server.cpp
│
├── network
│ ├── acceptLoop()
│ ├── handleClient()
│ ├── broadcast()
│
├── logging
│ ├── logEvent()
│ ├── readLogFile()
│
├── monitoring
│ ├── monitoringLoop()
│ ├── getCPUtemp()
│ ├── getRAMusage()
│
├── sensors
│ ├── sensorLoop()
│ ├── parseSensorLine()
│ └── processSensorValue()
│
└── commands
├── commandHandler()
├── /shutdown
├── /status
├── /time
└── /cpu
```
---

# 4. Протокол сообщений

Обмен сообщениями осуществляется через структуру:
```
struct MessageHeader
{
uint32_t type;
uint32_t size;
}
```
Типы сообщений:
1 Text
2 Connect
3 Disconnect
4 LogRequest
5 LogResponse
---

## Сообщение клиента
```
[username time] message
```
Пример:
```
[Alex 14:20:01] Hello
```
---

## Сообщение сервера
```
[color][username] message
```
Цвет реализован через ANSI escape-коды.

---

# 5. Работа с логами

Сервер создаёт три файла логов.

## server.log

Основные события:
- Client connected
- Client disconnected
- Messages
- Server start/stop

Пример:
```
[2026-03-16 14:22:10] Client 192.168.1.12 joined as 'Alex'
```

---

## board.log

Мониторинг системы:
- CPU
- RAM
- Uptime
Пример:
```
[2026-03-16 14:22:10] MONITOR | CPU: 45 C | RAM: 30% | Uptime: 2h 15m
```
---

## sensor.log

Данные с датчиков:
- TEMP
- HUM
- LIGHT
Пример:
```
[2026-03-16 14:22:10] SENSOR | TEMP:25 HUM:50 LIGHT:300
```
---

# 6. Мониторинг системы

Каждые 10 секунд сервер проверяет:
- CPU temperature
- RAM usage
- System uptime
Пороговые значения:
```
CPU_TEMP_LIMIT = 70°C
RAM_LIMIT_PERCENT = 20% //для примера
```
Если значение превышено:
```
[WARNING] CPU TEMP HIGH
```
Сообщение:

- выводится в консоль
- записывается в лог
- отправляется всем клиентам

---

# 7. Датчики Arduino

Используются датчики:
DHT11
LDR (фоторезистор)
Параметры:
TEMP
HUM
LIGHT

---

## Формат данных

Arduino отправляет строку:
```
TEMP:25 HUM:50 LIGHT:300
```
Сервер разбирает её функцией:
```
parseSensorLine()
```
---

## Проверка лимитов

TEMP > 40
HUM > 100
LIGHT > 1200

Если лимит превышен:
```
[WARNING] SENSOR | TEMP LIMIT (45)
```
---

# 8. Команды сервера

Команды вводятся в консоль сервера.
```
/shutdown
/status
/time
/uptime
/ram
/cpu
/help
```
---

## Примеры
```
/status
Clients: 3
```

```
/cpu
CPU temp: 42 C
```
---

# 9. Команды клиента

Клиентские команды:
```
/connect <ip> <port>
/quit
/exit
/help
/spam
/logs
```
---

## Запрос логов
```
/logs server
/logs board
/logs sensor
/logs all
/logs warnings
/logs last <minutes>
```
--

## Пример
```
/logs last 10
```
вернёт логи за последние 10 минут.

---

# 10. Запуск

## Сервер (Linux)

Компиляция:
```
g++ Server.cpp -std=c++17 -pthread -o server
```
Запуск:
```
./server
```
---

## Клиент (Windows)

Компиляция:
```
g++ Client.cpp -lws2_32 -o client.exe
```
Запуск:
```
client.exe
```

---

## Arduino

Прошивка отправляет данные:
```
TEMP:25 HUM:50 LIGHT:300
```
Сервер читает их через /dev/ttyUSB0

---

# Потоки сервера

Сервер использует несколько потоков:

- Main thread — accept клиентов
- Client thread — обработка клиента
- Command thread — команды сервера
- Monitoring thread — мониторинг системы
- Sensor thread — данные Arduino

  ---

# Ограничения
Максимальный размер сообщения: 1 MB
Количество клиентов ограничено ресурсами системы

---

# Итог

Chat Sensor Server объединяет:

- TCP чат
- мониторинг системы
- сенсоры
- логирование
в одном серверном приложении.
