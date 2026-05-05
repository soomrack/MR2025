#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <thread>
#include <atomic>
#pragma comment(lib, "Ws2_32.lib")

const int PORT = 8080;

class SimpleClient {
private:
    SOCKET sock;
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
            int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);
            if (bytes > 0) {
                std::cout << "\n[Server]: " << buffer << std::endl;
                std::cout << "[You]: " << std::flush;
            } else {
                std::cout << "\n[System]: Server disconnected" << std::endl;
                running = false;
                break;
            }
        }
    }
    
public:
    SimpleClient() : sock(INVALID_SOCKET), running(true) {}
    
    bool connect(const std::string& serverIP) {
        if (!initWinsock()) return false;
        
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET) return false;
        
        sockaddr_in serverAddr{};
        serverAddr.sin_family = AF_INET;
        serverAddr.sin_port = htons(PORT);
        serverAddr.sin_addr.s_addr = inet_addr(serverIP.c_str());
        
        if (::connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
            std::cerr << "Connect failed: " << WSAGetLastError() << std::endl;
            return false;
        }
        
        std::cout << "Connected to server!" << std::endl;
        receiveThread = std::thread(&SimpleClient::receiveMessages, this);
        return true;
    }
    
    void send(const std::string& message) {
        if (message == "*quit") {
            running = false;
            closesocket(sock);
            return;
        }
        ::send(sock, message.c_str(), message.length(), 0);
    }
    
    void run() {
        std::string input;
        std::cout << "[You]: " << std::flush;
        
        while (running && std::getline(std::cin, input)) {
            send(input);
            if (input == "*quit") break;
            std::cout << "[You]: " << std::flush;
        }
        
        if (receiveThread.joinable()) receiveThread.join();
        closesocket(sock);
        WSACleanup();
    }
};

int main() {
    std::string serverIP;
    std::cout << "Enter server IP (127.0.0.1 for local): ";
    std::getline(std::cin, serverIP);
    
    SimpleClient client;
    if (client.connect(serverIP)) {
        client.run();
    } else {
        std::cerr << "Connection failed!" << std::endl;
    }
    return 0;
}