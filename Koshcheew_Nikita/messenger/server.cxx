#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <string>


class Server {
private:
    int server_socket;
    int client_socket;
    bool running = false;
public:
    ~Server() {
        close(client_socket);
        close(server_socket);
    }

    int start(int port) {
        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        server_socket = socket(AF_INET, SOCK_STREAM, 0);

        if (bind(server_socket, (sockaddr*)&address, sizeof(address)) < 0) {
            perror("bind");
            return 1;
        }

        if (listen(server_socket, 1) < 0) {
            perror("listen");
            return 1;
        }

        std::cout << "Server is listening on port " << port << "...\n";

        client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket < 0) {
            perror("accept");
            return 1;
        }

        std::cout << "Connnected \n";
        return 0;
    }


    void receive_message(){
        char buffer[1024] = {};
        while (running) {
            ssize_t bytes = read(client_socket, buffer, sizeof(buffer) - 1);

            if (bytes > 0) {
                std::cout << "Received message: " << buffer << std::endl;
            }

            if (strcmp(buffer, "/exit") == 0) {
                running = false;
                const char* message = "connection closed";

                send(client_socket, message, strlen(message), 0);
            }
        }
    }


    void send_message(){
        std::string message;
        while (running) {
            std::getline(std::cin, message);
            send(client_socket, message.c_str(), message.length(), 0);

            if (message == "/exit") {
                running = false;
                std::cout << "Press Enter to leave programm";
            }
        }
    }

    
    void run(){
        running = true;

        std::thread receiver(&Server::receive_message, this);
        std::thread sender(&Server::send_message, this);

        receiver.join();
        sender.join();
    }
};

int main() {
    Server server;
    if (server.start(5600)) {
        std::cout << "error";
        return 1;
    }
    server.run();
}
