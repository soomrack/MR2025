# Raspberry Pi Hardware Monitor

Простая клиент-серверная система для мониторинга Raspberry Pi по TCP.  
Сервер запускается на Pi, принимает подключения от Windows-клиентов, пишет лог и транслирует данные с Arduino через UART.

---

## Архитектура

```
Windows Client  ──TCP──►  Raspberry Pi Server  ──UART──►  Arduino
                                    │
                              server.log (файл)
```

- **Сервер** (Linux/Raspberry Pi) — принимает несколько клиентов, мониторит CPU/RAM, читает UART с Arduino, рассылает предупреждения.
- **Клиент** (Windows) — консольный чат + команды для запроса статуса и логов.

---

## Сборка

### Сервер (Raspberry Pi)

```bash
g++ -std=c++17 -O2 -pthread server.cpp -o server
./server
```

### Клиент (Windows)

```cmd
cl /W3 /EHsc client.cpp /link Ws2_32.lib
client.exe
```

или через MinGW:

```bash
g++ -std=c++17 -O2 client.cpp -o client -lws2_32
```

---

## Запуск

1. Запустить сервер на Pi:
   ```bash
   ./server
   ```
2. Запустить клиент на Windows, ввести имя пользователя.
3. Подключиться командой:
   ```
   /connect 192.168.1.100 54002
   ```

---

## Команды клиента

| Команда | Псевдоним | Описание |
|---|---|---|
| `/connect <ip> <port>` | `/c` | Подключиться к серверу |
| `/disconnect` | `/d` | Отключиться |
| `/temp` | `/t` | Текущая температура CPU |
| `/status` | `/s` | CPU / RAM / Uptime / список клиентов |
| `/test` | — | Получить 10 тестовых сообщений |
| `/logs all` | `/l a` | Весь буфер лога сервера |
| `/logs warnings` | `/l w` | Только WARNING-записи |
| `/logs last <N>` | `/l l N` | Лог за последние N минут |
| `/sensor_all` | — | Все данные с Arduino (UART) |
| `/sensor_last <N>` | — | Данные с Arduino за N минут |
| `/help` | `/h`, `/?` | Справка |
| `/exit` | `/q`, `/quit` | Выйти |

Любой текст без `/` отправляется как сообщение в чат.

---

## Протокол

Бинарный фрейм: 8-байтовый заголовок + payload.

```
[ type: uint32_t ][ size: uint32_t ][ payload: bytes... ]
```

| type | Назначение |
|---|---|
| 1 — Text | Обычное сообщение чата |
| 2 — Connect | Handshake (username в payload) |
| 3 — Disconnect | Уведомление об отключении |
| 4 — LogRequest | Запрос данных от клиента |
| 5 — LogResponse | Ответ сервера на LogRequest |
| 6 — Warning | Предупреждение сервера (CPU/RAM) |

---

## Мониторинг и предупреждения

Каждые **10 секунд** сервер проверяет:

| Параметр | Источник | Порог по умолчанию |
|---|---|---|
| Температура CPU | `/sys/class/thermal/thermal_zone0/temp` | > 70 °C |
| Использование RAM | `/proc/meminfo` | > 80 % |

При превышении порога всем подключённым клиентам отправляется `Warning`-фрейм и запись попадает в лог.

---

## UART / Arduino

Сервер открывает `/dev/ttyUSB0` на скорости **9600 бод**.  
Строки, которые присылает Arduino (разделитель `\n`), накапливаются в кольцевом буфере (до 10 000 записей) и доступны через `/sensor_all` и `/sensor_last`.

Если устройство недоступно при старте — сервер продолжает работу без UART.

---

## Конфигурация

Константы в `server.cpp`:

```cpp
constexpr int    SERVER_PORT              = 54002;
constexpr int    MONITOR_INTERVAL_SEC     = 10;
constexpr double CPU_TEMP_WARN_THRESHOLD  = 70.0;  // °C
constexpr int    RAM_USAGE_WARN_THRESHOLD = 80;     // %
constexpr const char* LOG_FILE_PATH       = "server.log";
constexpr int    TEST_MESSAGE_COUNT       = 10;
constexpr int    TEST_MESSAGE_INTERVAL_MS = 500;
```

---

## Структура файлов

```
├── server.cpp      # Сервер (Raspberry Pi / Linux)
├── client.cpp      # Клиент (Windows)
├── server.log      # Лог (создаётся автоматически рядом с бинарником)
└── README.md
```
