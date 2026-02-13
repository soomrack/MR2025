#include <WinSock2.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <string>
#include <stdexcept>
#include <cstdint>
#include <algorithm>
#include <atomic>
#include <conio.h>
#include <map>

#pragma comment(lib, "Ws2_32.lib")

// ================= MESSAGE =================

enum class MessageType : uint32_t {
    Text = 1
};

struct MessageHeader {
    MessageType type;
    uint32_t size;
};

// ================= CLIENT =================

struct Client {
    SOCKET socket;
    std::string color;
    int colorIndex;
    std::string clientId; // уникальный идентификатор (IP:порт)
};

std::vector<Client> clients;
std::mutex clientsMutex;
std::map<std::string, int> usedColors; // карта занятых цветов (clientId -> индекс цвета)

std::vector<std::string> colorPool = {
    "\033[31m", // red
    "\033[32m", // green
    "\033[33m", // yellow
    "\033[34m", // blue
    "\033[35m", // magenta
    "\033[36m"  // cyan
};

const std::string RESET = "\033[0m";

// ================= GLOBAL =================

std::atomic<bool> serverRunning{true};
SOCKET serverSock = INVALID_SOCKET;

// ================= NETWORK =================

void sendAll(SOCKET sock, const char* data, int size) {
    int sent = 0;
    while (sent < size) {
        int res = send(sock, data + sent, size - sent, 0);
        if (res <= 0)
            throw std::runtime_error("send failed");
        sent += res;
    }
}

void recvAll(SOCKET sock, char* data, int size) {
    int received = 0;
    while (received < size) {
        int res = recv(sock, data + received, size - received, 0);
        if (res <= 0)
            throw std::runtime_error("recv failed");
        received += res;
    }
}

// ================= EMOJI =================

std::string replaceEmoji(std::string text) {

    std::vector<std::pair<std::string, std::string>> emojis = {
        {":fire:",  u8"🔥"},
        {":smile:", u8"😄"},
        {":sad:",   u8"😢"},
        {":heart:", u8"❤️"},
        {":ok:",    u8"👌"}
    };

    for (auto& e : emojis) {
        size_t pos;
        while ((pos = text.find(e.first)) != std::string::npos)
            text.replace(pos, e.first.length(), e.second);
    }

    return text;
}

void printEmojiHelp() {
    std::cout << "\n=== Доступные эмодзи ===\n";
    std::cout << ":fire:  -> 🔥\n";
    std::cout << ":smile: -> 😄\n";
    std::cout << ":sad:   -> 😢\n";
    std::cout << ":heart: -> ❤️\n";
    std::cout << ":ok:    -> 👌\n";
    std::cout << "==========================\n\n";
}

void printCommands() {
    std::cout << "\n=== Команды сервера ===\n";
    std::cout << "/shutdown - завершить работу сервера\n";
    std::cout << "/status   - показать статус сервера\n";
    std::cout << "/clients  - список подключенных клиентов\n";
    std::cout << "/colors   - показать занятые цвета\n";
    std::cout << "/help     - показать эту справку\n";
    std::cout << "========================\n\n";
}

// ================= CLIENT ID MANAGEMENT =================

std::string getClientId(SOCKET clientSocket) {
    sockaddr_in addr;
    int addrLen = sizeof(addr);
    if (getpeername(clientSocket, (sockaddr*)&addr, &addrLen) == 0) {
        std::string ip = inet_ntoa(addr.sin_addr);
        int port = ntohs(addr.sin_port);
        return ip + ":" + std::to_string(port); // уникальная комбинация IP + порт
    }
    return "unknown:" + std::to_string(clientSocket);
}

int assignColorIndex(const std::string& clientId) {
    // Если у клиента уже был цвет - возвращаем его
    if (usedColors.find(clientId) != usedColors.end()) {
        return usedColors[clientId];
    }
    
    // Ищем первый свободный цвет
    std::vector<bool> colorUsed(colorPool.size(), false);
    
    // Отмечаем занятые цвета
    for (const auto& pair : usedColors) {
        if (pair.second >= 0 && pair.second < colorPool.size()) {
            colorUsed[pair.second] = true;
        }
    }
    
    // Ищем свободный индекс
    for (int i = 0; i < colorPool.size(); i++) {
        if (!colorUsed[i]) {
            usedColors[clientId] = i;
            return i;
        }
    }
    
    // Если все цвета заняты - используем белый
    return -1;
}

void releaseColorIndex(const std::string& clientId) {
    usedColors.erase(clientId);
}

// ================= BROADCAST =================

void broadcast(const MessageHeader& header,
               const std::vector<char>& data,
               SOCKET sender)
{
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (auto& c : clients) {
        if (c.socket == sender)
            continue;

        try {
            sendAll(c.socket, (char*)&header, sizeof(header));
            sendAll(c.socket, data.data(), header.size);
        }
        catch (...) {}
    }
}

// ================= CLIENT HANDLER =================

