/**
 * client_pc.cpp
 * Клиент для Windows, подключается к Raspberry Pi (порт 8081).
 *
 * Команды:
 *   *GetLogs - получить отчёт о превышениях температуры
 *   *Help    - справка
 *   *Quit    - выйти из клиента (сервер продолжает работать)
 */

#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <conio.h>   // _getch()

#pragma comment(lib, "Ws2_32.lib")

// ──────────────────────── Константы ────────────────────────
const unsigned short kServerPort = 8081;
const char* kDefaultServerIP = "10.42.0.1";   // измените при необходимости

// ──────────────────────── Инициализация Winsock ────────────────────────
bool WinSockInit() {
    WSADATA wsa_data;
    int res = WSAStartup(MAKEWORD(2, 2), &wsa_data);
    if (res != 0) {
        std::cerr << "WSAStartup failed: " << res << std::endl;
        return false;
    }
    return true;
}

// ──────────────────────── Создание сокета ────────────────────────
SOCKET CreateSocket() {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
    }
    return sock;
}

// ──────────────────────── Подключение к серверу ────────────────────────
bool ConnectToServer(SOCKET sock, const std::string& server_ip) {
    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(kServerPort);
    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());

    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        std::cerr << "Invalid server IP: " << server_ip << std::endl;
        return false;
    }

    std::cout << "Connecting to " << server_ip << ":" << kServerPort << "..." << std::endl;
    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
        std::cerr << "Connect failed: " << WSAGetLastError() << std::endl;
        return false;
    }
    std::cout << "Connected successfully." << std::endl;
    return true;
}

// ──────────────────────── Отправка команды и получение ответа ────────────────────────
bool SendCommand(SOCKET sock, const std::string& command, std::string& response) {
    if (send(sock, command.c_str(), command.length(), 0) == SOCKET_ERROR) {
        std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
        return false;
    }

    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    int received = recv(sock, buffer, sizeof(buffer) - 1, 0);
    if (received <= 0) {
        std::cerr << "Receive failed or connection closed." << std::endl;
        return false;
    }
    response = std::string(buffer);
    return true;
}

// ──────────────────────── Главная функция клиента ────────────────────────
int main() {
    std::cout << "Raspberry Pi Temperature Log Client" << std::endl;

    if (!WinSockInit()) return 1;

    std::string server_ip;
    std::cout << "Enter Raspberry Pi IP address [" << kDefaultServerIP << "]: ";
    std::getline(std::cin, server_ip);
    if (server_ip.empty()) server_ip = kDefaultServerIP;

    SOCKET client_socket = CreateSocket();
    if (client_socket == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }

    if (!ConnectToServer(client_socket, server_ip)) {
        closesocket(client_socket);
        WSACleanup();
        return 1;
    }

    std::cout << "\nEnter commands (*Help for list, *Quit to exit).\n";

    while (true) {
        std::cout << "> ";
        std::string command;
        std::getline(std::cin, command);
        if (command.empty()) continue;

        std::string response;
        if (!SendCommand(client_socket, command, response)) {
            std::cerr << "Connection lost." << std::endl;
            break;
        }

        std::cout << "\n=== Server response ===" << std::endl;
        std::cout << response;
        std::cout << "========================" << std::endl;

        if (command == "*Quit") {
            break;
        }
    }

    closesocket(client_socket);
    WSACleanup();

    std::cout << "Client stopped." << std::endl;
    std::cout << "Press any key to exit...";
    _getch();
    return 0;
}