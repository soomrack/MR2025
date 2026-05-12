#define _WIN32_WINNT 0x0600
#define _CRT_SECURE_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <conio.h>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <vector>
#include <memory>
#include <unordered_map>

#include <winsock2.h>
#include <windows.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;


// ========== ОБЪЯВЛЕНИЕ КЛАССОВ ВПЕРЁД ==========
class RobotClient;
class RobotClientApp;


// ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ==========
void clear_screen();
string get_current_time();
string get_current_filename();
void save_to_file(const string& content);


// ========== КЛАСС СЕТЕВОГО КЛИЕНТА ==========
// Управляет TCP-соединением с сервером робота: подключение, отправка команд, приём ответов.
class RobotClient {
private:
    SOCKET sock_;
    string server_ip_;
    int port_;
    bool connected_;

public:
    RobotClient(const string& ip, int p);
    ~RobotClient();

    bool connect_to_server();
    void disconnect();
    string send_command(const string& cmd);
    string send_request(const string& cmd);
    bool is_connected() const;
    string get_ip() const;
};


// ========== ГЛАВНЫЙ КЛАСС ПРИЛОЖЕНИЯ ==========
// Интерфейс клиента: главное меню, режим управления роботом, просмотр статистики, мониторинг.
class RobotClientApp {
private:
    unique_ptr<RobotClient> client_;
    bool running_;

    void print_header();
    void print_menu();
    void control_mode();
    void stats_mode();
    void monitoring_mode();

public:
    RobotClientApp();
    ~RobotClientApp();

    bool initialize();
    void run();
    void shutdown();
};


// ============================================================
// ========== РЕАЛИЗАЦИЯ ВСЕХ ФУНКЦИЙ =========================
// ============================================================


// ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ==========
void clear_screen() {
    system("cls");
}

string get_current_time() {
    auto now = chrono::system_clock::now();
    time_t now_time = chrono::system_clock::to_time_t(now);
    struct tm timeinfo;
    localtime_s(&timeinfo, &now_time);

    stringstream ss;
    ss << setw(2) << setfill('0') << timeinfo.tm_hour << ":"
        << setw(2) << setfill('0') << timeinfo.tm_min << ":"
        << setw(2) << setfill('0') << timeinfo.tm_sec;
    return ss.str();
}

string get_current_filename() {
    auto now = chrono::system_clock::now();
    time_t now_time = chrono::system_clock::to_time_t(now);
    struct tm timeinfo;
    localtime_s(&timeinfo, &now_time);

    stringstream ss;
    ss << "robot_report_"
        << setw(4) << setfill('0') << timeinfo.tm_year + 1900 << "-"
        << setw(2) << setfill('0') << timeinfo.tm_mon + 1 << "-"
        << setw(2) << setfill('0') << timeinfo.tm_mday << "_"
        << setw(2) << setfill('0') << timeinfo.tm_hour << "-"
        << setw(2) << setfill('0') << timeinfo.tm_min << "-"
        << setw(2) << setfill('0') << timeinfo.tm_sec << ".txt";
    return ss.str();
}

void save_to_file(const string& content) {
    string filename = get_current_filename();
    ofstream file(filename);

    if (file.is_open()) {
        file << content;
        file.close();
        cout << "\n[OK] Report saved: " << filename << endl;
    }
    else {
        cout << "\n[ERROR] Cannot save file!" << endl;
    }
}


// ========== ROBOT CLIENT ==========
RobotClient::RobotClient(const string& ip, int p)
    : server_ip_(ip), port_(p), connected_(false) {
    sock_ = INVALID_SOCKET;
}

RobotClient::~RobotClient() {
    disconnect();
}

bool RobotClient::connect_to_server() {
    WSADATA wsaData;

    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "[ERROR] WSAStartup failed" << endl;
        return false;
    }

    sock_ = socket(AF_INET, SOCK_STREAM, 0);

    if (sock_ == INVALID_SOCKET) {
        cout << "[ERROR] Socket creation failed" << endl;
        WSACleanup();
        return false;
    }

    int timeout = 5000;
    setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sock_, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

    sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);

    if (inet_pton(AF_INET, server_ip_.c_str(), &server_addr.sin_addr) <= 0) {
        cout << "[ERROR] Invalid IP address: " << server_ip_ << endl;
        closesocket(sock_);
        WSACleanup();
        return false;
    }

    cout << "Connecting to " << server_ip_ << ":" << port_ << "... ";

    if (::connect(sock_, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cout << "FAILED!" << endl;
        cout << "[ERROR] Cannot connect to server. Check:" << endl;
        cout << "  1. Server is running on Raspberry Pi" << endl;
        cout << "  2. IP address is correct" << endl;
        cout << "  3. Port 8888 is open" << endl;
        closesocket(sock_);
        WSACleanup();
        return false;
    }

    cout << "OK!" << endl;
    connected_ = true;
    return true;
}

