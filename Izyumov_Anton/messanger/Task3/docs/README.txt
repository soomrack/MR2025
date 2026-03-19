Документация к мессенджеру

Чат между двумя пользователями с возможностью запроса данных с сервера, фоновым запросом данных от сервера, а также передачей показаний осей джойстика по uart от arduino uno к raspberry pi

Платформа: Raspberry Pi 3 model a+ / Raspbian
Клиенты (протестировано на): Ubuntu 22.04
Контроллер низкого уровня: Arduino Uno

Содержание:
1. Описание
2. Схема системы и подключения
3. Архитектура
4. UML-диаграммы
5. Запуск сервера
6. Запуск клиента
7. Прочие функции

1. Опсиание
Данный мессенджер — это чат-сервер на C, расчитанный на двух пользователей, работающий на Raspberry Pi 3 model a+ / Raspbian. Принимает TCP-подключения на портах 7000 и 7021, также можно через UART получать данные с осей джойстика с arduino UNO.

2. Схема системы и подключения
Общая схема
  ┌─────────────────────┐        TCP :7000          ┌─────────────────────────────────┐        TCP :7021          ┌─────────────────────┐
  │   Клиент            │  <─────────────────────>  │   Сервер                        │  <─────────────────────>  │   Клиент            │
  │   Ubuntu 22.04      │    чат и \команды         │   Raspberry Pi 3 model a+       │    чат и \команды         │   Ubuntu 22.04      │
  │                     │                           │   Raspbian  aarch64             │                           │                     │
  │   client.cpp        │                           │   server.cpp                    │                           │   client.cpp        │
  └─────────────────────┘                           └──────────┬──────────────────────┘                           └─────────────────────┘
                                                               │
                                                    UART 9600  │
                                                               │
                                                   ┌────────────────────────┐
                                                   │   Arduino UNO	    │
                                                   │   Serial  pins 0, 1    │
                                                   │   Оси джойстика        │
                                                   └────────────────────────┘

Физическое подключение UART

  Raspberry Pi 3 model a+                Arduino UNO
  ┌──────────────────┐                  ┌──────────────────┐
  │  GPIO 14  (TX)   │ ───────────────> │  Pin 0  (RX)     │
  │  GPIO 15  (RX)   │ <─────────────── │  Pin 1  (TX)     │
  │  GND             │ ──────────────── │  GND             │
  └──────────────────┘                  └──────────────────┘
  Питание raspberry pi 3 model a+ и arduino UNO осуществляется отдельно от USB портов ноутбука
  
  Пины осей джойстика
  
  ┌──────────┬───────┐
  │  Ось     │  Пин  │
  ├──────────┼───────┤
  │  X       │  A0   │
  │  Y       │  A1   │
  └──────────┴───────┘
  
  Предупреждение: ось X > 700 или < 10
  		  ось Y > 500 или < 20 
 
 3. Архитектура
 
 Структура файлов
 
 Task3/
  ├── server.cpp            			   сервер: сеть, клиенты, отправка ответа на серверные команды
  ├── client.cpp            			   клиент: I/O, проверка команд, лог метрик сервера
  ├── metrics_client_name_YYYYMMDD_HHMMSS.log      авто-лог метрик сервера (каждые 30 с)
  ├── client_log_client_name_YYYYMMDD_HHMMSS.log   лог терминала клиента
  └── joy_test/
      └── joy_test.ino		прошивка Arduino UNO
      

Какие классы и функции за что отвечают

