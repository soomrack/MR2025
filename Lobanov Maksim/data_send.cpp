#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <unistd.h>
#include <csignal>
#include <ctime>
#include <thread>
#include <mutex>
#include <atomic>
#include <string>
#include <array>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <random>
#include <chrono>
#include <sys/select.h>

// ============================================================================
// Константы и структуры данных
// ============================================================================

struct SensorData {
    // Температура
    int engineTemp;
    int coolantTemp;
    int ambientTemp;
    
    // Давление и уровни
    int oilPressure;
    int coolantLevel;
    int fuelLevel;
    
    // Работа двигателя
    int rpm;
    int load;
    int runtime;
    
    // Электрика
    float voltage;
    
    // Статусы
    bool engineRunning;
    bool warning;  // ← ДОБАВЬТЕ ЭТУ СТРОКУ
};

constexpr int BUFFER_SIZE {1024};
constexpr int DEFAULT_PORT {55555};

// ============================================================================
// Глобальные состояния (с защитой потоков)
// ============================================================================

std::atomic<bool> running {true};
int peerSocket {-1};
std::mutex socketLock;

// ============================================================================
// Обработчик сигнала
// ============================================================================

void signalHandler(int sig) {
    (void)sig;
    running.store(false);
    std::printf("\n[Shutting down server...]\n");
    
    std::lock_guard<std::mutex> lock(socketLock);
    if (peerSocket != -1) {
        shutdown(peerSocket, SHUT_RDWR);
    }
}

// ============================================================================
// Инициализация и завершение
// ============================================================================

void initializeProgram() {
    std::signal(SIGINT, signalHandler);
    std::signal(SIGPIPE, SIG_IGN);
}

void cleanupResources() {
    {
        std::lock_guard<std::mutex> lock(socketLock);
        if (peerSocket != -1) {
            close(peerSocket);
            peerSocket = -1;
        }
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// ============================================================================
// Создание слушающего сокета
// ============================================================================

int createListeningSocket(int port) {
    const int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == -1) {
        std::perror("[Server] socket() failed");
        return -1;
    }
    
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(static_cast<uint16_t>(port));
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(sock, reinterpret_cast<struct sockaddr*>(&addr), sizeof(addr)) == -1) {
        std::perror("[Server] bind() failed");
        close(sock);
        return -1;
    }
    
    if (listen(sock, SOMAXCONN) == -1) {
        std::perror("[Server] listen() failed");
        close(sock);
        return -1;
    }
    
    return sock;
}

// ============================================================================
// Генерация данных сенсоров
// ============================================================================

int getRandom(int min, int max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_int_distribution<> dist(min, max);
    return dist(gen);
}

float getRandomFloat(float min, float max) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    std::uniform_real_distribution<> dist(min, max);
    return static_cast<float>(dist(gen));
}

SensorData generate_data() {
    SensorData data{};
    
    static int simulatedRuntime = 0;
    simulatedRuntime += getRandom(1, 3);
    data.runtime = simulatedRuntime;
    
    int baseEngineTemp = 20 + std::min(simulatedRuntime * 5, 70);
    data.engineTemp = baseEngineTemp + getRandom(-3, 3);
    data.coolantTemp = std::max(20, data.engineTemp - getRandom(5, 15));
    data.ambientTemp = getRandom(15, 35);
    
    if (data.engineTemp < 40) {
        data.oilPressure = getRandom(200, 350);
    } else {
        data.oilPressure = getRandom(350, 550);
    }
    
    data.coolantLevel = std::max(60, 100 - simulatedRuntime / 10 + getRandom(-2, 2));
    data.fuelLevel = std::max(10, 100 - simulatedRuntime / 5 + getRandom(-3, 3));
    
    if (simulatedRuntime % 20 < 15) {
        data.engineRunning = true;
        data.rpm = getRandom(750, 2500);
        data.load = getRandom(20, 85);
    } else {
        data.engineRunning = (getRandom(0, 10) > 2);
        data.rpm = data.engineRunning ? getRandom(650, 850) : 0;
        data.load = data.engineRunning ? getRandom(5, 15) : 0;
    }
    
    if (data.engineRunning) {
        data.voltage = getRandomFloat(13.5f, 14.8f);
    } else {
        data.voltage = getRandomFloat(11.8f, 12.8f);
    }

    data.warning = (data.engineTemp > 105) || 
                   (data.oilPressure < 150) || 
                   (data.coolantLevel < 25) ||
                   (data.fuelLevel < 15) ||
                   (data.voltage < 12.0f);
    
    return data;
}

