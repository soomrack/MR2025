#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <chrono>
#include <queue>              // FIFO-контейнер. 
#include <condition_variable> // Сигналы между потоками. 
#include <fstream>            // Чтение и запись файлов
#include <sstream>            // Сборка строк через потоковый синтаксис
#include <iomanip>            // Форматирование вывода (ширина, точность, дата)
#include <ctime>              // Работа с датой и временем в C-стиле
#include <vector>             // Динамический массив
#include <algorithm>          // Алгоритмы (min, max, find)
#include <functional>         // Контейнер для функций и лямбд
#include <memory>             // Умные указатели
#include <unordered_map>      // Хеш-таблица (ключ -> значение)

#include <sys/select.h>       // Мультиплексирование I/O. 
#include <sys/statvfs.h>      // Информация о файловой системе. 
#include <unistd.h>           // POSIX I/O: чтение, запись, закрытие
#include <fcntl.h>            // Открытие файлов и устройств
#include <termios.h>          // Настройка последовательного порта
#include <sys/socket.h>       // Создание TCP-сокетов
#include <netinet/in.h>       // Структуры для IP-адресов
#include <arpa/inet.h>        // Преобразование IP в строку

using namespace std;

// ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ (ОБЪЯВЛЕНИЯ) ==========
string get_current_time();
void print_server_banner();

// ========== ИНТЕРФЕЙС КОМАНДЫ ==========
class ICommandHandler {
public:
    virtual ~ICommandHandler() = default;
    virtual string execute(const string& command, class SystemMonitor& monitor,
        class ArduinoComm& arduino, class Logger& logger) = 0;
    virtual string get_command_name() const = 0;
};

// ========== КЛАСС ЛОГГЕР ==========
class Logger {
private:
    mutex log_mutex_;
    vector<string> log_history_;
    ofstream log_file_;
    string get_current_time();

public:
    Logger();
    ~Logger();
    void log(const string& message, const string& type = "INFO");
    void log_command(const string& client_ip, const string& command);
    string get_recent_logs(int count);
};

// ========== КЛАСС ARDUINO_COMM ==========
class ArduinoComm {                          // Класс для общения с Arduino по UART
private:
    int serial_port_;                        // Файловый дескриптор последовательного порта
    mutex serial_mutex_;                     // Мьютекс для потокобезопасной отправки команд
    bool running_;                           // Флаг работы потока-читателя
    class Logger& logger_;                   // Ссылка на логгер для записи событий
    string read_buffer_;                     // Буфер накопления принятых данных (неполные строки)

    int open_serial(const char* port);       // Приватный метод открытия и настройки порта

public:
    ArduinoComm(Logger& log_ref);            // Конструктор: сохраняет ссылку на логгер
    ~ArduinoComm();                          // Деструктор: вызывает disconnect()
    bool connect(const char* port);          // Подключение к порту (например, "/dev/ttyACM0")
    void disconnect();                       // Закрытие порта
    bool send_command(const string& cmd);    // Отправка команды с добавлением "\n"
    void start_reader_thread(function<void(const string&)> data_callback); // Запуск фонового чтения
    void stop();                             // Остановка потока-читателя
    bool is_connected() const;               // Проверка, открыт ли порт
};

// ========== КЛАСС SYSTEM_MONITOR ==========
class SystemMonitor {                                    // Класс мониторинга системы (температура, CPU, критические события)
private:
    mutex monitor_mutex_;                                // Мьютекс для защиты данных при многопоточном доступе
    string temp_log_file_;                               // Путь к CSV-файлу истории температуры
    string critical_log_file_;                           // Путь к файлу критических событий
    string cpu_load_log_file_;                           // Путь к CSV-файлу истории загрузки CPU

    struct TempRecord {                                  // Структура записи о температуре
        string timestamp;                                //   Время измерения
        float temperature;                               //   Значение температуры в °C
    };

    struct CpuLoadRecord {                               // Структура записи о загрузке CPU
        string timestamp;                                //   Время измерения
        float load_percent;                              //   Процент загрузки
    };

    vector<TempRecord> temp_history_;                    // История измерений температуры в памяти
    vector<CpuLoadRecord> cpu_load_history_;             // История измерений загрузки CPU в памяти

    float min_temp_;                                     // Минимальная зафиксированная температура
    float max_temp_;                                     // Максимальная зафиксированная температура
    float max_cpu_load_;                                 // Максимальная зафиксированная загрузка CPU
    float critical_temp_threshold_;                      // Порог критической температуры (по умолчанию 55°C)
    float critical_cpu_threshold_;                       // Порог критической загрузки CPU (по умолчанию 80%)

