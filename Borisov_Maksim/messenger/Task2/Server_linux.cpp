// ============================================================
// TCP Сервер мониторинга для Raspberry Pi
// Компилируется на Linux: g++ server.cpp -o server -lpthread
// ============================================================

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

// Linux сетевые заголовки (не Winsock — это Linux!)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>      // Для close(), read()

using namespace std;

// ============================================================
// КОНСТАНТЫ
// ============================================================
const int    PORT = 54000;
const int    MAX_CLIENTS = 5;
const int    MONITOR_INTERVAL = 5;   // Секунд между проверками
const string LOG_FILE = "server_logs.txt";

// Пороговые значения для предупреждений
const float  CPU_WARN_THRESHOLD = 85.0f;  // %
const float  RAM_WARN_THRESHOLD = 85.0f;  // %
const float  TEMP_WARN_THRESHOLD = 70.0f;  // °C
const float  DISK_WARN_THRESHOLD = 90.0f;  // %

// ============================================================
// ГЛОБАЛЬНЫЕ ДАННЫЕ (доступны из разных потоков)
// mutex — это "замок": только один поток за раз может
// читать/писать защищённые данные, иначе будет "гонка данных"
// ============================================================
std::mutex logsMutex;                  // Замок для логов
std::vector<std::string> allLogs;      // Все логи в памяти
std::vector<std::string> allWarnings;  // Все предупреждения
std::atomic<bool> serverRunning(true); // Флаг работы сервера
// atomic — безопасно менять из разных потоков

// ============================================================
// ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================

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

    // lock_guard автоматически берёт и отпускает замок
    // Пока lock_guard жив — другие потоки ждут
    std::lock_guard<std::mutex> lock(logsMutex);
    allLogs.push_back(entry);

    // Пишем в файл сразу (дозапись в конец)
    std::ofstream file(LOG_FILE, std::ios::app);
    if (file.is_open()) {
        file << entry << "\n";
    }

    // Выводим в консоль сервера
    std::cout << entry << std::endl;
}

// Добавляет предупреждение (в оба списка)
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
}

// ============================================================
// ЧТЕНИЕ СИСТЕМНЫХ ДАННЫХ RASPBERRY PI
// На Linux системная информация читается из /proc/ — это
// виртуальная файловая система, файлы в ней создаёт ядро
// ============================================================

