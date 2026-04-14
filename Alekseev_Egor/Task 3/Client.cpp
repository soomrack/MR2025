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
const std::string SERVER_ADDRESS = "192.168.1.56";

class CommandHandler {
private:
    std::map<std::string, std::function<bool(SOCKET, const std::string&)>> commands;
    std::map<std::string, std::string> commandDescriptions;
    std::set<std::string> terminateCommands;  
    mutable std::mutex cout_mutex;
    std::string current_input;
    bool streaming_mode = false;

public:
    enum class CommandType {
        LOCAL,
        SERVER,      
        TERMINATE   
    };
    
    CommandHandler() {
        registerCommand("QUIT", "Disconnect and exit", CommandType::TERMINATE,
            [this](SOCKET sock, const std::string& cmd) -> bool {
                {
                    std::lock_guard<std::mutex> lock(this->cout_mutex);
                    std::cout << "\nClosing connection and exiting..." << std::endl;
                }
                send(sock, cmd.c_str(), cmd.length(), 0);
                return false;
            });

        registerCommand("SHUTDOWN", "Exit and stop server", CommandType::TERMINATE,
            [this](SOCKET sock, const std::string& cmd) -> bool {
                {
                    std::lock_guard<std::mutex> lock(this->cout_mutex);
                    std::cout << "\nExiting and stopping server..." << std::endl;
                }
                send(sock, cmd.c_str(), cmd.length(), 0);
                return false;
            });
        
        registerCommand("HELP", "Show available commands", CommandType::LOCAL,
            [this](SOCKET, const std::string&) -> bool {
                std::lock_guard<std::mutex> lock(this->cout_mutex);
                std::cout << "\n=== Available commands ===" << std::endl;
                for (const auto& cmd : commandDescriptions) {
                    std::cout << "  " << cmd.first << " - " << cmd.second << std::endl;
                }
                std::cout << "==========================" << std::endl;
                return true;  
            });
        
        registerCommand("GET_DATA", "Get sensor's data", CommandType::SERVER,
            [this](SOCKET sock, const std::string& cmd) -> bool {
                send(sock, cmd.c_str(), cmd.length(), 0);
                {
                    std::lock_guard<std::mutex> lock(this->cout_mutex);
                    std::cout << "\n[System]: Requesting sensor data..." << std::endl;
                }
                return true;
            });
        
        registerCommand("START_STREAM", "Start streaming sensor data every 2 seconds", CommandType::SERVER,
            [this](int sock, const std::string& cmd) -> bool {
                send(sock, cmd.c_str(), cmd.length(), 0);
                {
                    std::lock_guard<std::mutex> lock(this->cout_mutex);
                    streaming_mode = true;
                    std::cout << "\n[System]: Starting streaming data..." << std::endl;
                }
                return true;
            });
        
        registerCommand("STOP_STREAM", "Stop streaming sensor data", CommandType::SERVER,
            [this](int sock, const std::string& cmd) -> bool {
                send(sock, cmd.c_str(), cmd.length(), 0);
                {
                    std::lock_guard<std::mutex> lock(this->cout_mutex);
                    streaming_mode = false;
                    std::cout << "\n[System]: Stopping streaming data..." << std::endl;
                }
                return true;
            });
        
        registerCommand("GET_LOG", "Get last 120 log entries from sensor data history", CommandType::SERVER,
            [this](int sock, const std::string& cmd) -> bool {
                send(sock, cmd.c_str(), cmd.length(), 0);
                {
                    std::lock_guard<std::mutex> lock(this->cout_mutex);
                    std::cout << "\n[System]: Getting log data..." << std::endl;
                }
                return true;
            });
        
        registerCommand("GET_DANGER_LOG", "Get last 120 dangerous events from sensor data history", CommandType::SERVER,
            [this](int sock, const std::string& cmd) -> bool {
                send(sock, cmd.c_str(), cmd.length(), 0);
                {
                    std::lock_guard<std::mutex> lock(this->cout_mutex);
                    std::cout << "\n[System]: Getting danger log data..." << std::endl;
                }
                return true;
            });
    }
    
