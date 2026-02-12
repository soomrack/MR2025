#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#pragma comment(lib, "ws2_32.lib")

int main() {
    std::cout << "Starting server" << std::endl;
    //Инициализация WinSock
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed\n";
        return 1;
    }
    
//Создание сокета
SOCKET server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
if (server_fd == INVALID_SOCKET) {
    std::cerr << "Socket creation failed: " << WSAGetLastError() << "\n";
    WSACleanup();
    return 1;
}

//Привязка сокета к адресу и порту
sockaddr_in address{}; //Структура для IPv4 адреса
address.sin_family = AF_INET;
address.sin_addr.s_addr = INADDR_ANY; //Принимаются соединения на всех сетевых интерфейсах
address.sin_port = htons(8080);

if (bind(server_fd, (sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
    std::cerr << "Bind failed: " << WSAGetLastError() << "\n";
    closesocket(server_fd);
    WSACleanup();
    return 1;
}

//Режим ожидания подключения    
if (listen(server_fd, 1) == SOCKET_ERROR) {
    std::cerr << "Listen failed: " << WSAGetLastError() << "\n";
    closesocket(server_fd);
    WSACleanup();
    return 1;
}

//Прием подключения от клиента
SOCKET client_fd = accept(server_fd, nullptr, nullptr);
if (client_fd == INVALID_SOCKET) {
    std::cerr << "Accept failed: " << WSAGetLastError() << "\n";
    closesocket(server_fd);
    WSACleanup();
    return 1;
}

while (true){
    //Прием сообщения
    char buffer[1024] = {};
    int recieved_bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    if (recieved_bytes > 0) {
        buffer[recieved_bytes] = '\0';  // Завершаем строку
         if (std::string(buffer) == "*Quit"){
                std::cout << "Connection closed by client..." << std::endl;
                break;
            }
        std::cout << '\n' << "Received message: " << buffer << std::endl;
        std::cout << "---------------------------------------" << std::endl;
    }
    //Отправка сообщения
    std::string message;
    std::cout <<"Enter message" << std::endl;
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
}

closesocket(client_fd);
closesocket(server_fd);
WSACleanup();  // Освобождение ресурсов WinSock
return 0;
}
