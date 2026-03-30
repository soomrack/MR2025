#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <vector>
#include <sys/select.h>
#include <fstream>
#include <iomanip>
#include <ctime>
#include <limits>
#include <thread>
#include <atomic>
#include <chrono>
#include <algorithm>
#include <cmath>
#include <termios.h>  // для настройки serial
#include <fcntl.h>    // для serial open
#include <sys/stat.h>
#include <sys/types.h>

// Глобальные переменные для Arduino
int arduino_fd = -1;
std::atomic<bool> arduino_connected{false};

const int MAX_CLIENTS = 10;
static int client_fd[MAX_CLIENTS];
std::atomic<bool> serverRunning{true};

// Логирование
std::vector<std::string> logEvents;
enum LogLevel { OFF, INFO, WARN };
std::string logLevelNames[] = {"OFF", "INFO", "WARN"};
std::vector<int> clientLogLevel(MAX_CLIENTS, OFF);  // статус мониторинга для каждого клиента

LogLevel parseLogLevel(const std::string& levelStr) {
    if (levelStr == "off") return OFF;
    if (levelStr == "info") return INFO;
    if (levelStr == "warn") return WARN;
    return OFF;  // по умолчанию
}

void send_to_client(std::string line, int this_client_fd) {
    if (this_client_fd >= 0) {
        send(this_client_fd, line.c_str(), line.size(), 0);
    }
}

std::string getSystemTimeFull() {
    time_t now = time(nullptr);
    struct tm timeinfo{};
    localtime_r(&now, &timeinfo);
    char buffer[100];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return std::string(buffer);
}

void logEvent(LogLevel level, const std::string& msg) {
    std::string fullMsg = "[" + std::string(logLevelNames[level]) + "] " + getSystemTimeFull() + " " + msg;
    logEvents.push_back(fullMsg);
    std::cout << fullMsg << std::endl;
    
    // рассылаем только клиентам с включённым мониторингом
    for (int k = 0; k < MAX_CLIENTS; ++k) {
        if (client_fd[k] >= 0 && clientLogLevel[k] != OFF && level >= clientLogLevel[k]) {
            std::string toSend = fullMsg + "\n";
            send_to_client(toSend, client_fd[k]);
        }
    }
}

LogLevel parseLogHistoryLevelFromString(const std::string& logLine) {
    size_t start = logLine.find('[');
    if (start == std::string::npos) return OFF;
    
    size_t end = logLine.find(']', start + 1);
    if (end == std::string::npos) return OFF;
    
    std::string levelStr = logLine.substr(start + 1, end - start - 1);
    std::transform(levelStr.begin(), levelStr.end(), levelStr.begin(), 
                  [](unsigned char c){ return std::tolower(c); }
                  ); // convert to lowercase

    return parseLogLevel(levelStr);
}

void setupSerial(int fd) {
    struct termios tty{};
    if (tcgetattr(fd, &tty) != 0) return;
    
    cfmakeraw(&tty);
    cfsetispeed(&tty, B9600);  // скорость Arduino
    cfsetospeed(&tty, B9600);
    
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag |= CS8;
    
    tcsetattr(fd, TCSANOW, &tty);
}

bool readLineArduino(int fd, std::string& line) {
    line.clear();
    char c;
    
    while (true) {
        fd_set set;
        struct timeval timeout{0, 100000};  // таймаут 100мс
        
        FD_ZERO(&set);
        FD_SET(fd, &set);
        
        int rv = select(fd + 1, &set, nullptr, nullptr, &timeout);
        if (rv == -1) return false;  // ошибка
        if (rv == 0) return false;   // таймаут
        
        ssize_t res = read(fd, &c, 1);
        if (res <= 0) return false;
        
        if (c == '\n') return true;
        line += c;
    }
}

std::string findArduinoPort() {
    // Возможные пути Arduino (по убыванию приоритета)
    std::vector<std::string> candidates = {
        "/dev/serial/by-id/usb-Arduino_*",
        "/dev/ttyACM0",
        "/dev/ttyACM1", 
        "/dev/ttyUSB0",
        "/dev/ttyUSB1"
    };
    
    for (const auto& path : candidates) {
        struct stat st;
        if (stat(path.c_str(), &st) == 0) {
            return path;
        }
    }
    return "";  // не найдено
}

