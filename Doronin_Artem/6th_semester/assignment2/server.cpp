#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <iomanip>
#include <ctime>
#include <algorithm>
#include <sstream>

enum class MessageType : uint32_t {
    Text = 1,
    Connect = 2,
    Disconnect = 3,
    LogRequest = 4,
    LogResponse = 5,
    StatusRequest = 6,
    StatusResponse = 7,
    Warning = 8,
    AutoLogStart = 9,
    AutoLogStop = 10
};

struct MessageHeader {
    uint32_t type;
    uint32_t size;
};

struct ConnectedClient {
    int socket;
    std::string username;
    bool autoLog;
    int autoLogInterval;
    int autoLogCounter;
};

constexpr int PORT = 54000;
constexpr double TEMP_WARNING_THRESHOLD = 70.0;
constexpr int RAM_WARNING_THRESHOLD = 30;
constexpr int POLL_INTERVAL_SEC = 1;
const std::string LOG_FILE = "pi_monitor.log";

std::vector<ConnectedClient> clients;
std::mutex clientsMutex;

struct LogEntry {
    time_t timestamp;
    std::string text;
};

std::vector<LogEntry> logHistory; 
std::mutex logMutex;

void writeLog(const std::string& text, bool isWarning = false) {
    std::lock_guard<std::mutex> lock(logMutex);
    auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now), "[%Y-%m-%d %H:%M:%S] ");
    if (isWarning) ss << "[WARNING] ";
    ss << text;
    
    std::string fullEntry = ss.str();
    logHistory.push_back({now, fullEntry});
    
    std::ofstream file(LOG_FILE, std::ios::app);
    if (file.is_open()) file << fullEntry << std::endl;
}

double getCPUTemp() {
    std::ifstream file("/sys/class/thermal/thermal_zone0/temp");
    if (!file.is_open()) return -1.0;
    long temp;
    file >> temp;
    return temp / 1000.0;
}

int getRAMUsage() {
    std::ifstream file("/proc/meminfo");
    std::string key;
    long total = 0, avail = 0;
    while (file >> key) {
        if (key == "MemTotal:") file >> total;
        else if (key == "MemAvailable:") { file >> avail; break; }
    }
    if (total == 0) return 0;
    return ((total - avail) * 100) / total;
}

void sendPacket(int sock, MessageType type, const std::string& data) {
    MessageHeader header{ static_cast<uint32_t>(type), static_cast<uint32_t>(data.size()) };
    send(sock, (char*)&header, sizeof(header), 0);
    send(sock, data.data(), data.size(), 0);
}

void recvAll(int sock, char* data, int size) {
    int received = 0;
    while (received < size) {
        int res = recv(sock, data + received, size - received, 0);
        if (res <= 0) throw std::runtime_error("Pipe broken");
        received += res;
    }
}

void broadcast(MessageType type, const std::string& msg, int excludeSock = -1) {
    std::lock_guard<std::mutex> lock(clientsMutex);
    for (auto it = clients.begin(); it != clients.end(); ) {
        if (it->socket == excludeSock) { ++it; continue; }
        try {
            sendPacket(it->socket, type, msg);
            ++it;
        } catch (...) {
            close(it->socket);
            it = clients.erase(it);
        }
    }
}

