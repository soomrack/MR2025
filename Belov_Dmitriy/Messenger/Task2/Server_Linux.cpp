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
    Disconnect = 3
};

struct MessageHeader {
    MessageType type;
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

// ===== LOGGING =====
std::mutex logMutex;
std::ofstream logFile;


// ===== MONITORING SETTINGS =====
constexpr int MONITOR_INTERVAL_SEC = 10;      // опрос раз в 10 секунд
constexpr double CPU_TEMP_LIMIT = 70.0;      // порог температуры
constexpr int RAM_LIMIT_PERCENT = 80;        // порог RAM
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
    if (logFile.is_open())
        logFile << "[" << getSystemTimeFull() << "] "
                << text << std::endl;
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

std::string getClientId(int clientSocket) {
    sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    if (getpeername(clientSocket, (sockaddr*)&addr, &len) == 0) {
        return std::string(inet_ntoa(addr.sin_addr)) + ":" +
               std::to_string(ntohs(addr.sin_port));
    }
    return "unknown";
}

// ============================================================
// COLOR MANAGEMENT
// ============================================================

int assignColorIndex(const std::string& clientId) {
    if (usedColors.count(clientId))
        return usedColors[clientId];

    std::vector<bool> used(colorPool.size(), false);
    for (auto& p : usedColors)
        if (p.second >= 0 && p.second < (int)colorPool.size())
            used[p.second] = true;

    for (int i = 0; i < (int)colorPool.size(); i++)
        if (!used[i]) {
            usedColors[clientId] = i;
            return i;
        }

    return -1;
}

void releaseColorIndex(const std::string& clientId) {
    usedColors.erase(clientId);
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

// ============================================================
// CLIENT THREAD
// ============================================================

void handleClient(int clientSocket) {

    std::string clientId = getClientId(clientSocket);
    std::string username = "Unknown";
    std::string clientColor;

    try {
        MessageHeader header{};
        recvAll(clientSocket, (char*)&header, sizeof(header));

        if (header.type != MessageType::Connect || header.size > 256) {
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

        while (serverRunning) {

            recvAll(clientSocket, (char*)&header, sizeof(header));
            if (header.size > 1024 * 1024) break;

            std::vector<char> msgData(header.size);
            recvAll(clientSocket, msgData.data(), header.size);

            if (header.type == MessageType::Text) {

                std::string message(msgData.begin(), msgData.end());

                std::cout << clientColor
                          << "[" << username << "] "
                          << message
                          << RESET << "\n";

                logEvent("[" + username + "] " + message);

                std::string coloredMessage =
                    clientColor + message + RESET;

                MessageHeader outHeader{
                    MessageType::Text,
                    (uint32_t)coloredMessage.size()
                };

                std::vector<char> outData(
                    coloredMessage.begin(),
                    coloredMessage.end());

                broadcast(outHeader, outData, clientSocket);
            }
            else if (header.type == MessageType::Disconnect) {
                break;
            }
        }
    }
    catch (...) {}

    logEvent("Client " + clientId +
             " ('" + username + "') disconnected");

    close(clientSocket);

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients.erase(std::remove_if(clients.begin(), clients.end(),
            [clientSocket](Client& c){ return c.socket == clientSocket; }),
            clients.end());
        releaseColorIndex(clientId);
    }
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
                MessageType::Text,
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

        std::string id = getClientId(client);

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

    close(serverSock);
}
// ============================================================
// MAIN
// ============================================================

int main() {

    initializeServer();

    printCommands();

    std::thread cmdThread(commandHandler);
    std::thread monitorThread(monitoringLoop);

    runAcceptLoop();

    if (monitorThread.joinable())
         monitorThread.join();

    shutdownServer(cmdThread);

    return 0;
}
