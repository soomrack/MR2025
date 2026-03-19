#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <string>
#include <stdexcept>
#include <cstdint>
#include <algorithm>
#include <atomic>
#include <map>
#include <cstring>
#include <chrono>
#include <ctime>
#include <sstream>
#include <fstream>
#include <limits>
#include <iomanip>
#include <termios.h>
#include <sys/select.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>

// ============================================================
// MESSAGE
// ============================================================

enum class MessageType : uint32_t {
    Text = 1,
    Connect = 2,
    Disconnect = 3,
    LogRequest = 4,
    LogResponse = 5
};

struct MessageHeader {
    uint32_t type;
    uint32_t size;
};

struct Client {
    int socket;
    std::string color;
    int colorIndex;
    std::string clientId;
    std::string username;
    std::string joinTime;
};

// ============================================================
// GLOBAL
// ============================================================

constexpr int PORT = 54000;

std::vector<Client> clients;
std::mutex clientsMutex;

std::map<std::string, int> usedColors;

std::vector<std::string> colorPool = {
    "\033[31m","\033[32m","\033[33m",
    "\033[34m","\033[35m","\033[36m"
};
const std::string RESET = "\033[0m";

std::atomic<bool> serverRunning{true};
int serverSock = -1;

struct SensorRule {

    std::string name;
    double limit;
    bool greater; // true = больше лимита плохо
};


std::map<std::string, SensorRule> sensorRules = {

    {"TEMP", {"TEMP", 40, true}},
    {"HUM",  {"HUM",  100, true}},
    {"LIGHT", {"LIGHT", 1200, true}}

};

// ===== LOGGING =====
std::mutex logMutex;
std::ofstream logFile;
std::ofstream boardLog;
std::ofstream sensorLog;
std::vector<std::string> logBuffer;
std::vector<std::string> warningBuffer;
std::mutex logBufferMutex;



// ===== MONITORING SETTINGS =====
constexpr int MONITOR_INTERVAL_SEC = 10;      // опрос раз в 10 секунд
constexpr double CPU_TEMP_LIMIT = 70.0;      // порог температуры
constexpr int RAM_LIMIT_PERCENT = 20;       // порог RAM

// ============================================================
// TIME UTILS
// ============================================================

std::string getSystemTimeFull() {
    time_t now = time(nullptr);
    struct tm timeinfo{};
    localtime_r(&now, &timeinfo);

    char buffer[100];
    strftime(buffer, sizeof(buffer),
             "%Y-%m-%d %H:%M:%S", &timeinfo);

    return std::string(buffer);
}

void logEvent(const std::string& text) {
    std::lock_guard<std::mutex> lock(logMutex);

    std::string full =
        "[" + getSystemTimeFull() + "] " + text;

    if (logFile.is_open())
        logFile << full << std::endl;

    {
        std::lock_guard<std::mutex> bufLock(logBufferMutex);
        logBuffer.push_back(full);

        if (text.find("[WARNING]") != std::string::npos)
            warningBuffer.push_back(full);
    }
}
std::string getCurrentTime() {
    time_t now = time(nullptr);
    struct tm timeinfo{};
    localtime_r(&now, &timeinfo);
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
    return std::string(buffer);
}

// ============================================================
// SOCKET UTILS
// ============================================================

void sendAll(int sock, const char* data, int size) {
    int sent = 0;
    while (sent < size) {
        int res = send(sock, data + sent, size - sent, 0);
        if (res <= 0) throw std::runtime_error("send failed");
        sent += res;
    }
}

void recvAll(int sock, char* data, int size) {
    int received = 0;
    while (received < size) {
        int res = recv(sock, data + received, size - received, 0);
        if (res <= 0) throw std::runtime_error("recv failed");
        received += res;
    }
}

std::string getId(int Socket) {
    sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    if (getpeername(Socket, (sockaddr*)&addr, &len) == 0) {
        return std::string(inet_ntoa(addr.sin_addr)) + ":" +
               std::to_string(ntohs(addr.sin_port));
    }
    return "unknown";
}


std::string readLine(int fd) {

    std::string result;
    char c;

    while (read(fd, &c, 1) == 1) {

        if (c == '\n')
            break;

        result += c;
    }

    return result;
}
// ============================================================
// COLOR MANAGEMENT
// ============================================================

int assignColorIndex(const std::string& Id) {
    if (usedColors.count(Id))
        return usedColors[Id];

    std::vector<bool> used(colorPool.size(), false);
    for (auto& p : usedColors)
        if (p.second >= 0 && p.second < (int)colorPool.size())
            used[p.second] = true;

    for (int i = 0; i < (int)colorPool.size(); i++)
        if (!used[i]) {
            usedColors[Id] = i;
            return i;
        }

    return -1;
}