void arduino_loop() {
    auto last_data_time = std::chrono::steady_clock::now(); // время последнего получения данных
    constexpr int SILENCE_TIMEOUT_SEC = 5;

    while (serverRunning) {
        // Подключение
        if (arduino_fd < 0) {
            
            std::string port = findArduinoPort();
            if (port.empty()) {
                // ожидаем подключения
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }
            arduino_fd = open(port.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
            if (arduino_fd < 0) {
                // не удалось подключиться
                std::this_thread::sleep_for(std::chrono::seconds(1));
                continue;
            }
            
            setupSerial(arduino_fd);
            std::this_thread::sleep_for(std::chrono::seconds(2));  // стабилизация
            arduino_connected = true;
            logEvent(INFO, "Arduino подключён ("+ port +")");
            // std::cout << "Arduino подключён\n";
        }
        
        // Чтение строки
        std::string arduino_data;
        bool data_received = readLineArduino(arduino_fd, arduino_data);
        auto now = std::chrono::steady_clock::now();
        auto silence_duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_data_time).count();
        if (data_received) {
            // Данные получены (даже если пустые)
            last_data_time = now; // обновляем время
            if (!arduino_data.empty()) {
                // Логируем данные от Arduino
                std::string log_msg = "Arduino: " + arduino_data;
                logEvent(INFO, log_msg);
                std::cout << "[Arduino] " << arduino_data << std::endl;
            }
        } else if (silence_duration >= SILENCE_TIMEOUT_SEC) {
            // Отключено (таймаут) — переподключение
            close(arduino_fd);
            arduino_fd = -1;
            arduino_connected = false;
            logEvent(WARN, "Arduino отключён, переподключение...");
            std::cout << "Arduino отключён, переподключение...\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }
    }
    
    if (arduino_fd >= 0) {
        close(arduino_fd);
        arduino_fd = -1;
    }
}

bool handle_client_command(std::string msg, int client_index) {
    int this_client_fd = client_fd[client_index];

    if (msg == "/users") {
        int count = 0;
        for (int i=0; i<MAX_CLIENTS; i++) {
            if (client_fd[i]>=0) count++;
        }

        std::string toSend = "Всего клиентов: " + std::to_string(count) + "\n";
        send_to_client(toSend, this_client_fd);
    }
    else if (msg.substr(0, 6) == "/logs ") {
        // выбор уровня логирования для клиента

        LogLevel newLevel = INFO; // default level
        if (msg.size() > 7) {
            std::string levelStr = msg.substr(6);
            newLevel = parseLogLevel(levelStr);
        }

        clientLogLevel[client_index] = newLevel;
        std::string levelName = logLevelNames[newLevel];
        std::string toSend = "Отображение логов: " + levelName + "\n";
        send_to_client(toSend, this_client_fd);
    }
    else if (msg.substr(0, 12) == "/logshistory") {
        // отображение истории

        LogLevel filterLevel = INFO;
        if (msg.size() > 12 && msg[12] == ' ') {
            filterLevel = parseLogLevel(msg.substr(13));
        }
        
        std::string history = "=== LOG HISTORY (level >= " + logLevelNames[filterLevel] + ") ===\n";
        for (const auto& log : logEvents) {
            LogLevel logLevel = parseLogHistoryLevelFromString(log);
            if (logLevel >= filterLevel) {
                history += log + "\n";
            }
        }
        send_to_client(history, this_client_fd);
    }
    else if (msg == "/p") {
        logEvent(WARN, "Пышки закончились");
    }
    else {
        send_to_client("Unknown command\n", this_client_fd);
    }
    return 0;
}

void handle_chat_message(std::string msg, const int client_index) {
    std::string msg_cropped=msg;
    while (msg_cropped.back() == '\n') {
        msg_cropped.pop_back();
    }
    while (!msg_cropped.empty() && msg_cropped.back() == ' ') {  // убираем пробелы по краям
        msg_cropped.pop_back();
    }
    
    if (!msg_cropped.empty() && msg_cropped.at(0) == '/') {
        // команда с клиента
        if ( !handle_client_command(msg_cropped, client_index) ) return; // skip sending to other clients only if returned 0
    }

    std::cout << "Client " << client_index << ": " << msg << std::endl;

    // рассылаем сообщение всем клиентам
    std::string toSend = "Client " + std::to_string(client_index) + ": " + msg + "\n";
    for (int k = 0; k < MAX_CLIENTS; ++k) {
        send_to_client(toSend, client_fd[k]);
    }
}

int getRAMUsagePercent() {
    std::ifstream file("/proc/meminfo");
    std::string key;
    long total = 0, available = 0;
    while (file >> key) {
        if (key == "MemTotal:") file >> total;
        else if (key == "MemAvailable:") { file >> available; break; }
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    long used = total - available;
    return (used * 100) / total;
}

float getCPUTemp() {
    std::ifstream file("/sys/class/thermal/thermal_zone0/temp");
    if (!file.is_open()) return 0; // Not available

    long temp;
    file >> temp;
    float celsius = temp / 1000.0;

    // std::ostringstream oss;
    // oss << celsius << " C";
    return celsius;
}

void monitoring_loop() {
    while (serverRunning) {
        int ram = getRAMUsagePercent();
        int cpu = std::round(getCPUTemp());
        logEvent(INFO, "Monitor: RAM " + std::to_string(ram) + "% CPU temp " + std::to_string(cpu) + " °C");
        
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    // чтобы быстро перезапускать сервер
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        return 1;
    }

    std::cout << "Server is listening on port 8080...\n";
    logEvent(INFO, "Server started");

    // ветка мониторинга и логирования
    std::thread monitorThread(monitoring_loop);
    monitorThread.detach();

    // Arduino thread
    std::thread arduinoThread(arduino_loop);
    arduinoThread.detach();

    // массив клиентов и буфер текущего сообщения для каждого
    std::string clientBuffer[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        client_fd[i] = -1;
        clientBuffer[i].clear();
    }

    fd_set readfds;

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        int max_fd = server_fd;
        if (STDIN_FILENO > max_fd) max_fd = STDIN_FILENO;

        // добавить клиентов в набор
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            int fd = client_fd[i];
            if (fd >= 0) {
                FD_SET(fd, &readfds);
                if (fd > max_fd) max_fd = fd;
            }
        }

        int activity = select(max_fd + 1, &readfds, nullptr, nullptr, nullptr);
        if (activity < 0) {
            perror("select");
            serverRunning = false;
            break;
        }
        
        // Завершение работы по Ctrl+D
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char c;
            if (read(STDIN_FILENO, &c, 1) > 0) {
                // if (c == 'q') {  // 'q' + Enter
                //     std::cout << "Сервер завершается...\n";
                //     serverRunning = false;
                //     break;
                // }
            } else {
                std::cout << "Сервер завершается (Ctrl+D)...\n";
                serverRunning = false;
                break;
            }
        }

        // новое подключение
        if (FD_ISSET(server_fd, &readfds)) {
            int new_fd = accept(server_fd, nullptr, nullptr);
            if (new_fd < 0) {
                perror("accept");
            } else {
                bool added = false;
                for (int i = 0; i < MAX_CLIENTS; ++i) {
                    if (client_fd[i] < 0) {
                        client_fd[i] = new_fd;
                        clientBuffer[i].clear();
                        std::cout << "New client connected, slot " << i << ", fd=" << new_fd << "\n";
                        added = true;
                        break;
                    }
                }
                if (!added) {
                    std::cout << "Too many clients, rejecting connection\n";
                    close(new_fd);
                }
            }
        }

        // данные от клиентов
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            int fd = client_fd[i];
            if (fd < 0) continue;

            if (FD_ISSET(fd, &readfds)) {
                char buffer[1024];
                ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);
                if (bytes <= 0) {
                    if (bytes < 0) perror("recv");
                    std::cout << "Client " << i << " disconnected\n";
                    close(fd);
                    client_fd[i] = -1;
                    clientBuffer[i].clear();
                    continue;
                }

                // накапливаем и разбиваем по '\n'
                for (ssize_t j = 0; j < bytes; ++j) {
                    char c = buffer[j];
                    if (c == '\n') {
                        std::string msg = clientBuffer[i];
                        clientBuffer[i].clear();

                        handle_chat_message(msg, i);
                    } else {
                        clientBuffer[i].push_back(c);
                    }
                }
            }
        }
    }

    // закрываем всё
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_fd[i] >= 0) close(client_fd[i]);
    }
    close(server_fd);
    serverRunning = false;

    return 0;
}