    class Logger& logger_;                               // Ссылка на логгер
    bool running_;                                       // Флаг работы фонового мониторинга

    string get_current_time();                           // Получение текущего времени строкой
    float read_cpu_temperature();                        // Чтение температуры из /sys/class/thermal/thermal_zone0/temp
    float read_cpu_load();                               // Чтение загрузки из /proc/loadavg
    void check_critical_conditions();                    // Проверка превышения порогов и вызов log_critical_event
    void log_critical_event(const string& message);      // Запись критического события в файл

public:
    SystemMonitor(Logger& log_ref);                      // Конструктор: создаёт CSV-файлы, задаёт пороги
    ~SystemMonitor();                                    // Деструктор: вызывает stop()
    void log_temperature();                              // Измерение и запись температуры в историю и файл
    void log_cpu_load();                                 // Измерение и запись загрузки CPU в историю и файл
    void process_arduino_data(const string& data);       // Разбор данных от Arduino (TEMP_STATS, CMD, SPEED)
    string get_temperature_stats();                      // Формирование отчёта по температуре
    string get_cpu_load_stats();                         // Формирование отчёта по загрузке CPU
    string get_critical_events(int last_n = 10);         // Чтение последних N критических событий из файла
    string get_full_report();                            // Формирование полного отчёта (температура + CPU + крит. события)
    void start_monitoring();                             // Запуск фонового потока (каждые 10 сек log + check)
    void stop();                                         // Остановка фонового потока
};

// ========== КОМАНДЫ-ОБРАБОТЧИКИ ==========
class MoveCommand : public ICommandHandler {
public:
    string execute(const string& command, SystemMonitor& monitor,
        ArduinoComm& arduino, Logger& logger) override;
    string get_command_name() const override { return "move"; }
};

class SpeedCommand : public ICommandHandler {
public:
    string execute(const string& command, SystemMonitor& monitor,
        ArduinoComm& arduino, Logger& logger) override;
    string get_command_name() const override { return "speed"; }
};

class GetLogsCommand : public ICommandHandler {
public:
    string execute(const string& command, SystemMonitor& monitor,
        ArduinoComm& arduino, Logger& logger) override;
    string get_command_name() const override { return "GET_LOGS"; }
};

class GetTempStatsCommand : public ICommandHandler {
public:
    string execute(const string& command, SystemMonitor& monitor,
        ArduinoComm& arduino, Logger& logger) override;
    string get_command_name() const override { return "GET_TEMP_STATS"; }
};

class GetCpuLoadCommand : public ICommandHandler {
public:
    string execute(const string& command, SystemMonitor& monitor,
        ArduinoComm& arduino, Logger& logger) override;
    string get_command_name() const override { return "GET_CPU_LOAD"; }
};

class GetCriticalEventsCommand : public ICommandHandler {
public:
    string execute(const string& command, SystemMonitor& monitor,
        ArduinoComm& arduino, Logger& logger) override;
    string get_command_name() const override { return "GET_CRITICAL_EVENTS"; }
};

class GetFullReportCommand : public ICommandHandler {
public:
    string execute(const string& command, SystemMonitor& monitor,
        ArduinoComm& arduino, Logger& logger) override;
    string get_command_name() const override { return "GET_FULL_REPORT"; }
};

// ========== МЕНЕДЖЕР КОМАНД ==========
class CommandManager {
private:
    unordered_map<string, unique_ptr<ICommandHandler>> handlers_;

public:
    CommandManager() {
        register_handler(make_unique<MoveCommand>());
        register_handler(make_unique<SpeedCommand>());
        register_handler(make_unique<GetLogsCommand>());
        register_handler(make_unique<GetTempStatsCommand>());
        register_handler(make_unique<GetCpuLoadCommand>());
        register_handler(make_unique<GetCriticalEventsCommand>());
        register_handler(make_unique<GetFullReportCommand>());
    }

    void register_handler(unique_ptr<ICommandHandler> handler) {
        handlers_[handler->get_command_name()] = move(handler);
    }

    string process(const string& command, SystemMonitor& monitor,
        ArduinoComm& arduino, Logger& logger);
    void print_available_commands() const;
};

// ========== КЛАСС CLIENT_HANDLER ==========
class ClientHandler {                                            // Обработчик одного клиентского соединения
private:
    int client_socket_;                                          // Сокет подключившегося клиента
    string client_ip_;                                           // IP-адрес клиента (строка)
    class Logger& logger_;                                       // Ссылка на логгер
    class SystemMonitor& monitor_;                               // Ссылка на системный монитор
    class ArduinoComm& arduino_;                                 // Ссылка на связь с Arduino
    class CommandManager& cmd_manager_;                          // Ссылка на менеджер команд
    bool monitoring_mode_;                                       // Флаг режима мониторинга 

