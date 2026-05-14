#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <functional>

using namespace std::chrono_literals;

class Client {
private:
    int server_socket;
    int socket_port;
    sockaddr_in server{};
    std::atomic<bool> running{false};
    std::atomic<bool> connected{false};
    std::atomic<bool> sent;
    std::unordered_map<std::string, std::function<void()>> action_map;
    std::thread receiver;
    std::thread sender;
    std::thread connection_checker;
    std::thread logger;

public:
    Client() {
        init_action_map();
    }

    ~Client() {
        close(server_socket);
    }

    void init(int port, const char* server_addr = "127.0.0.1") {
        server.sin_family = AF_INET;
        server.sin_port = htons(port);

        inet_pton(AF_INET, server_addr, &server.sin_addr);
    }


    int connect_to_server(){
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket < 0) {
            perror("socket");
            return 1;
        }        

        if (connect(server_socket, (sockaddr*)&server, sizeof(server)) < 0) {
            perror("connect");
            return 1;
        }

        std::cout << "Connected" << std::endl;
        set_recv_timeout(server_socket, 1); 
        connected = true;

        return 0;
    }


    

    void start(){
        running = true;

        receiver = std::thread(&Client::receive_message, this);
        sender = std::thread(&Client::send_message, this);
        connection_checker = std::thread(&Client::check_connection, this);

        connection_checker.join();
        receiver.join();
        sender.join();
    }

private:
void receive_message(){
        char buffer[1024] = {};
        while (running && connected) {
            ssize_t bytes = read(server_socket, buffer, sizeof(buffer) - 1);

            if (bytes > 0) {
                buffer[bytes] = '\0';
                processing_mesage(buffer);
            }
        }
    }


    void send_message(){
        std::string message;
        fd_set readfds;
        struct timeval tv;

        while (running && connected) {

            FD_ZERO(&readfds);
            FD_SET(STDIN_FILENO, &readfds);
            tv.tv_sec = 1;
            tv.tv_usec = 0;

            int sel = select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &tv);
            if (sel < 0) {
                perror("select");
                break;
            }
            if (sel == 0) {
                continue;
            }

            std::getline(std::cin, message);
            send(server_socket, message.c_str(), message.length(), 0);
            if (message == "/exit") {
                running = false;
                std::cout << "Отключение от сервера\n";
            }
            if (message == "/shutdown") {
                running = false;
                std::cout << "Выключение сервера\n";
            }
        }
    }


    void check_connection() {
        while (running) {
            sent = false;
            send(server_socket, "/y", 3, 0);
            
            auto start = std::chrono::steady_clock::now(); 

            while (!sent) {
                auto now = std::chrono::steady_clock::now(); 
                if (now-start > 1s) {
                    break;
                }
            }

            if (!running) return;
            if (!sent) {
                reconnect_to_server();
            }
            std::this_thread::sleep_for(0.5s);
        }
    }


    void processing_mesage(const char* message){
        std::string command = message;
        if (auto it = action_map.find(command); it != action_map.end()) {
            it->second();  // вызов без дополнительных параметров
        } else {
            print_message(message);
        }
    }

    
    void reconnect_to_server() {
        connected = false;
        std::this_thread::sleep_for(1s);
        std::cout << "Disconnect. Trying to reconnecting...\n";
        
        close(server_socket);
        while (connect(server_socket, (sockaddr*)&server, sizeof(server)) < 0) {
            perror("connect");
            std::this_thread::sleep_for(1s);
        }
        std::cout << "Reconnected\n";
        sent = true;
        set_recv_timeout(server_socket, 1);
        connected = true;
        
        receiver = std::thread(&Client::receive_message, this);
        sender = std::thread(&Client::send_message, this);
        receiver.detach();
        sender.detach();
        
        return;
    }


    void set_recv_timeout(int socket_fd, int seconds) {
        struct timeval tv;
        tv.tv_sec = seconds;
        tv.tv_usec = 0;
        if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            perror("setsockopt SO_RCVTIMEO");
        }
    }


    void print_message(const char* message) {
        std::cout << "Received message: \033[32m" << message << "\033[0m" << std::endl;   
    }


    void init_action_map() {
        action_map = {
            {"/exit", [this]() {this->exit_from_chat();}},
            {"/y", [this]() {this->confirm_sended_message();}}
        };
    }

    void confirm_sended_message(){
        sent = true;
    }    

    void exit_from_chat() {
        running = false;
        std::cout << "Клиент разорвал соединение";
    }

};


int main(int argc, char *argv[]) {
    Client client;
    int port = std::stoi(argv[1]);
    client.init(port, "192.168.50.1");
    if (client.connect_to_server()) {
        std::cout << "error";
        return 1;
    }
    client.start();
}