void RobotClient::disconnect() {
    if (connected_) {
        closesocket(sock_);
        WSACleanup();
        connected_ = false;
    }
}

string RobotClient::send_command(const string& cmd) {
    if (!connected_) return "ERROR: Not connected";

    if (send(sock_, cmd.c_str(), cmd.length(), 0) == SOCKET_ERROR) {
        return "ERROR: Send failed";
    }

    return "OK";
}

string RobotClient::send_request(const string& cmd) {
    if (!connected_) return "ERROR: Not connected";

    if (send(sock_, cmd.c_str(), cmd.length(), 0) == SOCKET_ERROR) {
        return "ERROR: Send failed";
    }

    int timeout = 30000;
    setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    string full_response;
    char buffer[4096];

    Sleep(300);

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes_received = recv(sock_, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            full_response += buffer;

            Sleep(50);
            u_long available = 0;
            ioctlsocket(sock_, FIONREAD, &available);

            if (available == 0) break;
        }
        else if (bytes_received == 0) {
            break;
        }
        else {
            int error = WSAGetLastError();

            if (error == WSAETIMEDOUT) {
                if (!full_response.empty()) break;
            }

            break;
        }
    }

    timeout = 5000;
    setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    return full_response.empty() ? "ERROR: No response" : full_response;
}

bool RobotClient::is_connected() const {
    return connected_;
}

string RobotClient::get_ip() const {
    return server_ip_;
}


// ========== ROBOT CLIENT APP ==========
RobotClientApp::RobotClientApp() : running_(true) {}

RobotClientApp::~RobotClientApp() {
    shutdown();
}

void RobotClientApp::print_header() {
    cout << "========================================" << endl;
    cout << "    ROBOT CONTROL CLIENT v5.0           " << endl;
    cout << "========================================" << endl;
}

void RobotClientApp::print_menu() {
    clear_screen();
    print_header();
    cout << "\nMAIN MENU" << endl;
    cout << "----------------------------------------" << endl;
    cout << "1 - Control robot (WASD)" << endl;
    cout << "2 - Get statistics" << endl;
    cout << "3 - Monitoring mode" << endl;
    cout << "4 - Change connection" << endl;
    cout << "0 - Exit" << endl;
    cout << "----------------------------------------" << endl;
    cout << "Connected to: " << client_->get_ip() << endl;
    cout << "----------------------------------------" << endl;
    cout << "Choice: ";
}

void RobotClientApp::control_mode() {
    clear_screen();
    print_header();
    cout << "\n=== CONTROL MODE ===" << endl;
    cout << "----------------------------------------" << endl;
    cout << "W - Forward    S - Backward" << endl;
    cout << "A - Left       D - Right" << endl;
    cout << "X - Stop       Q - Exit mode" << endl;
    cout << "----------------------------------------" << endl;
    cout << "Current speed: 150" << endl;
    cout << "\nCommands: speed 200 - Set speed (100-255)" << endl;
    cout << "----------------------------------------\n" << endl;

    bool in_control = true;

    while (in_control) {
        if (_kbhit()) {
            char key = _getch();
            key = tolower(key);

            switch (key) {
            case 'q':
                in_control = false;
                cout << "\nExiting control mode..." << endl;
                Sleep(500);
                break;

            case 'w':
            case 'a':
            case 's':
            case 'd':
                client_->send_command(string(1, key));
                cout << "[" << get_current_time() << "] Command: " << (char)toupper(key) << endl;
                break;

            case 'x':
                client_->send_command("x");
                cout << "[" << get_current_time() << "] STOP" << endl;
                break;
            }
        }

        Sleep(50);
    }
}

void RobotClientApp::stats_mode() {
    while (true) {
        clear_screen();
        print_header();
        cout << "\n=== STATISTICS MODE ===" << endl;
        cout << "----------------------------------------" << endl;
        cout << "1 - CPU Temperature" << endl;
        cout << "2 - CPU Load" << endl;
        cout << "3 - Line Sensors" << endl;
        cout << "4 - Full Report" << endl;
        cout << "5 - Server Logs" << endl;
        cout << "0 - Back" << endl;
        cout << "----------------------------------------" << endl;
        cout << "Choice: ";

        string choice;
        getline(cin, choice);

        if (choice == "0") return;

        string command, title;

        if (choice == "1") {
            command = "GET_TEMP_STATS";
            title = "CPU TEMPERATURE";
        }
        else if (choice == "2") {
            command = "GET_CPU_LOAD";
            title = "CPU LOAD";
        }
        else if (choice == "3") {
            command = "GET_LINE";
            title = "LINE SENSORS";
        }
        else if (choice == "4") {
            command = "GET_FULL_REPORT";
            title = "FULL REPORT";
        }
        else if (choice == "5") {
            command = "GET_LOGS";
            title = "SERVER LOGS";
        }
        else {
            cout << "\nInvalid choice!" << endl;
            Sleep(1000);
            continue;
        }

        cout << "\n[" << get_current_time() << "] Requesting " << title << "...";
        cout.flush();

        string response = client_->send_request(command);

        cout << "\n\n" << string(60, '=') << endl;
        cout << title << endl;
        cout << string(60, '=') << endl;
        cout << response << endl;
        cout << string(60, '=') << endl;

        cout << "\nSave to file? (y/n): ";
        string save;
        getline(cin, save);

        if (save == "y" || save == "Y") {
            save_to_file(title + "\n" + string(60, '=') + "\n" + response);
        }

        cout << "\nPress Enter to continue...";
        cin.get();
    }
}

