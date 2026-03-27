#define _WIN32_WINNT 0x0600
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <conio.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// класс клиента для управления омегаботом
class RobotClient {
private:
    SOCKET sock;
    string server_ip;
    int port;
    bool connected;

public:
    RobotClient(const string& ip, int p) : server_ip(ip), port(p), connected(false) { //конструктор
        sock = INVALID_SOCKET;
    }

    ~RobotClient() { //деструктор
        disconnect();
    }

    bool connect() { //метод подключения к серверу
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            cout << "Ошибка WSAStartup" << endl;
            return false;
        }

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            cout << "Ошибка создания сокета" << endl;
            WSACleanup();
            return false;
        }
        
        sockaddr_in server_addr; // настройка адреса сервера
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());

        if (server_addr.sin_addr.s_addr == INADDR_NONE) { //проверка правильности айпи
            cout << "Неверный IP адрес" << endl;
            closesocket(sock);
            WSACleanup();
            return false;
        }

        //подключение к серверу
        cout << "Подключение к " << server_ip << ":" << port << "...";
        if (::connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            cout << " ОШИБКА" << endl;
            closesocket(sock);
            WSACleanup();
            return false;
        }

        cout << " УСПЕШНО!" << endl;
        connected = true;
        return true;
    }
    //отключение от сервера
    void disconnect() {
        if (connected) {
            closesocket(sock);
            WSACleanup();
            connected = false;
        }
    }
    // отправка команд
    void send_command(const string& cmd) {
        if (connected) {
            send(sock, cmd.c_str(), cmd.length(), 0);
        }
    }
    // основной метод управления роботом
    void run() { //подсказки по управлению
        cout << "\nW - вперёд" << endl;
        cout << "S - назад" << endl;
        cout << "A - влево" << endl;
        cout << "D - вправо" << endl;
        cout << "X - стоп" << endl;
        cout << "Q - выход" << endl;

        bool running = true;
        while (running) { // бесконечный цикл обработки нажатий клавиш
            if (_kbhit()) {
                char key = _getch();
                key = tolower(key);

                if (key == 'q') {
                    running = false;
                    cout << "\nВыход..." << endl;
                }
                else if (key == 'w' || key == 'a' || key == 's' || key == 'd' || key == 'x') {
                    send_command(string(1, key));
                    cout << "Команда: " << key << endl;
                }
            }
            Sleep(50);
        }
    }
};

int main() {
    // установка кодировки консоли для поддержки русского языка
    SetConsoleOutputCP(1251); //вывод
    SetConsoleCP(1251); //ввод

    cout << "=== Управление роботом ===" << endl; // вывод заголовка
    cout << "IP: "; //запрос айпи
    string ip; 
    getline(cin, ip); //чтение айпи

    RobotClient client(ip, 8888); 

    if (!client.connect()) { //подключение 
        cout << "Не удалось подключиться!" << endl;
        system("pause");
        return 1;
    }

    client.run(); //запуск основного цикла

    client.disconnect(); //отключение
    return 0;
}