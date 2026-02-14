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


// ============================================================
// MESSAGE 
// ============================================================

// Минимальный протокол обмена:каждый пакет = заголовок + данные
enum class MessageType : uint32_t {
    Text = 1
};


struct MessageHeader {// Заголовок сообщает тип и размер полезной нагрузки
    MessageType type;
    uint32_t size;
};


// ============================================================
// CLIENT 
// ============================================================

struct Client {// Структура хранит состояние подключенного клиента
    SOCKET socket;// Сервер не хранит сложного состояния — только сокет,
    std::string color;// цвет 
    int colorIndex;
    std::string clientId;//и уникальный идентификатор
};


// Глобальное хранилище клиентов.
std::vector<Client> clients;
std::mutex clientsMutex;// Доступ защищён mutex, так как клиенты обслуживаются в отдельных потоках


std::map<std::string, int> usedColors;// Карта занятых цветов


std::vector<std::string> colorPool = {// Пул доступных цветов 
    "\033[31m","\033[32m","\033[33m",
    "\033[34m","\033[35m","\033[36m"
};


const std::string RESET = "\033[0m";


// ============================================================
// GLOBAL 
// ============================================================
std::atomic<bool> serverRunning{ true };// serverRunning — флаг жизненного цикла сервера
SOCKET serverSock = INVALID_SOCKET;

// ============================================================
// UTILITY FUNCTIONS 
// ============================================================

void setupConsole() { // Включаем UTF-8 для корректной работы с эмодзи
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}


void initWinSock() { // Инициализация сетевой подсистемы Windows
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        throw std::runtime_error("WSAStartup failed");
}

void cleanupWinSock() {// Освобождение сетевых ресурсов, завершение
    WSACleanup();
}


void sendAll(SOCKET sock, const char* data, int size) {// Гарантированная отправка всех байт сообщения
    int sent = 0;
    while (sent < size) {
        int res = send(sock, data + sent, size - sent, 0);
        if (res <= 0) throw std::runtime_error("send failed");
        sent += res;
    }
}


void recvAll(SOCKET sock, char* data, int size) {// Гарантированное получение точного количества байт
    int received = 0;
    while (received < size) {
        int res = recv(sock, data + received, size - received, 0);
        if (res <= 0) throw std::runtime_error("recv failed");
        received += res;
    }
}


// ============================================================
// EMOJI 
// ============================================================

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
    std::cout << "/clients  - список клиентов\n";
    std::cout << "/colors   - занятые цвета\n";
    std::cout << "/help     - помощь\n";
    std::cout << "========================\n\n";
}

// ============================================================
//  CLIENT MANAGEMENT 
// ============================================================

std::string getClientId(SOCKET clientSocket) {// Логика управления подключёнными клиентами
    sockaddr_in addr;
    int len = sizeof(addr);
    if (getpeername(clientSocket, (sockaddr*)&addr, &len) == 0) {
        return std::string(inet_ntoa(addr.sin_addr)) + ":" +
               std::to_string(ntohs(addr.sin_port));
    }
    return "unknown";
}

int assignColorIndex(const std::string& clientId) {// Назначение цвета клиенту, гарантирует уникальность цвета среди активных клиентов

    if (usedColors.count(clientId))
        return usedColors[clientId];

    std::vector<bool> used(colorPool.size(), false);

    for (auto& p : usedColors)
        if (p.second >= 0 && p.second < colorPool.size())
            used[p.second] = true;

    for (int i = 0; i < colorPool.size(); i++)
        if (!used[i]) {
            usedColors[clientId] = i;
            return i;
        }

    return -1;
}

void releaseColorIndex(const std::string& clientId) {
    usedColors.erase(clientId);
}

void broadcast(const MessageHeader& header, // Рассылка сообщения всем клиентам кроме отправителя
               const std::vector<char>& data,
               SOCKET sender) {

    std::lock_guard<std::mutex> lock(clientsMutex);

    for (auto& c : clients) {
        if (c.socket == sender) continue;
        try {
            sendAll(c.socket, (char*)&header, sizeof(header));
            sendAll(c.socket, data.data(), header.size);
        }
        catch (...) {}
    }
}

// ============================================================
// ================= CLIENT THREAD ============================
// ============================================================

