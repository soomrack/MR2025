# ДОКУМЕНТАЦИЯ — CHAT MAX 10.0
Чат с дистанционным управлением роботом через UART

Платформа: Raspberry Pi 5 / Raspbian

Клиенты (протестировано на): Ubuntu 25.10, NixOS 26.05

Контроллер низкоуровневый: Arduino Mega 2560

---

## Содержание

- [1. Описание](#1-описание)
- [2. Схема системы и подключения](#2-схема-системы-и-подключения)
- [3. Архитектура и функции](#3-архитектура-и-функции)
- [4. Управление роботом](#4-управление-роботом)
- [5. Установка](#5-установка)
- [6. Прочие функции чата](#6-прочие-функции-чата)
- [7. Тестирование](#7-тестирование)

---

## 1. Описание

Chat MAX 10.0 — многопользовательский чат-сервер на C, работающий на Raspberry Pi 5 / Raspbian.
Принимает TCP-подключения на выбранном порту, также можно управлять гусеничным роботом через UART с Arduino Mega 2560.

---

## 2. Схема системы и подключения

### Общая схема

```
  ┌─────────────────────┐        TCP :5000          ┌────────────────────────┐
  │   Клиент            │  <─────────────────────>  │   Сервер               │
  │   Ubuntu / NixOS    │    чат и \команды         │   Raspberry Pi 5       │
  │                     │                           │   Raspbian  aarch64    │
  │   chat_client       │                           │   chat_server          │
  └─────────────────────┘                           └──────────┬─────────────┘
                                                               │
                                                    UART 115200│
                                                               │
                                                   ┌────────────────────────┐
                                                   │   Arduino Mega 2560    │
                                                   │   Serial1  pins 18/19  │
                                                   │    моторы + датчики    │
                                                   └────────────────────────┘
```

### Физическое подключение UART + питание Arduibo через Raspberry

```
  Raspberry Pi 5                        Arduino Mega 2560
  ┌──────────────────┐                  ┌──────────────────┐
  │  GPIO 14  (TX)   │ ───────────────> │  Pin 19  (RX1)   │
  │  GPIO 15  (RX)   │ <─────────────── │  Pin 18  (TX1)   │
  │  GND             │ ──────────────── │  GND             │
  │  5v              │ ──────────────── │  5v              │
  └──────────────────┘                  └──────────────────┘
```

### Пины моторов и датчиков тока (Arduino Mega 2560)

```
  ┌────────────────────┬───────┬───────┬───────┬───────┬───────────────┐
  │  Канал             │  PWM  │  INA  │  INB  │  EN   │  Датчики      │
  ├────────────────────┼───────┼───────┼───────┼───────┼───────────────┤
  │  Левый мотор       │   6   │   7   │  30   │  31   │  A0           │
  │  Правый мотор      │   5   │  32   │   4   │  33   │  A1           │
  └────────────────────┴───────┴───────┴───────┴───────┴───────────────┘

  Чтение тока: analogRead(A0/A1) → (raw × 5.0 / 1023) / 0.1 В/А → значение в А
  Педупреждение: > 3.0 А → WARNING в motor.log
```

---

## 3. Архитектура приложения

### Структура файлов

```
  message/
  ├── server.c              сервер: сеть, клиенты, команды, лог CPU/RAM
  ├── motor.c               UART-управление моторами, чтение тока
  ├── motor.h               заголовок для motor.c
  ├── client.c              клиент: I/O, WASD-режим, автопереподключение
  ├── types.h               структуры Client, ServerCommand
  ├── Makefile
  ├── server_stats.log      авто-лог CPU temp + RAM (каждые 10 с)
  ├── motor.log             лог команд и тока двигателей
  └── motor_arduino/
      └── motor_arduino.ino прошивка Arduino Mega 2560
```

### Какая функция за что отвечает

```
  server.c
  ├── main()                    инициализация, запуск loop, выключение
  ├── server_main_loop()        select()-цикл: новые подключения + сообщения
  │     └── uart_read_arduino() ← из motor.c, вызывается каждый цикл
  ├── accept_new_client()       регистрация клиента, запрос имени
  ├── process_client_message()  маршрутизация: команда или сообщение
  │     ├── process_command()   разбор \команды → вызов handler-а из таблицы
  │     └── process_regular_message()  замена эмодзи → broadcast()
  ├── broadcast()               рассылка всем кроме отправителя
  ├── log_write_entry()         запись CPU temp + RAM в server_stats.log
  └── cmd_...()                 обработчики команд (\help, \users, \OSinfo …)

  motor.c
  ├── motor_init()              открыть /dev/ttyAMA0, настроить 8N1 115200
  ├── motor_cleanup()           STOP + close(uart_fd)
  ├── uart_read_arduino()       неблокирующее чтение CURR:l,r → motor.log
  ├── motor_set_speed()         SPEED:N → uart_send()
  ├── cmd_drive()               войти в WASD-режим (маркер клиенту)
  ├── cmd_drive_key()           разобрать w/a/s/d/spc/q → uart_send()
  └── cmd_drive_speed()         установить / показать скорость

  client.c
  ├── chat_loop()               select() на sockfd + stdin
  ├── handle_server_message()   получить данные с сервера, детектировать маркеры
  │     ├── DRIVE_MODE_START    → drive_mode_enter() — raw terminal
  │     └── DRIVE_MODE_END      → drive_mode_exit()  — восстановить tty
  ├── handle_user_input()       читать stdin: в WASD → \drive_key, иначе текст
  └── try_reconnect()           попытка каждые 3с, через 60с выключение клиента

  motor_arduino.ino
  ├── setup()                   init Serial(115200) + Serial1(115200), пины, STOP
  ├── loop()                    serial_process() + send_current() раз в секунду
  ├── serial_process()          читать Serial1, накапливать команду до \n
  ├── parse_command()           FORWARD/BACK/LEFT/RIGHT/STOP/SPEED:N → set_motor()
  ├── set_motor()               управление INA/INB/PWM/EN одного мотора
  └── send_current()            analogRead → CURR:l,r\n → Serial1
```

### Поток данных: нажатие клавиши → движение

```
  Клиент                    Сервер (RPi)             Arduino
  ──────                    ────────────             ───────
  нажата W
      │
  handle_user_input()
  g_drive_mode == 1
      │
  send "\drive_key w"
      │ TCP
      ▼
                        process_command()
                        cmd_drive_key(…, "w")
                              │
                        uart_send("SPEED:70\n")
                        uart_send("FORWARD\n")
                              │ UART /dev/ttyAMA0
                              ▼
                                                    serial_process()
                                                    parse_command("FORWARD")
                                                    do_forward()
                                                    set_motor(6,7,30,31, +spd)
                                                    set_motor(5,32,4,33, +spd)
                                                        
```

### Обратный поток: ток → логи

```
  Arduino                   Сервер (RPi)
  ───────                   ────────────
  send_current()
  CURR:1.23,0.98\n
      │ UART Serial1
      ▼
                        uart_read_arduino()   ← в каждом цикле select()
                        парсит "CURR:l,r"
                        motor_log("INFO", "Current: left=1.23A right=0.98A")
                        если > 3.0 А:
                        motor_log("WARNING", "LEFT motor overcurrent: 3.45A")
```

---

## 4. Управление роботом

### Команды

```
  ┌──────────────────────────────┬───────────────────────────────────────────┐
  │  Команда                     │  Действие                                 │
  ├──────────────────────────────┼───────────────────────────────────────────┤
  │  \drive                      │  войти в WASD-режим (raw terminal)        │
  │  \drive_speed <0–100>        │  установить скорость (% от max PWM 255)   │
  │  \drive_key <w/a/s/d/spc/q>  │  отправить одну команду движения          │
  └──────────────────────────────┴───────────────────────────────────────────┘
```

### UART-команды (RPi → Arduino)

```
  ┌──────────────┬───────────────────────────────────────────────────────┐
  │  Команда     │  Действие на Arduino                                  │
  ├──────────────┼───────────────────────────────────────────────────────┤
  │  FORWARD     │  оба мотора вперёд: set_motor(+spd, +spd)             │
  │  BACK        │  оба мотора назад:  set_motor(-spd, -spd)             │
  │  LEFT        │  левый назад, правый вперёд: (-spd, +spd)             │
  │  RIGHT       │  левый вперёд, правый назад: (+spd, -spd)             │
  │  STOP        │  PWM = 0 на оба: set_motor(0, 0)                      │
  │  SPEED:N     │  motor_speed = N; spd = map(N, 0,100, 0,255)          │
  └──────────────┴───────────────────────────────────────────────────────┘
  Перед каждой командой движения сервер автоматически отправляет SPEED:N.
```

### WASD-режим на клиенте

```bash
\drive_speed 70        # установить скорость
\drive                 # войти в режим — клиент переходит в raw tty

w  — FORWARD    s  — BACK
a  — LEFT       d  — RIGHT
пробел — STOP   q  — выйти (STOP + восстановить tty)
```

### Лог мотора (motor.log)

```
[2026-03-04 12:00:00] SERVER: INIT — motors stopped at startup
[2026-03-04 12:00:05] Artem: ENTERED DRIVE MODE
[2026-03-04 12:00:06] Artem: KEY 'w' -> FORWARD speed=70%
[2026-03-04 12:00:07] INFO: Current: left=1.23A right=1.18A
[2026-03-04 12:00:08] Artem: KEY ' ' -> STOP speed=70%
[2026-03-04 12:00:09] WARNING: LEFT motor overcurrent: 3.45A
[2026-03-04 12:00:10] Artem: EXIT DRIVE MODE — motors stopped
```

---

## 5. Установка

### Сервер (Raspberry Pi 5)

```bash
sudo apt install -y build-essential
cd message/
make

# Настройка UART (один раз):
sudo raspi-config
#  Interface Options → Serial Port
#    login shell over serial → NO
#    serial port hardware    → YES
sudo usermod -aG dialout $USER   # перелогиниться
echo "enable_uart=1"
sudo tee -a /boot/firmware/config.txt

./chat_server
```

Если `/dev/ttyAMA0` недоступен — сервер запустится без мотора, чат будет работать.

### Клиент (Ubuntu / NixOS / другой линукс)

```bash
cd message/ 
make
./chat_client   # подключается к 127.0.0.1:5000
```

### Arduino

Открыть `motor_arduino/motor_arduino.ino` в Arduino IDE, залить на Mega 2560.
Serial Monitor: 115200 бод (если требуется отладка, подклчить по USB).
Рабочий канал — Serial1 (пины 18/19).

---

## 6. Прочие функции чата

```
  \help        список команд
  \users       активные пользователи
  \OSinfo      OS / ядро / RAM / CPU temp / uptime
  \log_last    последние 15 записей server_stats.log
  \log_all     весь server_stats.log
  \log_clear   очистить server_stats.log
  \disconnect  выйти из чата

  Эмодзи-шорткаты: :) :D :( :love: :cool: :heart: :fire:
  До 10 клиентов. Автопереподключение клиента: каждые 3 с, до 60 с.
  server_stats.log: CPU temp + свободная RAM — каждые 10 с.
  WARNING в лог при temp > 30 °C (рекомендуется поднять до 60–70 °C).
```

---

## 7. Тестирование

### UART без Arduino (loopback)

```bash
# Замкнуть GPIO 14 (TX) → GPIO 15 (RX) физически, затем:
echo -n "CURR:1.23,0.98" | socat - /dev/ttyAMA0,raw,echo=0
# В motor.log должна появиться строка INFO: Current: left=1.23A right=0.98A
```

### Команды мотора

```bash
# В чате:
\drive_speed 50
\drive_key w    # motor.log: KEY 'w' -> FORWARD speed=50%
\drive_key " "  # motor.log: KEY ' ' -> STOP speed=50%
```

### Лимит клиентов

```bash
for i in $(seq 1 11); do nc 127.0.0.1 5000 & done
# 11-й получит: Server is full. Try again later.
```

### Автопереподключение

```
1. Запустить сервер и клиент.
2. Убить сервер (Ctrl+C) — клиент начнёт попытки каждые 3 с.
3. Перезапустить сервер в течение 60 с — клиент переподключится.
```

### Overcurrent

```
Симулировать: закоротить мотор или подать на A0/A1 > 3.0 В.
Ожидание в motor.log:
[2026-03-04 12:01:05] WARNING: LEFT motor overcurrent: 3.45A
```