void releaseColorIndex(const std::string& Id) {
    usedColors.erase(Id);
}

// ============================================================
// BROADCAST
// ============================================================

void broadcast(const MessageHeader& header,
               const std::vector<char>& data,
               int sender) {

    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto& c : clients) {
        if (c.socket == sender) continue;
        try {
            sendAll(c.socket, (char*)&header, sizeof(header));
            sendAll(c.socket, data.data(), header.size);
        } catch (...) {}
    }
}


std::vector<std::string> readLogFile(const std::string& filename) {

    std::vector<std::string> lines;
    std::ifstream file(filename);

    if (!file.is_open())
        return lines;

    std::string line;

    while (std::getline(file, line)) {
        lines.push_back(line);
    }

    return lines;
}
// ============================================================
//  THREAD
// ============================================================

void handleClient(int clientSocket) {

    std::string clientId = getId(clientSocket);
    std::string username = "Unknown";
    std::string clientColor;

    try {
        MessageHeader header{};
        recvAll(clientSocket, (char*)&header, sizeof(header));

        if (header.type != static_cast<uint32_t>(MessageType::Connect) || header.size > 256) {
            close(clientSocket);
            return;
        }

        std::vector<char> data(header.size);
        recvAll(clientSocket, data.data(), header.size);
        username = std::string(data.begin(), data.end());

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            for (auto& c : clients)
                if (c.socket == clientSocket) {
                    c.username = username;
                    c.joinTime = getCurrentTime();
                    clientColor = c.color;
                }
        }

        std::cout << clientColor
                  << "Client " << clientId
                  << " joined as '" << username << "'"
                  << RESET << "\n";

        logEvent("Client " + clientId +
                 " joined as '" + username + "'");

        // ===== Основной цикл обработки сообщений =====
        while (serverRunning) {

            recvAll(clientSocket, (char*)&header, sizeof(header));
            if (header.size > 1024 * 1024) break;

            std::vector<char> msgData(header.size);
            recvAll(clientSocket, msgData.data(), header.size);

            if (header.type == static_cast<uint32_t>(MessageType::Text)) {

                std::string message(msgData.begin(), msgData.end());

                std::cout << clientColor
                          << "[" << username << "] "
                          << message
                          << RESET << "\n";

                logEvent("[" + username + "] " + message);

                std::string coloredMessage =
                    clientColor + message + RESET;

                MessageHeader outHeader{
                    static_cast<uint32_t>(MessageType::Text),
                    (uint32_t)coloredMessage.size()
                };

                std::vector<char> outData(
                    coloredMessage.begin(),
                    coloredMessage.end());

                broadcast(outHeader, outData, clientSocket);
            }

            else if (header.type == static_cast<uint32_t>(MessageType::LogRequest)) {

          std::string request(msgData.begin(), msgData.end());

          std::vector<std::string> result;

          if (request == "SERVER") {
               result = readLogFile("server.log");
          }
          else if (request == "BOARD") {
               result = readLogFile("board.log");
          }
           else if (request == "SENSOR") {
             result = readLogFile("sensor.log");
         }
          else if (request == "ALL") {

    auto s1 = readLogFile("server.log");
    auto s2 = readLogFile("board.log");
    auto s3 = readLogFile("sensor.log");

    result.push_back("=== SERVER LOG ===");
    result.insert(result.end(), s1.begin(), s1.end());

    result.push_back("=== BOARD LOG ===");
    result.insert(result.end(), s2.begin(), s2.end());

    result.push_back("=== SENSOR LOG ===");
    result.insert(result.end(), s3.begin(), s3.end());
}
         else if (request == "WARNINGS") {

             auto s1 = readLogFile("server.log");
             auto s2 = readLogFile("board.log");
             auto s3 = readLogFile("sensor.log");

             std::vector<std::string> all;

             all.insert(all.end(), s1.begin(), s1.end());
             all.insert(all.end(), s2.begin(), s2.end());
             all.insert(all.end(), s3.begin(), s3.end());

             for (auto& line : all) {
                 if (line.find("WARNING") != std::string::npos)
                      result.push_back(line);
             }
         }
         else if (request.rfind("LAST ", 0) == 0) {

              int minutes = std::stoi(request.substr(5));
              time_t now = time(nullptr);

              auto s1 = readLogFile("server.log");
              auto s2 = readLogFile("board.log");
              auto s3 = readLogFile("sensor.log");

             std::vector<std::string> all;

              all.insert(all.end(), s1.begin(), s1.end());
              all.insert(all.end(), s2.begin(), s2.end());
              all.insert(all.end(), s3.begin(), s3.end());

             for (auto& line : all) {

                 if (line.size() < 20)
                     continue;

                 std::tm tm{};
                 std::istringstream ss(line.substr(1, 19));
                ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");

                 time_t logTime = mktime(&tm);

                 if (difftime(now, logTime) <= minutes * 60)
                     result.push_back(line);
              }
         }

          std::ostringstream joined;

         for (auto& l : result)
              joined << l << "\n";

         std::string output = joined.str();

         MessageHeader outHeader{
             static_cast<uint32_t>(MessageType::LogResponse),
              (uint32_t)output.size()
          };

          sendAll(clientSocket, (char*)&outHeader, sizeof(outHeader));
          sendAll(clientSocket, output.data(), output.size());
      }

            else if (header.type == static_cast<uint32_t>(MessageType::Disconnect)) {
                break;
            }
        } // <- конец while(serverRunning)

    } catch (const std::exception& e) {
        std::cerr << "Client " << clientId << " disconnected (error: "
                  << e.what() << ")\n";
        logEvent("Client " + clientId + " disconnected (error: " + e.what() + ")");
    }

    // ===== Очистка после отключения =====
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        auto it = std::remove_if(clients.begin(), clients.end(),
                                 [clientSocket](const Client& c){ return c.socket == clientSocket; });
        clients.erase(it, clients.end());
    }

    releaseColorIndex(clientId);
    close(clientSocket);
}
// ============================================================
// COMMAND THREAD
// ============================================================

