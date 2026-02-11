#include <WinSock2.h>
#include <windows.h>
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <string>
#include <stdexcept>
#include <cstdint>
#include <algorithm>

#pragma comment(lib, "Ws2_32.lib")

// ================= MESSAGE =================

enum class MessageType : uint32_t {
    Text = 1
};

struct MessageHeader {
    MessageType type;
    uint32_t size;
};

// ================= CLIENT =================

struct Client {
    SOCKET socket;
    std::string color;
};

std::vector<Client> clients;
std::mutex clientsMutex;

std::vector<std::string> colorPool = {
    "\033[31m", // red
    "\033[32m", // green
    "\033[33m", // yellow
    "\033[34m", // blue
    "\033[35m", // magenta
    "\033[36m"  // cyan
};

const std::string RESET = "\033[0m";

// ================= NETWORK =================

void sendAll(SOCKET sock, const char* data, int size) {
    int sent = 0;
    while (sent < size) {
        int res = send(sock, data + sent, size - sent, 0);
        if (res <= 0)
            throw std::runtime_error("send failed");
        sent += res;
    }
}

void recvAll(SOCKET sock, char* data, int size) {
    int received = 0;
    while (received < size) {
        int res = recv(sock, data + received, size - received, 0);
        if (res <= 0)
            throw std::runtime_error("recv failed");
        received += res;
    }
}

// ================= EMOJI =================

std::string replaceEmoji(std::string text) {

    std::vector<std::pair<std::string, std::string>> emojis = {
        {":fire:",  u8"🔥"},
        {":smile:", u8"😄"},
        {":sad:",   u8"😢"},
        {":heart:", u8"❤️"},
        {":ok:",    u8"👌"}
    };

    for (auto& e : emojis) {
        size_t pos;
        while ((pos = text.find(e.first)) != std::string::npos)
            text.replace(pos, e.first.length(), e.second);
    }

    return text;
}

// ================= BROADCAST =================

void broadcast(const MessageHeader& header,
               const std::vector<char>& data,
               SOCKET sender)
{
    std::lock_guard<std::mutex> lock(clientsMutex);

    for (auto& c : clients) {
        if (c.socket == sender)
            continue;

        try {
            sendAll(c.socket, (char*)&header, sizeof(header));
            sendAll(c.socket, data.data(), header.size);
        }
        catch (...) {}
    }
}

// ================= CLIENT HANDLER =================

void handleClient(SOCKET clientSocket) {

    try {
        while (true) {

            MessageHeader header{};
            recvAll(clientSocket, (char*)&header, sizeof(header));

            if (header.size > 1024 * 1024)
                break;

            std::vector<char> data(header.size);
            recvAll(clientSocket, data.data(), header.size);

            std::string message(data.begin(), data.end());

            message = replaceEmoji(message);

            std::string color;

            {
                std::lock_guard<std::mutex> lock(clientsMutex);
                for (auto& c : clients)
                    if (c.socket == clientSocket)
                        color = c.color;
            }

            std::string coloredMessage = color + message + RESET;

            std::cout << coloredMessage << std::endl;

            MessageHeader outHeader{
                MessageType::Text,
                static_cast<uint32_t>(coloredMessage.size())
            };

            std::vector<char> outData(
                coloredMessage.begin(),
                coloredMessage.end()
            );

            broadcast(outHeader, outData, clientSocket);
        }
    }
    catch (...) {}

    closesocket(clientSocket);

    {
        std::lock_guard<std::mutex> lock(clientsMutex);

        clients.erase(
            std::remove_if(clients.begin(), clients.end(),
                [clientSocket](Client& c) {
                    return c.socket == clientSocket;
                }),
            clients.end()
        );

        std::cout << "Client disconnected. Total: "
                  << clients.size() << std::endl;
    }
}

// ================= MAIN =================

int main() {

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(54000);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSock, (sockaddr*)&addr, sizeof(addr));
    listen(serverSock, SOMAXCONN);

    std::cout << "Server listening on port 54000\n";

    int colorIndex = 0;

    while (true) {

        SOCKET client =
            accept(serverSock, nullptr, nullptr);

        if (client == INVALID_SOCKET)
            continue;

        Client newClient;
        newClient.socket = client;

        {
            std::lock_guard<std::mutex> lock(clientsMutex);

            if (colorIndex < colorPool.size())
                newClient.color = colorPool[colorIndex++];
            else
                newClient.color = "\033[37m";

            clients.push_back(newClient);

            std::cout << "Client connected. Total: "
                      << clients.size() << std::endl;
        }

        std::thread(handleClient, client).detach();
    }

    WSACleanup();
    return 0;
}
