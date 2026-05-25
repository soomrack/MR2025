#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <cstdint>
#include <cstring>
#include <signal.h>

// ============================================================
// ПРОТОКОЛ СООБЩЕНИЙ 
// ============================================================

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


// Глобальный сокет клиента
int clientSocket = -1;
bool connected = false;
bool running = true;
std::mutex coutMutex; 
std::string username;


// ============================================================
// НИЗКОУРОВНЕВАЯ ОТПРАВКА 
// ============================================================


void sendAll(const char* data, int size) {
    int sent = 0;
    while (sent < size) {
        int res = send(clientSocket, data + sent, size - sent, 0);
        if (res <= 0) throw std::runtime_error("send failed");
        sent += res;
    }
}

void recvAll(char* data, int size) {
    int received = 0;
    while (received < size) {
        int res = recv(clientSocket, data + received, size - received, 0);
        if (res <= 0) throw std::runtime_error("recv failed");
        received += res;
    }
}

// ============================================================
// Ввод имени пользователя
// ============================================================
std::string getUsername() {
    std::string name;
    while (true) {
        std::cout << "Enter your name (3-20 characters): ";
        std::getline(std::cin, name);
        
        size_t first = name.find_first_not_of(" \t");
        size_t last = name.find_last_not_of(" \t");
        if (first == std::string::npos || last == std::string::npos) {
            std::cout << "Name cannot be empty!\n";
            continue;
        }
        name = name.substr(first, last - first + 1);
        
        if (name.length() < 3) {
            std::cout << "Name too short! Minimum 3 characters.\n";
            continue;
        }
        if (name.length() > 20) {
            std::cout << "Name too long! Maximum 20 characters.\n";
            continue;
        }
        
        bool valid = true;
        for (char c : name) {
            if (!isalnum(c) && c != '_' && c != '-' && c != ' ') {
                valid = false;
                break;
            }
        }
        if (!valid) {
            std::cout << "Name can only contain letters, numbers, spaces, underscores and hyphens!\n";
            continue;
        }
        
        return name;
    }
}

// ============================================================
// Получение текущего времени
// ============================================================
std::string getCurrentTime() {
    time_t now = time(0);
    struct tm *timeinfo;
    timeinfo = localtime(&now);
    
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", timeinfo);
    return std::string(buffer);
}

// ============================================================
// EMOJI
// ============================================================

std::string replaceEmoji(std::string text) {
    std::vector<std::pair<std::string, std::string>> emojis = {
        {":fire:", u8"🔥"},
        {":smile:", u8"😄"},
        {":sad:", u8"😢"},
        {":heart:", u8"❤️"},
        {":ok:", u8"👌"}
    };

    for (auto& e : emojis) {
        size_t pos;
        while ((pos = text.find(e.first)) != std::string::npos)
            text.replace(pos, e.first.length(), e.second);
    }
    return text;
}


// ============================================================
// ПРИЁМ СООБЩЕНИЙ 
// ============================================================


void receiveLoop() {
    try {
        while (connected) {
            MessageHeader header{};
            recvAll((char*)&header, sizeof(header));

            std::vector<char> data(header.size);
            recvAll(data.data(), header.size);

            std::string text(data.begin(), data.end());

            std::lock_guard<std::mutex> lock(coutMutex);
            
            if (header.type == static_cast<uint32_t>(MessageType::LogResponse)) {
                std::cout << "\n=== LOG RESPONSE ===\n"
                          << text
                          << "====================\n> ";
            }
            else if (header.type == static_cast<uint32_t>(MessageType::StatusResponse)) {
                std::cout << "\n[STATUS] " << text << "\n> ";
            }
            else if (header.type == static_cast<uint32_t>(MessageType::Warning)) {
                std::cout << "\n\033[31m[WARNING] " << text << "\033[0m\n> ";
            }
            else {
                std::cout << "\n" << text << "\n> ";
            }
        }
    }
    catch (...) {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "\nDisconnected.\n";
        connected = false;
        close(clientSocket);
    }
}

// ============================================================
// ВЫСОКОУРОВНЕВАЯ ЛОГИКА 
// ============================================================

bool connectToServer(const std::string& ip, int port) {
    username = getUsername();
    if (username.empty()) return false;
    
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket < 0) {
        std::cout << "Socket creation failed.\n";
        return false;
    }
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    
    if (connect(clientSocket, (sockaddr*)&addr, sizeof(addr)) == 0) {
        connected = true;
        
        try {
            std::string connectMsg = username;
            MessageHeader header{ static_cast<uint32_t>(MessageType::Connect), (uint32_t)connectMsg.size() };
            sendAll((char*)&header, sizeof(header));
            sendAll(connectMsg.data(), connectMsg.size());
            
            std::thread(receiveLoop).detach();
            
            std::cout << "Connected to " << ip << ":" << port << " as '" << username << "'\n";
            return true;
        }
        catch (...) {
            std::cout << "Failed to send username.\n";
            close(clientSocket);
            connected = false;
            return false;
        }
    }
    
    std::cout << "Connection failed.\n";
    return false;
}

void disconnectFromServer() {
    if (connected) {
        try {
            std::string disconnectMsg = username;
            MessageHeader header{ static_cast<uint32_t>(MessageType::Disconnect), (uint32_t)disconnectMsg.size()};
            sendAll((char*)&header, sizeof(header));
            sendAll(disconnectMsg.data(), disconnectMsg.size());
        }
        catch (...) {}
        
        connected = false;
        close(clientSocket);
        std::cout << "Disconnected from server.\n";
    }
}

