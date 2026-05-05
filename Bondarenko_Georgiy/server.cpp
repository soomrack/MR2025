#include <winsock2.h>
#include <windows.h>
#include <iostream>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <atomic>
#pragma comment(lib, "Ws2_32.lib")

const int PORT = 8080;

class SimpleServer {
private:
    SOCKET serverSock, clientSock;
    std::atomic<bool> running;
    std::thread receiveThread;
    
    bool initWinsock() {
        WSADATA wsaData;
        return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
    }
    
    void receiveMessages() {
        char buffer[1024];
        while (running) {
            memset(buffer, 0, sizeof(buffer));
            int bytes = recv(clientSock, buffer, sizeof(buffer) - 1, 0);
            if (bytes > 0) {
                std::cout << "\n[Client]: " << buffer << std::endl;
                std::cout << "[Server]: " << std::flush;
            } else {
                std::cout << "\n[System]: Client disconnected" << std::endl;
                running = false;
                break;
            }
        }
    }
    
public:
    SimpleServer() : serverSock(INVALID_SOCKET), clientSock(INVALID_SOCKET), running(true) {}
    
    bool start() {
        if (!initWinsock()) return false;
        
        serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (serverSock == INVALID_SOCKET) return false;
        
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(PORT);
        serverAddr.sin_addr.s_addr = INADDR_ANY;
        
        if (bind(serverSock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Bind failed" << std::endl;
            return false;
        }
        
        if (listen(serverSock, SOMAXCONN) == SOCKET_ERROR) {
            std::cerr << "Listen failed" << std::endl;
            return false;
        }
        
        std::cout << "Server listening on port " << PORT << "..." << std::endl;
        
        sockaddr_in clientAddr;
        int addrLen = sizeof(clientAddr);
        clientSock = accept(serverSock, (sockaddr*)&clientAddr, &addrLen);
        
        if (clientSock == INVALID_SOCKET) return false;
        
        std::cout << "Client connected!" << std::endl;
        receiveThread = std::thread(&SimpleServer::receiveMessages, this);
        return true;
    }
    
    void send(const std::string& message) {
        if (message == "*quit") {
            running = false;
            closesocket(clientSock);
            closesocket(serverSock);
            return;
        }
        ::send(clientSock, message.c_str(), message.length(), 0);
    }
    
    void run() {
        std::string input;
        std::cout << "[Server]: " << std::flush;
        
        while (running && std::getline(std::cin, input)) {
            send(input);
            if (input == "*quit") break;
            std::cout << "[Server]: " << std::flush;
        }
        
        if (receiveThread.joinable()) receiveThread.join();
        closesocket(clientSock);
        closesocket(serverSock);
        WSACleanup();
    }
};

int main() {
    SimpleServer server;
    if (server.start()) {
        server.run();
    } else {
        std::cerr << "Server failed to start!" << std::endl;
    }
    return 0;
}
