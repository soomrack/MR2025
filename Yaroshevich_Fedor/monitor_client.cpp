#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <iomanip>
#include <csignal>

#pragma comment(lib, "ws2_32.lib")

volatile std::sig_atomic_t g_running = 1;
void handleSignal(int) { g_running = 0; }

struct SensorData {
    std::string time;
    double temp_c, cpu_pct, mem_pct, disk_pct;
    std::string status;
};

SensorData parseJson(const std::string& json) {
    auto extractStr = [&](const std::string& key) -> std::string {
        std::string s = "\"" + key + "\":\"";
        size_t pos = json.find(s);
        if (pos == std::string::npos) return "N/A";
        pos += s.length();
        size_t end = json.find("\"", pos);
        return (end == std::string::npos) ? "N/A" : json.substr(pos, end - pos);
        };
    auto extractDbl = [&](const std::string& key) -> double {
        std::string s = "\"" + key + "\":";
        size_t pos = json.find(s);
        if (pos == std::string::npos) return 0.0;
        pos += s.length();
        size_t end = json.find_first_of(",}", pos);
        if (end == std::string::npos) return 0.0;
        try {
            return std::stod(json.substr(pos, end - pos));
        }
        catch (...) { return 0.0; }
        };
    return { extractStr("time"), extractDbl("temp_c"), extractDbl("cpu_pct"),
             extractDbl("mem_pct"), extractDbl("disk_pct"), extractStr("status") };
}

std::string receiveLine(SOCKET sock) {
    std::string line;
    char c;
    while (g_running && recv(sock, &c, 1, 0) == 1) {
        if (c == '\n') break;
        if (c == '\r') continue;
        line += c;
    }
    return line;
}

void sendCommand(SOCKET sock, const std::string& cmd) {
    std::string full = cmd + "\r\n";
    send(sock, full.c_str(), static_cast<int>(full.length()), 0);
}

void runLive(SOCKET sock) {
    std::cout << "\033[2J\033[H";
    std::cout << "\033[36mLIVE MONITORING MODE\033[0m\n";
    std::cout << "Press Ctrl+C to stop\n";
    std::cout << "----------------------------------------\n";
    std::cout.flush();

    while (g_running) {
        std::string line = receiveLine(sock);
        if (line.empty()) break;
        if (line.find("Live stream") != std::string::npos) continue;

        SensorData d = parseJson(line);

        std::string icon = "\033[32m[OK]\033[0m";
        if (d.status == "CRITICAL_TEMP") icon = "\033[31m[TEMP]\033[0m";
        else if (d.status == "CRITICAL_CPU") icon = "\033[33m[CPU]\033[0m";

        std::cout << "[" << d.time << "] " << icon
            << " T:" << std::fixed << std::setprecision(1) << d.temp_c << "C"
            << " | CPU:" << d.cpu_pct << "%"
            << " | MEM:" << d.mem_pct << "%"
            << " | DISK:" << d.disk_pct << "%\n";
        std::cout.flush();
    }
    std::cout << "\n\033[33mConnection closed.\033[0m\n";
}

void runLogs(SOCKET sock) {
    std::cout << "\n\033[35mCRITICAL EVENTS LOG\033[0m\n";
    std::cout << "----------------------------------------\n";
    std::cout.flush();

    std::string line;
    bool hasContent = false;
    while (g_running) {
        line = receiveLine(sock);
        if (line.empty()) break;
        if (line.find("Reading") != std::string::npos) continue;
        if (line.find("End of") != std::string::npos) break;

        std::cout << "  " << line << "\n";
        hasContent = true;
    }
    if (!hasContent) std::cout << "  (No critical events recorded)\n";
    std::cout << "----------------------------------------\n";
}

int main(int argc, char* argv[]) {
    signal(SIGINT, handleSignal);

    std::string serverIp = "127.0.0.1";
    if (argc > 1) serverIp = argv[1];

    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "WSAStartup failed\n"; return 1;
    }

    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n"; return 1;
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(5000);
    if (inet_pton(AF_INET, serverIp.c_str(), &addr.sin_addr) <= 0) {
        std::cerr << "Invalid IP address\n"; return 1;
    }

    std::cout << "Connecting to " << serverIp << ":5000 ... ";
    std::cout.flush();

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "Failed. Is server running?\n";
        closesocket(sock); WSACleanup(); return 1;
    }
    std::cout << "\033[32mOK\033[0m\n";

    std::cout << "\nChoose mode:\n";
    std::cout << "  1. LIVE (real-time monitoring)\n";
    std::cout << "  2. LOGS (read critical events)\n";
    std::cout << "> ";
    std::cout.flush();

    char choice;
    std::cin >> choice;
    std::cin.ignore(1000, '\n');

    if (choice == '1') {
        sendCommand(sock, "LIVE");
        runLive(sock);
    }
    else if (choice == '2') {
        sendCommand(sock, "LOGS");
        runLogs(sock);
    }
    else {
        std::cerr << "Invalid choice.\n";
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}