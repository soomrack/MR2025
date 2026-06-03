#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>

#pragma comment(lib, "ws2_32.lib")

#include <locale>

int main() {
    setlocale(LC_ALL, "Russian");
    // Инициализация Winsock
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cout << "Ошибка инициализации Winsock" << std::endl;
        return 1;
    }

    // Создание сокета
    SOCKET clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cout << "Ошибка создания сокета" << std::endl;
        WSACleanup();
        return 1;
    }

    //Указываем адрес сервера к которому подключаемся
    sockaddr_in serverAddr;
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(54000);

    inet_pton(AF_INET, "127.0.0.1", &serverAddr.sin_addr);
    int result = connect(clientSocket,
        (sockaddr*)&serverAddr,
        sizeof(serverAddr));
    if (result == SOCKET_ERROR) {
        std::cout << "Не удалось подключиться к серверу. Ошибка: "
            << WSAGetLastError() << std::endl;
        closesocket(clientSocket);
        WSACleanup();
        return 1;
    }
    std::cout << "Подключено к серверу! Введите сообщение (или 'выход' для завершения):" << std::endl;

    // Отправка сообщений в цикле
    std::string message;
    while (true) {
        std::getline(std::cin, message);  // Читаем строку от пользователя

        if (message == "exit") {
            break;
        }

        int bytesSent = send(clientSocket, message.c_str(), (int)message.size(), 0);
        if (bytesSent == SOCKET_ERROR) {
            std::cout << "Ошибка отправки: " << WSAGetLastError() << std::endl;
            break;
        }
        std::cout << "Отправлено " << bytesSent << " байт." << std::endl;
    }

    // Очистка
    closesocket(clientSocket);
    WSACleanup();
    return 0;
}