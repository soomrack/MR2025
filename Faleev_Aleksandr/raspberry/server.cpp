// ============================================================
//  Server (Raspberry Pi side)
//  Build: g++ -std=c++17 -O2 -pthread server.cpp -o server
// ============================================================
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <memory>
#include <string>
#include <stdexcept>
#include <cstdint>
#include <algorithm>
#include <atomic>
#include <map>
#include <chrono>
#include <ctime>
#include <sstream>
#include <fstream>
#include <limits>
#include <iomanip>
#include <csignal>
#include <cstring>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>
#include <termios.h>

// ============================================================
// ПРОТОКОЛ
// ============================================================
enum class MessageType : uint32_t {
    Text        = 1,
    Connect     = 2,
    Disconnect  = 3,
    LogRequest  = 4,
    LogResponse = 5,
    Warning     = 6,
};

#pragma pack(push, 1)
struct MessageHeader {
    uint32_t type;
    uint32_t size;
};
#pragma pack(pop)

constexpr uint32_t MAX_PAYLOAD_BYTES = 1u << 20;
constexpr uint32_t MAX_USERNAME_LEN  = 64;

// ============================================================
// КОНФИГ
// ============================================================
constexpr int    SERVER_PORT              = 54003;
constexpr int    MONITOR_INTERVAL_SEC     = 10;
constexpr double CPU_TEMP_WARN_THRESHOLD  = 70.0;
constexpr int    RAM_USAGE_WARN_THRESHOLD = 80;
constexpr const char* LOG_FILE_PATH       = "server.log";
constexpr int    TEST_MESSAGE_COUNT       = 10;
constexpr int    TEST_MESSAGE_INTERVAL_MS = 500;

// ============================================================
// СТРУКТУРЫ
// ============================================================
struct ClientInfo {
    int         socket;
    std::string ansiColor;
    int         colorIndex;
    std::string peerAddress;
    std::string username;
    std::string joinTime;
    std::shared_ptr<std::mutex> sendMutex;
};

// ============================================================
// ГЛОБАЛЬНОЕ СОСТОЯНИЕ
// ============================================================
static std::vector<ClientInfo>    g_clients;
static std::mutex                 g_clientsMutex;

static std::map<int, std::string> g_usedColorSlots;
static std::mutex                 g_colorSlotsMutex;

static int        g_uartFd = -1;
static std::mutex g_uartMutex;

static const std::vector<std::string> g_colorPool = {
    "\033[31m", "\033[32m", "\033[33m",
    "\033[34m", "\033[35m", "\033[36m"
};
static const std::string ANSI_RESET = "\033[0m";

static std::atomic<bool> g_running{true};
static int               g_serverSocket = -1;

static std::ofstream             g_logFile;
static std::vector<std::string>  g_logBuffer;
static std::vector<std::string>  g_warningBuffer;
static std::vector<std::string>  g_uartDataBuffer;
static std::mutex                g_logMutex;

// ============================================================
// ОБЪЯВЛЕНИЯ ФУНКЦИЙ
// ============================================================

// UART
static bool openUartPort(const char* device = "/dev/ttyUSB0", int baud = B9600);
static void uartReadLoop();

// Утилиты времени
static std::string formatTime(const char* fmt);
static std::string getTimestampFull();
static std::string getTimeHMS();

// Логирование
static void logEvent(const std::string& text);

// Сетевые утилиты
static void sendAllRaw(int sock, const char* data, size_t size);
static void sendFrame(int sock, std::mutex& sendMutex, MessageType type, const std::string& payload);
static void recvAll(int sock, char* data, size_t size);
static std::string getPeerAddress(int sock);

// Цвета
static int  acquireColorSlot(const std::string& peerId);
static void releaseColorSlot(int slotIndex);

// Рассылка
static void broadcastToOthers(MessageType type, const std::string& payload, int senderSocket);

// Системная информация
static std::string getUptimeString();
static int         getRAMUsagePercent();
static double      getCPUTempCelsius();
static std::string formatCpuTemp(double tempC);
static std::string buildStatusReport();

// Мониторинг
static void monitoringLoop();

// Тест
static void runTestMessageFlood(int clientSocket, std::shared_ptr<std::mutex> sendMutex, std::string username);

// Обработка log-запросов
static std::string handleLogRequest(const std::string& request);

