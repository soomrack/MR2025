// ============================================================
//  Client (Windows side)
//  Build: cl /W3 /EHsc client.cpp /link Ws2_32.lib
// ============================================================
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS

#include <WinSock2.h>
#include <Ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <atomic>
#include <sstream>
#include <stdexcept>
#include <cstdint>
#include <ctime>

#pragma comment(lib, "Ws2_32.lib")

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

// ============================================================
// СОСТОЯНИЕ
// ============================================================
static SOCKET            g_socket    = INVALID_SOCKET;
static std::atomic<bool> g_connected {false};
static std::atomic<bool> g_running   {true};
static std::mutex        g_printMutex;
static std::string       g_username;

// ============================================================
// ОБЪЯВЛЕНИЯ ФУНКЦИЙ
// ============================================================

// Инициализация
static void setupConsole();
static void initWinSock();
static void cleanupWinSock();

// Утилиты
static std::string getCurrentTimeHMS();
static std::string askUsername();

// Сеть (низкий уровень)
static void sendAllRaw(const char* data, int size);
static void recvAll(char* data, int size);

// Отправка сообщений
static void sendChatMessage(const std::string& text);
static void sendLogRequest(const std::string& payload);

// Поток приёма
static void printInputPrompt();
static void receiveLoop();

// Подключение
static bool connectToServer(const std::string& ip, int port);
static void disconnectFromServer();

// Парсинг команд
static std::string normalizeCommand(const std::string& input, std::string& outRest);
static std::string buildLogsPayload(const std::string& args);

// UI
static void printHelp();
static void clientInputLoop();

// ============================================================
// ИНИЦИАЛИЗАЦИЯ
// ============================================================
static void setupConsole() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut != INVALID_HANDLE_VALUE) {
        DWORD mode = 0;
        if (GetConsoleMode(hOut, &mode))
            SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
}

static void initWinSock()    { WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa); }
static void cleanupWinSock() { WSACleanup(); }

// ============================================================
// УТИЛИТЫ
// ============================================================
static std::string getCurrentTimeHMS() {
    time_t now = time(nullptr);
    struct tm ti;
    localtime_s(&ti, &now);
    char buf[16];
    strftime(buf, sizeof(buf), "%H:%M:%S", &ti);
    return buf;
}

static std::string askUsername() {
    while (true) {
        std::cout << "Enter your name (3-20 characters): ";
        std::string name;
        std::getline(std::cin, name);

        size_t first = name.find_first_not_of(" \t");
        if (first == std::string::npos) {
            std::cout << "Name cannot be empty.\n";
            continue;
        }
        size_t last = name.find_last_not_of(" \t");
        name = name.substr(first, last - first + 1);

        if (name.length() < 3 || name.length() > 20) {
            std::cout << "Length must be between 3 and 20.\n";
            continue;
        }
        return name;
    }
}

// ============================================================
// СЕТЬ
// ============================================================
static void sendAllRaw(const char* data, int size) {
    int sent = 0;
    while (sent < size) {
        int r = send(g_socket, data + sent, size - sent, 0);
        if (r <= 0) throw std::runtime_error("send failed");
        sent += r;
    }
}

static void recvAll(char* data, int size) {
    int got = 0;
    while (got < size) {
        int r = recv(g_socket, data + got, size - got, 0);
        if (r <= 0) throw std::runtime_error("recv failed");
        got += r;
    }
}

// ============================================================
// ОТПРАВКА СООБЩЕНИЙ
// ============================================================
static void sendChatMessage(const std::string& text) {
    std::string body = "[" + g_username + " " + getCurrentTimeHMS() + "] " + text;
    MessageHeader header{ (uint32_t)MessageType::Text, (uint32_t)body.size() };
    sendAllRaw((char*)&header, sizeof(header));
    sendAllRaw(body.data(), (int)body.size());
}

static void sendLogRequest(const std::string& payload) {
    MessageHeader header{ (uint32_t)MessageType::LogRequest,
                          (uint32_t)payload.size() };
    sendAllRaw((char*)&header, sizeof(header));
    if (!payload.empty())
        sendAllRaw(payload.data(), (int)payload.size());
}

// ============================================================
// ПОТОК ПРИЁМА
// ============================================================
static void printInputPrompt() {
    std::cout << "> " << std::flush;
}