server.cpp
├── class Client                       представление подключённого клиента
│     ├── getFd() / getAddress() / getPort()
│     ├── send()                         отправка данных
│     └── receive()                       чтение (с детекцией DISCONNECT/ERROR)
│
├── class TCPServer                     обёртка над серверным сокетом
│     ├── start()                         socket + bind + listen
│     └── accept()                         принятие нового клиента (возвращает unique_ptr<Client>)
│
├── class ClientManager                  хранение клиентов по портам
│     ├── addClient() / removeClient()     добавление / удаление клиента
│     ├── findClient() / findClientPort()  поиск по дескриптору
│     └── getClientsOnPort()               список клиентов на заданном порту
│
├── class MessageForwarder                пересылка сообщений между портами A и B
│     └── forwardMessage()                  отправитель → получатели на другом порту
│
├── class SystemInfo                      статические методы получения информации о системе
│     ├── getTime()                         текущее время
│     ├── getCpuLoad()                       load average из /proc/loadavg
│     ├── getCpuTemp()                       температура CPU (для Raspberry Pi)
│     ├── getRamInfo()                        использование RAM через sysinfo
│     └── getRomInfo()                        использование SD-карты (statvfs)
│
├── class SerialPort                       чтение данных с последовательного порта (Arduino)
│     ├── open()                             открытие порта, настройка termios
│     ├── processInput()                      неблокирующее чтение, накопление строк
│     ├── parseLine()                          разбор строки вида "x:123 y:456"
│     └── getJoyX() / getJoyY()                последние значения джойстика
│
├── class PollLoop                          цикл poll() для всех дескрипторов
│     ├── addServer() / addClient() / removeFd()  управление списком дескрипторов
│     ├── run()                                 основной цикл обработки событий
│     ├── handleInput()                           диспетчеризация по типу дескриптора
│     ├── handleNewConnection()                    принятие нового клиента
│     ├── handleClientMessage()                      чтение сообщения от клиента
│     ├── handleServerCommand()                       выполнение команд /server (time, cpu, ...)
│     └── handleError()                               обработка ошибок / отключений
│
└── class Application                     инициализация и запуск сервера
      ├── initialize()                      создание двух TCPServer (порты 7000, 7021), открытие Serial
      ├── run()                              запуск PollLoop
      └── shutdown()                         остановка цикла

client.cpp
├── namespace ClientThresholds       пороговые значения для предупреждений (CPU, RAM, ROM, джойстик)
│
├── class Socket                     обёртка над BSD-сокетом (RAII)
│     ├── create()                   создание сокета
│     ├── connect()                  подключение к серверу (с TCP_NODELAY)
│     ├── send() / receive()          отправка / получение данных
│     └── close() / деструктор        закрытие сокета
│
├── class Message                    структура сообщения (отправитель, текст, время)
│     ├── format()                    "[отправитель]: текст"
│     └── toString()                  временная метка + format()
│
├── class MessageReceiver             фоновый приём сообщений в отдельном потоке
│     ├── start() / stop()             запуск / останов потока
│     ├── receiveLoop()                цикл чтения из сокета, вызов колбэков
│     ├── setMessageCallback()          колбэк на полученное сообщение
│     └── setErrorCallback()            колбэк на ошибку / отключение
│
├── class ConsoleUI                   потокобезопасный вывод в консоль
│     ├── printMessage() / printError() / printWarning() / printRaw() ...
│     └── printPrompt()                приглашение для ввода
│
├── class ClientConfig                чтение имени клиента (аргумент командной строки / stdin)
│     └── readFromCommandLine()        парсинг argv
│
├── class TCPClient                   основная логика клиента
│     ├── initialize()                 установка имени, настройка колбэков
│     ├── connectTo() / disconnect()    подключение / отключение от сервера
│     ├── sendMessage()                 отправить текстовое сообщение (с заменой стикеров)
│     ├── sendRaw() / sendRawInternal() отправить сырую команду (с блокировкой фонового опроса)
│     ├── handleLocalCommand()          обработка команд, начинающихся с '/' (help, connect, server, logs ...)
│     ├── onMessageReceived()            обработка входящих сообщений (логирование, проверка порогов)
│     ├── onError()                      реакция на ошибку приёма
│     ├── stickerToEmoji()               преобразование названия стикера в emoji
│     ├── replaceStickers()               замена /sticker <name> в тексте на emoji
│     ├── checkAndWarn()                  анализ ответа сервера и вывод предупреждений при превышении порогов
│     ├── openLogFile() / closeLogFile()  лог-файл истории чата
│     ├── openMetricsFile() / closeMetricsFile()  файл для записи метрик
│     ├── readMetricsFile()                чтение файла метрик с применением функции-обработчика
│     ├── showAllMetrics() / showWarnings() / showMetricsSince() / showMetricsRange()  – команды просмотра логов
│     ├── startMetricsLoop() / stopMetricsLoop()  запуск / останов фонового опроса метрик
│     ├── metricsLoop()                     периодическая отправка всех /server команд (time, cpu, ...) 
│     │                                       с ожиданием ответов и записью в metrics-файл
│     └── run()                              главный цикл: чтение stdin, выполнение команд / отправка сообщений
│
└── class Application                 точка входа
      └── run()                        создание клиента, инициализация, запуск
         
