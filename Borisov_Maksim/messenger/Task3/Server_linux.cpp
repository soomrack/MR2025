
#include <iostream>
#include <fstream>       // Для работы с файлами (логи)
#include <sstream>       // Для std::ostringstream (сборка строк)
#include <string>
#include <vector>
#include <thread>        // Для std::thread (многопоточность)
#include <mutex>         // Для std::mutex (защита данных)
#include <chrono>        // Для работы со временем
#include <ctime>         // Для форматирования времени
#include <algorithm>     // Для std::remove_if
#include <atomic>        // Для std::atomic<bool>
#include <cstring>
#include <fcntl.h>      // для open()
#include <termios.h>    // для работы с последовательным портом

// Linux сетевые заголовки
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>      // Для close(), read()

using namespace std;

// КОНСТАНТЫ
const int    PORT = 54000;
const int    MAX_CLIENTS = 5;
const int    MONITOR_INTERVAL = 5;   // Секунд между проверками
const string LOG_FILE = "server_logs.txt";

// Пороговые значения для предупреждений
const float  CPU_WARN_THRESHOLD = 85.0f;  // %
const float  RAM_WARN_THRESHOLD = 85.0f;  // %
const float  TEMP_WARN_THRESHOLD = 70.0f;  // °C
const float  DISK_WARN_THRESHOLD = 90.0f;  // %

// ГЛОБАЛЬНЫЕ ДАННЫЕ (доступны из разных потоков)
std::mutex logsMutex;                  // Замок для логов
std::vector<std::string> allLogs;      // Все логи в памяти
std::vector<std::string> allWarnings;  // Все предупреждения
std::atomic<bool> serverRunning(true); // Флаг работы сервера

// Для датчика линии
std::mutex sensorMutex;
std::string lastLineSensorValue = "N/A";
std::atomic<bool> sensorThreadRunning(true);

std::vector<int> streamingClients;        // сокеты клиентов, подписанных на стриминг
std::mutex streamingMutex;                // мьютекс для доступа к списку


// Возвращает текущее время в виде строки "[2024-01-15 14:23:05]"
std::string getCurrentTime() {
    auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    char buf[32];
    // strftime форматирует время в строку по шаблону
    std::strftime(buf, sizeof(buf), "[%Y-%m-%d %H:%M:%S]", std::localtime(&t));
    return std::string(buf);
}

// Добавляет запись в логи (и в память, и в файл)
void addLog(const std::string& message) {
    std::string entry = getCurrentTime() + " " + message;

    std::lock_guard<std::mutex> lock(logsMutex);
    allLogs.push_back(entry);

    // Пишем в файл сразу (дозапись в конец)
    std::ofstream file(LOG_FILE, std::ios::app);
    if (file.is_open()) {
        file << entry << "\n";
    }

    std::cout << entry << std::endl;

    // Отправка всем подписанным клиентам
    std::lock_guard<std::mutex> lockStream(streamingMutex);
    for (auto it = streamingClients.begin(); it != streamingClients.end(); ) {
        int clientSock = *it;
        std::string data = "[STREAM] " + entry + "\n";
        int sent = send(clientSock, data.c_str(), data.size(), 0);
        if (sent <= 0) {
            it = streamingClients.erase(it);
        }
        else {
            ++it;
        }
    }
}

void addWarning(const std::string& message) {
    std::string entry = getCurrentTime() + " [WARNING] " + message;

    std::lock_guard<std::mutex> lock(logsMutex);
    allLogs.push_back(entry);
    allWarnings.push_back(entry);

    std::ofstream file(LOG_FILE, std::ios::app);
    if (file.is_open()) {
        file << entry << "\n";
    }

    std::cout << "\033[33m" << entry << "\033[0m" << std::endl; // Жёлтый цвет в консоли

    std::lock_guard<std::mutex> lockStream(streamingMutex);
    for (auto it = streamingClients.begin(); it != streamingClients.end(); ) {
        int clientSock = *it;
        std::string data = "[STREAM] " + entry + "\n";
        int sent = send(clientSock, data.c_str(), data.size(), 0);
        if (sent <= 0) {
            it = streamingClients.erase(it);
        }
        else {
            ++it;
        }
    }
}

// ЧТЕНИЕ СИСТЕМНЫХ ДАННЫХ RASPBERRY PI

