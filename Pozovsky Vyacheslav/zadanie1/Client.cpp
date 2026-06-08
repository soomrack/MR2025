#include <iostream>
#include <thread>
#include <cstring>
#include <string>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
using socket_t = SOCKET;
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
using socket_t = int;
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#endif

void receive_messages(socket_t sock) {
    char buffer[1024];         //создание буфера
    while (true) {
        memset(buffer, 0, sizeof(buffer)); //зануление буфера
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) {
            std::cout << "\nDisconnected from server.\n";
            break;
        }
        std::cout << "\n" << buffer;
        std::cout << "> ";  // снова показать приглашение ввода
    }
}

int main() {
#ifdef _WIN32
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    socket_t sock = socket(AF_INET, SOCK_STREAM, 0); //создание клиентского сокета и для приёма и для отправки 
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";
        return 1;
    }

    sockaddr_in server_addr{}; //структура указывающая данные в сокет клиента 
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(12345);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {   //устанавливающая TCP-соединение с сервером
        std::cerr << "Connection failed\n";
        return 1;
    }

    std::cout << "Connected to server.\n";

    // Запрос имени
    std::string nickname;
    std::cout << "Enter your nickname: ";
    std::getline(std::cin, nickname);
    std::cout << "You are '" << nickname << "'. Type messages:\n> ";

    // Запуск потока приёма сообщений
    std::thread receiver(receive_messages, sock);

    receiver.detach(); //делает поток независимым: он будет работать параллельно

    // Главный поток: отправка сообщений с подписью
    std::string input;
    while (std::getline(std::cin, input)) {      // ждёт, пока пользователь введёт строку и нажмёт Enter. Строка сохраняется в input
        if (input.empty()) continue;   // игнорируем пустые строки
        std::string full_msg = "[" + nickname + "]: " + input + "\n";
        send(sock, full_msg.c_str(), full_msg.size(), 0); //отправляет байты сообщения через сокет на сервер
        std::cout << "> ";
    }

#ifdef _WIN32
    closesocket(sock);
    WSACleanup();
#else
    close(sock);
#endif
    return 0;
}