void handleClient(int clientSock) {
    std::string myName = "Unknown";
    try {
        while (true) {
            MessageHeader header;
            recvAll(clientSock, (char*)&header, sizeof(header));

            if (header.size > 1024) throw std::runtime_error("Musor v portu");

            std::vector<char> payload(header.size);
            if (header.size > 0) recvAll(clientSock, payload.data(), header.size);
            std::string data(payload.begin(), payload.end());

            switch (static_cast<MessageType>(header.type)) {
                case MessageType::Connect:
                    myName = data;
                    {
                        std::lock_guard<std::mutex> lock(clientsMutex);
                        for (auto& c : clients) if (c.socket == clientSock) c.username = myName;
                    }
                    writeLog("User connected: " + myName);
                    break;

                case MessageType::Text:
                    std::cout << data << std::endl;
                    writeLog(data);
                    broadcast(MessageType::Text, data, clientSock);
                    break;

                case MessageType::LogRequest: {
                    std::string response;
                    std::lock_guard<std::mutex> lock(logMutex);
                    
                    if (data == "ALL") {
                        for (const auto& entry : logHistory) response += entry.text + "\n";
                    } else if (data == "WARNINGS") {
                        for (const auto& entry : logHistory) 
                            if (entry.text.find("[WARNING]") != std::string::npos) response += entry.text + "\n";
                    } else if (data.find("LAST ") == 0) {
                        try {
                            int mins = std::stoi(data.substr(5));
                            time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
                            for (const auto& entry : logHistory) {
                                if (now - entry.timestamp <= mins * 60) {
                                    response += entry.text + "\n";
                                }
                            }
                        } catch (...) {}
                    }
                    sendPacket(clientSock, MessageType::LogResponse, response);
                    break;
                }
                
                case MessageType::StatusRequest: {
                    double t = getCPUTemp();
                    int ram = getRAMUsage();
                    std::string msg = "Temp: " + std::to_string(t).substr(0, 4) + "C, RAM: " + std::to_string(ram) + "%";
                    bool isWarning = (t > TEMP_WARNING_THRESHOLD) || (ram > RAM_WARNING_THRESHOLD);
                    sendPacket(clientSock, isWarning ? MessageType::Warning : MessageType::StatusResponse, msg);
                    break;
                }
                
                case MessageType::AutoLogStart: {
                    int seconds = 10;
                    if (data.find("AUTOLOG ") == 0) {
                        try {
                            seconds = std::stoi(data.substr(8));
                        } catch (...) {}
                    }
                    std::lock_guard<std::mutex> lock(clientsMutex);
                    for (auto& c : clients) {
                        if (c.socket == clientSock) {
                            c.autoLog = true;
                            c.autoLogInterval = seconds;
                            c.autoLogCounter = 0;
                        }
                    }
                    break;
                }
                
                case MessageType::AutoLogStop: {
                    std::lock_guard<std::mutex> lock(clientsMutex);
                    for (auto& c : clients) {
                        if (c.socket == clientSock) {
                            c.autoLog = false;
                        }
                    }
                    break;
                }

                case MessageType::Disconnect:
                    throw std::runtime_error("Client exit");
            }
        }
    } catch (...) {
        close(clientSock);
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients.erase(std::remove_if(clients.begin(), clients.end(),[clientSock](const ConnectedClient& c){ return c.socket == clientSock; }), clients.end());
        writeLog("Client disconnected: " + myName);
    }
}

void autoLogLoop() {
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(POLL_INTERVAL_SEC));
        double t = -100.0;
        int ram = 0;
        bool fetched = false;

        std::lock_guard<std::mutex> lock(clientsMutex);
        for (auto& c : clients) {
            if (c.autoLog && c.autoLogInterval > 0) {
                c.autoLogCounter += POLL_INTERVAL_SEC;
                if (c.autoLogCounter >= c.autoLogInterval) {
                    c.autoLogCounter = 0;
                    if (!fetched) {
                        t = getCPUTemp();
                        ram = getRAMUsage();
                        fetched = true;
                    }
                    std::string msg = "Temp: " + std::to_string(t).substr(0, 4) + "C, RAM: " + std::to_string(ram) + "%";
                    bool isWarning = (t > TEMP_WARNING_THRESHOLD) || (ram > RAM_WARNING_THRESHOLD);
                    try {
                        sendPacket(c.socket, isWarning ? MessageType::Warning : MessageType::StatusResponse, msg);
                    } catch (...) {}
                }
            }
        }
    }
}

int main() {
    int serverSock = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(PORT);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSock, (sockaddr*)&addr, sizeof(addr)) < 0) return -1;
    listen(serverSock, 10);
    
    writeLog("--- Server Started ---");
    std::thread(autoLogLoop).detach();

    while (true) {
        int sock = accept(serverSock, nullptr, nullptr);
        if (sock >= 0) {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back({sock, "Pending", false, 0, 0});
            std::thread(handleClient, sock).detach();
        }
    }
    return 0;
}