joy_test.ino
├── setup()                инициализация Serial на 9600 бод
│
└── loop()                 бесконечный цикл
      ├── analogRead(PIN_X)   чтение значения с оси X
      ├── analogRead(PIN_Y)   чтение значения с оси Y
      ├── Serial.print()      вывод "x:123 y:456\n"
      └── delay(100)          пауза 100 мс
      

4. UML-диаграммы

   В папке docs/uml/ находятся следующие диаграммы в формате PNG,
   созданные с помощью PlantUML для наглядного представления архитектуры
   и ключевых сценариев работы:

   - server_class.png        – диаграмма классов серверной части (server.cpp)
   - client_class.png        – диаграмма классов клиентской части (client.cpp)
   - send_message.png        – диаграмма последовательности отправки сообщения
                                от одного клиента другому через сервер
   - server_command.png      – диаграмма последовательности обработки
                                команды /server cpu (запрос метрики)
   - metrics_loop.png        – диаграмма последовательности фонового опроса
                                метрик (цикл каждые 30 секунд)
   - components.png          – диаграмма компонентов (клиенты, сервер, Arduino)
   - deployment.png          – диаграмма развёртывания (физические узлы)

   Эти диаграммы дополняют текстовое описание архитектуры и помогают
   быстрее понять структуру кода и взаимодействие частей системы.
   
5. Запуск сервера

# Делаем nmap в сети для нахождения адреса raspberry pi
sudo nmap -sn 192.168.0.0/24

# Нам нужна эта часть, она о raspberry pi
Nmap scan report for 192.168.0.100
Host is up (0.030s latency).
MAC Address: B8:27:EB:60:2B:5D (Raspberry Pi Foundation)

# Подключаемся к raspberry под пользователем genesis-pi (задан при загрузке ОС через RPI Imager)
ssh genesis-pi@192.168.0.100

#  После ввода пароля raspberry видим успешное подключение
Linux genesis-pi 6.12.62+rpt-rpi-v8 #1 SMP PREEMPT Debian 1:6.12.62-1+rpt1 (2025-12-18) aarch64

The programs included with the Debian GNU/Linux system are free software;
the exact distribution terms for each program are described in the
individual files in /usr/share/doc/*/copyright.

Debian GNU/Linux comes with ABSOLUTELY NO WARRANTY, to the extent
permitted by applicable law.

# Черех новый терминал можем передать необходимые файлы
scp ~/Task3/server.cpp genesis-pi@192.168.0.100:/home/genesis-pi/

# Компилируем код сервера
g++ -o server server.cpp

# Запускаем код сервера
./server

# Выключение raspberry 
sudo shutdown -h now

6. Запуск клиента

Запускаем терминал в рабочей папке Task3

# Компиляция файла клиента
g++ -o client.exe client.cpp

# Запуск исполняемого файла
./client.exe

7. Прочие функции

Команды клиента (ввод в консоли):

/help - показать список всех доступных команд
/connect <ip> <port> - подключиться к серверу (пример: /connect 127.0.0.1 7000)
/disconnect - отключиться от текущего сервера
/quit (или /exit, /q) - завершить работу клиента

Команды для запроса информации с сервера:
/server time - текущее время на сервере
/server cpu - загрузка процессора (load average 1,5,15 мин)
/server cpu_temp - температура CPU (для Raspberry Pi)
/server ram - использование оперативной памяти
/server rom - использование дискового пространства (корневая ФС)
/server joystick - текущие значения джойстика, подключённого через Arduino

Стикеры (эмодзи):
/sticker <название> - отправить стикер как отдельное сообщение.
Доступные названия: smile 😊, sad 😢, thumbs_up 👍, thumbs_down 👎.
Можно также вставлять "/sticker <название>" прямо в текст сообщения – оно автоматически заменится на эмодзи.

Локальные команды для работы с файлом метрик (metrics_<имя>_<дата>.log):
/logs show - показать все записи из файла метрик
/logs warnings - показать только предупреждения (превышение порогов)
/logs since YYYY-MM-DD HH:MM:SS - показать метрики, начиная с указанной даты и времени
/logs from YYYY-MM-DD HH:MM:SS to YYYY-MM-DD HH:MM:SS - показать метрики за заданный временной диапазон
