#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "Ws2_32.lib")
#include <WinSock2.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <map>
#include <functional>
#include <cstdlib>
#include <mutex>

SOCKET server_fd = INVALID_SOCKET;
SOCKET client_fd = INVALID_SOCKET;
const int PORT = 8080;
const int BUFFER_SIZE = 1024;
std::atomic<bool> connectionActive(true);

std::mutex coutMutex;

using CommandHandler = std::function<void()>;
std::map<std::string, CommandHandler> commandRegistry;

void registerCommand(const std::string& name, CommandHandler handler) {
    commandRegistry[name] = handler;
}


bool executeCommand(const std::string& input) {
    if (input.empty() || input[0] != '/') {
        return false;
    }

    auto it = commandRegistry.find(input);
    if (it != commandRegistry.end()) {
        it->second();
        return true;
    }

    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "[!] Неизвестная команда: " << input << "\n";
    }
    return false;
}

void cmdExit() {
    {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "[!] Завершение чата...\n";
    }

    if (client_fd != INVALID_SOCKET) {
        send(client_fd, "/exit", 5, 0);
    }
    connectionActive = false;
}

void cmdHelp() {
    std::lock_guard<std::mutex> lock(coutMutex);
    std::cout << "\n=== ДОСТУПНЫЕ КОМАНДЫ ===\n";
    std::cout << "  /exit   - Выйти из чата\n";
    std::cout << "  /help   - Показать эту справку\n";
    std::cout << "  /time   - Показать текущее время\n";
    std::cout << "  /clear  - Очистить консоль\n";
    std::cout << "========================\n\n";
}

void cmdTime() {
    time_t now = time(0);
    char* dt = ctime(&now);
    std::lock_guard<std::mutex> lock(coutMutex);
    std::cout << "[Время] " << dt;
}

void cmdClear() {
    system("cls");
    std::lock_guard<std::mutex> lock(coutMutex);
    std::cout << "Консоль очищена\n\n";
}

void initCommands() {
    registerCommand("/exit", cmdExit);
    registerCommand("/help", cmdHelp);
    registerCommand("/time", cmdTime);
    registerCommand("/clear", cmdClear);
}

void setupConsole() {
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);
    setlocale(LC_ALL, "Russian");
    std::cout << "=== TCP ЧАТ (СЕРВЕР / КЛИЕНТ) ===\n";
}

void initWinSock() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "[!] Ошибка WSAStartup\n";
        exit(1);
    }
}

void cleanupSockets() {
    connectionActive = false;

    if (client_fd != INVALID_SOCKET) {
        shutdown(client_fd, SD_BOTH);
        closesocket(client_fd);
        client_fd = INVALID_SOCKET;
    }
    if (server_fd != INVALID_SOCKET) {
        closesocket(server_fd);
        server_fd = INVALID_SOCKET;
    }
    WSACleanup();

    std::lock_guard<std::mutex> lock(coutMutex);
    std::cout << "\n[+] Сокеты закрыты\n";
}

void showMessage(const std::string& name, const std::string& message) {
    std::lock_guard<std::mutex> lock(coutMutex);
    std::cout << "\r[" << name << "]: " << message << "\n";
    std::cout << "\r[Отправлено]: ";
    std::cout.flush();
}

void receiveThread() {
    char buffer[BUFFER_SIZE] = {};

    while (connectionActive) {
        int bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);

        if (bytes <= 0) {
            showMessage("Система", "Соединение разорвано");
            connectionActive = false;
            break;
        }

        buffer[bytes] = '\0';
        std::string msg(buffer);

        if (msg == "/exit") {
            showMessage("Система", "Собеседник завершил чат");
            connectionActive = false;
            break;
        }

        showMessage("Собеседник", msg);
    }
}

void runChat() {
    std::cout << "\n=== ЧАТ ЗАПУЩЕН ===\n";
    std::cout << "Для выхода: /exit\n";

    std::thread recvThread(receiveThread);
    std::string inputMsg;

    while (connectionActive) {
        std::cout << "[Отправлено]: ";
        std::cout.flush();
        std::getline(std::cin, inputMsg);

        if (inputMsg.empty()) continue;

        if (executeCommand(inputMsg)) {
            if (!connectionActive) break;
            continue;
        }

        if (send(client_fd, inputMsg.c_str(), static_cast<int>(inputMsg.length()), 0) == SOCKET_ERROR) {
            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "[!] Ошибка отправки (код: " << WSAGetLastError() << ")\n";
            connectionActive = false;
            break;
        }
    }

    if (recvThread.joinable()) {
        recvThread.join();
    }
}

void serverStart() {
    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "[!] Ошибка создания сокета: " << WSAGetLastError() << "\n";
        exit(1);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in address = {};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "[!] Ошибка bind (порт занят?): " << WSAGetLastError() << "\n";
        exit(1);
    }

    if (listen(server_fd, 1) == SOCKET_ERROR) {
        std::cerr << "[!] Ошибка listen: " << WSAGetLastError() << "\n";
        exit(1);
    }

    std::cout << "[+] Сервер запущен на порту " << PORT << "\n";
    std::cout << "[+] Ожидание подключения клиента...\n";

    client_fd = accept(server_fd, nullptr, nullptr);
    if (client_fd == INVALID_SOCKET) {
        std::cerr << "[!] Ошибка accept: " << WSAGetLastError() << "\n";
        exit(1);
    }
    std::cout << "[+] Клиент подключился!\n";
}

void clientConnect() {
    client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_fd == INVALID_SOCKET) {
        std::cerr << "[!] Ошибка создания сокета: " << WSAGetLastError() << "\n";
        exit(1);
    }

    sockaddr_in serverAddr = {};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(PORT);
    serverAddr.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
        std::cerr << "[!] Неверный адрес\n";
        exit(1);
    }

    std::cout << "[+] Подключение к серверу...\n";
    if (connect(client_fd, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "[!] Ошибка подключения (сервер запущен?): " << WSAGetLastError() << "\n";
        exit(1);
    }
    std::cout << "[+] Подключено!\n";
}

int chooseMode() {
    int choice;
    std::cout << "\nВыберите режим (1 или 2):\n  1. Сервер\n  2. Клиент\n";
    while (true) {
        std::cout << "Ваш выбор: ";
        if (std::cin >> choice) {
            if (choice == 1 || choice == 2) {
                // 🔥 Очистка буфера после cin, чтобы не сломать getline
                std::cin.ignore(32767, '\n');
                return choice;
            }
        }
        std::cin.clear();
        std::cin.ignore(32767, '\n');
        std::cout << "[!] Такого варианта не существует\n";
    }
}

int main() {
    setupConsole();
    initCommands();
    initWinSock();

    int mode = chooseMode();

    switch (mode) {
    case 1:
        serverStart();
        break;
    case 2:
        clientConnect();
        break;
    default:
        std::cout << "[!] Ошибка выбора режима\n";
        return 1;
    }

    runChat();
    cleanupSockets();

    return 0;
}