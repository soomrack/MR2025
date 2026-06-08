#include <iostream>
#include <string>
#include <cstdint>
#include <csignal>
#include <stdexcept>
#include <atomic>
#include <thread>
#include <algorithm>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

constexpr int PORT = 54000;

int sock = -1;
std::atomic<bool> running{true};

void sendAll(int s, const char* data, int size) {
    int sent = 0;
    while (sent < size) {
        int n = send(s, data + sent, size - sent, 0);
        if (n <= 0) return;
        sent += n;
    }
}

bool recvAll(int s, char* buf, int size) {
    int got = 0;
    while (got < size) {
        int n = recv(s, buf + got, size - got, 0);
        if (n <= 0) return false;
        got += n;
    }
    return true;
}

// Отправить команду серверу: [uint32_t len][bytes]
void sendCommand(const std::string& cmd) {
    if (!running) return;
    uint32_t len = static_cast<uint32_t>(cmd.size());
    sendAll(sock, reinterpret_cast<const char*>(&len), sizeof(len));
    sendAll(sock, cmd.data(), static_cast<int>(len));
}

void printHelp() {
    std::cout << "\nКоманды:\n"
              << "  log all            — весь лог\n"
              << "  log warnings       — только предупреждения\n"
              << "  log last <мин>     — последние N минут\n"
              << "  arduino all        — весь лог Arduino\n"
              << "  arduino last <мин> — последние N минут с Arduino\n"
              << "  quit               — выход\n\n";
}

void inputLoop() {
    printHelp();

    std::string line;
    while (running && std::getline(std::cin, line)) {
        if (line.empty()) continue;

        // Приводим к верхнему регистру для сравнения с протоколом
        std::string up = line;
        std::transform(up.begin(), up.end(), up.begin(), ::toupper);

        if (up == "QUIT" || up == "EXIT") {
            running = false;
            shutdown(sock, SHUT_RDWR);
            break;
        }
        else if (up == "HELP") {
            printHelp();
        }
        else if (up == "LOG ALL"      ||
                 up == "LOG WARNINGS" ||
                 up.rfind("LOG LAST ", 0) == 0 ||
                 up == "ARDUINO ALL"  ||
                 up.rfind("ARDUINO LAST ", 0) == 0) {
            sendCommand(up);
        }
        else {
            std::cout << "Неизвестная команда. Введите help.\n";
        }
    }
}


void onSignal(int) {
    running = false;
    if (sock >= 0) shutdown(sock, SHUT_RDWR);
    std::exit(0);
}


int main(int argc, char* argv[]) {
    const char* host = (argc >= 2) ? argv[1] : "127.0.0.1";

    std::signal(SIGINT,  onSignal);
    std::signal(SIGTERM, onSignal);

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
              << "  (Ctrl+C to quit)\n"
              << std::string(50, '-') << "\n";

    // Ввод команд — в отдельном потоке
    std::thread inputThread(inputLoop);
    inputThread.detach();  // не ждём — поток умрёт вместе с процессом

    // Получение данных — в главном потоке
    // Один формат для всего: метрики и ответы на запросы логов
    while (running) {
        uint32_t len = 0;
        if (!recvAll(sock, reinterpret_cast<char*>(&len), sizeof(len))) break;

        std::string msg(len, '\0');
        if (!recvAll(sock, msg.data(), static_cast<int>(len))) break;

        std::cout << msg << "\n";
    }

    running = false;
    close(sock);
    std::cout << "Disconnected.\n";
    return 0;
}
