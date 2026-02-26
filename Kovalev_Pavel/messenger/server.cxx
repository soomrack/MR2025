#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <unistd.h>


int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, 1) < 0) {
        perror("listen");
        return 1;
    }

    std::cout << "Server is listening on port 8080...\n";

    int client_fd = accept(server_fd, nullptr, nullptr);
    if (client_fd < 0) {
        perror("accept");
        return 1;
    }

    std::cout << "Client connected\n";

    char buffer[1024] = {};
    std::string currentMessage;
    
    while (true) {
        ssize_t bytes = read(client_fd, buffer, sizeof(buffer));
        if (bytes < 0) {
            perror("read");
            break;
        }
        if (bytes == 0) {
            // клиент закрыл соединение
            std::cout << "Client disconnected\n";
            break;
        }

        // накапливаем данные и разбиваем по '\n' как по границе сообщения
        for (ssize_t i = 0; i < bytes; ++i) {
            char c = buffer[i];
            if (c == '\n') {
                std::cout << "Received message: " << currentMessage << std::endl;
                currentMessage.clear();
            } else {
                currentMessage.push_back(c);
            }
        }
    }

    close(client_fd);
    close(server_fd);
    return 0;
}
