#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <string>


class Client {
private:
    bool running = false;
    int sock;
public:
    ~Client() {
        close(sock);
    }

    int start(int port, const char* server_addr = "127.0.0.1") {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("socket");
            return 1;
        }

        sockaddr_in server{};
        server.sin_family = AF_INET;
        server.sin_port = htons(port);

        inet_pton(AF_INET, server_addr, &server.sin_addr);

        if (connect(sock, (sockaddr*)&server, sizeof(server)) < 0) {
            perror("connect");
            return 1;
        }

        std::cout << "Connected";

        return 0;
    }

    
    void receive_message(){
        char buffer[1024] = {};
        while (running) {
            ssize_t bytes = read(sock, buffer, sizeof(buffer) - 1);

            if (bytes > 0) {
                std::cout << "Received message: " << buffer << std::endl;
            }

            if (strcmp(buffer, "/exit") == 0) {
                running = false;
            }
        }
    }


    void send_message(){
        std::string message;
        while (running) {
            std::getline(std::cin, message);
            send(sock, message.c_str(), message.length(), 0);

            if (message == "/exit") {
                running = false;
            }
        }
    }


    void run(){
        running = true;

        std::thread receiver(&Client::receive_message, this);
        std::thread sender(&Client::send_message, this);

        receiver.join();
        sender.join();
    }
};


int main() {
    Client client;
    if(client.start(5600)) {
        perror("error");
        return 1;
    }
    client.run();
}
