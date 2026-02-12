#include <iostream>
#include <winsock2.h>  // Основной заголовок WinSock
#include <ws2tcpip.h>   
#include <string.h>
#pragma comment(lib, "Ws2_32.lib")  

int main() {
    // Инициализация WinSock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return 1;
    }

    // Создание сокета
    SOCKET client_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_fd == INVALID_SOCKET) {
        std::cerr << "socket failed: " << WSAGetLastError() << std::endl;
        WSACleanup();
        return 1;
    }

    // Настройка адреса сервера для приема сообщений
    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(8080);

    // Преобразование IP-адреса сервера
    result = inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);
    if (result <= 0) {
        std::cerr << "inet_pton failed: " << result << std::endl;
        closesocket(client_fd);
        WSACleanup();
        return 1;
    }

    // Подключение к серверу
    result = connect(client_fd, (sockaddr*)&server, sizeof(server));
    if (result == SOCKET_ERROR) {
        std::cerr << "connect failed: " << WSAGetLastError() << std::endl;
        closesocket(client_fd);
        WSACleanup();
        return 1;
    }
    else{
        std::cout <<"Connected to server"<<std::endl;
    }

    while (true){
        // Отправка сообщения
        std::string message;
        std::cout <<"Enter message"<<std::endl;
        std::getline(std::cin, message);

        if (message == "*Quit"){        //Принудительная остановка
             std::cout << "Closing connection..." << std::endl;
             result = send(client_fd, message.data(), message.length(), 0);
            if (result == SOCKET_ERROR) {
                std::cerr << "send failed: " << WSAGetLastError() << std::endl;
                return 1;
            }
            break;
        }

        result = send(client_fd, message.data(), message.length(), 0);
        if (result == SOCKET_ERROR) {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
            closesocket(client_fd);
            WSACleanup();
            return 1;
        }

        //Прием сообщения
        char buffer[1024] = {};
        int recieved_bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (recieved_bytes > 0) {
            buffer[recieved_bytes] = '\0';  // Завершаем строку
            if (std::string(buffer) == "*Quit"){
                std::cout << "Connection closed by server..." << std::endl;
                break;
            }
            std::cout << '\n' << "Received message: " << buffer << std::endl;
            std::cout << "---------------------------------------" << std::endl;
        }
    }

    // Закрытие сокета и очистка
    closesocket(client_fd);
    WSACleanup();
    return 0;
}