static void receiveLoop() {
    try {
        while (g_connected) {
            MessageHeader header{};
            recvAll((char*)&header, sizeof(header));

            if (header.size > MAX_PAYLOAD_BYTES)
                throw std::runtime_error("payload too big");

            std::vector<char> data(header.size);
            if (header.size > 0) recvAll(data.data(), (int)header.size);
            std::string text(data.begin(), data.end());

            std::lock_guard<std::mutex> lock(g_printMutex);
            switch ((MessageType)header.type) {
                case MessageType::LogResponse:
                    std::cout << "\n--- response ---\n"
                              << text
                              << "----------------\n";
                    break;
                case MessageType::Warning:
                    std::cout << "\n\033[31m[!! SERVER WARNING] "
                              << text << "\033[0m\n";
                    break;
                case MessageType::Text:
                    std::cout << "\n" << text << "\n";
                    break;
                default:
                    std::cout << "\n" << text << "\n";
                    break;
            }
            printInputPrompt();
        }
    }
    catch (...) {
        std::lock_guard<std::mutex> lock(g_printMutex);
        std::cout << "\n[disconnected from server]\n";
        printInputPrompt();
        g_connected = false;
        closesocket(g_socket);
        g_socket = INVALID_SOCKET;
    }
}

// ============================================================
// ПОДКЛЮЧЕНИЕ / ОТКЛЮЧЕНИЕ
// ============================================================
static bool connectToServer(const std::string& ip, int port) {
    g_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (g_socket == INVALID_SOCKET) {
        std::cout << "socket() failed\n";
        return false;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port   = htons((u_short)port);
    if (inet_pton(AF_INET, ip.c_str(), &serverAddr.sin_addr) != 1) {
        std::cout << "Invalid IP address.\n";
        closesocket(g_socket);
        g_socket = INVALID_SOCKET;
        return false;
    }
    if (connect(g_socket, (sockaddr*)&serverAddr, sizeof(serverAddr)) != 0) {
        std::cout << "Connection failed (code " << WSAGetLastError() << ").\n";
        closesocket(g_socket);
        g_socket = INVALID_SOCKET;
        return false;
    }

    // Отправляем handshake с именем пользователя
    try {
        MessageHeader header{ (uint32_t)MessageType::Connect,
                              (uint32_t)g_username.size() };
        sendAllRaw((char*)&header, sizeof(header));
        sendAllRaw(g_username.data(), (int)g_username.size());
    }
    catch (...) {
        std::cout << "Failed to send handshake.\n";
        closesocket(g_socket);
        g_socket = INVALID_SOCKET;
        return false;
    }

    g_connected = true;
    std::thread(receiveLoop).detach();
    std::cout << "Connected to " << ip << ":" << port
              << " as '" << g_username << "'\n";
    return true;
}

static void disconnectFromServer() {
    if (!g_connected) return;
    try {
        MessageHeader header{ (uint32_t)MessageType::Disconnect,
                              (uint32_t)g_username.size() };
        sendAllRaw((char*)&header, sizeof(header));
        sendAllRaw(g_username.data(), (int)g_username.size());
    } catch (...) {}

    g_connected = false;
    if (g_socket != INVALID_SOCKET) {
        closesocket(g_socket);
        g_socket = INVALID_SOCKET;
    }
    std::cout << "Disconnected.\n";
}

// ============================================================
// ПАРСИНГ КОМАНД
// ============================================================
static std::string normalizeCommand(const std::string& input, std::string& outRest) {
    if (input.empty() || input[0] != '/') return "";

    size_t spacePos = input.find(' ');
    std::string head = (spacePos == std::string::npos) ? input : input.substr(0, spacePos);
    outRest          = (spacePos == std::string::npos) ? "" : input.substr(spacePos + 1);

    for (auto& ch : head) ch = (char)tolower((unsigned char)ch);

    static const std::vector<std::pair<std::string, std::string>> aliasTable = {
        {"/connect",     "/connect"},    {"/c",           "/connect"},
        {"/disconnect",  "/disconnect"}, {"/d",           "/disconnect"},
        {"/exit",        "/exit"},       {"/q",           "/exit"}, {"/quit", "/exit"},
        {"/help",        "/help"},       {"/h",           "/help"}, {"/?",    "/help"},
        {"/temp",        "/temp"},       {"/t",           "/temp"},
        {"/status",      "/status"},     {"/s",           "/status"},
        {"/test",        "/test"},
        {"/logs",        "/logs"},       {"/l",           "/logs"},
        {"/sensor_all",  "/sensor_all"},
        {"/sensor_last", "/sensor_last"},
    };

    for (auto& [alias, canonical] : aliasTable)
        if (head == alias) return canonical;

    return head;
}

static std::string buildLogsPayload(const std::string& args) {
    if (args.empty()) return "";

    std::stringstream ss(args);
    std::string sub;
    ss >> sub;
    for (auto& ch : sub) ch = (char)tolower((unsigned char)ch);

    if (sub == "all"  || sub == "a")                           return "ALL";
    if (sub == "warnings" || sub == "warn" || sub == "w")      return "WARNINGS";
    if (sub == "last" || sub == "l") {
        int minutes = 0;
        if (!(ss >> minutes) || minutes <= 0) return "";
        return "LAST " + std::to_string(minutes);
    }
    return "";
}

// ============================================================
// HELP
// ============================================================
static void printHelp() {
    std::cout <<
        "\nCommands (short aliases in parentheses):\n"
        "  /connect <ip> <port>      (/c)     connect to the Raspberry Pi server\n"
        "  /disconnect               (/d)     close current connection\n"
        "  /temp                     (/t)     current CPU temperature\n"
        "  /status                   (/s)     full status (CPU/RAM/uptime/clients)\n"
        "  /test                              receive " + std::to_string(10) + " TEST messages\n"
        "  /logs all                 (/l a)   full server log buffer\n"
        "  /logs warnings            (/l w)   only WARNING entries\n"
        "  /logs last <minutes>      (/l l N) server logs for last N minutes\n"
        "  /sensor_all                        all Arduino sensor data\n"
        "  /sensor_last <minutes>             sensor data for last N minutes\n"
        "  /help                     (/h, /?) this help\n"
        "  /exit                     (/q)     quit\n"
        "  Anything else is sent as a chat message.\n\n";
}

// ============================================================
// ОСНОВНОЙ ЦИКЛ ВВОДА
// ============================================================
static void clientInputLoop() {
    std::string input;
    while (g_running) {
        printInputPrompt();
        if (!std::getline(std::cin, input)) {
            g_running = false;
            break;
        }
        if (input.empty()) continue;

        // Обычное сообщение в чат
        if (input[0] != '/') {
            try {
                if (!g_connected) { std::cout << "Not connected.\n"; continue; }
                sendChatMessage(input);
            } catch (const std::exception& e) {
                std::cout << "Error: " << e.what() << "\n";
            }
            continue;
        }

        std::string rest;
        std::string cmd = normalizeCommand(input, rest);

        try {
            if (cmd == "/connect") {
                if (g_connected) { std::cout << "Already connected.\n"; continue; }
                std::stringstream ss(rest);
                std::string ip; int port = 0;
                ss >> ip >> port;
                if (ip.empty() || port <= 0) {
                    std::cout << "Usage: /connect <ip> <port>\n"; continue;
                }
                connectToServer(ip, port);
            }
            else if (cmd == "/disconnect") {
                disconnectFromServer();
            }
            else if (cmd == "/exit") {
                g_running = false;
                disconnectFromServer();
            }
            else if (cmd == "/help") {
                printHelp();
            }
            else if (cmd == "/temp") {
                if (!g_connected) { std::cout << "Not connected.\n"; continue; }
                sendLogRequest("TEMP");
            }
            else if (cmd == "/status") {
                if (!g_connected) { std::cout << "Not connected.\n"; continue; }
                sendLogRequest("STATUS");
            }
            else if (cmd == "/test") {
                if (!g_connected) { std::cout << "Not connected.\n"; continue; }
                sendLogRequest("TEST");
            }
            else if (cmd == "/logs") {
                if (!g_connected) { std::cout << "Not connected.\n"; continue; }
                std::string payload = buildLogsPayload(rest);
                if (payload.empty()) {
                    std::cout << "Usage: /logs all | /logs warnings | /logs last <minutes>\n";
                    continue;
                }
                sendLogRequest(payload);
            }
            else if (cmd == "/sensor_all") {
                if (!g_connected) { std::cout << "Not connected.\n"; continue; }
                sendLogRequest("SENSOR_ALL");
            }
            else if (cmd == "/sensor_last") {
                if (!g_connected) { std::cout << "Not connected.\n"; continue; }
                if (rest.empty()) { std::cout << "Usage: /sensor_last <minutes>\n"; continue; }
                sendLogRequest("SENSOR_LAST " + rest);
            }
            else {
                std::cout << "Unknown command '" << cmd << "'. Type /help.\n";
            }
        }
        catch (const std::exception& e) {
            std::cout << "Error: " << e.what() << "\n";
            g_connected = false;
            if (g_socket != INVALID_SOCKET) {
                closesocket(g_socket);
                g_socket = INVALID_SOCKET;
            }
        }
    }
}

// ============================================================
// MAIN
// ============================================================
int main() {
    setupConsole();
    initWinSock();

    std::cout << "=== Hardware Monitor Client ===\n";
    g_username = askUsername();
    printHelp();
    clientInputLoop();

    cleanupWinSock();
    return 0;
}
