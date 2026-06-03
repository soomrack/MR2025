#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <string>
#include <stdexcept>
#include <cstdint>
#include <algorithm>
#include <atomic>
#include <chrono>
#include <ctime>
#include <sstream>
#include <fstream>
#include <limits>
#include <csignal>
#include <iomanip>
#include <memory>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <errno.h>



constexpr int    PORT                 = 54000;
constexpr int    MONITOR_INTERVAL_SEC = 10;
constexpr double CPU_TEMP_LIMIT       = 70.0;
constexpr int    RAM_LIMIT_PERCENT    = 80;
constexpr size_t LOG_BUFFER_MAX       = 1000;  // максимум строк в памяти


struct Client {
    int         socket;
    std::string id;
    // shared_ptr — чтобы mutex можно было копировать вместе со структурой
    std::shared_ptr<std::mutex> sendMu{std::make_shared<std::mutex>()};
};

std::vector<Client> clients;
std::mutex          clientsMutex;

std::atomic<bool> serverRunning{true};
int               serverSock = -1;

std::mutex               logMutex;
std::ofstream            logFile;
std::vector<std::string> logBuffer;      // лог в памяти
std::mutex               logBufferMutex;

std::string timestamp() {
    time_t now = time(nullptr);
    struct tm tm{};
    localtime_r(&now, &tm);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
}

void logEvent(const std::string& text) {
    std::string line = "[" + timestamp() + "] " + text;

    {
        std::lock_guard<std::mutex> lock(logMutex);
        std::cout << line << "\n";
        if (logFile.is_open())
            logFile << line << "\n" << std::flush;
    }
    {
        std::lock_guard<std::mutex> lock(logBufferMutex);
        logBuffer.push_back(line);
        if (logBuffer.size() > LOG_BUFFER_MAX)
            logBuffer.erase(logBuffer.begin());
    }
}

void sendAll(int sock, const char* data, int size) {
    int sent = 0;
    while (sent < size) {
        int n = send(sock, data + sent, size - sent, 0);
        if (n <= 0) throw std::runtime_error("send failed");
        sent += n;
    }
}

void recvAll(int sock, char* data, int size) {
    int got = 0;
    while (got < size) {
        int n = recv(sock, data + got, size - got, 0);
        if (n <= 0) throw std::runtime_error("connection closed");
        got += n;
    }
}

std::string peerAddr(int sock) {
    sockaddr_in addr{};
    socklen_t len = sizeof(addr);
    if (getpeername(sock, (sockaddr*)&addr, &len) == 0)
        return std::string(inet_ntoa(addr.sin_addr)) + ":" +
               std::to_string(ntohs(addr.sin_port));
    return "unknown";
}

// Отправить одному клиенту — блокируем его личный mutex
void sendMsg(int sock, std::mutex& mu, const std::string& msg) {
    std::lock_guard<std::mutex> lock(mu);
    uint32_t len = static_cast<uint32_t>(msg.size());
    sendAll(sock, reinterpret_cast<const char*>(&len), sizeof(len));
    sendAll(sock, msg.data(), static_cast<int>(len));
}

// Разослать всем
void broadcast(const std::string& msg) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto& c : clients) {
        try {
            sendMsg(c.socket, *c.sendMu, msg);
        } catch (...) {}
    }
}

double cpuTemp() {
    std::ifstream f("/sys/class/thermal/thermal_zone0/temp");
    if (!f) return -1.0;
    long raw; f >> raw;
    return raw / 1000.0;
}

struct RamInfo { int percent; std::string str; };

RamInfo ramUsage() {
    std::ifstream f("/proc/meminfo");
    std::string key;
    long total = 0, avail = 0;
    while (f >> key) {
        if      (key == "MemTotal:")     f >> total;
        else if (key == "MemAvailable:") { f >> avail; break; }
        f.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    }
    long used = total - avail;
    int  pct  = total ? static_cast<int>((used * 100) / total) : 0;
    std::ostringstream s;
    s << pct << "% (" << used/1024 << " MB / " << total/1024 << " MB)";
    return {pct, s.str()};
}

std::string uptime() {
    std::ifstream f("/proc/uptime");
    double sec; f >> sec;
    int h = (int)sec/3600, m = ((int)sec%3600)/60, s = (int)sec%60;
    std::ostringstream o;
    o << h << "h " << m << "m " << s << "s";
    return o.str();
}

// Читает все строки из файла лога (под logMutex)
static std::vector<std::string> readLogFile() {
    std::vector<std::string> lines;
    std::lock_guard<std::mutex> lock(logMutex);
    std::ifstream f("server.log");
    std::string line;
    while (std::getline(f, line))
        lines.push_back(line);
    return lines;
}

