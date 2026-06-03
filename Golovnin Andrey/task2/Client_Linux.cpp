#include <iostream>
#include <string>
#include <cstdint>
#include <cstring>
#include <csignal>
#include <stdexcept>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

constexpr int PORT = 54000;

int sock = -1;

void recvAll(int s, char* buf, int size) {
    int received = 0;
    while (received < size) {
        int n = recv(s, buf + received, size - received, 0);
        if (n <= 0) throw std::runtime_error("connection closed");
        received += n;
    }
}

void onSignal(int) {
    std::cout << "\nDisconnecting...\n";
    if (sock >= 0) close(sock);
    std::exit(0);
}

int main(int argc, char* argv[]) {
    const char* host = (argc >= 2) ? argv[1] : "127.0.0.1";

    std::signal(SIGINT,  onSignal);
    std::signal(SIGTERM, onSignal);

    // Connect
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Failed to create socket\n";
        return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(PORT);

    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        std::cerr << "Invalid address: " << host << "\n";
        return 1;
    }

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) < 0) {
        std::cerr << "Cannot connect to " << host << ":" << PORT << "\n";
        return 1;
    }

    std::cout << "Connected to " << host << ":" << PORT
              << std::string(50, '-') << "\n";

    // --- Receive loop ---
    // Protocol: [uint32_t len][len bytes of message]
    try {
        while (true) {
            uint32_t len = 0;
            recvAll(sock, reinterpret_cast<char*>(&len), sizeof(len));

            std::string msg(len, '\0');
            recvAll(sock, msg.data(), static_cast<int>(len));

            std::cout << msg << "\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Disconnected: " << e.what() << "\n";
    }

    close(sock);
    return 0;
}