    void setCurrentInput(const std::string& input) {
        std::lock_guard<std::mutex> lock(cout_mutex);
        current_input = input;
    }
    
    std::string getCurrentInput() const {
        std::lock_guard<std::mutex> lock(cout_mutex);
        return current_input;
    }
    
    bool isStreamingMode() const {
        std::lock_guard<std::mutex> lock(cout_mutex);
        return streaming_mode;
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

        std::string upperMessage = message;
        std::transform(upperMessage.begin(), upperMessage.end(), upperMessage.begin(), ::toupper);
        
        auto it = commands.find(upperMessage);
        if (it != commands.end()) {
            bool result = it->second(socket, message);
            if (!result) {
                shouldExit = true;
            }
            return true;
        }

        {
            std::lock_guard<std::mutex> lock(cout_mutex);
            std::cout << "\n[System]: Command not found. Enter HELP to see available commands." << std::endl;
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
        std::string upperMessage = message;
        std::transform(upperMessage.begin(), upperMessage.end(), upperMessage.begin(), ::toupper);
        return terminateCommands.find(upperMessage) != terminateCommands.end();
    }

    void safePrint(const std::string& message, bool newline = true) {
        std::lock_guard<std::mutex> lock(cout_mutex);

        std::cout << "\r\033[K";

        if (newline)
            std::cout << message << std::endl;
        else
            std::cout << message;
    }
    
    void showPrompt() {
        std::lock_guard<std::mutex> lock(cout_mutex);
        std::cout << "> " << current_input << std::flush;
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
    char buffer[65536];
    
    while (running) {
        memset(buffer, 0, sizeof(buffer));
        int received_bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (received_bytes > 0) {
            buffer[received_bytes] = '\0';
            std::string message(buffer);

            while (!message.empty() && (message.back() == '\n' || message.back() == '\r')) {
                message.pop_back();
            }
            
            if (cmdHandler.isTerminateCommand(message)) {
                cmdHandler.safePrint("\n[Server]: " + message);
                cmdHandler.safePrint("\nServer disconnected. Press any key to exit...");
                running = false;
                break;
            }

            cmdHandler.safePrint(message, true);

            if (running) {
                cmdHandler.showPrompt();
            }
        } 
        else if (received_bytes == 0) {
            cmdHandler.safePrint("\n[System]: Server disconnected");
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
    
    cmdHandler.safePrint("=== Chat started ===");
    cmdHandler.safePrint("Enter HELP for available commands");
    cmdHandler.showPrompt();
    
    while (running && !shouldExit) {
        if (_kbhit()) {
            char ch = _getch();

            if (ch == '\r') { // Enter
                std::cout << std::endl;  
                
                if (!message.empty()) {
                    bool commandHandled = cmdHandler.handleCommand(client_socket, message, shouldExit);
                    
                    if (shouldExit) {
                        running = false;
                        break;
                    }

                    if (!commandHandled) {
                        if (!cmdHandler.sendToServer(client_socket, message)) {
                            cmdHandler.safePrint("[Error]: Failed to send message");
                            running = false;
                            break;
                        }
                    }
                    
                    message.clear();
                    cmdHandler.setCurrentInput("");
                }
                
                if (running) {
                    cmdHandler.showPrompt();
                }
            }
            else if (ch == '\b') { // Backspace
                if (!message.empty()) {
                    message.pop_back();
                    std::cout << "\b \b";
                    cmdHandler.setCurrentInput(message);
                }
            }
            else if (ch == 27) { // ESC key
                std::cout << std::endl;
                cmdHandler.handleCommand(client_socket, "QUIT", shouldExit);
                running = false;
                break;
            }
            else if (ch >= 32 && ch <= 126) { 
                message += ch;
                std::cout << ch;
                cmdHandler.setCurrentInput(message);
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
    
    if (!convert_address(address, SERVER_ADDRESS)) {
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