    void send_full_response(const string& response);             // Отправка ответа клиенту частями, пока не уйдёт всё

public:
    ClientHandler(int socket, sockaddr_in addr,                  // Конструктор: сохраняет сокет, извлекает IP из addr
        Logger& log_ref, SystemMonitor& mon_ref,                 //   Сохраняет ссылки на все сервисы
        ArduinoComm& ard_ref, CommandManager& cmd_ref);
    void handle();                                               // Главный цикл: приём команд, вызов process(), отправка ответа
};

// ========== КЛАСС TCP_SERVER ==========
class TcpServer {                                                // TCP-сервер: принимает клиентов, создаёт потоки-обработчики
private:
    int server_socket_;                                          // Слушающий сокет сервера
    int port_;                                                   // Порт, на котором сервер принимает подключения
    bool running_;                                               // Флаг работы главного цикла accept()
    class Logger& logger_;                                       // Ссылка на логгер
    class SystemMonitor& monitor_;                               // Ссылка на системный монитор
    class ArduinoComm& arduino_;                                 // Ссылка на связь с Arduino
    class CommandManager& cmd_manager_;                          // Ссылка на менеджер команд

public:
    TcpServer(int p, Logger& log_ref, SystemMonitor& mon_ref,   // Конструктор: сохраняет порт и ссылки на все сервисы
        ArduinoComm& ard_ref, CommandManager& cmd_ref);
    ~TcpServer();                                                // Деструктор: вызывает stop()
    bool start();                                                // Создание сокета, bind, listen
    void run();                                                  // Главный цикл: accept() + создание потока с ClientHandler
    void stop();                                                 // Остановка сервера, закрытие слушающего сокета
};

// ========== ГЛАВНОЕ ПРИЛОЖЕНИЕ СЕРВЕРА ==========
class RobotServerApp {
private:
    Logger logger_;
    SystemMonitor monitor_;
    ArduinoComm arduino_;
    CommandManager cmd_manager_;
    TcpServer server_;

public:
    RobotServerApp(int tcp_port = 8888);
    bool initialize(int argc, char* argv[]);
    void run();
    void shutdown();
};