void RobotClientApp::monitoring_mode() {
    clear_screen();
    print_header();
    cout << "\n=== MONITORING MODE ===" << endl;
    cout << "----------------------------------------" << endl;
    cout << "1 - Temperature" << endl;
    cout << "2 - CPU Load" << endl;
    cout << "3 - Line Sensors" << endl;
    cout << "0 - Back" << endl;
    cout << "----------------------------------------" << endl;
    cout << "Choice: ";

    string choice;
    getline(cin, choice);

    if (choice == "0") return;

    string command, title;

    if (choice == "1") {
        command = "GET_TEMP_STATS";
        title = "TEMPERATURE MONITOR";
    }
    else if (choice == "2") {
        command = "GET_CPU_LOAD";
        title = "CPU LOAD MONITOR";
    }
    else if (choice == "3") {
        command = "GET_LINE";
        title = "LINE SENSORS MONITOR";
    }
    else {
        cout << "\nInvalid choice!" << endl;
        Sleep(1000);
        return;
    }

    cout << "\n" << string(60, '=') << endl;
    cout << title << " (updates every 5 sec)" << endl;
    cout << "Press 'q' to exit" << endl;
    cout << string(60, '=') << "\n" << endl;

    int counter = 0;

    while (true) {
        if (_kbhit()) {
            char key = _getch();

            if (key == 'q' || key == 'Q') break;
        }

        counter++;
        string response = client_->send_request(command);

        clear_screen();
        cout << string(60, '=') << endl;
        cout << title << " - Update #" << counter << endl;
        cout << string(60, '=') << endl;
        cout << response << endl;
        cout << string(60, '=') << endl;
        cout << "\nNext update in 5 seconds..." << endl;

        for (int i = 0; i < 50; i++) {
            if (_kbhit()) {
                char key = _getch();

                if (key == 'q' || key == 'Q') goto exit_monitoring;
            }

            Sleep(100);
        }
    }

exit_monitoring:
    cout << "\nExiting monitoring..." << endl;
    Sleep(500);
}

bool RobotClientApp::initialize() {
    clear_screen();
    cout << "========================================" << endl;
    cout << "    ROBOT CONTROL CLIENT v5.0           " << endl;
    cout << "========================================" << endl;
    cout << "\nEnter Raspberry Pi IP address: ";

    string ip;
    getline(cin, ip);

    if (ip.empty()) {
        ip = "192.168.1.100";
        cout << "Using default IP: " << ip << endl;
    }

    client_ = make_unique<RobotClient>(ip, 8888);

    if (!client_->connect_to_server()) {
        cout << "\nPress any key to exit...";
        _getch();
        return false;
    }

    cout << "\nTesting connection... ";
    string response = client_->send_request("PING test");

    if (response.find("PONG") != string::npos) {
        cout << "OK!" << endl;
    }
    else {
        cout << "Warning: No PONG response" << endl;
    }

    Sleep(1000);
    return true;
}

void RobotClientApp::run() {
    if (!client_ || !client_->is_connected()) {
        cout << "\n[ERROR] Not connected to server!" << endl;
        return;
    }

    while (running_) {
        print_menu();

        string choice;
        getline(cin, choice);

        if (choice == "0") {
            running_ = false;
        }
        else if (choice == "1") {
            control_mode();
        }
        else if (choice == "2") {
            stats_mode();
        }
        else if (choice == "3") {
            monitoring_mode();
        }
        else if (choice == "4") {
            cout << "\nEnter new IP: ";
            string new_ip;
            getline(cin, new_ip);

            if (!new_ip.empty()) {
                client_->disconnect();
                client_ = make_unique<RobotClient>(new_ip, 8888);

                if (!client_->connect_to_server()) {
                    cout << "\nFailed to connect. Press any key...";
                    _getch();
                }
            }
        }
        else if (!choice.empty()) {
            cout << "\nInvalid choice! Press any key...";
            _getch();
        }
    }
}

void RobotClientApp::shutdown() {
    if (client_) {
        client_->disconnect();
    }

    cout << "\nGoodbye!" << endl;
    Sleep(1000);
}


// ========== ГЛАВНАЯ ФУНКЦИЯ ==========
int main() {
    SetConsoleOutputCP(65001);
    SetConsoleCP(65001);

    RobotClientApp app;

    if (app.initialize()) {
        app.run();
    }

    app.shutdown();
    return 0;
}