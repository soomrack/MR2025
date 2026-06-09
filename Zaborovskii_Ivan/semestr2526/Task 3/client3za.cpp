#define _WIN32_WINNT 0x0600
#include <iostream>
#include <string>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

#define PORT 5555

string server_ip;

void send_command(const string& command) {
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);
    
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    
    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    // Replace inet_pton with InetPtonA for Windows
    if (InetPtonA(AF_INET, server_ip.c_str(), &server_addr.sin_addr) != 1) {
        cout << "Invalid IP address!" << endl;
        closesocket(sock);
        WSACleanup();
        return;
    }
    
    if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cout << "Connection failed!" << endl;
        closesocket(sock);
        WSACleanup();
        return;
    }
    
    send(sock, command.c_str(), command.length(), 0);
    
    // For STREAM - read in a loop
    if (command == "STREAM") {
        cout << "\n=== LIVE DISTANCE STREAM ===" << endl;
        cout << "Press Ctrl+C to stop...\n" << endl;
        
        char buffer[8192];
        while (true) {
            memset(buffer, 0, sizeof(buffer));
            int received = recv(sock, buffer, sizeof(buffer) - 1, 0);
            
            if (received <= 0) {
                cout << "\nStream ended by server" << endl;
                break;
            }
            
            cout << buffer;
        }
    }
    // For regular commands - single response
    else {
        char buffer[8192] = {0};
        recv(sock, buffer, sizeof(buffer), 0);
        cout << buffer << endl;
    }
    
    closesocket(sock);
    WSACleanup();
}

int main() {
    cout << "Enter Orange Pi IP address: ";
    cin >> server_ip;
    cin.ignore();
    
    cout << "\n=== DISTANCE Monitor ===" << endl;
    cout << "Commands:" << endl;
    cout << "  GET       - Current DISTANCE level" << endl;
    cout << "  LAST_DISTANCE  - Last time sensor value" << endl;
    cout << "  STREAM    - Live DISTANCE updates" << endl;
    cout << "  quit      - Exit" << endl;
    cout << "=============================\n" << endl;
    
    while (true) {
        cout << "> ";
        string cmd;
        getline(cin, cmd);
        
        if (cmd == "quit") break;
        
        send_command(cmd);
        
        if (cmd != "STREAM") {
            cout << endl;
        }
    }
    
    return 0;
}