// Читает загрузку CPU из /proc/stat
// /proc/stat содержит счётчики времени процессора
float getCpuUsage() {
    // Читаем дважды с паузой и считаем разницу
    // (CPU usage это процент времени НЕ в режиме idle)
    static long prevIdle = 0, prevTotal = 0;

    std::ifstream file("/proc/stat");
    std::string line;
    std::getline(file, line); // Первая строка: "cpu  user nice system idle ..."

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

// Читает использование RAM из /proc/meminfo
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

// Читает температуру CPU Raspberry Pi
// Raspberry Pi предоставляет температуру в этом файле
float getCpuTemperature() {
    std::ifstream file("/sys/class/thermal/thermal_zone0/temp");
    long temp = 0;
    file >> temp;
    return temp / 1000.0f; // Файл хранит температуру в милли-градусах
}

// Читает использование диска через команду df
float getDiskUsage() {
    // popen — запускает команду и читает её вывод как файл
    FILE* pipe = popen("df / | tail -1 | awk '{print $5}'", "r");
    if (!pipe) return 0.0f;
    char buf[16];
    fgets(buf, sizeof(buf), pipe);
    pclose(pipe);
    // buf содержит что-то вроде "45%\n" — убираем % и конвертируем
    return std::stof(std::string(buf));
}

// Читает uptime (время работы системы) из /proc/uptime
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

// ============================================================
// ПОТОК МОНИТОРИНГА
// Эта функция запускается в отдельном потоке и постоянно
// проверяет состояние системы
// ============================================================
void monitoringThread() {
    addLog("[INFO] Поток мониторинга запущен.");

    // Первый вызов getCpuUsage() даёт некорректный результат
    // (нет предыдущих данных для сравнения), делаем "прогрев"
    getCpuUsage();
    std::this_thread::sleep_for(std::chrono::seconds(1));

    while (serverRunning) {
        float cpu = getCpuUsage();
        float ram = getRamUsage();
        float temp = getCpuTemperature();
        float disk = getDiskUsage();
        std::string uptime = getUptime();

        // Формируем строку статуса
        std::ostringstream status;
        status << "[INFO] Статус: CPU=" << cpu << "% | RAM=" << ram
            << "% | Temp=" << temp << "C | Disk=" << disk
            << "% | Uptime=" << uptime;
        addLog(status.str());

        // Проверяем пороговые значения и генерируем предупреждения
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

        // Ждём MONITOR_INTERVAL секунд до следующей проверки
        std::this_thread::sleep_for(std::chrono::seconds(MONITOR_INTERVAL));
    }
}

// ============================================================
// ОБРАБОТКА КОМАНД КЛИЕНТА
// Каждый клиент обслуживается в своём потоке
// ============================================================

// Отправляет строку клиенту (добавляет \n в конец)
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

        // ————————————————————————————————
        // Команда: logs — все логи
        // ————————————————————————————————
        if (cmd == "logs") {
            std::lock_guard<std::mutex> lock(logsMutex);
            sendToClient(clientSock, "--- НАЧАЛО ЛОГОВ (" + std::to_string(allLogs.size()) + " записей) ---");
            for (const auto& log : allLogs) {
                sendToClient(clientSock, log);
            }
            sendToClient(clientSock, "--- КОНЕЦ ЛОГОВ ---");
        }

        // ————————————————————————————————
        // Команда: warnings — все предупреждения
        // ————————————————————————————————
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

        // ————————————————————————————————
        // Команда: status — текущее состояние
        // ————————————————————————————————
        else if (cmd == "status") {
            // Снимаем показания прямо сейчас
            float cpu = getCpuUsage();
            float ram = getRamUsage();
            float temp = getCpuTemperature();
            float disk = getDiskUsage();
            std::string uptime = getUptime();

            sendToClient(clientSock, "--- ТЕКУЩЕЕ СОСТОЯНИЕ ---");

            // Лямбда-функция для отображения "статуса" по порогу
            // [&] означает что лямбда может использовать переменные из внешней области
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

        // ————————————————————————————————
        // Команда: logs_last <N> — логи за N минут
        // ————————————————————————————————
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

        // ————————————————————————————————
        // Команда: clear_warnings
        // ————————————————————————————————
        else if (cmd == "clear_warnings") {
            std::lock_guard<std::mutex> lock(logsMutex);
            int count = allWarnings.size();
            allWarnings.clear();
            addLog("[INFO] Предупреждения сброшены клиентом " + clientIP);
            sendToClient(clientSock, "Сброшено " + std::to_string(count) + " предупреждений.");
        }

        // ————————————————————————————————
        // Команда: help
        // ————————————————————————————————
        else if (cmd == "help") {
            sendToClient(clientSock, "Доступные команды:");
            sendToClient(clientSock, "  logs            - все логи за всё время");
            sendToClient(clientSock, "  warnings        - все предупреждения");
            sendToClient(clientSock, "  status          - текущее состояние платы");
            sendToClient(clientSock, "  logs_last <N>   - логи за последние N минут");
            sendToClient(clientSock, "  clear_warnings  - сбросить список предупреждений");
            sendToClient(clientSock, "  exit            - отключиться от сервера");
        }

        // ————————————————————————————————
        // Команда: exit
        // ————————————————————————————————
        else if (cmd == "exit") {
            sendToClient(clientSock, "До свидания!");
            break;
        }

        // ————————————————————————————————
        // Неизвестная команда
        // ————————————————————————————————
        else {
            sendToClient(clientSock, "Неизвестная команда: '" + cmd + "'. Введите 'help'.");
        }
    }

    close(clientSock);
}

// ============================================================
// ТОЧКА ВХОДА — main()
// ============================================================
int main() {
    std::cout << "=== Сервер мониторинга Raspberry Pi ===" << std::endl;

    // Запускаем поток мониторинга
    // std::thread создаёт новый поток выполнения
    // .detach() — поток работает независимо от main()
    std::thread monitor(monitoringThread);
    monitor.detach();

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