void handleClient(SOCKET clientSocket) {

    std::string clientId = getClientId(clientSocket);

    try {
        while (serverRunning) {

            MessageHeader header{};
            recvAll(clientSocket, (char*)&header, sizeof(header));

            if (header.size > 1024 * 1024) break;

            std::vector<char> data(header.size);
            recvAll(clientSocket, data.data(), header.size);

            std::string message(data.begin(), data.end());
            message = replaceEmoji(message);

            std::string color, clientInfo;

            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                for (auto& c : clients)
                    if (c.socket == clientSocket) {
                        color = c.color;
                        clientInfo = "[Client " + c.clientId + "] ";
                    }
            }

            std::string colored = color + message + RESET;

            std::cout << clientInfo << colored << std::endl;

            MessageHeader outHeader{ MessageType::Text,
                                     (uint32_t)colored.size() };

            std::vector<char> outData(colored.begin(), colored.end());
            broadcast(outHeader, outData, clientSocket);
        }
    }
    catch (...) {}

    closesocket(clientSocket);

    {
        std::lock_guard<std::mutex> lock(clientsMutex);

        clients.erase(std::remove_if(clients.begin(), clients.end(),
            [clientSocket](Client& c) { return c.socket == clientSocket; }),
            clients.end());

        releaseColorIndex(clientId);

        std::cout << "Client " << clientId
                  << " disconnected. Total: "
                  << clients.size() << "\n";
    }
}

// ============================================================
// ================= COMMAND THREAD ===========================
// ============================================================

void commandHandler() {// Отдельный поток для управления сервером.Позволяет серверу принимать клиентов, не блокируясь ожиданием ввода команд.

    std::string cmd;

    while (serverRunning) {

        if (_kbhit()) {
            std::getline(std::cin, cmd);

            if (cmd == "/shutdown") {
                serverRunning = false;
                break;
            }
            else if (cmd == "/status") {
                std::cout << "Active clients: "
                          << clients.size() << "\n";
            }
            else if (cmd == "/clients") {
                std::lock_guard<std::mutex> lock(clientsMutex);
                for (auto& c : clients)
                    std::cout << c.clientId << "\n";
            }
            else if (cmd == "/colors") {
                for (auto& p : usedColors)
                    std::cout << p.first
                              << " -> " << p.second << "\n";
            }
            else if (cmd == "/help") {
                printCommands();
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

// ============================================================
// ================= SERVER START/STOP ========================
// ============================================================

void createServerSocket() {

    serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(54000);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSock, (sockaddr*)&addr, sizeof(addr));
    listen(serverSock, SOMAXCONN);
}

void handleNewClient(SOCKET client) {

    u_long mode = 0;
    ioctlsocket(client, FIONBIO, &mode);

    std::string id = getClientId(client);

    Client newClient;
    newClient.socket = client;
    newClient.clientId = id;
    newClient.colorIndex = assignColorIndex(id);

    if (newClient.colorIndex >= 0)
        newClient.color = colorPool[newClient.colorIndex];
    else
        newClient.color = "\033[37m";

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        clients.push_back(newClient);
        std::cout << "New client: " << id << "\n";
    }

    std::thread(handleClient, client).detach();
}

void runAcceptLoop() {// Основной цикл приёма новых клиентов

    u_long mode = 1;
    ioctlsocket(serverSock, FIONBIO, &mode);

    while (serverRunning) {

        SOCKET client = accept(serverSock, nullptr, nullptr);

        if (client == INVALID_SOCKET) {
            if (WSAGetLastError() == WSAEWOULDBLOCK) {
                std::this_thread::sleep_for(
                    std::chrono::milliseconds(100));
                continue;
            }
            break;
        }

        handleNewClient(client);
    }
}

void shutdownServer(std::thread& cmdThread) {// Централизованная процедура завершения сервера.

    if (cmdThread.joinable())
        cmdThread.join();

    {
        std::lock_guard<std::mutex> lock(clientsMutex);
        for (auto& c : clients)
            closesocket(c.socket);

        clients.clear();
        usedColors.clear();
    }

    if (serverSock != INVALID_SOCKET)
        closesocket(serverSock);

    cleanupWinSock();
    std::cout << "Server stopped.\n";
}

// ============================================================
// ================= MAIN =====================================
// ============================================================

int main() {
    // Вся логика распределена по слоям выше.

    setupConsole();
    initWinSock();
    createServerSocket();

    printEmojiHelp();
    printCommands();

    std::thread cmdThread(commandHandler);

    runAcceptLoop();
    shutdownServer(cmdThread);

    return 0;
}

