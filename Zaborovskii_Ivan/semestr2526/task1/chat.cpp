// Определяем версию Windows для поддержки inet_pton
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0600 // Windows Vista и выше
#endif



#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <cstring>
#include <string>
#include <thread>
#include <atomic>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

std::atomic<bool> running(true);

// Функция для приема сообщений (работает в отдельном потоке)
void receiveMessages(SOCKET sock, int& messageCount) {
    char buffer[4096];
    
    while (running) {
        memset(buffer, 0, sizeof(buffer));
        
        // Получаем сообщение
        int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
        
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            messageCount++;
            std::cout << "\n[Received] " << buffer << std::endl;
            std::cout << "You: "; // Приглашение для ввода
            std::cout.flush();
        } else if (bytesReceived == 0) {
            std::cout << "\n[System] Connection closed by remote peer" << std::endl;
            running = false;
            break;
        } else {
            int error = WSAGetLastError();
            if (error != WSAEWOULDBLOCK) {
                std::cerr << "\n[Error] Receive failed: " << error << std::endl;
                running = false;
                break;
            }
        }
        
        // Небольшая задержка для снижения нагрузки
        Sleep(10);
    }
}

// Функция для отправки сообщений (главный поток)
void sendMessages(SOCKET sock, int& messageCount) {
    std::string message;
    
    while (running) {
        std::cout << "You: ";
        std::getline(std::cin, message);
        
        if (message == "quit" || message == "exit") {
            std::cout << "[System] Closing connection..." << std::endl;
            running = false;
            break;
        }
        
        if (message.empty()) {
            continue;
        }
        
        // Отправляем сообщение
        int bytesSent = send(sock, message.c_str(), message.length(), 0);
        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "[Error] Send failed: " << WSAGetLastError() << std::endl;
            running = false;
            break;
        }
        
        messageCount++;
    }
}

int main() {
    WSADATA wsaData;
    std::string choice;
    
    std::cout << "========================================" << std::endl;
    std::cout << "      CHAT PROGRAM (Client & Server)   " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "1. Create server (wait for connection)" << std::endl;
    std::cout << "2. Connect to server" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Choose mode (1 or 2): ";
    std::getline(std::cin, choice);
    
    // Инициализация Winsock
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }
    
    SOCKET mainSocket = INVALID_SOCKET;
    int port = 8080;
    
    if (choice == "1") {
        // Режим сервера - ожидание подключения
        std::cout << "\n[Server Mode] Creating server..." << std::endl;
        std::cout << "Enter port (default 8080): ";
        std::string portStr;
        std::getline(std::cin, portStr);
        if (!portStr.empty()) {
            port = std::stoi(portStr);
        }
        
        // Создаем сокет для прослушивания
        SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (listenSocket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed" << std::endl;
            WSACleanup();
            return 1;
        }
        
        // Настраиваем адрес
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        
        // Привязываем сокет
        if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed: " << WSAGetLastError() << std::endl;
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }
        
        // Начинаем прослушивание
        if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Listen failed: " << WSAGetLastError() << std::endl;
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }
        
        // Получаем локальный IP
        char hostname[256];
        gethostname(hostname, sizeof(hostname));
        struct hostent* host = gethostbyname(hostname);
        
        std::cout << "\n[Server] Waiting for connection..." << std::endl;
        std::cout << "[Server] Your IP addresses:" << std::endl;
        if (host) {
            for (int i = 0; host->h_addr_list[i] != NULL; i++) {
                struct in_addr addr;
                memcpy(&addr, host->h_addr_list[i], sizeof(addr));
                std::cout << "        - " << inet_ntoa(addr) << ":" << port << std::endl;
            }
        }
        std::cout << "        - 127.0.0.1:" << port << " (localhost)" << std::endl;
        std::cout << "\n[Server] Waiting for client to connect..." << std::endl;
        
        // Принимаем подключение
        sockaddr_in clientAddr{};
        int clientSize = sizeof(clientAddr);
        mainSocket = accept(listenSocket, (sockaddr*)&clientAddr, &clientSize);
        
        if (mainSocket == INVALID_SOCKET) {
            std::cerr << "Accept failed: " << WSAGetLastError() << std::endl;
            closesocket(listenSocket);
            WSACleanup();
            return 1;
        }
        
        // Получаем IP клиента
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
        std::cout << "\n[Server] Client connected from: " << clientIP << ":" << ntohs(clientAddr.sin_port) << std::endl;
        
        // Закрываем слушающий сокет, он больше не нужен
        closesocket(listenSocket);
        
    } else if (choice == "2") {
        // Режим клиента - подключение к серверу
        std::string serverIP;
        std::cout << "\n[Client Mode] Enter server IP address: ";
        std::getline(std::cin, serverIP);
        
        std::cout << "Enter port (default 8080): ";
        std::string portStr;
        std::getline(std::cin, portStr);
        if (!portStr.empty()) {
            port = std::stoi(portStr);
        }
        
        // Создаем сокет
        mainSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (mainSocket == INVALID_SOCKET) {
            std::cerr << "Socket creation failed" << std::endl;
            WSACleanup();
            return 1;
        }
        
        // Настраиваем адрес сервера
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(port);
        serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());
        
        if (serverAddr.sin_addr.s_addr == INADDR_NONE) {
            std::cerr << "Invalid IP address" << std::endl;
            closesocket(mainSocket);
            WSACleanup();
            return 1;
        }
        
        std::cout << "[Client] Connecting to " << serverIP << ":" << port << "..." << std::endl;
        
        // Подключаемся
        if (connect(mainSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Connection failed: " << WSAGetLastError() << std::endl;
            closesocket(mainSocket);
            WSACleanup();
            return 1;
        }
        
        std::cout << "[Client] Connected successfully!" << std::endl;
        
    } else {
        std::cerr << "Invalid choice" << std::endl;
        WSACleanup();
        return 1;
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "      CHAT STARTED!                    " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Commands:" << std::endl;
    std::cout << "  - Type your message and press Enter" << std::endl;
    std::cout << "  - Type 'quit' or 'exit' to close" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "\nYou can start chatting!\n" << std::endl;
    
    // Счетчики сообщений
    int sentCount = 0;
    int receivedCount = 0;
    
    // Запускаем поток для приема сообщений
    std::thread receiver(receiveMessages, mainSocket, std::ref(receivedCount));
    
    // В главном потоке отправляем сообщения
    sendMessages(mainSocket, sentCount);
    
    // Ожидаем завершения потока приема
    if (receiver.joinable()) {
        receiver.join();
    }
    
    // Закрываем сокет
    std::cout << "\n========================================" << std::endl;
    std::cout << "Chat statistics:" << std::endl;
    std::cout << "  Messages sent: " << sentCount << std::endl;
    std::cout << "  Messages received: " << receivedCount << std::endl;
    std::cout << "========================================" << std::endl;
    
    closesocket(mainSocket);
    WSACleanup();
    
    std::cout << "Chat finished. Press Enter to exit..." << std::endl;
    std::cin.get();
    
    return 0;
}