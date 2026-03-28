#include <iostream>
#include <ws2tcpip.h>   
#include <string>
#include <map>
#include <functional>
#include <iomanip>
#include <sstream>
#include <vector>
#include <algorithm>
#include <set>
#include <thread>
#include <atomic>
#include <mutex>
#include <conio.h>  
#pragma comment(lib, "Ws2_32.lib")  

const unsigned short SERVER_PORT = 8080;

class CommandHandler {
public:
    enum class CommandType {
        LOCAL,      
        TERMINATE   
    };

private:
    std::map<std::string, std::function<bool(SOCKET, const std::string&)>> commands;
    std::map<std::string, std::string> commandDescriptions;
    std::set<std::string> terminateCommands; 
    mutable std::mutex cout_mutex;

public:
    CommandHandler() {
        registerCommand("*Quit", "Disconnect and exit", CommandType::TERMINATE,
            [this](SOCKET sock, const std::string& cmd) -> bool {
                {
                    std::lock_guard<std::mutex> lock(this->cout_mutex);
                    std::cout << "\nClosing connection and exiting..." << std::endl;
                }
                send(sock, cmd.c_str(), cmd.length(), 0);
                return false;
            });
        
        registerCommand("*Help", "Show available commands", CommandType::LOCAL,
            [this](SOCKET, const std::string&) -> bool {
                std::lock_guard<std::mutex> lock(this->cout_mutex);
                std::cout << "\n=== Available commands ===" << std::endl;
                for (const auto& cmd : commandDescriptions) {
                    std::cout << "  " << cmd.first << " - " << cmd.second << std::endl;
                }
                std::cout << "==========================\n" << std::endl;
                return true;
            });

        registerCommand("*Clear", "Clear screen", CommandType::LOCAL,
            [this](SOCKET, const std::string&) -> bool {
                system("cls"); 
                {
                    std::lock_guard<std::mutex> lock(this->cout_mutex);
                    std::cout << "Screen cleared. Enter *Help for commands.\n" << std::endl;
                }
                return true;
            });
    }
    
    void registerCommand(const std::string& command, const std::string& description, 
                        CommandType type, std::function<bool(SOCKET, const std::string&)> handler) {
        commands[command] = handler;
        commandDescriptions[command] = description;

        if (type == CommandType::TERMINATE) {
            terminateCommands.insert(command);
        }
    }
    
    bool handleCommand(SOCKET socket, const std::string& message, bool& shouldExit) {
        if (message.empty()) {
            return true; 
        }
        
        auto it = commands.find(message);
        if (it != commands.end()) {
            bool result = it->second(socket, message);
            if (!result) {
                shouldExit = true;
            }
            return true;
        }

        return false; 
    }
    
    bool sendToClient(SOCKET socket, const std::string& message) {
        int result = send(socket, message.c_str(), message.length(), 0);
        if (result == SOCKET_ERROR) {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
            return false;
        }
        return true;
    }

    bool isTerminateCommand(const std::string& message) const {
        return terminateCommands.find(message) != terminateCommands.end();
    }

    void safePrint(const std::string& message, bool newline = true) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        if (newline)
            std::cout << message << std::endl;
        else
            std::cout << message << std::flush;
    }
};

bool WinSock_Init(){
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed\n";
        return false;
    }
    return true;    
}

SOCKET create_socket(){
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
    }
    return server_socket;
}

sockaddr_in configure_socket(){
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(SERVER_PORT);    
    return address;
}

bool bind_socket(SOCKET server_socket, const sockaddr_in& address){
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));
    
    if (bind(server_socket, (sockaddr*)&address, sizeof(address)) == SOCKET_ERROR){
        std::cerr << "Bind failed: " << WSAGetLastError() << "\n";
        return false;
    }
    return true;  
}

bool start_listening(SOCKET server_socket){
    if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << "\n";
        return false;
    }
    return true;     
}

SOCKET accept_connection(SOCKET server_socket){
    sockaddr_in client_addr;
    int addr_len = sizeof(client_addr);
    SOCKET client_socket = accept(server_socket, (sockaddr*)&client_addr, &addr_len);
    
    if (client_socket == INVALID_SOCKET) {
        std::cerr << "Accept failed: " << WSAGetLastError() << "\n";
    }
    else{
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        std::cout << "Client connected from " << client_ip << ":" 
                  << ntohs(client_addr.sin_port) << std::endl;
    }
    return client_socket;
}