void handleClient(SOCKET clientSocket) {

    std::string clientId = getClientId(clientSocket);
    
    try {
        while (serverRunning) {

            MessageHeader header{};
            recvAll(clientSocket, (char*)&header, sizeof(header));

            if (header.size > 1024 * 1024)
                break;

            std::vector<char> data(header.size);
            recvAll(clientSocket, data.data(), header.size);

            std::string message(data.begin(), data.end());

            message = replaceEmoji(message);

            std::string color;
            std::string clientInfo;

            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                for (auto& c : clients)
                    if (c.socket == clientSocket) {
                        color = c.color;
                        clientInfo = "[Client " + c.clientId + "] ";
                    }
            }

            std::string coloredMessage = color + message + RESET;

            std::cout << clientInfo << coloredMessage << std::endl;

            MessageHeader outHeader{
                MessageType::Text,
                static_cast<uint32_t>(coloredMessage.size())
            };

            std::vector<char> outData(
                coloredMessage.begin(),
                coloredMessage.end()
            );

            broadcast(outHeader, outData, clientSocket);
        }
    }
    catch (...) {}

    closesocket(clientSocket);

    {
        std::lock_guard<std::mutex> lock(clientsMutex);

        clients.erase(
            std::remove_if(clients.begin(), clients.end(),
                [clientSocket](Client& c) {
                    return c.socket == clientSocket;
                }),
            clients.end()
        );

        // Освобождаем цвет этого конкретного клиента
        releaseColorIndex(clientId);

        std::cout << "Client " << clientId << " disconnected. Total: "
                  << clients.size() << std::endl;
    }
}

// ================= COMMAND HANDLER =================

void commandHandler() {
    std::string cmd;
    
    while (serverRunning) {
        if (_kbhit()) {
            std::getline(std::cin, cmd);
            
            if (cmd == "/shutdown") {
                std::cout << "\nShutting down server...\n";
                serverRunning = false;
                break;
            }
            else if (cmd == "/status") {
                std::cout << "\n=== Server Status ===\n";
                std::cout << "Running: " << (serverRunning ? "Yes" : "No") << "\n";
                std::cout << "Active clients: " << clients.size() << "\n";
                std::cout << "Used colors: " << usedColors.size() << "\n";
                std::cout << "=====================\n\n";
            }
            else if (cmd == "/clients") {
                std::lock_guard<std::mutex> lock(clientsMutex);
                std::cout << "\n=== Connected Clients (" << clients.size() << ") ===\n";
                for (auto& c : clients) {
                    std::cout << "ID: " << c.clientId
                              << ", Socket: " << c.socket
                              << ", Color: " << c.color << "text" << RESET 
                              << " (index: " << c.colorIndex << ")\n";
                }
                std::cout << "================================\n\n";
            }
            else if (cmd == "/colors") {
                std::lock_guard<std::mutex> lock(clientsMutex);
                std::cout << "\n=== Used Colors (" << usedColors.size() << ") ===\n";
                for (const auto& pair : usedColors) {
                    std::string colorStr = (pair.second >= 0 && pair.second < colorPool.size()) 
                                         ? colorPool[pair.second] : "\033[37m";
                    std::cout << "Client ID: " << pair.first 
                              << ", Color: " << colorStr << "text" << RESET 
                              << " (index: " << pair.second << ")\n";
                }
                std::cout << "================================\n\n";
            }
            else if (cmd == "/help") {
                printCommands();
            }
            else if (!cmd.empty()) {
                std::cout << "Unknown command. Type /help for list of commands.\n";
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// ================= MAIN =================

int main() {

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(54000);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSock, (sockaddr*)&addr, sizeof(addr));
    listen(serverSock, SOMAXCONN);

    std::cout << "========================================\n";
    std::cout << "Messenger server started\n";
    std::cout << "Listening on port 54000\n";
    std::cout << "Server IP: 127.0.0.1\n";
    std::cout << "========================================\n\n";
    
    printEmojiHelp();
    printCommands();

    // Запускаем обработчик команд в отдельном потоке
    std::thread cmdThread(commandHandler);

    // Устанавливаем неблокирующий режим для accept
    u_long mode = 1;
    ioctlsocket(serverSock, FIONBIO, &mode);

    while (serverRunning) {

        SOCKET client = accept(serverSock, nullptr, nullptr);

        if (client == INVALID_SOCKET) {
            int error = WSAGetLastError();
            if (error == WSAEWOULDBLOCK) {
                // Нет входящих соединений, продолжаем цикл
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                continue;
            }
            else {
                if (serverRunning) {
                    std::cout << "Accept failed: " << error << std::endl;
                }
                break;
            }
        }

        // Возвращаем блокирующий режим для клиентского сокета
        mode = 0;
        ioctlsocket(client, FIONBIO, &mode);

        std::string clientId = getClientId(client);
        
        Client newClient;
        newClient.socket = client;
        newClient.clientId = clientId;
        newClient.colorIndex = assignColorIndex(clientId);
        
        if (newClient.colorIndex >= 0 && newClient.colorIndex < colorPool.size()) {
            newClient.color = colorPool[newClient.colorIndex];
        } else {
            newClient.color = "\033[37m"; // белый цвет
            newClient.colorIndex = -1;
        }

        {
            std::lock_guard<std::mutex> lock(clientsMutex);
            clients.push_back(newClient);

            std::cout << "\n[+" << std::to_string(client) << "] "
                      << "New client connected.\n"
                      << "  ID: " << clientId << "\n"
                      << "  Color: " << newClient.color << "text" << RESET
                      << " (index: " << newClient.colorIndex << ")\n"
                      << "  Total clients: " << clients.size() << "\n";
        }

        std::thread(handleClient, client).detach();
    }

    // Ожидаем завершения потока команд
    if (cmdThread.joinable()) {
        cmdThread.join();
    }

    // Закрываем все клиентские соединения
    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (auto& c : clients) {
            closesocket(c.socket);
        }
        clients.clear();
        usedColors.clear();
    }

    // Закрываем серверный сокет
    if (serverSock != INVALID_SOCKET) {
        closesocket(serverSock);
    }

    WSACleanup();
    std::cout << "Server stopped.\n";
    return 0;
}