float getCpuUsage() {
    static long prevIdle = 0, prevTotal = 0;

    std::ifstream file("/proc/stat");
    std::string line;
    std::getline(file, line);

    std::istringstream ss(line);
    std::string cpu;
    long user, nice, system, idle, iowait, irq, softirq;
    ss >> cpu >> user >> nice >> system >> idle >> iowait >> irq >> softirq;

    long total = user + nice + system + idle + iowait + irq + softirq;
    long diffIdle = idle - prevIdle;
    long diffTotal = total - prevTotal;

    prevIdle = idle;
    prevTotal = total;

    if (diffTotal == 0) return 0.0f;
    // Формула: (1 - доля_idle) * 100
    return (float)(diffTotal - diffIdle) / diffTotal * 100.0f;
}

float getRamUsage() {
    std::ifstream file("/proc/meminfo");
    std::string line;
    long memTotal = 0, memAvailable = 0;

    // Читаем строки пока не найдём нужные
    while (std::getline(file, line)) {
        if (line.find("MemTotal:") != std::string::npos) {
            std::istringstream ss(line);
            std::string label; std::string unit;
            ss >> label >> memTotal >> unit;
        }
        if (line.find("MemAvailable:") != std::string::npos) {
            std::istringstream ss(line);
            std::string label; std::string unit;
            ss >> label >> memAvailable >> unit;
        }
    }
    if (memTotal == 0) return 0.0f;
    return (float)(memTotal - memAvailable) / memTotal * 100.0f;
}

float getCpuTemperature() {
    std::ifstream file("/sys/class/thermal/thermal_zone0/temp");
    long temp = 0;
    file >> temp;
    return temp / 1000.0f; // Файл хранит температуру в милли-градусах
}

float getDiskUsage() {
    FILE* pipe = popen("df / | tail -1 | awk '{print $5}'", "r");
    if (!pipe) return 0.0f;
    char buf[16];
    fgets(buf, sizeof(buf), pipe);
    pclose(pipe);
    return std::stof(std::string(buf));
}

std::string getUptime() {
    std::ifstream file("/proc/uptime");
    double uptime;
    file >> uptime;

    int days = (int)uptime / 86400;
    int hours = ((int)uptime % 86400) / 3600;
    int minutes = ((int)uptime % 3600) / 60;
    int seconds = (int)uptime % 60;

    std::ostringstream oss;
    oss << days << "д " << hours << "ч " << minutes << "м " << seconds << "с";
    return oss.str();
}

// ПОТОК МОНИТОРИНГА
void monitoringThread() {
    addLog("[INFO] Поток мониторинга запущен.");

    getCpuUsage();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    while (serverRunning) {
        float cpu = getCpuUsage();
        float ram = getRamUsage();
        float temp = getCpuTemperature();
        float disk = getDiskUsage();
        std::string uptime = getUptime();

        std::ostringstream status;
        status << "[INFO] Статус: CPU=" << cpu << "% | RAM=" << ram
            << "% | Temp=" << temp << "C | Disk=" << disk
            << "% | Uptime=" << uptime;
        addLog(status.str());

        if (cpu > CPU_WARN_THRESHOLD) {
            std::ostringstream warn;
            warn << "Высокая загрузка CPU: " << cpu << "% (порог: " << CPU_WARN_THRESHOLD << "%)";
            addWarning(warn.str());
        }
        if (ram > RAM_WARN_THRESHOLD) {
            std::ostringstream warn;
            warn << "Высокое использование RAM: " << ram << "% (порог: " << RAM_WARN_THRESHOLD << "%)";
            addWarning(warn.str());
        }
        if (temp > TEMP_WARN_THRESHOLD) {
            std::ostringstream warn;
            warn << "Высокая температура CPU: " << temp << "C (порог: " << TEMP_WARN_THRESHOLD << "C)";
            addWarning(warn.str());
        }
        if (disk > DISK_WARN_THRESHOLD) {
            std::ostringstream warn;
            warn << "Диск заполнен: " << disk << "% (порог: " << DISK_WARN_THRESHOLD << "%)";
            addWarning(warn.str());
        }

        std::this_thread::sleep_for(std::chrono::seconds(MONITOR_INTERVAL));
    }
}

// ОБРАБОТКА КОМАНД КЛИЕНТА

void sendToClient(int clientSock, const std::string& msg) {
    std::string data = msg + "\n";
    send(clientSock, data.c_str(), data.size(), 0);
}

