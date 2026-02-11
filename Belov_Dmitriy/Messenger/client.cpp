#include <WinSock2.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <cstdint>

#pragma comment(lib, "Ws2_32.lib")

enum class MessageType : uint32_t {
    Text = 1
};

struct MessageHeader {
    MessageType type;
    uint32_t size;
};

SOCKET clientSocket = INVALID_SOCKET;
bool connected = false;
bool running = true;
std::mutex coutMutex;

void sendAll(const char* data, int size) {
    int sent = 0;
    while (sent < size) {
        int res = send(clientSocket, data + sent, size - sent, 0);
        if (res <= 0) throw std::runtime_error("send failed");
        sent += res;
    }
}

void recvAll(char* data, int size) {
    int received = 0;
    while (received < size) {
        int res = recv(clientSocket, data + received, size - received, 0);
        if (res <= 0) throw std::runtime_error("recv failed");
        received += res;
    }
}

void receiveLoop() {
    try {
        while (connected) {
            MessageHeader header{};
            recvAll((char*)&header, sizeof(header));

            std::vector<char> data(header.size);
            recvAll(data.data(), header.size);

            std::string text(data.begin(), data.end());

            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "\n" << text << "\n> ";
        }
    }
    catch (...) {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "\nDisconnected.\n";
        connected = false;
        closesocket(clientSocket);
    }
}

std::string replaceEmoji(std::string text) {
    std::vector<std::pair<std::string, std::string>> emojis = {
        {":fire:", u8"🔥"},
        {":smile:", u8"😄"},
        {":sad:", u8"😢"},
        {":heart:", u8"❤️"},
        {":ok:", u8"👌"}
    };

    for (auto& e : emojis) {
        size_t pos;
        while ((pos = text.find(e.first)) != std::string::npos)
            text.replace(pos, e.first.length(), e.second);
    }
    return text;
}

int main() {

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    std::cout << "Messenger client started.\n";

    std::string input;

    while (running) {
        std::cout << "> ";
        std::getline(std::cin, input);

        if (input.rfind("/connect", 0) == 0) {

            if (connected) continue;

            std::stringstream ss(input);
            std::string cmd, ip;
            int port;
            ss >> cmd >> ip >> port;

            clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(port);
            addr.sin_addr.s_addr = inet_addr(ip.c_str());

            if (connect(clientSocket, (sockaddr*)&addr, sizeof(addr)) == 0) {
                connected = true;
                std::thread(receiveLoop).detach();
                std::cout << "Connected.\n";
            }
            else {
                std::cout << "Connection failed.\n";
            }
        }
        else if (input == "/quit") {
            if (connected) {
                connected = false;
                closesocket(clientSocket);
            }
        }
        else if (input == "/exit") {
            running = false;
            if (connected)
                closesocket(clientSocket);
        }
        else {
            if (!connected) continue;

            std::string text = replaceEmoji(input);

            MessageHeader header{
                MessageType::Text,
                static_cast<uint32_t>(text.size())
            };

            sendAll((char*)&header, sizeof(header));
            sendAll(text.data(), text.size());
        }
    }

    WSACleanup();
    return 0;
}