// ========== РЕАЛИЗАЦИЯ ВСПОМОГАТЕЛЬНЫХ ФУНКЦИЙ ==========
string get_current_time() {
    auto now = chrono::system_clock::now();
    time_t now_time = chrono::system_clock::to_time_t(now);
    struct tm local_time;
    localtime_r(&now_time, &local_time);

    stringstream ss;
    ss << put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void print_server_banner() {
    cout << "\n" << string(60, '=') << endl;
    cout << "РОБОТ-СЕРВЕР С МОНИТОРИНГОМ CPU" << endl;
    cout << string(60, '=') << endl;
}

// ========== РЕАЛИЗАЦИЯ LOGGER ==========
string Logger::get_current_time() {
    auto now = chrono::system_clock::now();
    time_t now_time = chrono::system_clock::to_time_t(now);
    struct tm local_time;
    localtime_r(&now_time, &local_time);

    stringstream ss;
    ss << put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

Logger::Logger() {
    log_file_.open("server.log", ios::app);
}

Logger::~Logger() {
    if (log_file_.is_open()) log_file_.close();
}

void Logger::log(const string& message, const string& type) {  // Основной метод логирования
    lock_guard<mutex> lock(log_mutex_);                         //   Блокировка мьютекса до конца функции (потокобезопасность)
    string timestamp = get_current_time();                      //   Получение текущего времени: "ГГГГ-ММ-ДД ЧЧ:ММ:СС"
    string entry = "[" + timestamp + "] [" + type + "] " + message; // Формирование строки: [время] [тип] сообщение

    cout << entry << endl;                                      //   Вывод в консоль сервера
    log_history_.push_back(entry);                              //   Добавление в историю логов (оперативная память)
    if (log_history_.size() > 1000)                             //   Ограничение истории:
        log_history_.erase(log_history_.begin());               //     удаление самой старой записи при превышении 1000

    if (log_file_.is_open()) {                                  //   Если файл server.log открыт:
        log_file_ << entry << endl;                             //     запись строки в файл
        log_file_.flush();                                      //     принудительный сброс буфера на диск
    }
}

void Logger::log_command(const string& client_ip, const string& command) { // Логирование команды от клиента
    log("CMD from " + client_ip + ": " + command, "COMMAND");              //   Вызов основного log() с типом "COMMAND"
}                                                                          //   Формат: [время] [COMMAND] CMD from IP: команда

string Logger::get_recent_logs(int count) {                              // Получение последних N записей лога
    lock_guard<mutex> lock(log_mutex_);                                  //   Блокировка мьютекса (потокобезопасное чтение истории)
    stringstream ss;                                                     //   Поток для сборки результирующей строки
    ss << "=== ПОСЛЕДНИЕ ЛОГИ (последние " << count << ") ===\n";       //   Заголовок с количеством
    ss << string(60, '=') << "\n";                                       //   Разделительная линия из 60 символов '='

    int start = log_history_.size() > count                              //   Вычисление индекса начала:
        ? log_history_.size() - count                                    //     если история больше count — берём последние count
        : 0;                                                             //     иначе — с самого начала (0)
    for (size_t i = start; i < log_history_.size(); i++) {               //   Цикл от start до конца истории
        ss << log_history_[i] << "\n";                                   //     Добавление каждой записи с переводом строки
    }
    return ss.str();                                                     //   Возврат собранной строки (вызывается из GetLogsCommand)
}

// ========== РЕАЛИЗАЦИЯ ARDUINO_COMM ==========
ArduinoComm::ArduinoComm(Logger& log_ref) : serial_port_(-1), running_(true), logger_(log_ref) {}

ArduinoComm::~ArduinoComm() { disconnect(); }

int ArduinoComm::open_serial(const char* port) {                     // Открытие и настройка последовательного порта
    int fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);                //   Открытие порта: чтение/запись, не терминал, синхронно
    if (fd < 0) return fd;                                           //   Ошибка открытия — возврат -1

    struct termios tty;                                              //   Структура с настройками порта
    memset(&tty, 0, sizeof(tty));                                    //   Обнуление структуры перед заполнением
    if (tcgetattr(fd, &tty) != 0) { close(fd); return -1; }         //   Чтение текущих настроек, при ошибке — закрыть порт

    cfsetospeed(&tty, B9600);                                        //   Установка скорости отправки: 9600 бод
    cfsetispeed(&tty, B9600);                                        //   Установка скорости приёма: 9600 бод

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;                     //   8 бит данных (очистка битов размера, установка CS8)
    tty.c_iflag &= ~IGNBRK;                                          //   Не игнорировать сигнал BREAK
    tty.c_lflag = 0;                                                 //   Отключение канонического режима, эха, сигналов
    tty.c_oflag = 0;                                                 //   Отключение обработки вывода
    tty.c_cc[VMIN] = 0;                                              //   Минимальное количество байт для read(): 0 (неблокирующий)
    tty.c_cc[VTIME] = 5;                                             //   Таймаут чтения: 0.5 секунды (5 * 100 мс)
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);                          //   Отключение программного управления потоком (XON/XOFF)
    tty.c_cflag |= (CLOCAL | CREAD);                                 //   Игнорировать модемные сигналы, разрешить приём
    tty.c_cflag &= ~(PARENB | PARODD);                               //   Без бита чётности
    tty.c_cflag &= ~CSTOPB;                                          //   1 стоп-бит (очистка CSTOPB = 1 бит, установка = 2 бита)
    tty.c_cflag &= ~CRTSCTS;                                         //   Отключение аппаратного управления потоком (RTS/CTS)

    if (tcsetattr(fd, TCSANOW, &tty) != 0) { close(fd); return -1; } // Применение настроек немедленно, при ошибке — закрыть порт
    return fd;                                                       //   Возврат файлового дескриптора открытого порта
}

bool ArduinoComm::connect(const char* port) {
    serial_port_ = open_serial(port);
    return serial_port_ >= 0;
}

void ArduinoComm::disconnect() {
    if (serial_port_ >= 0) {
        close(serial_port_);
        serial_port_ = -1;
    }
}

bool ArduinoComm::send_command(const string& cmd) {
    if (serial_port_ < 0) return false;
    lock_guard<mutex> lock(serial_mutex_);
    string to_send = cmd + "\n";
    write(serial_port_, to_send.c_str(), to_send.length());
    return true;
}

void ArduinoComm::start_reader_thread(function<void(const string&)> data_callback) { // Запуск фонового потока чтения из Arduino
    thread([this, data_callback]() {                                                   //   Создание потока с захватом this и колбэка
        char buffer[256];                                                              //   Буфер для чтения данных
        while (running_) {                                                             //   Пока разрешена работа:
            int bytes = read(serial_port_, buffer, sizeof(buffer) - 1);                //     Чтение из порта (неблокирующее, до 255 байт)
            if (bytes > 0) {                                                           //     Если данные получены:
                buffer[bytes] = '\0';                                                  //       Добавление нуль-терминатора
                read_buffer_ += buffer;                                                //       Дописывание в буфер-накопитель

                size_t pos;                                                            //       Поиск всех завершённых строк (по '\n')
                while ((pos = read_buffer_.find('\n')) != string::npos) {              //       Пока есть символ перевода строки:
                    string line = read_buffer_.substr(0, pos);                         //         Извлечение строки до '\n'
                    line.erase(line.find_last_not_of(" \r\n\t") + 1);                  //         Удаление пробельных символов в конце
                    if (!line.empty()) data_callback(line);                            //         Если строка не пуста — вызов колбэка
                    read_buffer_.erase(0, pos + 1);                                    //         Удаление обработанной строки из буфера
                }
            }
            this_thread::sleep_for(chrono::milliseconds(10));                          //     Пауза 10 мс между чтениями
        }
        }).detach();                                                                       //   Отсоединение потока (работает независимо)
}

