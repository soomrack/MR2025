#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <string>

class ChatServer {
private:
    int server_fd;
    int client_socket;

public:
    ChatServer(int port) {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == -1) {
            perror("Socket failed");
            exit(EXIT_FAILURE);
        }

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(server_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            perror("Bind failed");
            exit(EXIT_FAILURE);
        }

        if (listen(server_fd, 3) < 0) {
            perror("Listen failed");
            exit(EXIT_FAILURE);
        }
        std::cout << "Server started on port " << port << " (Text Protocol)" << std::endl;
    }

    void start() {
        std::cout << "Waiting for connection..." << std::endl;
        client_socket = accept(server_fd, NULL, NULL);
        if (client_socket < 0) {
            perror("Accept failed");
            return;
        }
        std::cout << "Client connected!" << std::endl;

        char buffer[1024];
        while (true) {
            memset(buffer, 0, 1024);
            ssize_t bytes_read = recv(client_socket, buffer, 1024, 0);
            
            if (bytes_read <= 0) {
                std::cout << "Client disconnected." << std::endl;
                break;
            }

            processPacket(std::string(buffer));
        }
    }

    ~ChatServer() {
        if (client_socket >= 0) close(client_socket);
        if (server_fd >= 0) close(server_fd);
    }

private:
    void processPacket(std::string raw_data) {
        while (!raw_data.empty() && (raw_data.back() == '\n' || raw_data.back() == '\r')) {
            raw_data.pop_back();
        }

        if (raw_data.empty() || raw_data[0] != '/') {
            std::cout << "Invalid format: " << raw_data << std::endl;
            send(client_socket, "Error: Use /command format\n", 28, 0);
            return;
        }

        size_t space_pos = raw_data.find(' ');
        std::string cmd;
        std::string content;

        if (space_pos != std::string::npos) {
            cmd = raw_data.substr(1, space_pos - 1);
            content = raw_data.substr(space_pos + 1);
        } else {
            cmd = raw_data.substr(1);
            content = "";
        }

        // 2. Логика обработки команд
        if (cmd == "message") {
            std::cout << "[MSG]: " << content << std::endl;
            send(client_socket, "Message received\n", 17, 0);
        }
        else {
            std::cout << "Unknown command: " << cmd << std::endl;
            std::string err = "Error: Unknown command '" + cmd + "'\n";
            send(client_socket, err.c_str(), err.size(), 0);
        }
    }
};

int main() {
    ChatServer server(8080);
    server.start();
    return 0;
}
