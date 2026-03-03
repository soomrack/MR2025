#include <iostream>
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
public:
    enum class CommandType {
        LOCAL,      // Команда выполняется локально, не требует ответа от сервера
        TERMINATE   // Команда завершает соединение
    };

private:
    std::map<std::string, std::function<bool(SOCKET, const std::string&)>> commands;
    std::map<std::string, std::string> commandDescriptions;
    std::map<std::string, CommandType> commandTypes;  

public:
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
    
    bool handleCommand(SOCKET socket, const std::string& message, bool& shouldWaitResponse) {
        if (message.empty()) {
            shouldWaitResponse = false;
            return true; 
        }
        
        auto it = commands.find(message);
        if (it != commands.end()) {
            bool shouldContinue = it->second(socket, message);
            CommandType type = commandTypes[message];
            
            // Для LOCAL и TERMINATE не ждем ответа
            shouldWaitResponse = false;
            
            return shouldContinue;
        }

        // Обычные сообщения отправляем на сервер и ждем ответ
        bool sendResult = sendToServer(socket, message);
        shouldWaitResponse = sendResult;  // Ждем ответ только если успешно отправили
        return sendResult;
    }
    
    bool sendToServer(SOCKET socket, const std::string& message) {
        int result = send(socket, message.c_str(), message.length(), 0);
        if (result == SOCKET_ERROR) {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
            return false;
        }
        return true;
    }
    
    std::string getHelpText() const {
        std::ostringstream help;
        help << "\nAvailable commands:\n";
        for (const auto& cmd : commandDescriptions) {
            help << "  " << cmd.first << " - " << cmd.second << "\n";
        }
        return help.str();
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

bool bind_socket(SOCKET server_socket, const sockaddr_in&address){
    if (bind(server_socket, (sockaddr*)&address, sizeof(address)) == SOCKET_ERROR){
            std::cerr << "Bind failed: " << WSAGetLastError() << "\n";
            return false;
    }
    return true;  
}

bool start_listening(SOCKET server_socket){
    if (listen(server_socket, 1) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << "\n";
        return false;
    }
    return true;     
}

SOCKET accept_connection(SOCKET server_socket){
    SOCKET client_socket = accept(server_socket, nullptr, nullptr);
    if (client_socket == INVALID_SOCKET) {
        std::cerr << "Accept failed: " << WSAGetLastError() << "\n";
    }
    else{
        std::cout << "Client connected successfully" << std::endl;
    }
    return client_socket;
}

bool receive_message(SOCKET client_socket, std::string& out_message){
    char buffer[1024] = {};
    int received_bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (received_bytes > 0) {
        buffer[received_bytes] = '\0';
        out_message = buffer;
        return true;
    } 
    else if (received_bytes == 0) {
        std::cout << "Client disconnected" << std::endl;
        return false;
    }
    else {
        std::cerr << "Error receiving data: " << WSAGetLastError() << std::endl;
        return false;
    }
}

bool send_message(SOCKET client_socket, const std::string& message){
    int result = send(client_socket, message.c_str(), message.length(), 0);
    if (result == SOCKET_ERROR) {
        std::cerr << "Send failed: " << WSAGetLastError() << std::endl;
        return false;
    }
    return true;
}

std::string get_server_response() {
    std::string response;
    std::cout << "Enter response to client: ";
    std::getline(std::cin, response);
    return response;
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
    std::cout << "Starting server...\n" << std::endl;
    SOCKET server_socket = INVALID_SOCKET;
    SOCKET client_socket = INVALID_SOCKET;
    
    if (!WinSock_Init()) return 1;
    server_socket = create_socket();
    if (server_socket == INVALID_SOCKET) return 1;
    sockaddr_in address = configure_socket();
    if (!bind_socket(server_socket, address)) return 1;
    if (!start_listening(server_socket)) return 1;
    
    std::cout << "Server listening on port " << SERVER_PORT << ". Waiting for client...\n" << std::endl;
    client_socket = accept_connection(server_socket);
    if (client_socket == INVALID_SOCKET) return 1;

    CommandHandler cmdHandler;
    bool running = true;
    std::string received_message;
    std::string server_response;

    std::cout << "\nWaiting for client message..." << std::endl;

    while (running) {
        if (!receive_message(client_socket, received_message)) {
            running = false;
            break;
        }

        if (received_message == "*Quit") {
            std::cout << "Client disconnected" << std::endl;
            running = false;
            break;
        }

        std::cout << "\n[Client]: " << received_message << std::endl;
        
        bool needServerResponse = true;

        while (needServerResponse) {
            std::cout << "\n[Server]: ";
            std::getline(std::cin, server_response);

            bool shouldWaitResponse = false;
            bool shouldContinue = cmdHandler.handleCommand(client_socket, server_response, shouldWaitResponse);
            
            if (!shouldContinue) {
                running = false;
                break;
            }
            
            // Для локальных команд не ждем ответа и продолжаем ввод
            if (!shouldWaitResponse) {
                std::cout << "\nNow enter your message to client:";
                continue;
            } else {
                needServerResponse = false;
            }
        }
        
        if (!running) break;
    }
    
    close_socket(server_socket, client_socket);
    std::cout << "Chat server stopped." << std::endl;
    return 0;
}