void ArduinoComm::stop() { running_ = false; }
bool ArduinoComm::is_connected() const { return serial_port_ >= 0; }

// ========== РЕАЛИЗАЦИЯ SYSTEM_MONITOR ==========
SystemMonitor::SystemMonitor(Logger& log_ref)
    : logger_(log_ref), running_(true), min_temp_(999.0f), max_temp_(-999.0f),
    max_cpu_load_(0.0f), critical_temp_threshold_(55.0f), critical_cpu_threshold_(80.0f) {

    temp_log_file_ = "/home/vlad/rpi_server/temperature_history.csv";
    critical_log_file_ = "/home/vlad/rpi_server/critical_events.log";
    cpu_load_log_file_ = "/home/vlad/rpi_server/cpu_load_history.csv";

    system("mkdir -p /home/vlad/rpi_server");

    ifstream temp_check(temp_log_file_);
    if (!temp_check.good()) {
        ofstream temp_file(temp_log_file_);
        temp_file << "Timestamp,Temperature_C\n";
    }

    ifstream load_check(cpu_load_log_file_);
    if (!load_check.good()) {
        ofstream load_file(cpu_load_log_file_);
        load_file << "Timestamp,CPU_Load_Percent\n";
    }
}

SystemMonitor::~SystemMonitor() { stop(); }