void handleClient(int clientSock, std::string clientIP) {
    addLog("[INFO] Клиент подключился: " + clientIP);
    sendToClient(clientSock, "=== Сервер мониторинга Raspberry Pi ===");
    sendToClient(clientSock, "Доступные команды:");
    sendToClient(clientSock, "  logs            - все логи");
    sendToClient(clientSock, "  warnings        - все предупреждения");
    sendToClient(clientSock, "  status          - текущее состояние платы");
    sendToClient(clientSock, "  logs_last <N>   - логи за последние N минут");
    sendToClient(clientSock, "  clear_warnings  - сбросить список предупреждений");
    sendToClient(clientSock, "  line_sensor     - получить данные с датчика");
    sendToClient(clientSock, "  stream_logs     - включить поток логов в реальном времени");
    sendToClient(clientSock, "  stop_stream     - остановить поток логов");
    sendToClient(clientSock, "  help            - показать это меню");
    sendToClient(clientSock, "  exit            - отключиться");
    sendToClient(clientSock, "========================================");

    char buffer[1024];

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytesRecv = recv(clientSock, buffer, sizeof(buffer) - 1, 0);

        if (bytesRecv <= 0) {
            addLog("[INFO] Клиент отключился: " + clientIP);
            break;
        }

        // Получаем команду и убираем пробелы/переносы строк по краям
        std::string cmd(buffer, bytesRecv);
        // Убираем \r и \n и пробелы в конце
        while (!cmd.empty() && (cmd.back() == '\n' || cmd.back() == '\r' || cmd.back() == ' '))
            cmd.pop_back();
        // Убираем пробелы в начале
        while (!cmd.empty() && cmd.front() == ' ')
            cmd.erase(cmd.begin());

        addLog("[CMD] От " + clientIP + ": '" + cmd + "'");

        // Команда: logs — все логи
        if (cmd == "logs") {
            std::lock_guard<std::mutex> lock(logsMutex);
            sendToClient(clientSock, "--- НАЧАЛО ЛОГОВ (" + std::to_string(allLogs.size()) + " записей) ---");
            for (const auto& log : allLogs) {
                sendToClient(clientSock, log);
            }
            sendToClient(clientSock, "--- КОНЕЦ ЛОГОВ ---");
        }

        // Команда: warnings — все предупреждения
        else if (cmd == "warnings") {
            std::lock_guard<std::mutex> lock(logsMutex);
            if (allWarnings.empty()) {
                sendToClient(clientSock, "Предупреждений нет.");
            }
            else {
                sendToClient(clientSock, "--- ПРЕДУПРЕЖДЕНИЯ (" + std::to_string(allWarnings.size()) + ") ---");
                for (const auto& w : allWarnings) {
                    sendToClient(clientSock, w);
                }
                sendToClient(clientSock, "--- КОНЕЦ ---");
            }
        }

        // Команда: status — текущее состояние
        else if (cmd == "status") {
            float cpu = getCpuUsage();
            float ram = getRamUsage();
            float temp = getCpuTemperature();
            float disk = getDiskUsage();
            std::string uptime = getUptime();

            sendToClient(clientSock, "--- ТЕКУЩЕЕ СОСТОЯНИЕ ---");

            auto statusLabel = [](float value, float threshold) -> std::string {
                return value > threshold ? " [!!! ПРЕВЫШЕН ПОРОГ !!!]" : " [OK]";
                };

            std::ostringstream oss;
            oss.precision(1); oss << std::fixed;
            oss << "CPU:         " << cpu << "%" << statusLabel(cpu, CPU_WARN_THRESHOLD);
            sendToClient(clientSock, oss.str()); oss.str("");

            oss << "RAM:         " << ram << "%" << statusLabel(ram, RAM_WARN_THRESHOLD);
            sendToClient(clientSock, oss.str()); oss.str("");

            oss << "Температура: " << temp << "°C" << statusLabel(temp, TEMP_WARN_THRESHOLD);
            sendToClient(clientSock, oss.str()); oss.str("");

            oss << "Диск:        " << disk << "%" << statusLabel(disk, DISK_WARN_THRESHOLD);
            sendToClient(clientSock, oss.str()); oss.str("");

            sendToClient(clientSock, "Uptime:      " + uptime);

            {
                std::lock_guard<std::mutex> lock(logsMutex);
                sendToClient(clientSock, "Всего логов: " + std::to_string(allLogs.size()));
                sendToClient(clientSock, "Предупреждений: " + std::to_string(allWarnings.size()));
            }
            sendToClient(clientSock, "--- КОНЕЦ ---");
        }

        // Команда: logs_last <N> — логи за N минут
        else if (cmd.rfind("logs_last", 0) == 0) {
            // rfind с позицией 0 проверяет начало строки (аналог startsWith)
            int minutes = 10; // По умолчанию 10 минут

            // Парсим число после "logs_last "
            if (cmd.size() > 10) {
                try {
                    minutes = std::stoi(cmd.substr(10));
                }
                catch (...) {
                    sendToClient(clientSock, "Ошибка: укажите число минут. Пример: logs_last 30");
                    continue;
                }
            }

            // Считаем время N минут назад
            auto now = std::chrono::system_clock::now();
            auto cutoff = now - std::chrono::minutes(minutes);
            std::time_t cutoffTime = std::chrono::system_clock::to_time_t(cutoff);

            std::lock_guard<std::mutex> lock(logsMutex);
            std::vector<std::string> filtered;

            for (const auto& log : allLogs) {
                // Парсим время из начала строки лога "[2024-01-15 14:23:05]"
                if (log.size() < 21) continue;
                struct tm tm = {};
                // strptime парсит строку по шаблону в структуру tm
                if (strptime(log.c_str() + 1, "%Y-%m-%d %H:%M:%S", &tm)) {
                    time_t logTime = mktime(&tm);
                    if (logTime >= cutoffTime) {
                        filtered.push_back(log);
                    }
                }
            }

            sendToClient(clientSock, "--- ЛОГИ ЗА ПОСЛЕДНИЕ " + std::to_string(minutes) +
                " МИН. (" + std::to_string(filtered.size()) + " записей) ---");
            for (const auto& log : filtered) {
                sendToClient(clientSock, log);
            }
            sendToClient(clientSock, "--- КОНЕЦ ---");
        }

        // Команда: clear_warnings
        else if (cmd == "clear_warnings") {
            std::lock_guard<std::mutex> lock(logsMutex);
            int count = allWarnings.size();
            allWarnings.clear();
            addLog("[INFO] Предупреждения сброшены клиентом " + clientIP);
            sendToClient(clientSock, "Сброшено " + std::to_string(count) + " предупреждений.");
        }

        // Команда: line_sensor
        else if (cmd == "line_sensor") {
            std::lock_guard<std::mutex> lock(sensorMutex);
            //std::cout << "[DEBUG2] sending sensor value: " << lastLineSensorValue << std::endl;
            sendToClient(clientSock, "--- ДАТЧИК ЧЁРНОЙ ЛИНИИ ---");
            sendToClient(clientSock, "Последнее значение: " + lastLineSensorValue);
            sendToClient(clientSock, "--- КОНЕЦ ---");
        }

        // Команда: stream_logs
        else if (cmd == "stream_logs") {
            std::lock_guard<std::mutex> lock(streamingMutex);
            // Проверим, не подписан ли уже этот сокет
            if (std::find(streamingClients.begin(), streamingClients.end(), clientSock) == streamingClients.end()) {
                streamingClients.push_back(clientSock);
                sendToClient(clientSock, "=== СТРИМИНГ ЛОГОВ ВКЛЮЧЁН ===");
                sendToClient(clientSock, "Новые логи будут появляться здесь в реальном времени.");
                sendToClient(clientSock, "Для остановки введите 'stop_stream'");
            }
            else {
                sendToClient(clientSock, "Стриминг уже активен.");
            }
        }

        // Команда: stop_stream
        else if (cmd == "stop_stream") {
            std::lock_guard<std::mutex> lock(streamingMutex);
            auto it = std::find(streamingClients.begin(), streamingClients.end(), clientSock);
            if (it != streamingClients.end()) {
                streamingClients.erase(it);
                sendToClient(clientSock, "=== СТРИМИНГ ЛОГОВ ОСТАНОВЛЕН ===");
            }
            else {
                sendToClient(clientSock, "Стриминг не был активен.");
            }
        }

        // Команда: help
        else if (cmd == "help") {
            sendToClient(clientSock, "Доступные команды:");
            sendToClient(clientSock, "  logs            - все логи за всё время");
            sendToClient(clientSock, "  warnings        - все предупреждения");
            sendToClient(clientSock, "  status          - текущее состояние платы");
            sendToClient(clientSock, "  logs_last <N>   - логи за последние N минут");
            sendToClient(clientSock, "  clear_warnings  - сбросить список предупреждений");
            sendToClient(clientSock, "  line_sensor     - получить данные с датчика");
            sendToClient(clientSock, "  stream_logs     - включить поток логов в реальном времени");
            sendToClient(clientSock, "  stop_stream     - остановить поток логов");
            sendToClient(clientSock, "  exit            - отключиться от сервера");
        }

        // Команда: exit
        else if (cmd == "exit") {
            sendToClient(clientSock, "До свидания!");
            break;
        }

        // Неизвестная команда
        else {
            sendToClient(clientSock, "Неизвестная команда: '" + cmd + "'. Введите 'help'.");
        }
    }
    std::lock_guard<std::mutex> lock(streamingMutex);
    auto it = std::find(streamingClients.begin(), streamingClients.end(), clientSock);
    if (it != streamingClients.end()) streamingClients.erase(it);
    close(clientSock);
}