// Клиентский поток
static void handleClient(int clientSocket, int colorSlotIndex, std::string clientColor,
                         std::shared_ptr<std::mutex> sendMutex);

// Accept loop
static void runAcceptLoop();

// Сигналы
static void onSignal(int);

// ============================================================
// UART
// ============================================================
static bool openUartPort(const char* device, int baud) {
    g_uartFd = open(device, O_RDWR | O_NOCTTY | O_NDELAY);
    if (g_uartFd < 0) return false;

    termios opts{};
    tcgetattr(g_uartFd, &opts);
    cfsetispeed(&opts, baud);
    cfsetospeed(&opts, baud);
    opts.c_cflag |= (CLOCAL | CREAD);
    opts.c_cflag &= ~PARENB;
    opts.c_cflag &= ~CSTOPB;
    opts.c_cflag &= ~CSIZE;
    opts.c_cflag |= CS8;
    opts.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tcsetattr(g_uartFd, TCSANOW, &opts);
    return true;
}

// Читает строки с Arduino и кладёт в g_uartDataBuffer (без рассылки клиентам)
static void uartReadLoop() {
    logEvent('started UART Read Loop')
    char buf[256];
    std::string partial;
    while (g_running && g_uartFd >= 0) {
        logEvent('entered UART Read Loop')
        int n = read(g_uartFd, buf, sizeof(buf));
        if (n > 0) {
            logEvent('New Data in UART Read Loop')
            partial.append(buf, n);
            size_t pos;
            while ((pos = partial.find('\n')) != std::string::npos) {
                std::string line = partial.substr(0, pos);
                if (!line.empty() && line.back() == '\r') line.pop_back();
                if (!line.empty()) {
                    std::lock_guard<std::mutex> lock(g_logMutex);
                    std::string entry = "[" + getTimestampFull() + "] " + line;
                    g_uartDataBuffer.push_back(entry);
                    if (g_uartDataBuffer.size() > 10000)
                        g_uartDataBuffer.erase(g_uartDataBuffer.begin(),
                                               g_uartDataBuffer.begin() + 2000);
                    logEvent('Write data in UART Read Loop COMPLETED')
                }
                partial = partial.substr(pos + 1);
            }
        } else {
            logEvent('Sleep in UART Read Loop')
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

// ============================================================
// УТИЛИТЫ ВРЕМЕНИ
// ============================================================
static std::string formatTime(const char* fmt) {
    time_t now = time(nullptr);
    struct tm tmv{};
    localtime_r(&now, &tmv);
    char buf[64];
    strftime(buf, sizeof(buf), fmt, &tmv);
    return buf;
}

static std::string getTimestampFull() { return formatTime("%Y-%m-%d %H:%M:%S"); }
static std::string getTimeHMS()       { return formatTime("%H:%M:%S"); }

// ============================================================
// ЛОГИРОВАНИЕ
// ============================================================
static void logEvent(const std::string& text) {
    const std::string entry = "[" + getTimestampFull() + "] " + text;
    std::lock_guard<std::mutex> lock(g_logMutex);

    if (g_logFile.is_open()) {
        g_logFile << entry << '\n';
        g_logFile.flush();
    }

    g_logBuffer.push_back(entry);
    if (text.find("[WARNING]") != std::string::npos)
        g_warningBuffer.push_back(entry);

    // Ограничение размера буферов
    if (g_logBuffer.size() > 50000)
        g_logBuffer.erase(g_logBuffer.begin(), g_logBuffer.begin() + 10000);
    if (g_warningBuffer.size() > 10000)
        g_warningBuffer.erase(g_warningBuffer.begin(), g_warningBuffer.begin() + 2000);
}

// ============================================================
// СЕТЕВЫЕ УТИЛИТЫ
// ============================================================
static void sendAllRaw(int sock, const char* data, size_t size) {
    size_t sent = 0;
    while (sent < size) {
        ssize_t res = send(sock, data + sent, size - sent, MSG_NOSIGNAL);
        if (res <= 0) throw std::runtime_error("send failed");
        sent += static_cast<size_t>(res);
    }
}

static void sendFrame(int sock, std::mutex& sendMutex,
                      MessageType type, const std::string& payload)
{
    MessageHeader header{ (uint32_t)type, (uint32_t)payload.size() };
    std::lock_guard<std::mutex> lock(sendMutex);
    sendAllRaw(sock, (char*)&header, sizeof(header));
    if (!payload.empty())
        sendAllRaw(sock, payload.data(), payload.size());
}

static void recvAll(int sock, char* data, size_t size) {
    size_t got = 0;
    while (got < size) {
        ssize_t res = recv(sock, data + got, size - got, 0);
        if (res <= 0) throw std::runtime_error("recv failed");
        got += static_cast<size_t>(res);
    }
}

static std::string getPeerAddress(int sock) {
    sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    if (getpeername(sock, (sockaddr*)&addr, &len) == 0) {
        char ipBuf[INET_ADDRSTRLEN] = {};
        inet_ntop(AF_INET, &addr.sin_addr, ipBuf, sizeof(ipBuf));
        return std::string(ipBuf) + ":" + std::to_string(ntohs(addr.sin_port));
    }
    return "unknown";
}

// ============================================================
// ЦВЕТА
// ============================================================
static int acquireColorSlot(const std::string& peerId) {
    std::lock_guard<std::mutex> lock(g_colorSlotsMutex);
    for (int i = 0; i < (int)g_colorPool.size(); ++i) {
        if (!g_usedColorSlots.count(i)) {
            g_usedColorSlots[i] = peerId;
            return i;
        }
    }
    return -1;
}

static void releaseColorSlot(int slotIndex) {
    if (slotIndex < 0) return;
    std::lock_guard<std::mutex> lock(g_colorSlotsMutex);
    g_usedColorSlots.erase(slotIndex);
}

// ============================================================
// BROADCAST
// ============================================================
static void broadcastToOthers(MessageType type, const std::string& payload, int senderSocket) {
    std::vector<std::pair<int, std::shared_ptr<std::mutex>>> targets;
    {
        std::lock_guard<std::mutex> lock(g_clientsMutex);
        targets.reserve(g_clients.size());
        for (auto& c : g_clients)
            if (c.socket != senderSocket)
                targets.emplace_back(c.socket, c.sendMutex);
    }
    for (auto& [sock, mutex] : targets) {
        try { sendFrame(sock, *mutex, type, payload); }
        catch (...) {}
    }
}

// ============================================================
// ЧТЕНИЕ /proc И /sys
// ============================================================
static std::string getUptimeString() {
    std::ifstream f("/proc/uptime");
    double seconds = 0;
    if (!(f >> seconds)) return "Unknown";
    int hrs  =  (int)seconds / 3600;
    int mins = ((int)seconds % 3600) / 60;
    int secs =  (int)seconds % 60;
    std::ostringstream oss;
    oss << hrs << "h " << mins << "m " << secs << "s";
    return oss.str();
}

static int getRAMUsagePercent() {
    std::ifstream f("/proc/meminfo");
    std::string key;
    long total = 0, available = 0;
    while (f >> key) {
        if (key == "MemTotal:")          f >> total;
        else if (key == "MemAvailable:") { f >> available; break; }
        f.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    if (total <= 0) return 0;
    return (int)(((total - available) * 100) / total);
}

static double getCPUTempCelsius() {
    std::ifstream f("/sys/class/thermal/thermal_zone0/temp");
    if (!f.is_open()) return -1.0;
    long raw = 0;
    if (!(f >> raw)) return -1.0;
    return raw / 1000.0;
}

static std::string formatCpuTemp(double tempC) {
    if (tempC < 0) return "n/a";
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(1) << tempC << " C";
    return oss.str();
}

static std::string buildStatusReport() {
    double cpuTemp  = getCPUTempCelsius();
    int    ramUsage = getRAMUsagePercent();
    auto   uptime   = getUptimeString();

    std::ostringstream oss;
    oss << "=== Raspberry Pi Status (" << getTimestampFull() << ") ===\n"
        << "  CPU temperature : " << formatCpuTemp(cpuTemp) << "\n"
        << "  RAM usage       : " << ramUsage << " %\n"
        << "  Uptime          : " << uptime   << "\n"
        << "  Clients online  : ";
    {
        std::lock_guard<std::mutex> lock(g_clientsMutex);
        oss << g_clients.size() << "\n";
        for (auto& c : g_clients)
            oss << "    - " << c.username
                << " (" << c.peerAddress
                << ", joined " << c.joinTime << ")\n";
    }
    return oss.str();
}

// ============================================================
// ФОНОВЫЙ МОНИТОР
// ============================================================
static void monitoringLoop() {
    while (g_running) {
        double cpuTemp  = getCPUTempCelsius();
        int    ramUsage = getRAMUsagePercent();
        auto   uptime   = getUptimeString();

        std::ostringstream entry;
        entry << "MONITOR | CPU: " << formatCpuTemp(cpuTemp)
              << " | RAM: " << ramUsage << "% | Uptime: " << uptime;
        logEvent(entry.str());

        // Проверка порогов и рассылка предупреждений
        std::string warningText;
        if (cpuTemp > 0 && cpuTemp > CPU_TEMP_WARN_THRESHOLD)
            warningText += "CPU TEMP HIGH (" + formatCpuTemp(cpuTemp) + ") ";
        if (ramUsage > RAM_USAGE_WARN_THRESHOLD)
            warningText += "RAM USAGE HIGH (" + std::to_string(ramUsage) + "%)";

        if (!warningText.empty()) {
            std::string fullWarning = "[WARNING] " + warningText;
            std::cout << "\033[31m" << fullWarning << ANSI_RESET << "\n";
            logEvent(fullWarning);
            broadcastToOthers(MessageType::Warning, fullWarning, -1);
        }

        for (int i = 0; i < MONITOR_INTERVAL_SEC * 10 && g_running; ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// ============================================================
// ТЕСТОВЫЙ ФЛУД
// ============================================================
static void runTestMessageFlood(int clientSocket,
                                std::shared_ptr<std::mutex> sendMutex,
                                std::string username)
{
    try {
        for (int i = 1; i <= TEST_MESSAGE_COUNT && g_running; ++i) {
            std::ostringstream oss;
            oss << "TEST " << i << "/" << TEST_MESSAGE_COUNT;
            sendFrame(clientSocket, *sendMutex, MessageType::Text, oss.str());
            std::this_thread::sleep_for(std::chrono::milliseconds(TEST_MESSAGE_INTERVAL_MS));
        }
        sendFrame(clientSocket, *sendMutex, MessageType::LogResponse,
                  "Test flood finished (" + std::to_string(TEST_MESSAGE_COUNT) + " messages).\n");
    } catch (...) {}
    logEvent("Test flood for '" + username + "' done.");
}

// ============================================================
// ОБРАБОТКА LOG-ЗАПРОСОВ
// ============================================================
static std::string handleLogRequest(const std::string& request) {
    if (request == "TEMP") {
        return "CPU temperature: " + formatCpuTemp(getCPUTempCelsius()) + "\n";
    }
    if (request == "STATUS") {
        return buildStatusReport();
    }
    if (request == "ALL") {
        std::lock_guard<std::mutex> lock(g_logMutex);
        std::ostringstream oss;
        for (auto& line : g_logBuffer) oss << line << '\n';
        auto s = oss.str();
        return s.empty() ? "No logs yet.\n" : s;
    }
    if (request == "WARNINGS") {
        std::lock_guard<std::mutex> lock(g_logMutex);
        std::ostringstream oss;
        for (auto& line : g_warningBuffer) oss << line << '\n';
        auto s = oss.str();
        return s.empty() ? "No warnings recorded.\n" : s;
    }
    if (request.rfind("LAST ", 0) == 0) {
        int minutes = 0;
        try { minutes = std::stoi(request.substr(5)); }
        catch (...) { return "Bad LAST argument.\n"; }
        if (minutes <= 0) return "Minutes must be > 0.\n";

        time_t now = time(nullptr);
        std::ostringstream oss;
        std::lock_guard<std::mutex> lock(g_logMutex);
        for (auto& line : g_logBuffer) {
            if (line.size() < 21) continue;
            std::tm tm{};
            std::istringstream ss(line.substr(1, 19));
            ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
            if (ss.fail()) continue;
            tm.tm_isdst = -1;
            if (difftime(now, mktime(&tm)) <= minutes * 60)
                oss << line << '\n';
        }
        auto s = oss.str();
        return s.empty() ? "No logs in the requested interval.\n" : s;
    }
    if (request == "SENSOR_ALL") {
        std::lock_guard<std::mutex> lock(g_logMutex);
        std::ostringstream oss;
        for (auto& line : g_uartDataBuffer) oss << line << '\n';
        auto s = oss.str();
        return s.empty() ? "No sensor data yet.\n" : s;
    }
    if (request.rfind("SENSOR_LAST ", 0) == 0) {
        int minutes = 0;
        try { minutes = std::stoi(request.substr(12)); }
        catch (...) { return "Bad argument.\n"; }
        if (minutes <= 0) return "Minutes must be > 0.\n";

        time_t now = time(nullptr);
        std::ostringstream oss;
        std::lock_guard<std::mutex> lock(g_logMutex);
        for (auto& line : g_uartDataBuffer) {
            if (line.size() < 21) continue;
            std::tm tm{};
            std::istringstream ss(line.substr(1, 19));
            ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
            if (ss.fail()) continue;
            tm.tm_isdst = -1;
            if (difftime(now, mktime(&tm)) <= minutes * 60)
                oss << line << '\n';
        }
        auto s = oss.str();
        return s.empty() ? "No sensor data in this interval.\n" : s;
    }
    return "Unknown log request.\n";
}

// ============================================================
// КЛИЕНТСКИЙ ПОТОК
// ============================================================
static void handleClient(int clientSocket, int colorSlotIndex, std::string clientColor,
                         std::shared_ptr<std::mutex> sendMutex)
{
    std::string peerAddr = getPeerAddress(clientSocket);
    std::string username = "Unknown";

    auto removeClient = [&]() {
        std::lock_guard<std::mutex> lock(g_clientsMutex);
        g_clients.erase(
            std::remove_if(g_clients.begin(), g_clients.end(),
                [clientSocket](const ClientInfo& c){ return c.socket == clientSocket; }),
            g_clients.end());
    };

    try {
        // Ожидаем Connect-фрейм с именем пользователя
        MessageHeader header{};
        recvAll(clientSocket, (char*)&header, sizeof(header));

        if (header.type != (uint32_t)MessageType::Connect ||
            header.size == 0 || header.size > MAX_USERNAME_LEN)
        {
            close(clientSocket);
            releaseColorSlot(colorSlotIndex);
            removeClient();
            return;
        }

        std::vector<char> nameData(header.size);
        recvAll(clientSocket, nameData.data(), header.size);
        username.assign(nameData.begin(), nameData.end());

        std::string joinTime = getTimeHMS();
        {
            std::lock_guard<std::mutex> lock(g_clientsMutex);
            for (auto& c : g_clients)
                if (c.socket == clientSocket) {
                    c.username = username;
                    c.joinTime = joinTime;
                }
        }

        std::cout << clientColor << "Client " << peerAddr
                  << " joined as '" << username << "'" << ANSI_RESET << "\n";
        logEvent("Client " + peerAddr + " joined as '" + username + "'");
        broadcastToOthers(MessageType::Text,
                          "*** " + username + " joined the chat ***", clientSocket);

        // Основной цикл чтения сообщений
        while (g_running) {
            recvAll(clientSocket, (char*)&header, sizeof(header));
            if (header.size > MAX_PAYLOAD_BYTES) break;

            std::vector<char> msgData(header.size);
            if (header.size > 0) recvAll(clientSocket, msgData.data(), header.size);

            switch ((MessageType)header.type) {
                case MessageType::Text: {
                    std::string text(msgData.begin(), msgData.end());
                    std::cout << clientColor << "[" << username << "] "
                              << text << ANSI_RESET << "\n";
                    logEvent("[" + username + "] " + text);
                    broadcastToOthers(MessageType::Text,
                                      clientColor + text + ANSI_RESET, clientSocket);
                    break;
                }
                case MessageType::LogRequest: {
                    std::string req(msgData.begin(), msgData.end());
                    if (req == "TEST") {
                        logEvent("Test flood started for '" + username + "'");
                        std::thread(runTestMessageFlood, clientSocket,
                                    sendMutex, username).detach();
                    } else {
                        std::string response = handleLogRequest(req);
                        sendFrame(clientSocket, *sendMutex,
                                  MessageType::LogResponse, response);
                    }
                    break;
                }
                case MessageType::Disconnect:
                    goto done;
                default:
                    break;
            }
        }
    }
    catch (...) {}

done:
    std::cout << "Client " << peerAddr << " (" << username << ") disconnected.\n";
    logEvent("Client " + peerAddr + " (" + username + ") disconnected.");
    broadcastToOthers(MessageType::Text,
                      "*** " + username + " left the chat ***", clientSocket);
    removeClient();
    releaseColorSlot(colorSlotIndex);
    close(clientSocket);
}

// ============================================================
// ACCEPT LOOP
// ============================================================
static void runAcceptLoop() {
    fcntl(g_serverSocket, F_SETFL, O_NONBLOCK);
    while (g_running) {
        sockaddr_in addr{};
        socklen_t   addrLen = sizeof(addr);
        int clientSocket = accept(g_serverSocket, (sockaddr*)&addr, &addrLen);

        if (clientSocket < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            if (!g_running) break;
            std::cerr << "accept() error: " << strerror(errno) << "\n";
            break;
        }

        std::string peerAddr  = getPeerAddress(clientSocket);
        int colorSlotIdx      = acquireColorSlot(peerAddr);
        std::string color     = (colorSlotIdx >= 0) ? g_colorPool[colorSlotIdx] : "\033[37m";
        auto sendMutex        = std::make_shared<std::mutex>();

        ClientInfo newClient;
        newClient.socket      = clientSocket;
        newClient.peerAddress = peerAddr;
        newClient.username    = "Pending...";
        newClient.colorIndex  = colorSlotIdx;
        newClient.ansiColor   = color;
        newClient.joinTime    = getTimeHMS();
        newClient.sendMutex   = sendMutex;

        {
            std::lock_guard<std::mutex> lock(g_clientsMutex);
            g_clients.push_back(newClient);
        }

        std::cout << "New connection from " << peerAddr << "\n";
        logEvent("New connection from " + peerAddr);
        std::thread(handleClient, clientSocket, colorSlotIdx, color, sendMutex).detach();
    }
}

// ============================================================
// СИГНАЛЫ
// ============================================================
static void onSignal(int) {
    g_running = false;
    if (g_serverSocket >= 0) {
        ::shutdown(g_serverSocket, SHUT_RDWR);
        ::close(g_serverSocket);
        g_serverSocket = -1;
    }
}

// ============================================================
// MAIN
// ============================================================
int main() {
    std::signal(SIGINT,  onSignal);
    std::signal(SIGTERM, onSignal);
    std::signal(SIGPIPE, SIG_IGN);

    g_logFile.open(LOG_FILE_PATH, std::ios::app);
    logEvent("=== Server started ===");

    if (!openUartPort("/dev/ttyUSB0", B9600))
        std::cerr << "Warning: UART not available\n";
    else
        std::thread(uartReadLoop).detach();

    g_serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (g_serverSocket < 0) { std::cerr << "socket() failed\n"; return 1; }

    int reuseAddr = 1;
    setsockopt(g_serverSocket, SOL_SOCKET, SO_REUSEADDR, &reuseAddr, sizeof(reuseAddr));

    sockaddr_in bindAddr{};
    bindAddr.sin_family      = AF_INET;
    bindAddr.sin_port        = htons(SERVER_PORT);
    bindAddr.sin_addr.s_addr = INADDR_ANY;

    if (bind(g_serverSocket, (sockaddr*)&bindAddr, sizeof(bindAddr)) < 0) {
        std::cerr << "bind() failed: " << strerror(errno) << "\n"; return 1;
    }
    if (listen(g_serverSocket, SOMAXCONN) < 0) {
        std::cerr << "listen() failed: " << strerror(errno) << "\n"; return 1;
    }

    std::cout << "Server running on port " << SERVER_PORT << " ...\n"
              << "Monitoring every " << MONITOR_INTERVAL_SEC << "s "
              << "(CPU > " << CPU_TEMP_WARN_THRESHOLD << " C, RAM > "
              << RAM_USAGE_WARN_THRESHOLD << "% -> WARNING)\n"
              << "Press Ctrl+C to stop.\n";

    std::thread monitorThread(monitoringLoop);
    runAcceptLoop();

    if (monitorThread.joinable()) monitorThread.join();
    
    logEvent("=== Server stopped ===");
    if (g_logFile.is_open()) g_logFile.close();
    if (g_serverSocket >= 0) close(g_serverSocket);
    return 0;
}