void printCommands() {
    std::cout << "\n/shutdown\n/status\n/clients\n/colors\n/help\n/time\n/uptime\n/ram\n/cpu\n\n";
}

std::string getUptime() {
    std::ifstream file("/proc/uptime");
    double seconds;
    file >> seconds;

    int hrs = seconds / 3600;
    int mins = ((int)seconds % 3600) / 60;
    int secs = (int)seconds % 60;

    std::ostringstream oss;
    oss << hrs << "h " << mins << "m " << secs << "s";
    return oss.str();
}

std::string getRAMUsage() {
    std::ifstream file("/proc/meminfo");
    std::string key;
    long total = 0, available = 0;

    while (file >> key) {
        if (key == "MemTotal:") file >> total;
        else if (key == "MemAvailable:") {
            file >> available;
            break;
        }
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    long used = total - available;
    int percent = (used * 100) / total;

    std::ostringstream oss;
    oss << percent << "% (" << used/1024
        << "MB / " << total/1024 << "MB)";
    return oss.str();
}

std::string getCPUTemp() {
    std::ifstream file("/sys/class/thermal/thermal_zone0/temp");
    if (!file.is_open()) return "Not available";

    long temp;
    file >> temp;
    double celsius = temp / 1000.0;

    std::ostringstream oss;
    oss << celsius << " C";
    return oss.str();
}


void handleShutdown() {
    logEvent("Command: /shutdown");
    std::cout << "Shutting down server...\n";
    serverRunning = false;
}


void handleStatus() {
    std::lock_guard<std::mutex> lock(clientsMutex);
    std::cout << "Clients: " << clients.size() << "\n";
    logEvent("Command: /status (clients: " +
             std::to_string(clients.size()) + ")");
}


void handleTime() {
    std::string t = getSystemTimeFull();
    std::cout << "System time: " << t << "\n";
    logEvent("Command: /time -> " + t);
}


void handleUptime() {
    std::string up = getUptime();
    std::cout << "Uptime: " << up << "\n";
    logEvent("Command: /uptime -> " + up);
}


void handleRAM() {
    std::string ram = getRAMUsage();
    std::cout << "RAM usage: " << ram << "\n";
    logEvent("Command: /ram -> " + ram);
}


void handleCPU() {
    std::string cpu = getCPUTemp();
    std::cout << "CPU temp: " << cpu << "\n";
    logEvent("Command: /cpu -> " + cpu);
}

void commandHandler() {
    std::string cmd;

    while (serverRunning) {
        std::getline(std::cin, cmd);

        if (cmd == "/shutdown") {
            handleShutdown();
            break;
        }
        else if (cmd == "/status") {
            handleStatus();
        }
        else if (cmd == "/time") {
            handleTime();
        }
        else if (cmd == "/uptime") {
            handleUptime();
        }
        else if (cmd == "/ram") {
            handleRAM();
        }
        else if (cmd == "/cpu") {
            handleCPU();
        }
        else if (cmd == "/help") {
            printCommands();
        }
        else if (!cmd.empty()) {
            std::cout << "Unknown command. Type /help\n";
        }
    }
}

// ============================================================
// Monitoring THREAD
// ============================================================
int getRAMUsagePercent() {
    std::ifstream file("/proc/meminfo");
    std::string key;
    long total = 0, available = 0;

    while (file >> key) {
        if (key == "MemTotal:") file >> total;
        else if (key == "MemAvailable:") {
            file >> available;
            break;
        }
        file.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }

    long used = total - available;
    return (used * 100) / total;
}


double getCPUTempValue() {
    std::ifstream file("/sys/class/thermal/thermal_zone0/temp");
    if (!file.is_open()) return -1;

    long temp;
    file >> temp;
    return temp / 1000.0;
}


void monitoringLoop() {

    while (serverRunning) {

        double cpuTemp = getCPUTempValue();
        int ramPercent = getRAMUsagePercent();
        std::string uptime = getUptime();

        std::ostringstream logLine;
        logLine << "MONITOR | CPU: " << cpuTemp
                << " C | RAM: " << ramPercent
                << "% | Uptime: " << uptime;

        logEvent(logLine.str());
        if (boardLog.is_open())
            boardLog << "[" << getSystemTimeFull() << "] "
                << logLine.str() << std::endl;

        // ===== Проверка порогов =====
        bool warning = false;
        std::string warningMsg;

        if (cpuTemp > CPU_TEMP_LIMIT) {
            warning = true;
            warningMsg += "CPU TEMP HIGH (" + std::to_string(cpuTemp) + " C) ";
        }

        if (ramPercent > RAM_LIMIT_PERCENT) {
            warning = true;
            warningMsg += "RAM USAGE HIGH (" + std::to_string(ramPercent) + "%)";
        }

        if (warning) {

            std::string fullMsg = "[WARNING] " + warningMsg;

            std::cout << "\033[31m" << fullMsg << RESET << "\n";
            logEvent(fullMsg);

            // отправка всем клиентам
            MessageHeader header{
               static_cast<uint32_t>(MessageType::Text),
               (uint32_t)fullMsg.size()
            };

            std::vector<char> data(fullMsg.begin(), fullMsg.end());
            broadcast(header, data, -1);
        }

        std::this_thread::sleep_for(
            std::chrono::seconds(MONITOR_INTERVAL_SEC));
    }
}


// ============================================================
// SENSOR THREAD
// ============================================================

void processSensorValue(const std::string& name, double value)
{
    if (!sensorRules.count(name))
        return;

    auto rule = sensorRules[name];

    bool warning = false;

    if (rule.greater && value > rule.limit)
        warning = true;

    if (!rule.greater && value < rule.limit)
        warning = true;

    if (warning)
    {
        std::string msg =
        "[WARNING] SENSOR | " + name +
        " LIMIT (" + std::to_string(value) + ")";

        std::cout << "\033[31m" << msg << RESET << "\n";
        logEvent(msg);

        MessageHeader header{
        static_cast<uint32_t>(MessageType::Text),
        (uint32_t)msg.size()
        };

        std::vector<char> data(msg.begin(), msg.end());
        broadcast(header, data, -1);
    }
}


void parseSensorLine(const std::string& line)
{
    std::istringstream ss(line);
    std::string token;

    while (ss >> token)
    {
        auto pos = token.find(':');
        if (pos == std::string::npos)
            continue;

        std::string name = token.substr(0, pos);

        try {
            double value = std::stod(token.substr(pos + 1));
            processSensorValue(name, value);
        } catch (...) {
            continue;
        }
    }
}

// ================= UART SETUP =================
void setupSerial(int fd) {
    struct termios tty{};

    if (tcgetattr(fd, &tty) != 0)
        return;

    cfmakeraw(&tty); // ВАЖНО

    cfsetispeed(&tty, B9600);
    cfsetospeed(&tty, B9600);

    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag |= CS8;

    tcsetattr(fd, TCSANOW, &tty);
}


// ================= SAFE READ LINE =================
bool readLineSafe(int fd, std::string& out) {
    out.clear();
    char c;

    fd_set set;
    struct timeval timeout;

    while (true) {
        FD_ZERO(&set);
        FD_SET(fd, &set);

        timeout.tv_sec = 2;
        timeout.tv_usec = 0;

        int rv = select(fd + 1, &set, nullptr, nullptr, &timeout);

        if (rv == -1) {
            return false; // ошибка
        }
        else if (rv == 0) {
            return false; // таймаут
        }

        int res = read(fd, &c, 1);

        if (res <= 0) {
            if (errno == EIO || errno == ENODEV) {
                std::cerr << "Arduino disconnected\n";
                return false;
            }
            continue;
        }

        if (c == '\n')
            return true;

        out += c;
    }
}


// ================= SENSOR LOOP =================
void sensorLoop() {

    int serial = -1;

    bool simulateSensors = false; // ВАЖНО: выключи симуляцию

    // ===== SIMULATION =====
    if (simulateSensors) {
        while (serverRunning) {

            std::ostringstream fake;
            int temp = 20 + rand() % 10;
            int humidity = 40 + rand() % 20;
            int light = 300 + rand() % 900;

            fake << "TEMP:" << temp
                 << " HUM:" << humidity
                 << " LIGHT:" << light;

            std::string full =
                "[" + getSystemTimeFull() + "] SENSOR | " + fake.str();

            logEvent(full);
            parseSensorLine(fake.str());

            if (sensorLog.is_open())
                sensorLog << full << std::endl;

            std::this_thread::sleep_for(
                std::chrono::seconds(MONITOR_INTERVAL_SEC));
        }
        return;
    }

    // ===== REAL ARDUINO MODE =====
    while (serverRunning) {

        // ===== Подключение =====
        if (serial < 0) {

            serial = open("/dev/ttyUSB0", O_RDWR | O_NOCTTY | O_NONBLOCK);

            if (serial < 0) {
                std::cerr << "Waiting for Arduino...\n";
                std::this_thread::sleep_for(std::chrono::seconds(2));
                continue;
            }

            setupSerial(serial);
            std::this_thread::sleep_for(std::chrono::seconds(2)); //  важно
            std::cout << "Arduino connected\n";
            logEvent("Arduino connected");
        }

        // ===== Чтение =====
        std::string line;

        bool ok = readLineSafe(serial, line);

        if (!ok) {
            close(serial);
            serial = -1;
            std::cerr << "Reconnecting Arduino...\n";
            logEvent("Arduino disconnected, retrying...");
            std::this_thread::sleep_for(std::chrono::seconds(2));
            continue;
        }

        if (line.empty())
            continue;

        // ===== Лог =====
        std::string full =
            "[" + getSystemTimeFull() + "] SENSOR | " + line;

        logEvent(full);

        if (sensorLog.is_open())
            sensorLog << full << std::endl;

        // ===== Парсинг =====
        try {
            parseSensorLine(line);
        } catch (...) {
            std::cerr << "Parse error: " << line << "\n";
        }
    }

    if (serial >= 0)
        close(serial);
}
// ============================================================
// ACCEPT LOOP
// ============================================================

void runAcceptLoop() {

    fcntl(serverSock, F_SETFL, O_NONBLOCK);

    while (serverRunning) {

        int client = accept(serverSock, nullptr, nullptr);

        if (client < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(100));
                continue;
            }
            break;
        }

        std::string id = getId(client);

        Client newClient;
        newClient.socket = client;
        newClient.clientId = id;
        newClient.username = "Pending...";
        newClient.joinTime = "";
        newClient.colorIndex = assignColorIndex(id);

        if (newClient.colorIndex >= 0)
            newClient.color = colorPool[newClient.colorIndex];
        else
            newClient.color = "\033[37m";

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(newClient);
            std::cout << "New connection from "
                      << id << "\n";
        }

        logEvent("New connection from " + id);

        std::thread(handleClient, client).detach();
    }
}

// ============================================================
// Server Initialization
// ============================================================

void initializeServer() {

    // Логирование
    logFile.open("server.log", std::ios::app);
    boardLog.open("board.log", std::ios::app);
    sensorLog.open("sensor.log", std::ios::app);
    logEvent("=== Server started ===");

    // Создание сокета
    serverSock = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSock, (sockaddr*)&addr, sizeof(addr));
    listen(serverSock, SOMAXCONN);
}


// ============================================================
// Server Shutdown
// ============================================================
void shutdownServer(std::thread& cmdThread) {

    if (cmdThread.joinable())
        cmdThread.join();

    logEvent("=== Server stopped ===");
    logFile.close();
    boardLog.close();
    sensorLog.close();

    close(serverSock);
}
// ============================================================
// MAIN
// ============================================================

int main() {

    initializeServer();

    printCommands();

    srand(time(nullptr));

    std::thread cmdThread(commandHandler);
    std::thread monitorThread(monitoringLoop);
    std::thread sensorThread(sensorLoop);

    runAcceptLoop();

    if (monitorThread.joinable())
         monitorThread.join();

    if (sensorThread.joinable())
    sensorThread.join();
    shutdownServer(cmdThread);

    return 0;
}
