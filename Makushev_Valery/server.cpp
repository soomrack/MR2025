#include <iostream>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#pragma warning(disable: 4996)

// Поток отправки сообщений
void SendThread(SOCKET sock) {
    std::string message;
    while (true) {
        std::cout << "You: ";
        std::getline(std::cin, message);
        if (message == "exit") {
            break;
        }
        int result = send(sock, message.c_str(), (int)message.size(), 0);
        if (result == SOCKET_ERROR) {
            std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
            break;
        }
    }
    closesocket(sock);
    exit(0); // Завершаем процесс при выходе
}

// Поток приёма сообщений
void ReceiveThread(SOCKET sock) {
    char buffer[1024];
    while (true) {
        int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::cout << "\nOther: " << buffer << std::endl;
            std::cout << "You: " << std::flush; // Восстанавливаем приглашение
        }
        else if (bytesReceived == 0) {
            std::cout << "Connection closed by other side." << std::endl;
            break;
        }
        else {
            std::cerr << "Recv failed: " << WSAGetLastError() << std::endl;
            break;
        }
    }
    closesocket(sock);
    exit(0);
}

int main() {
    // Инициализация Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    // Создание сокета
    SOCKET listenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listenSocket == INVALID_SOCKET) {
        std::cerr << "socket failed" << std::endl;
        WSACleanup();
        return 1;
    }

    // Привязка к порту 8888
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(8888);

    if (bind(listenSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "bind failed" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    // Прослушивание
    if (listen(listenSocket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "listen failed" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Server listening on port 8888. Waiting for client..." << std::endl;

    // Принимаем клиента
    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);
    SOCKET clientSocket = accept(listenSocket, (sockaddr*)&clientAddr, &clientAddrSize);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "accept failed" << std::endl;
        closesocket(listenSocket);
        WSACleanup();
        return 1;
    }

    closesocket(listenSocket); // Больше не нужен

    std::cout << "Client connected. You can start chatting. Type 'exit' to quit." << std::endl;

    // Запуск потоков отправки и приёма
    std::thread sender(SendThread, clientSocket);
    std::thread receiver(ReceiveThread, clientSocket);

    sender.join();
    receiver.join();

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}