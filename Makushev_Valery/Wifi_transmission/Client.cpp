#include <iostream>
#include <string>
#include <thread>
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#pragma warning(disable: 4996)

// Поток отправки сообщений (аналогично серверу)
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
    exit(0);
}

// Поток приёма сообщений
void ReceiveThread(SOCKET sock) {
    char buffer[1024];
    while (true) {
        int bytesReceived = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytesReceived > 0) {
            buffer[bytesReceived] = '\0';
            std::cout << "\nOther: " << buffer << std::endl;
            std::cout << "You: " << std::flush;
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
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed" << std::endl;
        return 1;
    }

    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cerr << "socket failed" << std::endl;
        WSACleanup();
        return 1;
    }

    // Адрес сервера (измените на реальный IP в локальной сети)
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(8888);
    const char* serverIP = "127.0.0.1"; // Замените на IP сервера, например "192.168.1.100"

    if (inet_pton(AF_INET, serverIP, &serverAddr.sin_addr) <= 0) {
        std::cerr << "Invalid address" << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    // Подключение к серверу
    if (connect(clientSocket, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "connect failed: " << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }

    std::cout << "Connected to server. Start chatting. Type 'exit' to quit." << std::endl;

    // Запуск потоков
    std::thread sender(SendThread, clientSocket);
    std::thread receiver(ReceiveThread, clientSocket);

    sender.join();
    receiver.join();

    closesocket(clientSocket);
    WSACleanup();
    return 0;
}