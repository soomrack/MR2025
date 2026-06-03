#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>

#pragma comment(lib, "ws2_32.lib")

#include <locale>

int main() {
    setlocale(LC_ALL, "Russian");
    WSADATA wsaData;


    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cout << "Ошибка инициализации Winsock: " << result << std::endl;
        return 1;
    }
    std::cout << "Winsock инициализирован успешно." << std::endl;


    SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSocket == INVALID_SOCKET) {
        std::cout << "Ошибка создания сокета: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }
    std::cout << "Сокет создан успешно." << std::endl;


    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(54000);
    
    result = bind(serverSocket, (sockaddr*)&serverAddr, sizeof(serverAddr));
    if (result == SOCKET_ERROR) {
        std::cout << "Ошибка bind: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Сокет привязан к порту 54000." << std::endl;

    result = listen(serverSocket, 1);
    if (result == SOCKET_ERROR) {
        std::cout << "Ошибка listen: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Сервер запущен. Ожидание подключений на порту 54000..." << std::endl;

    sockaddr_in clientAddr;
    int clientAddrSize = sizeof(clientAddr);

    SOCKET clientSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientAddrSize);
    if (clientSocket == INVALID_SOCKET) {
        std::cout << "Ошибка accept: " << WSAGetLastError() << std::endl;
        closesocket(serverSocket);
        WSACleanup();
        return 1;
    }

    char clientIP[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientAddr.sin_addr, clientIP, INET_ADDRSTRLEN);
    std::cout << "Клиент подключился с адреса: " << clientIP << std::endl;

    char buffer[4096];

    while (true) {
        ZeroMemory(buffer, sizeof(buffer));

        int bytesReceived = recv(clientSocket, buffer, sizeof(buffer), 0);

        if (bytesReceived == SOCKET_ERROR) {
            std::cout << "Ошибка при приёме данных: " << WSAGetLastError() << std::endl;
            break;
        }

        if (bytesReceived == 0) {
            std::cout << "Клиент отключился." << std::endl;
            break;
        }

        std::cout << "Получено сообщение: " << std::string(buffer, bytesReceived) << std::endl;
    }

    closesocket(clientSocket);
    closesocket(serverSocket);
    WSACleanup();
    std::cout << "Сервер завершил работу." << std::endl;

    return 0;
}