void serialReadThread() {
    const char* portName = "/dev/ttyACM0";
    int serialFd = open(portName, O_RDWR | O_NOCTTY);
    if (serialFd < 0) {
        addLog("[ERROR] Не удалось открыть последовательный порт " + std::string(portName));
        return;
    }

    // Настройка параметров порта
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(serialFd, &tty) != 0) {
        addLog("[ERROR] tcgetattr");
        close(serialFd);
        return;
    }

    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;     // 8-bit chars
    tty.c_iflag &= ~IGNBRK;                         // disable break processing
    tty.c_lflag = 0;                                // no signaling, echo, etc.
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 0;                            // read doesn't block
    tty.c_cc[VTIME] = 5;                            // 0.5 seconds read timeout

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);         // shut off flow control
    tty.c_cflag |= (CLOCAL | CREAD);                // ignore modem controls
    tty.c_cflag &= ~(PARENB | PARODD);              // no parity
    tty.c_cflag &= ~CSTOPB;                         // 1 stop bit
    tty.c_cflag &= ~CRTSCTS;                        // no hardware flow control

    if (tcsetattr(serialFd, TCSANOW, &tty) != 0) {
        addLog("[ERROR] tcsetattr");
        close(serialFd);
        return;
    }

    addLog("[INFO] Поток чтения датчика линии запущен. Порт: " + std::string(portName));

    char buffer[256];
    std::string accumulated;
    while (sensorThreadRunning) {
        int n = read(serialFd, buffer, sizeof(buffer) - 1);
        if (n > 0) {
            buffer[n] = '\0';
            accumulated += buffer;
            // Ищем полную строку (до \n)
            size_t pos;
            while ((pos = accumulated.find('\n')) != std::string::npos) {
                std::string line = accumulated.substr(0, pos);
                accumulated.erase(0, pos + 1);
                // Удаляем возможные символы \r
                if (!line.empty() && line.back() == '\r') line.pop_back();
                // Сохраняем последнее считанное значение
                if (!line.empty()) {
                    std::lock_guard<std::mutex> lock(sensorMutex);
                    lastLineSensorValue = line;
                    //std::cout << "[DEBUG1] saved sensor value: " << lastLineSensorValue << std::endl;
                    addLog("[SENSOR] Значение датчика: " + line);
                }
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }

    close(serialFd);
    addLog("[INFO] Поток чтения датчика остановлен.");
}

int main() {
    std::cout << "=== Сервер мониторинга Raspberry Pi ===" << std::endl;

    // Запускаем поток мониторинга
    // std::thread создаёт новый поток выполнения
    // .detach() — поток работает независимо от main()
    std::thread monitor(monitoringThread);
    monitor.detach();
    std::thread serialReader(serialReadThread);
    serialReader.detach();

    // Создаём серверный сокет
    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSock < 0) {
        std::cerr << "Ошибка создания сокета" << std::endl;
        return 1;
    }

    // SO_REUSEADDR — позволяет повторно использовать порт
    // сразу после перезапуска сервера (без этого нужно ждать ~2 мин)
    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Ошибка bind" << std::endl;
        return 1;
    }

    listen(serverSock, MAX_CLIENTS);
    addLog("[INFO] Сервер запущен на порту " + std::to_string(PORT));

    // Основной цикл — принимаем клиентов
    while (serverRunning) {
        sockaddr_in clientAddr{};
        socklen_t clientAddrLen = sizeof(clientAddr);

        // accept() блокирует до прихода нового клиента
        int clientSock = accept(serverSock, (sockaddr*)&clientAddr, &clientAddrLen);
        if (clientSock < 0) continue;

        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);

        // Каждый клиент в своём потоке — сервер не блокируется
        std::thread clientThread(handleClient, clientSock, std::string(clientIP));
        clientThread.detach();
    }

    close(serverSock);
    return 0;
}