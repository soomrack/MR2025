    #include <iostream>
    #include <string>
    #include <cstring>
    #include <winsock2.h>
    #include <ws2tcpip.h>

    #pragma comment(lib, "ws2_32.lib")

    using namespace std;

    #define PORT 5555

    void send_command(const string& server_ip, const string& command) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        
        SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            cerr << "Ошибка создания сокета" << endl;
            WSACleanup();
            return;
        }
        
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        
        
        server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());
        if (server_addr.sin_addr.s_addr == INADDR_NONE) {
            cerr << "Неверный IP адрес" << endl;
            closesocket(sock);
            WSACleanup();
            return;
        }
        
        if (connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            cerr << "Ошибка подключения" << endl;
            closesocket(sock);
            WSACleanup();
            return;
        }
        
        send(sock, command.c_str(), command.length(), 0);
        
        char buffer[8192] = {0};
        recv(sock, buffer, sizeof(buffer), 0);
        
        cout << buffer << endl;
        
        closesocket(sock);
        WSACleanup();
    }

    int main() {
        string server_ip;
        cout << "Введите IP адрес Orange Pi: ";
        cin >> server_ip;
        cin.ignore();
        
        cout << "\n=== Orange Pi Monitor Client ===" << endl;
        cout << "Команды:" << endl;
        cout << "  NOW       - Текущая температура CPU и процессы" << endl;
        cout << "  LAST_COOL - Время когда CPU был ниже 40C" << endl;
        cout << "  quit      - Выход" << endl;
        cout << "================================\n" << endl;
        
        while (true) {
            cout << "> ";
            string cmd;
            getline(cin, cmd);
            
            if (cmd == "quit") break;
            
            send_command(server_ip, cmd);
            cout << endl;
        }
        
        return 0;
    }