string SystemMonitor::get_current_time() {
    auto now = chrono::system_clock::now();
    time_t now_time = chrono::system_clock::to_time_t(now);
    struct tm local_time;
    localtime_r(&now_time, &local_time);

    stringstream ss;
    ss << put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

float SystemMonitor::read_cpu_temperature() {
    float temp = 0.0f;
    FILE* fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (fp) {
        int millideg;
        if (fscanf(fp, "%d", &millideg) == 1) temp = millideg / 1000.0f;
        fclose(fp);
    }
    return temp;
}

float SystemMonitor::read_cpu_load() {
    float load = 0.0f;
    FILE* fp = fopen("/proc/loadavg", "r");
    if (fp) {
        double load1, load5, load15;
        int processes;
        if (fscanf(fp, "%lf %lf %lf %d/%*d %*d", &load1, &load5, &load15, &processes) == 4) {
            load = load1 * 100.0 / sysconf(_SC_NPROCESSORS_ONLN);
        }
        fclose(fp);
    }
    return min(load, 100.0f);
}

void SystemMonitor::check_critical_conditions() {
    float current_temp = read_cpu_temperature();
    float current_load = read_cpu_load();

    if (current_temp > critical_temp_threshold_) {
        string message = "температура CPU " + to_string(current_temp) +
            "C превышает порог " + to_string(critical_temp_threshold_) + "C";
        log_critical_event(message);
        logger_.log(message, "CRITICAL");
    }

    if (current_load > critical_cpu_threshold_) {
        string message = "загрузка CPU " + to_string(current_load) +
            "% превышает порог " + to_string(critical_cpu_threshold_) + "%";
        log_critical_event(message);
        logger_.log(message, "CRITICAL");
    }
}

void SystemMonitor::log_critical_event(const string& message) {
    lock_guard<mutex> lock(monitor_mutex_);
    ofstream file(critical_log_file_, ios::app);
    file << get_current_time() << " - " << message << "\n";
}

void SystemMonitor::log_temperature() {
    lock_guard<mutex> lock(monitor_mutex_);
    float current_temp = read_cpu_temperature();

    TempRecord record{ get_current_time(), current_temp };
    temp_history_.push_back(record);

    if (current_temp < min_temp_) min_temp_ = current_temp;
    if (current_temp > max_temp_) max_temp_ = current_temp;

    static int temp_counter = 0;
    if (++temp_counter % 6 == 0) {
        ofstream file(temp_log_file_, ios::app);
        file << record.timestamp << "," << fixed << setprecision(2) << current_temp << "\n";
    }
}

void SystemMonitor::log_cpu_load() {
    lock_guard<mutex> lock(monitor_mutex_);
    float current_load = read_cpu_load();

    CpuLoadRecord record{ get_current_time(), current_load };
    cpu_load_history_.push_back(record);

    if (current_load > max_cpu_load_) max_cpu_load_ = current_load;

    static int load_counter = 0;
    if (++load_counter % 6 == 0) {
        ofstream file(cpu_load_log_file_, ios::app);
        file << record.timestamp << "," << fixed << setprecision(2) << current_load << "\n";
    }
}

void SystemMonitor::process_arduino_data(const string& data) {
    if (data.find("TEMP_STATS") != string::npos) {
        size_t comma = data.find(",");
        if (comma != string::npos) {
            try {
                float minT = stof(data.substr(11, comma - 11));
                float maxT = stof(data.substr(comma + 1));
                logger_.log("Температура от Arduino: мин=" + to_string(minT) +
                    " макс=" + to_string(maxT), "TEMP");
            }
            catch (...) {
                logger_.log("Ошибка парсинга TEMP_STATS: " + data, "ERROR");
            }
        }
    }
    else if (data.find("CMD:") != string::npos || data.find("SPEED:") != string::npos) {
        logger_.log("Arduino: " + data, "SERIAL");
    }
    else if (!data.empty() && data != "OK" && data.length() > 1) {
        logger_.log("Arduino: " + data, "SERIAL");
    }
}

string SystemMonitor::get_temperature_stats() {
    lock_guard<mutex> lock(monitor_mutex_);
    stringstream ss;
    float current = read_cpu_temperature();

    ss << "\n" << string(80, '=') << "\n";
    ss << "ТЕМПЕРАТУРА ПРОЦЕССОРА\n";
    ss << string(80, '=') << "\n";
    ss << "Текущая:     " << fixed << setprecision(2) << current << " °C\n";
    ss << "Минимальная: " << fixed << setprecision(2) << min_temp_ << " °C\n";
    ss << "Максимальная: " << fixed << setprecision(2) << max_temp_ << " °C\n";
    return ss.str();
}

string SystemMonitor::get_cpu_load_stats() {
    lock_guard<mutex> lock(monitor_mutex_);
    stringstream ss;
    float current = read_cpu_load();

    ss << "\n" << string(80, '=') << "\n";
    ss << "ЗАГРУЗКА ПРОЦЕССОРА\n";
    ss << string(80, '=') << "\n";
    ss << "Текущая:     " << fixed << setprecision(2) << current << " %\n";
    ss << "Максимальная: " << fixed << setprecision(2) << max_cpu_load_ << " %\n";
    ss << "Количество ядер: " << sysconf(_SC_NPROCESSORS_ONLN) << "\n";
    return ss.str();
}

string SystemMonitor::get_critical_events(int last_n) {                    // Получение последних N критических событий
    lock_guard<mutex> lock(monitor_mutex_);                                //   Блокировка мьютекса (потокобезопасность)
    stringstream ss;                                                       //   Поток для сборки ответа

    ss << "\n" << string(80, '=') << "\n";                                 //   Разделительная линия из 80 символов '='
    ss << "КРИТИЧЕСКИЕ СОБЫТИЯ (последние " << last_n << ")\n";           //   Заголовок с количеством
    ss << string(80, '=') << "\n";                                         //   Ещё одна разделительная линия

    ifstream crit_file(critical_log_file_);                                //   Открытие файла критических событий на чтение
    if (crit_file.is_open()) {                                             //   Если файл открыт успешно:
        vector<string> all_events;                                         //     Вектор для хранения всех строк файла
        string line;                                                       //     Переменная для чтения строки
        while (getline(crit_file, line)) {                                 //     Построчное чтение файла:
            if (!line.empty() && line.back() == '\r') line.pop_back();     //       Удаление '\r' в конце (Windows CRLF -> LF)
            if (line.empty()) continue;                                    //       Пропуск пустых строк
            all_events.push_back(line);                                    //       Добавление строки в вектор
            if (all_events.size() > 1000)                                  //       Ограничение размера вектора:
                all_events.erase(all_events.begin());                      //         удаление самой старой записи при >1000
        }

        if (all_events.empty()) {                                          //     Если событий нет:
            ss << "  Критических событий не случилось\n";                 //       Сообщение об отсутствии событий
        }
        else {                                                             //     Если события есть:
            ss << "  Обнаружены следующие критические события:\n\n";      //       Подзаголовок
            int start = all_events.size() > last_n                        //       Вычисление индекса начала:
                ? all_events.size() - last_n                               //         если больше last_n — последние last_n
                : 0;                                                       //         иначе — с самого начала
            for (size_t i = start; i < all_events.size(); i++) {           //       Цикл от start до конца:
                ss << "  * " << all_events[i] << "\n";                    //         Вывод события с маркером *
            }
        }
    }
    else {                                                                 //   Если файл не удалось открыть:
        ss << "  Критических событий не зарегистрировано\n";              //     Сообщение об отсутствии файла
    }
    return ss.str();                                                       //   Возврат собранной строки
}

string SystemMonitor::get_full_report() {
    lock_guard<mutex> lock(monitor_mutex_);
    stringstream ss;

    ss << "\n" << string(80, '#') << "\n";
    ss << "          ПОЛНЫЙ ОТЧЁТ О РАБОТЕ РОБОТА\n";
    ss << string(80, '#') << "\n";
    ss << "Сформирован: " << get_current_time() << "\n";
    ss << string(80, '#') << "\n";

    ss << get_temperature_stats();
    ss << get_cpu_load_stats();
    ss << get_critical_events(10);

    ss << "\n" << string(80, '=') << "\n";
    ss << "ФАЙЛЫ ДАННЫХ\n";
    ss << string(80, '=') << "\n";
    ss << "  - " << temp_log_file_ << "\n";
    ss << "  - " << cpu_load_log_file_ << "\n";
    ss << "  - " << critical_log_file_ << "\n";
    ss << "\n" << string(80, '#') << "\nКОНЕЦ ОТЧЁТА\n" << string(80, '#') << "\n";

    return ss.str();
}

void SystemMonitor::start_monitoring() {
    thread([this]() {
        while (running_) {
            log_temperature();
            log_cpu_load();
            check_critical_conditions();
            this_thread::sleep_for(chrono::seconds(10));
        }
        }).detach();
}

void SystemMonitor::stop() { running_ = false; }

// ========== РЕАЛИЗАЦИЯ КОМАНД ==========
string MoveCommand::execute(const string& command, SystemMonitor& monitor,
    ArduinoComm& arduino, Logger& logger) {
    arduino.send_command(command);
    return "OK";
}

string SpeedCommand::execute(const string& command, SystemMonitor& monitor,
    ArduinoComm& arduino, Logger& logger) {
    arduino.send_command(command);
    return "OK";
}

string GetLogsCommand::execute(const string& command, SystemMonitor& monitor,
    ArduinoComm& arduino, Logger& logger) {
    return logger.get_recent_logs(50);
}

string GetTempStatsCommand::execute(const string& command, SystemMonitor& monitor,
    ArduinoComm& arduino, Logger& logger) {
    return monitor.get_temperature_stats();
}

string GetCpuLoadCommand::execute(const string& command, SystemMonitor& monitor,
    ArduinoComm& arduino, Logger& logger) {
    return monitor.get_cpu_load_stats();
}

string GetCriticalEventsCommand::execute(const string& command, SystemMonitor& monitor,
    ArduinoComm& arduino, Logger& logger) {
    return monitor.get_critical_events(10);
}

string GetFullReportCommand::execute(const string& command, SystemMonitor& monitor,
    ArduinoComm& arduino, Logger& logger) {
    return monitor.get_full_report();
}

// ========== РЕАЛИЗАЦИЯ COMMAND_MANAGER ==========
string CommandManager::process(const string& command, SystemMonitor& monitor,
    ArduinoComm& arduino, Logger& logger) {
    string cmd_name = command;
    size_t space_pos = command.find(' ');
    if (space_pos != string::npos) {
        cmd_name = command.substr(0, space_pos);
    }

    auto it = handlers_.find(cmd_name);
    if (it != handlers_.end()) {
        return it->second->execute(command, monitor, arduino, logger);
    }

    if (command.length() == 1 && (command == "w" || command == "a" ||
        command == "s" || command == "d" || command == "x")) {
        return handlers_["move"]->execute(command, monitor, arduino, logger);
    }

    if (command.substr(0, 5) == "speed") {
        return handlers_["speed"]->execute(command, monitor, arduino, logger);
    }

    return "ERROR: Unknown command";
}

void CommandManager::print_available_commands() const {
    cout << "\nДоступные команды:" << endl;
    cout << "  w,a,s,d,x,speed N - управление движением" << endl;
    cout << "  GET_LOGS - последние логи" << endl;
    cout << "  GET_TEMP_STATS - статистика температуры" << endl;
    cout << "  GET_CPU_LOAD - загрузка CPU" << endl;
    cout << "  GET_CRITICAL_EVENTS - критические события" << endl;
    cout << "  GET_FULL_REPORT - полный отчет" << endl;
}

// ========== РЕАЛИЗАЦИЯ CLIENT_HANDLER ==========
ClientHandler::ClientHandler(int socket, sockaddr_in addr, Logger& log_ref,
    SystemMonitor& mon_ref, ArduinoComm& ard_ref, CommandManager& cmd_ref)
    : client_socket_(socket), logger_(log_ref), monitor_(mon_ref),
    arduino_(ard_ref), cmd_manager_(cmd_ref), monitoring_mode_(false) {
    client_ip_ = inet_ntoa(addr.sin_addr);
}

void ClientHandler::send_full_response(const string& response) {
    if (response.empty()) return;
    const char* data = response.c_str();
    size_t total = response.length();
    size_t sent = 0;

    while (sent < total) {
        int result = send(client_socket_, data + sent, total - sent, 0);
        if (result <= 0) {
            logger_.log("Ошибка отправки данных клиенту", "ERROR");
            break;
        }
        sent += result;
        if (sent < total) usleep(5000);
    }
}

void ClientHandler::handle() {
    logger_.log("Клиент подключился: " + client_ip_, "CONNECT");
    char buffer[4096];

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(client_socket_, buffer, sizeof(buffer) - 1, 0);

        if (bytes <= 0) {
            logger_.log("Клиент отключился: " + client_ip_, "DISCONNECT");
            break;
        }

        string command(buffer);
        command.erase(command.find_last_not_of(" \n\r\t") + 1);
        logger_.log_command(client_ip_, command);

        string response = cmd_manager_.process(command, monitor_, arduino_, logger_);
        if (!response.empty()) send_full_response(response);
    }
    close(client_socket_);
}