std::string processLogCommand(const std::string& cmd) {
    std::vector<std::string> result;

    if (cmd == "LOG ALL") {
        // Полная история — читаем файл
        result = readLogFile();
    }
    else if (cmd == "LOG WARNINGS") {
        // Полная история предупреждений — читаем файл, фильтруем
        for (auto& line : readLogFile())
            if (line.find("[WARNING]") != std::string::npos)
                result.push_back(line);
    }
    else if (cmd.rfind("LOG LAST ", 0) == 0) {
        // Свежие данные — читаем из буфера, они гарантированно там есть
        try {
            int    minutes = std::stoi(cmd.substr(9));
            time_t now     = time(nullptr);

            std::lock_guard<std::mutex> lock(logBufferMutex);
            for (auto& line : logBuffer) {
                std::tm tm{};
                std::istringstream ss(line.substr(1, 19));
                ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
                if (difftime(now, mktime(&tm)) <= minutes * 60)
                    result.push_back(line);
            }
        } catch (...) {
            return "[ERROR] usage: LOG LAST <minutes>\n";
        }
    }
    else {
        return "[ERROR] commands: LOG ALL | LOG WARNINGS | LOG LAST <min>\n";
    }

    if (result.empty())
        return "(no entries)\n";

    std::ostringstream out;
    for (auto& line : result) out << line << "\n";
    return out.str();
}

void handleClient(int sock) {
    std::string id = peerAddr(sock);
    logEvent("Connected: " + id);

    // Захватываем sendMu до входа в цикл — пока клиент ещё в векторе
    std::shared_ptr<std::mutex> sendMu;
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (auto& c : clients)
            if (c.socket == sock) { sendMu = c.sendMu; break; }
    }

    try {
        while (serverRunning) {
            // Читаем команду: [uint32_t len][bytes]
            uint32_t len = 0;
            recvAll(sock, reinterpret_cast<char*>(&len), sizeof(len));
            if (len == 0 || len > 65536) break;

            std::string cmd(len, '\0');
            recvAll(sock, cmd.data(), static_cast<int>(len));

            // Отвечаем тем же протоколом
            sendMsg(sock, *sendMu, processLogCommand(cmd));
        }
    } catch (...) {}  // disconnect или ошибка — идём на очистку

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients.erase(
            std::remove_if(clients.begin(), clients.end(),
                [sock](const Client& c){ return c.socket == sock; }),
            clients.end());
    }
    close(sock);
    logEvent("Disconnected: " + id);
}

void monitorLoop() {
    while (serverRunning) {
        double  cpu = cpuTemp();
        RamInfo ram = ramUsage();
        std::string up = uptime();

        std::ostringstream line;
        line << "CPU: " << cpu << " C | RAM: " << ram.str << " | Uptime: " << up;

        logEvent(line.str());
        broadcast("[MONITOR] " + line.str());

        if (cpu > CPU_TEMP_LIMIT) {
            std::string w = "[WARNING] CPU TEMP HIGH (" + std::to_string(cpu) + " C)";
            logEvent(w); broadcast(w);
        }
        if (ram.percent > RAM_LIMIT_PERCENT) {
            std::string w = "[WARNING] RAM HIGH (" + std::to_string(ram.percent) + "%)";
            logEvent(w); broadcast(w);
        }

        std::this_thread::sleep_for(std::chrono::seconds(MONITOR_INTERVAL_SEC));
    }
}

void acceptLoop() {
    fcntl(serverSock, F_SETFL, O_NONBLOCK);
    while (serverRunning) {
        int client = accept(serverSock, nullptr, nullptr);
        if (client < 0) {
            if (errno == EWOULDBLOCK || errno == EAGAIN) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            break;
        }
        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back({client, peerAddr(client)});
        }
        std::thread(handleClient, client).detach();
    }
}


void onSignal(int) {
    serverRunning = false;
    if (serverSock >= 0) shutdown(serverSock, SHUT_RDWR);
}

int main() {
    std::signal(SIGINT,  onSignal);
    std::signal(SIGTERM, onSignal);

    logFile.open("server.log", std::ios::app);
    logEvent("=== Server started ===");

    serverSock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSock,   (sockaddr*)&addr, sizeof(addr));
    listen(serverSock, SOMAXCONN);

    logEvent("Listening on port " + std::to_string(PORT));

    std::thread mon(monitorLoop);
    acceptLoop();

    serverRunning = false;
    if (mon.joinable()) mon.join();

    logEvent("=== Server stopped ===");
    logFile.close();
    close(serverSock);
    return 0;
}
