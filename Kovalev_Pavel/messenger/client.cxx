#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <unistd.h>


int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(8080);

    inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

    if (connect(sock, (sockaddr*)&server, sizeof(server)) < 0) {
        perror("connect");
        return 1;
    }

    std::string line;
    std::cout << "Введите сообщения (Ctrl+D для выхода):\n";
    while (std::getline(std::cin, line)) {
        // добавляем символ конца строки как разделитель сообщений
        line.push_back('\n');
        ssize_t sent = send(sock, line.c_str(), line.size(), 0);
        if (sent < 0) {
            perror("send");
            break;
        }
    }

    close(sock);
    return 0;
}