// ========== РЕАЛИЗАЦИЯ TCP_SERVER ==========
TcpServer::TcpServer(int p, Logger& log_ref, SystemMonitor& mon_ref,
    ArduinoComm& ard_ref, CommandManager& cmd_ref)
    : server_socket_(-1), port_(p), running_(true), logger_(log_ref),
    monitor_(mon_ref), arduino_(ard_ref), cmd_manager_(cmd_ref) {
}

TcpServer::~TcpServer() { stop(); }

bool TcpServer::start() {
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) return false;

    int opt = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(server_socket_);
        return false;
    }

    if (listen(server_socket_, 5) < 0) {
        close(server_socket_);
        return false;
    }
    return true;
}

void TcpServer::run() {
    cout << "\nСЕРВЕР ЗАПУЩЕН НА ПОРТУ " << port_ << endl;
    cout << string(60, '=') << endl;
    cout << "IP адреса:" << endl;
    system("hostname -I | awk '{print \"  - \" $1}'");
    cout << "\nОжидание клиентов..." << endl;

    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) continue;

        thread([this, client_socket, client_addr]() {
            ClientHandler handler(client_socket, client_addr, logger_, monitor_, arduino_, cmd_manager_);
            handler.handle();
            }).detach();
    }
}

void TcpServer::stop() {
    running_ = false;
    if (server_socket_ >= 0) {
        close(server_socket_);
        server_socket_ = -1;
    }
}

