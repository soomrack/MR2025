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

struct Client {
    int         socket;
    std::string id;
};

std::vector<Client> clients;
std::mutex          clientsMutex;

std::atomic<bool> serverRunning{true};
int               serverSock = -1;

std::mutex    logMutex;
std::ofstream logFile;

std::string timestamp() {
    time_t now = time(nullptr);
    struct tm tm{};
    localtime_r(&now, &tm);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tm);
    return buf;
}

void logEvent(const std::string& text) {
    std::lock_guard<std::mutex> lock(logMutex);
    std::string line = "[" + timestamp() + "] " + text;
    std::cout << line << "\n";
    if (logFile.is_open())
        logFile << line << "\n" << std::flush;
}

void sendAll(int sock, const char* data, int size) {
    int sent = 0;
    while (sent < size) {
        int n = send(sock, data + sent, size - sent, 0);
        if (n <= 0) throw std::runtime_error("send failed");
        sent += n;
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


// BROADCAST  (protocol: uint32_t length + message bytes)
void broadcast(const std::string& msg) {
    uint32_t len = static_cast<uint32_t>(msg.size());
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto& c : clients) {
        try {
            sendAll(c.socket, reinterpret_cast<const char*>(&len), sizeof(len));
            sendAll(c.socket, msg.data(), static_cast<int>(len));
        } catch (...) {}
    }
}


// METRICS
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
    s << pct << "% (" << used / 1024 << " MB / " << total / 1024 << " MB)";
    return {pct, s.str()};
}

std::string uptime() {
    std::ifstream f("/proc/uptime");
    double sec; f >> sec;
    int h = static_cast<int>(sec) / 3600;
    int m = (static_cast<int>(sec) % 3600) / 60;
    int s = static_cast<int>(sec) % 60;
    std::ostringstream o;
    o << h << "h " << m << "m " << s << "s";
    return o.str();
}

void handleClient(int sock) {
    std::string id = peerAddr(sock);
    logEvent("Connected: " + id);

    // Passive: just hold the connection open.
    // Metrics are pushed by monitorLoop via broadcast().
    char dummy[64];
    while (serverRunning) {
        int r = recv(sock, dummy, sizeof(dummy), 0);
        if (r <= 0) break;   // clean disconnect or error
    }

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
        broadcast(line.str());

        if (cpu > CPU_TEMP_LIMIT) {
            std::string w = "[WARNING] CPU TEMP HIGH (" + std::to_string(cpu) + " C)";
            logEvent(w);
            broadcast(w);
        }
        if (ram.percent > RAM_LIMIT_PERCENT) {
            std::string w = "[WARNING] RAM HIGH (" + std::to_string(ram.percent) + "%)";
            logEvent(w);
            broadcast(w);
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
