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

    const char* message = "Hello TCP/IP from C++!";
    send(sock, message, strlen(message), 0);

    close(sock);
    return 0;
}
