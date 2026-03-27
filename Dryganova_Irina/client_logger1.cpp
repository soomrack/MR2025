#define _WIN32_WINNT 0x0600
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define WIN32_LEAN_AND_MEAN

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <fstream>
#include <ctime>
#include <conio.h>
#include <sstream>
#include <cctype>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// работа с файлами
class FileHelper {
public:
    static string generate_filename() { //имя на основе текущего времени
        time_t now = time(nullptr);
        tm tm_info;
        localtime_s(&tm_info, &now);
        char buffer[64];
        strftime(buffer, sizeof(buffer), "log_%Y%m%d_%H%M%S.txt", &tm_info); // форматирование в log.txt
        return string(buffer);
    }
    // сохранение
    static bool save(const string& filename, const string& content) {
        ofstream file(filename);
        if (!file.is_open()) return false;
        file << content;
        file.close();
        return true;
    }
    // открываем файл
    static void open_in_notepad(const string& filename) {
        system(("notepad " + filename).c_str());
    }
};
// класс клиента
class NetworkClient {
private:
    SOCKET sock;
    string server_ip;
    int port;
    bool connected;
    // получение всех данных с сервера
    string receive_all() {
        string response;
        char buffer[4096];
        int bytes;
        while ((bytes = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) { //цикл приема данных
            buffer[bytes] = '\0';
            response += buffer;
            if (bytes < sizeof(buffer) - 1) break;
        }
        return response;
    }
    
public:
    NetworkClient(const string& ip, int p) : server_ip(ip), port(p), connected(false) { // конструктор
        sock = INVALID_SOCKET;
    }

    ~NetworkClient() { //деструктор
        disconnect();
    }

    //подключаемся к серверу
    bool connect() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) return false;

        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            WSACleanup();
            return false;
        }

        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());

        if (server_addr.sin_addr.s_addr == INADDR_NONE) {
            closesocket(sock);
            WSACleanup();
            return false;
        }

        if (::connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) == SOCKET_ERROR) {
            closesocket(sock);
            WSACleanup();
            return false;
        }

        connected = true;
        return true;
    }
    
    // отключение от сервера
    void disconnect() {
        if (connected) {
            closesocket(sock);
            WSACleanup();
            connected = false;
        }
    }

    // запрос на подключение, получение ответа
    string send_request(const string& request) {
        if (!connected) return "";
        send(sock, request.c_str(), request.length(), 0);
        return receive_all();
    }
    // проверка подключения
    bool is_connected() const { return connected; }
};

// парсинг параметров времени
class TimeParamParser {
public:
    static bool parse(const string& input, string& output) {
        if (input.empty()) return false;

        string num_part;
        string unit_part;

        size_t pos = 0;
        while (pos < input.length() && isdigit(input[pos])) {
            num_part += input[pos];
            pos++;
        }

        while (pos < input.length() && isalpha(input[pos])) {
            unit_part += tolower(input[pos]);
            pos++;
        }

        if (num_part.empty() || unit_part.empty()) return false;
        if (unit_part != "m" && unit_part != "h" && unit_part != "d") return false;

        output = num_part + unit_part;
        return true;
    }

    // вывод подсказок по верному формату
    static void print_format_hint() {
        cout << "Формат: число + буква" << endl;
        cout << "  m - минуты (например: 10m, 5m, 30m)" << endl;
        cout << "  h - часы   (например: 2h, 1h, 24h)" << endl;
        cout << "  d - дни    (например: 3d, 7d, 30d)" << endl;
        cout << "Примеры: 10m, 2h, 3d, 15m, 1h, 7d" << endl;
    }
};
// получаем логи
class LogFetcher {
private:
    NetworkClient& client;

public:
    LogFetcher(NetworkClient& cli) : client(cli) {}

    bool fetch(const string& param) { // запрос на сервер
        string request = "GET_LOGS_BY_TIME " + param;
        cout << "Отправка: " << request << endl;

        string response = client.send_request(request); //отправка и получение ответа
        if (response.empty()) {
            cout << "Пустой ответ от сервера" << endl;
            return false;
        }

        if (response.find("ERROR") != string::npos) { // проверка на ошибку
            cout << "Ошибка от сервера: " << response << endl;
            return false;
        }

        string filename = FileHelper::generate_filename(); //сохранение в файл
        if (FileHelper::save(filename, response)) {
            cout << "Логи сохранены в: " << filename << endl;
            FileHelper::open_in_notepad(filename);
            return true;
        }
        else { //
            cout << "Ошибка сохранения файла" << endl;
            return false;
        }
    }
};

//основной класс 
class LogApp {
private:
    NetworkClient client;
    LogFetcher fetcher;
    bool running;

public:
    LogApp(const string& ip) : client(ip, 8888), fetcher(client), running(false) {}

    bool init() {
        cout << "=== ЗАПРОС ЛОГОВ С СЕРВЕРА ===" << endl;
        cout << "Подключение к серверу..." << endl;

        if (!client.connect()) {
            cout << "Ошибка подключения к серверу" << endl;
            return false;
        }

        cout << "Подключено успешно!" << endl;
        running = true;
        return true;
    }
    // основной цикл
    void run() {
        TimeParamParser::print_format_hint(); // вывод подсказки
        cout << "Q - выход" << endl;

        while (running) { // обработка команд
            cout << "\n> ";

            string input;
            getline(cin, input);

            if (input == "q" || input == "Q") { //проверка на выход
                running = false;
                break;
            }

            string param; // парсинг введеной строки
            if (TimeParamParser::parse(input, param)) {
                fetcher.fetch(param); //получение логов
            }
            else {
                cout << "Неверный формат. Ожидается: число + m/h/d (например: 10m, 2h, 3d)" << endl;
            }
        }
    }

    void shutdown() { //завершение работы
        client.disconnect();
        cout << "Программа завершена" << endl;
    }
};

int main() {
    // установка кодировки консоли для поддержки русского языка
    SetConsoleOutputCP(1251); // вывод
    SetConsoleCP(1251); // ввод

    string ip; // запрос и чтение айпи
    cout << "IP сервера: ";
    getline(cin, ip);

    LogApp app(ip); // создание и инициализация приложения
    if (!app.init()) {
        system("pause");
        return 1;
    }

    app.run(); // запуск основного цикла
    app.shutdown(); //завершение работы

    return 0;
}