// ========== РЕАЛИЗАЦИЯ ROBOT_SERVER_APP ==========
RobotServerApp::RobotServerApp(int tcp_port)
    : monitor_(logger_), arduino_(logger_), server_(tcp_port, logger_, monitor_, arduino_, cmd_manager_) {
}

bool RobotServerApp::initialize(int argc, char* argv[]) {
    print_server_banner();

    const char* port = "/dev/ttyACM0";
    if (argc > 1) port = argv[1];

    cout << "Подключение к " << port << "... ";
    if (!arduino_.connect(port)) {
        cerr << "\nОшибка открытия порта " << port << endl;
        system("ls -la /dev/tty* 2>/dev/null | grep -E 'tty(ACM|USB)'");
        return false;
    }
    cout << "УСПЕШНО!" << endl;

    arduino_.start_reader_thread([this](const string& data) {
        monitor_.process_arduino_data(data);
        });

    this_thread::sleep_for(chrono::seconds(2));
    monitor_.start_monitoring();

    cout << "\nМониторинг запущен:" << endl;
    cout << "  - Температура CPU (каждые 10 сек)" << endl;
    cout << "  - Загрузка CPU (каждые 10 сек)" << endl;
    cout << "  - Критические события (порог 55°C / 80% CPU)" << endl;

    if (!server_.start()) return false;

    cmd_manager_.print_available_commands();
    return true;
}

void RobotServerApp::run() {
    server_.run();
}

void RobotServerApp::shutdown() {
    arduino_.stop();
    monitor_.stop();
    server_.stop();
}

// ========== ГЛАВНАЯ ФУНКЦИЯ ==========
int main(int argc, char* argv[]) {
    RobotServerApp app;

    if (!app.initialize(argc, argv)) {
        return 1;
    }

    app.run();
    app.shutdown();

    return 0;
}