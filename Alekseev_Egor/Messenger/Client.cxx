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
#pragma comment(lib, "Ws2_32.lib")  

const unsigned short SERVER_PORT = 8080;

class CommandHandler {
private:
    std::map<std::string, std::function<bool(SOCKET, const std::string&)>> commands;
    std::map<std::string, std::string> commandDescriptions;

public:
    enum class CommandType {
        LOCAL,      // Команда выполняется локально, не требует ответа от сервера
        SERVER,     // Команда отправляется на сервер и требует ответа
        TERMINATE   // Команда завершает соединение
    };
    
    struct CommandResult {
        bool shouldContinue;  
        bool waitForResponse; 
    };

    CommandHandler() {
        registerCommand("*Quit", "Disconnect", CommandType::TERMINATE,
            [](SOCKET sock, const std::string& cmd) -> bool {
                std::cout << "Closing connection..." << std::endl;
                int result = send(sock, cmd.c_str(), cmd.length(), 0);
                if (result == SOCKET_ERROR) {
                    std::cerr << "send failed: " << WSAGetLastError() << std::endl;
                }
                return false;  
            });
        
        registerCommand("*Help", "Show available commands", CommandType::LOCAL,
            [this](SOCKET, const std::string&) -> bool {
                std::cout << "\n=== Available commands ===" << std::endl;
                for (const auto& cmd : commandDescriptions) {
                    std::cout << "  " << cmd.first << " - " << cmd.second << std::endl;
                }
                std::cout << "==========================\n" << std::endl;
                return true;  
            });
    }
    
 
    void registerCommand(const std::string& command, const std::string& description, 
                        CommandType type, std::function<bool(SOCKET, const std::string&)> handler) {
        commands[command] = handler;
        commandDescriptions[command] = description;
        commandTypes[command] = type;  
    }

    CommandResult handleCommand(SOCKET socket, const std::string& message) {
        if (message.empty()) {
            return {true, false}; 
        }
        
        auto it = commands.find(message);
        if (it != commands.end()) {
            bool shouldContinue = it->second(socket, message);

            CommandType type = commandTypes[message];
            bool waitForResponse = (type == CommandType::SERVER) && shouldContinue;
            
            return {shouldContinue, waitForResponse};
        }

        bool sendResult = sendToServer(socket, message);
        return {sendResult, sendResult};  
    }

    bool sendToServer(SOCKET socket, const std::string& message) {
        int result = send(socket, message.data(), message.length(), 0);
        if (result == SOCKET_ERROR) {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
            return false;
        }
        return true;
    }

    std::string getHelpText() const {
        std::ostringstream help;
        help << "\n Available commads::\n";
        for (const auto& cmd : commandDescriptions) {
            help << "  " << cmd.first << " - " << cmd.second << "\n";
        }
        return help.str();
    }

private:
    std::map<std::string, CommandType> commandTypes;  
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
    std::getline(std::cin, address);
    return address;
}

bool convert_address(sockaddr_in& server, std::string address){ 
    int result = inet_pton(AF_INET, address.c_str(), &server.sin_addr);
    if (result <= 0) {
        std::cerr << "inet_pton failed: " << result << std::endl;
        return false;
    }
    return true;
}

bool connect_to_server(SOCKET client_socket, sockaddr_in server){
    int result = connect(client_socket, (sockaddr*)&server, sizeof(server));
    if (result == SOCKET_ERROR) {
        std::cerr << "connect failed: " << WSAGetLastError() << std::endl;
        return false;
    }
    else{
        std::cout << "Connected to server" << std::endl;
        return true;
    }
}

CommandHandler::CommandResult send_message(SOCKET client_socket, CommandHandler& cmdHandler){
    std::string message;
    std::cout << "\nEnter message:" << std::endl;
    std::cout << "> ";
    std::getline(std::cin, message);

    return cmdHandler.handleCommand(client_socket, message);
}

bool receive_message(SOCKET client_socket, bool& running){
    char buffer[1024] = {};
    int received_bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (received_bytes > 0) {
        buffer[received_bytes] = '\0';
        std::string received_message(buffer);
        
        std::cout << "\n[Server]: " << buffer << std::endl;
        std::cout << "---------------------------------------" << std::endl;

        if (received_message == "*Quit") {
            std::cout << "Client disconnected" << std::endl;
            running = false;
            return false;
        }
        
        return true;
    } 
    else if (received_bytes == 0) {
        std::cout << "Server disconnected..." << std::endl;
        return false;
    }
    else {
        std::cerr << "Error receiving data: " << WSAGetLastError() << std::endl;
        return false;
    }
}

void close_socket(SOCKET server_socket, SOCKET client_socket){
    if (client_socket != INVALID_SOCKET){
        closesocket(client_socket);
    }
    if (server_socket != INVALID_SOCKET){
        closesocket(server_socket);
    }
    WSACleanup();    
}

int main() {
    SOCKET server_socket = INVALID_SOCKET;
    SOCKET client_socket = INVALID_SOCKET;
    
    std::cout << "Enter *Help to show availbale commands\n" << std::endl;

    if (!WinSock_Init()) return 1;
    
    client_socket = create_socket();
    if (client_socket == INVALID_SOCKET) return 1;
    
    sockaddr_in address = configure_socket();
    if (!convert_address(address, enter_address())) return 1;
    
    if (!connect_to_server(client_socket, address)) return 1;

    CommandHandler cmdHandler;
    
    bool running = true;
    while (running){
        auto result = send_message(client_socket, cmdHandler);
        
        if (!result.shouldContinue) {
            running = false;
            break;
        }

        if (result.waitForResponse) {
            if (!receive_message(client_socket, running)) {
                if (running) { 
                    running = false;
                }
                break;
            }
        }
    }

    close_socket(server_socket, client_socket);
    std::cout << "Stopping client..." << std::endl;
    return 0;
}