// ============================================================================
// Форматирование "голых" данных (key=value, одна строка на параметр)
// ============================================================================

// ============================================================================
// Форматирование "голых" данных (key=value, одна строка на параметр)
// ============================================================================

std::string formatRawData(const SensorData& data) {
    char buffer[512];
    std::snprintf(buffer, sizeof(buffer),
        "engineTemp=%d\n"
        "coolantTemp=%d\n"
        "ambientTemp=%d\n"
        "oilPressure=%d\n"
        "coolantLevel=%d\n"
        "fuelLevel=%d\n"
        "rpm=%d\n"
        "load=%d\n"
        "runtime=%d\n"
        "voltage=%.1f\n"
        "engineRunning=%d\n"
        "warning=%d\n",
        data.engineTemp,
        data.coolantTemp,
        data.ambientTemp,
        data.oilPressure,
        data.coolantLevel,
        data.fuelLevel,
        data.rpm,
        data.load,
        data.runtime,
        static_cast<double>(data.voltage),  // float → double для %f
        data.engineRunning ? 1 : 0,
        data.warning ? 1 : 0
    );
    return std::string(buffer);
}

// ============================================================================
// Отправка сообщения клиенту
// ============================================================================

bool sendToClient(const std::string& message) {
    std::lock_guard<std::mutex> lock(socketLock);
    if (peerSocket == -1) return false;
    
    ssize_t sent = send(peerSocket, message.c_str(), message.length(), 0);
    return (sent > 0);
}

// ============================================================================
// Обработка команд от клиента
// ============================================================================

void handleClientCommand(const std::string& command, bool& monitoring) {
    std::string cmd = command;
    for (char& c : cmd) c = std::tolower(static_cast<unsigned char>(c));
    
    if (cmd == "data") {
        SensorData data = generate_data();
        std::string msg = formatRawData(data);
        sendToClient(msg);
        std::printf("[Sent] raw data to client\n");
        std::fflush(stdout);
    }
    else if (cmd == "monitor") {
        monitoring = true;
        sendToClient("monitoring_started\n");
        std::printf("[Client] Monitoring enabled\n");
    }
    else if (cmd == "stop") {
        monitoring = false;
        sendToClient("monitoring_stopped\n");
        std::printf("[Client] Monitoring disabled\n");
    }
    else if (cmd == "help") {
        std::string help = 
            "commands: data, monitor, stop, help, exit\n";
        sendToClient(help);
    }
    else if (cmd == "exit" || cmd == "quit") {
        sendToClient("bye\n");
        std::lock_guard<std::mutex> lock(socketLock);
        if (peerSocket != -1) {
            shutdown(peerSocket, SHUT_RDWR);
        }
    }
    else {
        sendToClient("unknown_command\n");
    }
}

// ============================================================================
// Поток: обработка одного клиента
// ============================================================================