void sendMessage(const std::string& input) {
    std::string timeStr = getCurrentTime();
    std::string processedContent = replaceEmoji(input);
    
    std::string fullMessage = "[" + username + " " + timeStr + "] " + processedContent;
    
    MessageHeader header{
        static_cast<uint32_t>(MessageType::Text),
        (uint32_t)fullMessage.size()
    };
    
    sendAll((char*)&header, sizeof(header));
    sendAll(fullMessage.data(), fullMessage.size());
}


bool spam(const std::string& input) {
    if (!connected) {
        std::cout << "Not connected.\n";
        return false;
    }
    
    std::stringstream ss(input);
    std::string cmd;
    int count;
    std::string message;
    
    ss >> cmd >> count;
    std::getline(ss, message);
    
    if (!message.empty() && message[0] == ' ')
        message = message.substr(1);
    
    for (int i = 0; i < count; i++) {
        try {
            if (i > 0) std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            std::string spamMsg = message;
            if (count > 1) {
                spamMsg = "[" + std::to_string(i+1) + "/" + 
                         std::to_string(count) + "] " + message;
            }
            
            sendMessage(spamMsg);
        }
        catch (...) {
            std::cout << "Spam interrupted.\n";
            break;
        }
    }
    return true;
}

// ============================================================
// Запрос статуса CPU/RAM
// ============================================================

void requestStatus() {
    if (!connected) {
        std::cout << "Not connected.\n";
        return;
    }
    MessageHeader header{static_cast<uint32_t>(MessageType::StatusRequest), 0};
    sendAll((char*)&header, sizeof(header));
}

// ============================================================
// Запуск авто-отправки логов
// ============================================================

void startAutoLog(const std::string& input) {
    if (!connected) {
        std::cout << "Not connected.\n";
        return;
    }
    
    int seconds = 10; // по умолчанию
    std::stringstream ss(input);
    std::string cmd;
    ss >> cmd >> seconds;
    
    if (seconds < 1) seconds = 1;
    if (seconds > 3600) seconds = 3600;
    
    std::string request = "AUTOLOG " + std::to_string(seconds);
    MessageHeader header{static_cast<uint32_t>(MessageType::AutoLogStart), (uint32_t)request.size()};
    sendAll((char*)&header, sizeof(header));
    sendAll(request.data(), request.size());
}

// ============================================================
// Остановка авто-отправки логов
// ============================================================

void stopAutoLogCmd() {
    if (!connected) {
        std::cout << "Not connected.\n";
        return;
    }
    MessageHeader header{static_cast<uint32_t>(MessageType::AutoLogStop), 0};
    sendAll((char*)&header, sizeof(header));
}

// ============================================================
// Запрос логов
// ============================================================

void requestLogs(const std::string& input) {
    if (!connected) {
        std::cout << "Not connected.\n";
        return;
    }

    std::string request;

    if (input == "/logs all")
        request = "ALL";
    else if (input == "/logs warnings")
        request = "WARNINGS";
    else if (input.rfind("/logs last ", 0) == 0)
        request = "LAST " + input.substr(11);
    else {
        std::cout << "Usage:\n"
                  << "/logs all\n"
                  << "/logs warnings\n"
                  << "/logs last <minutes>\n";
        return;
    }

    MessageHeader header{static_cast<uint32_t>(MessageType::LogRequest), (uint32_t)request.size()};
    sendAll((char*)&header, sizeof(header));
    sendAll(request.data(), request.size());
}

// ============================================================
// Служебные сообщения
// ============================================================

void printHelp() {
    std::cout << "\n=== COMMANDS ===\n";
    std::cout << "/connect <ip> <port>  - Connect to server\n";
    std::cout << "/quit                 - Disconnect from server\n";
    std::cout << "/exit                 - Exit program\n";
    std::cout << "/help                 - Show this help\n";
    std::cout << "/status               - Request CPU/RAM status\n";
    std::cout << "/autolog <seconds>    - Start auto-logs every N seconds\n";
    std::cout << "/stopautolog          - Stop auto-logs\n";
    std::cout << "/logs all             - Get all logs\n";
    std::cout << "/logs warnings        - Get warning logs\n";
    std::cout << "/logs last <minutes>  - Get logs for last N minutes\n";
    std::cout << "/spam <N> <message>   - Send N messages\n";
    std::cout << "==================\n\n";
}

// ============================================================
// ОСНОВНОЙ ЦИКЛ 
// ============================================================

void runClientEventLoop() {
    std::string input;

    while (running) {
        std::cout << "> ";
        std::getline(std::cin, input);

        if (input.rfind("/connect", 0) == 0) {
            if (connected) {
                std::cout << "Already connected.\n";
                continue;
            }

            std::stringstream ss(input);
            std::string cmd, ip;
            int port;
            ss >> cmd >> ip >> port;

            connectToServer(ip, port);
        }
        else if (input == "/quit") {
            disconnectFromServer();
        }
        else if (input == "/exit") {
            running = false;
            disconnectFromServer();
        }
        else if (input.rfind("/spam", 0) == 0) {
            spam(input);
        }
        else if (input == "/help") {
            printHelp();
        }
        else if (input == "/status") {
            requestStatus();
        }
        else if (input.rfind("/autolog", 0) == 0) {
            startAutoLog(input);
        }
        else if (input == "/stopautolog") {
            stopAutoLogCmd();
        }
        else if (input.rfind("/logs", 0) == 0) {
            requestLogs(input);
        }
        else {
            if (!connected) {
                std::cout << "Not connected.\n";
                continue;
            }
            sendMessage(input);
        }
    }
}

// ============================================================
// MAIN 
// ============================================================

int main() {
    signal(SIGPIPE, SIG_IGN);

    std::cout << "Messenger client started.\n";
    printHelp();

    runClientEventLoop();
    return 0;
}