void receive_messages(SOCKET client_socket, std::atomic<bool>& running, 
                     CommandHandler& cmdHandler) {
    char buffer[4096]; 
    
    while (running) {
        memset(buffer, 0, sizeof(buffer));
        int received_bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (received_bytes > 0) {
            buffer[received_bytes] = '\0';
            std::string message(buffer);
            
            if (cmdHandler.isTerminateCommand(message)) {
                cmdHandler.safePrint("\n[Client]: " + message);
                cmdHandler.safePrint("\nClient disconnected. Press any key to exit...");
                running = false;
                break;
            }

            cmdHandler.safePrint("\r\033[K", false);
            cmdHandler.safePrint("[Client]: " + message);
            cmdHandler.safePrint("[Server]: ", false);
        } 
        else if (received_bytes == 0) {
            cmdHandler.safePrint("\n[System]: Client disconnected");
            cmdHandler.safePrint("\nPress any key to exit...");
            running = false;
            break;
        }
        else {
            int error = WSAGetLastError();
            if (error != WSAECONNRESET && running) {
                std::cerr << "Error receiving data: " << error << std::endl;
            }
            running = false;
            break;
        }
    }
}

void send_messages(SOCKET client_socket, std::atomic<bool>& running, 
                  CommandHandler& cmdHandler) {
    std::string message;
    bool shouldExit = false;
    
    cmdHandler.safePrint("\n=== Chat started ===");
    cmdHandler.safePrint("Enter *Help for available commands");
    cmdHandler.safePrint("[Server]: ", false);
    
    while (running && !shouldExit) {
        if (_kbhit()) {
            char ch = _getch();

            if (ch == '\r') {
                std::cout << std::endl;  
                
                if (!message.empty()) {
                    if (cmdHandler.handleCommand(client_socket, message, shouldExit)) {
                        if (shouldExit) {
                            running = false;
                            break;
                        }
                        message.clear();
                        if (running) {
                            cmdHandler.safePrint("[Server]: ", false);
                        }
                        continue;
                    }

                    if (!cmdHandler.sendToClient(client_socket, message)) {
                        cmdHandler.safePrint("[Error]: Failed to send message");
                        running = false;
                        break;
                    }
                    message.clear();
                }
                
                if (running) {
                    cmdHandler.safePrint("[Server]: ", false);
                }
            }
            // Обработка backspace
            else if (ch == '\b') {
                if (!message.empty()) {
                    message.pop_back();
                    std::cout << "\b \b";  
                }
            }
            // Обычные символы
            else {
                message += ch;
                std::cout << ch;
            }
        }
        else {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
    }
}

void close_socket(SOCKET server_socket, SOCKET client_socket){
    if (client_socket != INVALID_SOCKET){
        shutdown(client_socket, SD_BOTH);
        closesocket(client_socket);
    }
    if (server_socket != INVALID_SOCKET){
        closesocket(server_socket);
    }
    WSACleanup();    
}

int main() {
    std::cout << "Starting server...\n" << std::endl;
    SOCKET server_socket = INVALID_SOCKET;
    SOCKET client_socket = INVALID_SOCKET;
    
    if (!WinSock_Init()) {
        std::cerr << "Failed to initialize Winsock" << std::endl;
        return 1;
    }
    
    server_socket = create_socket();
    if (server_socket == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }
    
    sockaddr_in address = configure_socket();
    if (!bind_socket(server_socket, address)) {
        close_socket(server_socket, INVALID_SOCKET);
        return 1;
    }
    
    if (!start_listening(server_socket)) {
        close_socket(server_socket, INVALID_SOCKET);
        return 1;
    }
    
    std::cout << "Server listening on port " << SERVER_PORT << ". Waiting for client...\n" << std::endl;
    
    client_socket = accept_connection(server_socket);
    if (client_socket == INVALID_SOCKET) {
        close_socket(server_socket, INVALID_SOCKET);
        return 1;
    }

    CommandHandler cmdHandler;
    std::atomic<bool> running{true};

    std::thread receiver(receive_messages, client_socket, std::ref(running), 
                        std::ref(cmdHandler));

    send_messages(client_socket, running, cmdHandler);

    if (receiver.joinable()) {
        receiver.join();
    }
    
    close_socket(server_socket, client_socket);
    std::cout << "\nChat server stopped." << std::endl;

    std::cout << "Press any key to exit..." << std::endl;
    _getch();
    
    return 0;
}