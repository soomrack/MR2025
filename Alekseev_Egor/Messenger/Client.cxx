#include <iostream>
#include <winsock2.h>  
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
private:
    std::map<std::string, std::function<bool(SOCKET, const std::string&)>> commands;
    std::map<std::string, std::string> commandDescriptions;
    std::set<std::string> terminateCommands;  
    mutable std::mutex cout_mutex;

public:
    enum class CommandType {
        LOCAL,      
        TERMINATE   
    };
    
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

    bool sendToServer(SOCKET socket, const std::string& message) {
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
        std::cerr << "WSAStartup failed: " << result << std::endl;
        return false;
    }
    return true;    
}

SOCKET create_socket(){
    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
    }
    return client_socket;
}

sockaddr_in configure_socket(){
    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);    
    return server;
}

std::string enter_address(){
    std::string address;
    std::cout << "Enter server's address (e.g: 127.0.0.1):" << std::endl;
    std::cout << "> ";
    std::getline(std::cin, address);
    return address;
}

bool convert_address(sockaddr_in& server, std::string address){ 
    server.sin_addr.s_addr = inet_addr(address.c_str());
    if (server.sin_addr.s_addr == INADDR_NONE) {
        std::cerr << "Invalid address format: " << address << std::endl;
        return false;
    }
    return true;
}

bool connect_to_server(SOCKET client_socket, sockaddr_in server){
    std::cout << "Attempting to connect to " 
              << inet_ntoa(server.sin_addr) << ":" 
              << ntohs(server.sin_port) << "..." << std::endl;
    
    int result = connect(client_socket, (sockaddr*)&server, sizeof(server));
    if (result == SOCKET_ERROR) {
        int error = WSAGetLastError();
        std::cerr << "Connect failed. Error code: " << error << std::endl;
        return false;
    }
    else{
        std::cout << "Successfully connected to server!" << std::endl;
        return true;
    }
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
                cmdHandler.safePrint("\n[Server]: " + message);
                cmdHandler.safePrint("\nServer disconnected. Press any key to exit...");
                running = false;
                break;
            }

            cmdHandler.safePrint("\r\033[K", false);
            cmdHandler.safePrint("[Server]: " + message);
            cmdHandler.safePrint("[You]: ", false);
        } 
        else if (received_bytes == 0) {
            cmdHandler.safePrint("\n[System]: Server disconnected");
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
    cmdHandler.safePrint("[You]: ", false);
    
    while (running && !shouldExit) {
        if (_kbhit()) {
            char ch = _getch();
            
            // Обработка Enter
            if (ch == '\r') {
                std::cout << std::endl;  // Переход на новую строку
                
                if (!message.empty()) {
                    if (cmdHandler.handleCommand(client_socket, message, shouldExit)) {
                        if (shouldExit) {
                            running = false;
                            break;
                        }
                        message.clear();
                        if (running) {
                            cmdHandler.safePrint("[You]: ", false);
                        }
                        continue;
                    }
                    
                    if (!cmdHandler.sendToServer(client_socket, message)) {
                        cmdHandler.safePrint("[Error]: Failed to send message");
                        running = false;
                        break;
                    }
                    message.clear();
                }
                
                if (running) {
                    cmdHandler.safePrint("[You]: ", false);
                }
            }
            // Обработка backspace
            else if (ch == '\b') {
                if (!message.empty()) {
                    message.pop_back();
                    std::cout << "\b \b";  // Удаляем символ с экрана
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

void close_socket(SOCKET client_socket){
    if (client_socket != INVALID_SOCKET){
        shutdown(client_socket, SD_BOTH);
        closesocket(client_socket);
    }
    WSACleanup();    
}

int main() {
    SOCKET client_socket = INVALID_SOCKET;

    std::cout << "Starting client..." << std::endl;
    
    if (!WinSock_Init()) {
        std::cerr << "Failed to initialize Winsock" << std::endl;
        return 1;
    }
    
    client_socket = create_socket();
    if (client_socket == INVALID_SOCKET) {
        WSACleanup();
        return 1;
    }
    
    sockaddr_in address = configure_socket();
    std::string server_address = enter_address();
    
    if (!convert_address(address, server_address)) {
        close_socket(client_socket);
        return 1;
    }
    
    if (!connect_to_server(client_socket, address)) {
        close_socket(client_socket);
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

    close_socket(client_socket);
    std::cout << "\nClient stopped." << std::endl;

    std::cout << "Press any key to exit..." << std::endl;
    _getch();
    
    return 0;
}