void clientHandlerThread(int clientSocket, const std::string& clientInfo) {
    std::printf("[+] Client connected: %s\n", clientInfo.c_str());
    std::fflush(stdout);
    
    sendToClient("connected\n");
    
    std::array<char, BUFFER_SIZE> buffer{};
    bool monitoring = false;
    auto lastSendTime = std::chrono::steady_clock::now();
    constexpr auto MONITOR_INTERVAL = std::chrono::seconds(10);
    
    while (running.load()) {
        // Периодическая отправка в режиме мониторинга
        if (monitoring) {
            auto now = std::chrono::steady_clock::now();
            if (now - lastSendTime >= MONITOR_INTERVAL) {
                SensorData data = generate_data();
                std::string msg = formatRawData(data);
                sendToClient(msg);
                lastSendTime = now;
            }
        }
        
        // Приём команд (неблокирующий с таймаутом)
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(clientSocket, &readfds);
        
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int activity = select(clientSocket + 1, &readfds, nullptr, nullptr, &timeout);
        
        if (activity > 0 && FD_ISSET(clientSocket, &readfds)) {
            const int bytes = recv(clientSocket, buffer.data(), BUFFER_SIZE - 1, 0);
            if (bytes <= 0) {
                break;
            }
            
            buffer[bytes] = '\0';
            std::string message(buffer.data());
            
            while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
                message.pop_back();
            }
            
            if (!message.empty()) {
                std::printf("[Recv] %s: %s\n", clientInfo.c_str(), message.c_str());
                handleClientCommand(message, monitoring);
                
                std::string cmd = message;
                for (char& c : cmd) c = std::tolower(static_cast<unsigned char>(c));
                if (cmd == "exit" || cmd == "quit") {
                    break;
                }
            }
        }
        else if (activity < 0 && errno != EINTR) {
            std::perror("[Client] select() error");
            break;
        }
    }
    
    close(clientSocket);
    
    {
        std::lock_guard<std::mutex> lock(socketLock);
        if (peerSocket == clientSocket) {
            peerSocket = -1;
        }
    }
    
    std::printf("[-] Client disconnected: %s\n", clientInfo.c_str());
    std::printf("[Server] Waiting for next connection...\n");
    std::fflush(stdout);
}

// ============================================================================
// Поток: сервер
// ============================================================================

void serverThread(int listenPort) {
    const int serverSock = createListeningSocket(listenPort);
    if (serverSock == -1) return;
    
    std::printf("[Server] Listening on port %d...\n", listenPort);
    std::printf("[Server] Ready. Press Ctrl+C to stop.\n\n");
    std::fflush(stdout);
    
    while (running.load()) {
        sockaddr_in clientAddr{};
        socklen_t addrSize = sizeof(clientAddr);

        const int client = accept(serverSock, reinterpret_cast<struct sockaddr*>(&clientAddr), &addrSize);
        if (client == -1) {
            if (errno == EINTR) break;
            continue;
        }
        
        char* ip = inet_ntoa(clientAddr.sin_addr);
        uint16_t port = ntohs(clientAddr.sin_port);
        std::string clientInfo = std::string(ip) + ":" + std::to_string(port);
        
        {
            std::lock_guard<std::mutex> lock(socketLock);
            if (peerSocket != -1) {
                std::printf("[!] Connection rejected: %s\n", clientInfo.c_str());
                close(client);
                continue;
            }
            peerSocket = client;
        }
        
        std::thread clientThread(clientHandlerThread, client, clientInfo);
        clientThread.detach();
    }
    
    close(serverSock);
}

// ============================================================================
// Helper-функции
// ============================================================================

void startServerThread(int listenPort) {
    std::thread t(serverThread, listenPort);
    t.detach();
}

// ============================================================================
// Главная функция
// ============================================================================

int main(int argc, char* argv[]) {
    int listenPort = DEFAULT_PORT;
    
    if (argc >= 2) {
        listenPort = std::atoi(argv[1]);
        if (listenPort <= 0 || listenPort > 65535) {
            std::fprintf(stderr, "Invalid port: %s\n", argv[1]);
            return 1;
        }
    }
    
    initializeProgram();
    startServerThread(listenPort);
    
    while (running.load()) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    cleanupResources();
    std::printf("\n[Server] Stopped.\n");
    
    return 0;
}