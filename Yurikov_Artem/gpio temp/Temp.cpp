#include <iostream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <string>
#include <conio.h>
#pragma comment(lib, "Ws2_32.lib")

const unsigned short kServerPort = 8082;             // изменённый порт
const char* kDefaultServerIP = "10.42.0.1";

bool WinSockInit() {
    WSADATA wsa_data;
    if (WSAStartup(MAKEWORD(2, 2), &wsa_data) != 0) {
        std::cerr << "WSAStartup failed\n";
        return false;
    }
    return true;
}

SOCKET CreateSocket() {
    SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (s == INVALID_SOCKET)
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
    return s;
}

bool ConnectToServer(SOCKET s, const std::string& ip) {
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(kServerPort);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    if (addr.sin_addr.s_addr == INADDR_NONE) {
        std::cerr << "Invalid IP\n";
        return false;
    }
    if (connect(s, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        std::cerr << "Connect failed: " << WSAGetLastError() << std::endl;
        return false;
    }
    std::cout << "Connected.\n";
    return true;
}

bool SendCommand(SOCKET s, const std::string& cmd, std::string& resp) {
    if (send(s, cmd.c_str(), cmd.length(), 0) == SOCKET_ERROR) {
        std::cerr << "Send failed\n";
        return false;
    }
    char buf[4096] = {};
    int n = recv(s, buf, sizeof(buf)-1, 0);
    if (n <= 0) {
        std::cerr << "Recv failed\n";
        return false;
    }
    resp = buf;
    return true;
}

int main() {
    std::cout << "Raspberry Pi Temp Client (port " << kServerPort << ")\n";
    if (!WinSockInit()) return 1;

    std::string ip;
    std::cout << "Enter IP [" << kDefaultServerIP << "]: ";
    std::getline(std::cin, ip);
    if (ip.empty()) ip = kDefaultServerIP;

    SOCKET sock = CreateSocket();
    if (sock == INVALID_SOCKET) { WSACleanup(); return 1; }
    if (!ConnectToServer(sock, ip)) { closesocket(sock); WSACleanup(); return 1; }

    std::cout << "Commands: *GetLogs, *Help, *Quit\n";
    while (true) {
        std::cout << "> ";
        std::string cmd;
        std::getline(std::cin, cmd);
        if (cmd.empty()) continue;

        std::string resp;
        if (!SendCommand(sock, cmd, resp)) break;

        std::cout << "\n=== Server response ===\n" << resp << "========================\n";

        if (cmd == "*Quit") break;
    }

    closesocket(sock);
    WSACleanup();
    std::cout << "Client stopped.\nPress any key...";
    _getch();
    return 0;
}