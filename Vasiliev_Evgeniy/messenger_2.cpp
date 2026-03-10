#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <winsock2.h>
#include <string>
#include <thread>
#include <windows.h>
#include <chrono>
#include <atomic>
#include <map>
#include <memory>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <conio.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;


// Константы приложения
const int PORT = 8080;
const int MAX_CONNECTION_ATTEMPTS = 3;
const int INPUT_CHECK_DELAY_MS = 10;
const int REMINDER_CHECK_INTERVAL_SEC = 5;
const int REMINDER_IDLE_TIME_SEC = 60;
const int RECEIVE_BUFFER_SIZE = 1024;


// Цвета консоли
const int COLOR_MY = 10;           // зеленый
const int COLOR_OTHER = 9;          // синий
const int COLOR_SYSTEM = 14;        // желтый
const int COLOR_DEFAULT = 7;        // белый
const int COLOR_TIME = 8;           // серый
const int COLOR_INPUT = 15;         // ярко-белый


class Time_utils {
public:
    string get_current_time();
};


class Console_color {
public:
    HANDLE h_console;
public:
    Console_color();
    void set_color(int color);
    void set_my_color();
    void set_other_color();
    void set_system_color();
    void set_default_color();
    void set_time_color();
    void set_input_color();
};


class Chat_statistics {
public:
    int messages_sent;
    int messages_received;
    chrono::steady_clock::time_point start_time;
public:
    Chat_statistics();
    void increment_sent();
    void increment_received();
    int get_messages_sent();
    int get_messages_received();
    int get_total_messages();
    double get_session_duration();
    void show_stats(Console_color& console);
};


class Emoji_manager {
public:
    map<string, string> emoji_map;
public:
    Emoji_manager();
    void init_emoji_map();
    string replace_commands(const string& message);
    void show_help(Console_color& console);
};


class Reminder_tracker {
public:
    chrono::steady_clock::time_point last_message_time;
    chrono::steady_clock::time_point last_reminder_time;
    bool waiting_for_response;
public:
    Reminder_tracker();
    void message_received();
    void message_sent();
    bool need_reminder();
    void reset();
};


class Socket_manager {
public:
    SOCKET socket_;
public:
    Socket_manager();
    ~Socket_manager();
    bool create();
    bool bind(int port);
    bool listen(int backlog);
    SOCKET accept(sockaddr_in& client_addr);
    bool connect(const string& ip, int port);
    int send(const string& data);
    string receive();
    void close();
    SOCKET get_socket();
    void set_socket(SOCKET s);
};


class Chat_session {
public:
    Socket_manager socket_manager_;
    string my_name;
    string other_name;
    atomic<bool>& running_;
    Reminder_tracker& reminder_;
    Emoji_manager& emoji_;
    Console_color& console_;
    Chat_statistics& stats_;
public:
    Chat_session(SOCKET socket, const string& my_name, const string& other_name,
        atomic<bool>& run, Reminder_tracker& rem, Emoji_manager& em,
        Console_color& cons, Chat_statistics& stat);
    void start();
    void receive_messages();
    void reminder_loop();
    void clear_line();
    void show_input(const string& input);
    string read_input();
    void send_message(const string& msg);
    string get_name();
    void close();
};


class Connection_manager {
public:
    Socket_manager chat_socket;
    string my_name;
    string other_name;
    Console_color& console_;
public:
    Connection_manager(Console_color& cons);
    bool init_winsock();
    bool start_server();
    bool start_client();
    bool exchange_names();
    string get_other_name();
    SOCKET get_socket();
    void cleanup();
    void set_my_name(const string& name);
    void show_ip_addresses();
    bool try_connect(const string& ip);
};


class User_interface {
public:
    Console_color console_;
public:
    void setup();
    void show_welcome_screen();
    string get_name();
    string get_mode();
    void show_chat_start(const string& my_name, const string& other_name);
    Console_color& get_console();
};


// Вспомогательные функции
bool initialize_application(Connection_manager& connection, const string& my_name);
bool establish_connection(const string& mode, Connection_manager& connection);
void run_chat_session(const string& my_name, Connection_manager& connection, Console_color& console,
    Emoji_manager& emoji, Reminder_tracker& reminder, atomic<bool>& running, Chat_statistics& stats);
void shutdown_application(Connection_manager& connection);


// Реализация методов
string Time_utils::get_current_time() {
    auto now = chrono::system_clock::now();
    auto time_t = chrono::system_clock::to_time_t(now);

    tm timeinfo;
    localtime_s(&timeinfo, &time_t);

    stringstream ss;
    ss << "["
        << setw(2) << setfill('0') << timeinfo.tm_hour << ":"
        << setw(2) << setfill('0') << timeinfo.tm_min << ":"
        << setw(2) << setfill('0') << timeinfo.tm_sec
        << "]";

    return ss.str();
}


Console_color::Console_color() {
    h_console = GetStdHandle(STD_OUTPUT_HANDLE);
}


void Console_color::set_color(int color) {
    SetConsoleTextAttribute(h_console, color);
}


void Console_color::set_my_color() {
    set_color(COLOR_MY);
}


void Console_color::set_other_color() {
    set_color(COLOR_OTHER);
}


void Console_color::set_system_color() {
    set_color(COLOR_SYSTEM);
}


void Console_color::set_default_color() {
    set_color(COLOR_DEFAULT);
}


void Console_color::set_time_color() {
    set_color(COLOR_TIME);
}


void Console_color::set_input_color() {
    set_color(COLOR_INPUT);
}


Chat_statistics::Chat_statistics() : messages_sent(0), messages_received(0) {
    start_time = chrono::steady_clock::now();
}


void Chat_statistics::increment_sent() {
    messages_sent++;
}


void Chat_statistics::increment_received() {
    messages_received++;
}


int Chat_statistics::get_messages_sent() {
    return messages_sent;
}


int Chat_statistics::get_messages_received() {
    return messages_received;
}


int Chat_statistics::get_total_messages() {
    return messages_sent + messages_received;
}


double Chat_statistics::get_session_duration() {
    auto now = chrono::steady_clock::now();
    return chrono::duration_cast<chrono::seconds>(now - start_time).count();
}


void Chat_statistics::show_stats(Console_color& console) {
    console.set_system_color();
    cout << "\n========== СТАТИСТИКА ЧАТА ==========" << endl;
    console.set_default_color();

    cout << "Отправлено сообщений: " << messages_sent << endl;
    cout << "Получено сообщений: " << messages_received << endl;
    cout << "Всего сообщений: " << get_total_messages() << endl;

    int minutes = static_cast<int>(get_session_duration()) / 60;
    int seconds = static_cast<int>(get_session_duration()) % 60;
    cout << "Длительность сессии: " << minutes << " мин " << seconds << " сек" << endl;

    if (messages_sent + messages_received > 0) {
        double avg_per_minute = get_total_messages() / (get_session_duration() / 60.0);
        cout << "Средняя активность: " << avg_per_minute << " сообщ/мин" << endl;
    }

    console.set_system_color();
    cout << "======================================\n" << endl;
    console.set_default_color();
}


Emoji_manager::Emoji_manager() {
    init_emoji_map();
}


void Emoji_manager::init_emoji_map() {
    emoji_map["/smile"] = ":-)";
    emoji_map["/love"] = "<3";
    emoji_map["/cool"] = "B-)";
}


string Emoji_manager::replace_commands(const string& message) {
    string result = message;

    for (const auto& [cmd, emoji] : emoji_map) {
        size_t pos = 0;
        while ((pos = result.find(cmd, pos)) != string::npos) {
            result.replace(pos, cmd.length(), emoji);
            pos += emoji.length();
        }
    }
    return result;
}


void Emoji_manager::show_help(Console_color& console) {
    cout << endl;
    console.set_system_color();
    cout << "========== ДОСТУПНЫЕ КОМАНДЫ ==========" << endl;
    console.set_default_color();

    for (const auto& [cmd, emoji] : emoji_map) {
        cout << "  " << cmd << " -> " << emoji << endl;
    }

    console.set_system_color();
    cout << "----------------------------------------" << endl;
    console.set_default_color();
    cout << "  /help - показать это меню" << endl;
    cout << "  /exit - выход из чата" << endl;
    cout << "  /stats - показать статистику чата" << endl;
    console.set_system_color();
    cout << "========================================\n" << endl;
    console.set_default_color();
}


Reminder_tracker::Reminder_tracker()
    : last_message_time(chrono::steady_clock::now())
    , last_reminder_time(chrono::steady_clock::now())
    , waiting_for_response(false) {
}


void Reminder_tracker::message_received() {
    last_message_time = chrono::steady_clock::now();
    waiting_for_response = true;
}


void Reminder_tracker::message_sent() {
    last_message_time = chrono::steady_clock::now();
    waiting_for_response = false;
}


bool Reminder_tracker::need_reminder() {
    if (!waiting_for_response) return false;

    auto now = chrono::steady_clock::now();
    auto seconds_since_last_msg = chrono::duration_cast<chrono::seconds>(now - last_message_time).count();
    auto seconds_since_last_reminder = chrono::duration_cast<chrono::seconds>(now - last_reminder_time).count();

    if (seconds_since_last_msg >= REMINDER_IDLE_TIME_SEC && seconds_since_last_reminder >= REMINDER_IDLE_TIME_SEC) {
        last_reminder_time = now;
        return true;
    }
    return false;
}


void Reminder_tracker::reset() {
    waiting_for_response = false;
}


Socket_manager::Socket_manager() : socket_(INVALID_SOCKET) {}


Socket_manager::~Socket_manager() {
    close();
}


bool Socket_manager::create() {
    socket_ = ::socket(AF_INET, SOCK_STREAM, 0);
    return socket_ != INVALID_SOCKET;
}


bool Socket_manager::bind(int port) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    return ::bind(socket_, (sockaddr*)&addr, sizeof(addr)) != SOCKET_ERROR;
}


bool Socket_manager::listen(int backlog) {
    return ::listen(socket_, backlog) != SOCKET_ERROR;
}


SOCKET Socket_manager::accept(sockaddr_in& client_addr) {
    int client_size = sizeof(client_addr);
    return ::accept(socket_, (sockaddr*)&client_addr, &client_size);
}


bool Socket_manager::connect(const string& ip, int port) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (addr.sin_addr.s_addr == INADDR_NONE) {
        return false;
    }

    return ::connect(socket_, (sockaddr*)&addr, sizeof(addr)) != SOCKET_ERROR;
}


int Socket_manager::send(const string& data) {
    return ::send(socket_, data.c_str(), data.length(), 0);
}


string Socket_manager::receive() {
    char buffer[RECEIVE_BUFFER_SIZE] = { 0 };
    int bytes = recv(socket_, buffer, sizeof(buffer) - 1, 0);
    if (bytes <= 0) return "";
    return string(buffer);
}


void Socket_manager::close() {
    if (socket_ != INVALID_SOCKET) {
        closesocket(socket_);
        socket_ = INVALID_SOCKET;
    }
}


SOCKET Socket_manager::get_socket() {
    return socket_;
}


void Socket_manager::set_socket(SOCKET s) {
    socket_ = s;
}


Chat_session::Chat_session(SOCKET socket, const string& my_name, const string& other_name,
    atomic<bool>& run, Reminder_tracker& rem, Emoji_manager& em,
    Console_color& cons, Chat_statistics& stat)
    : my_name(my_name), other_name(other_name), running_(run), reminder_(rem),
    emoji_(em), console_(cons), stats_(stat) {
    socket_manager_.set_socket(socket);
}


void Chat_session::start() {
    thread receiver(&Chat_session::receive_messages, this);
    receiver.detach();

    thread reminder_thread(&Chat_session::reminder_loop, this);
    reminder_thread.detach();
}


void Chat_session::receive_messages() {
    while (running_) {
        string message = socket_manager_.receive();

        if (message.empty()) {
            clear_line();
            console_.set_system_color();
            cout << other_name << " отключился!" << endl;
            console_.set_default_color();
            running_ = false;
            break;
        }

        stats_.increment_received();
        reminder_.message_received();

        clear_line();

        console_.set_time_color();
        cout << Time_utils().get_current_time() << " ";
        console_.set_other_color();
        cout << other_name;
        console_.set_default_color();
        cout << ": " << message << endl;
    }
}


void Chat_session::reminder_loop() {
    while (running_) {
        this_thread::sleep_for(chrono::seconds(REMINDER_CHECK_INTERVAL_SEC));

        if (reminder_.need_reminder() && running_) {
            clear_line();

            console_.set_system_color();
            cout << "НАПОМИНАНИЕ: ";
            console_.set_time_color();
            cout << Time_utils().get_current_time() << " ";
            console_.set_other_color();
            cout << other_name;
            console_.set_default_color();
            cout << " ждет ответа!" << endl;
        }
    }
}


void Chat_session::clear_line() {
    cout << '\r' << string(80, ' ') << '\r' << flush;
}


void Chat_session::show_input(const string& input) {
    console_.set_input_color();
    cout << input << flush;
    console_.set_default_color();
}


string Chat_session::read_input() {
    string input;
    char ch;

    while (running_) {
        if (_kbhit()) {
            ch = _getch();

            if (ch == '\r') {
                clear_line();
                return input;
            }
            else if (ch == '\b') {
                if (!input.empty()) {
                    input.pop_back();
                    cout << '\r' << string(80, ' ') << '\r';
                    show_input(input);
                }
            }
            else {
                input += ch;
                cout << ch << flush;
            }
        }
        this_thread::sleep_for(chrono::milliseconds(INPUT_CHECK_DELAY_MS));
    }
    return "";
}


void Chat_session::send_message(const string& msg) {
    string processed_message = emoji_.replace_commands(msg);

    if (msg == "/help") {
        emoji_.show_help(console_);
        return;
    }

    if (msg == "/stats") {
        stats_.show_stats(console_);
        return;
    }

    if (msg != "/exit") {
        socket_manager_.send(processed_message);
        stats_.increment_sent();
        reminder_.message_sent();

        console_.set_time_color();
        cout << Time_utils().get_current_time() << " ";
        console_.set_my_color();
        cout << my_name;
        console_.set_default_color();
        cout << ": " << processed_message << endl;
    }
}


string Chat_session::get_name() {
    return my_name;
}


void Chat_session::close() {
    socket_manager_.close();
}


Connection_manager::Connection_manager(Console_color& cons) : console_(cons) {}


bool Connection_manager::init_winsock() {
    WSADATA wsaData;
    return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
}


bool Connection_manager::start_server() {
    console_.set_system_color();
    cout << "\n--- Режим сервера ---" << endl;
    console_.set_default_color();

    Socket_manager server_socket;
    if (!server_socket.create()) {
        cout << "Ошибка создания сокета!" << endl;
        return false;
    }

    if (!server_socket.bind(PORT)) {
        cout << "Ошибка bind! Порт занят." << endl;
        return false;
    }

    if (!server_socket.listen(1)) {
        cout << "Ошибка listen!" << endl;
        return false;
    }

    show_ip_addresses();

    cout << "\nОжидание подключения на порту " << PORT << "..." << endl;

    sockaddr_in client_addr;
    chat_socket.set_socket(server_socket.accept(client_addr));

    if (chat_socket.get_socket() == INVALID_SOCKET) {
        cout << "Ошибка подключения!" << endl;
        return false;
    }

    console_.set_system_color();
    cout << "\nПодключение от: ";
    console_.set_default_color();
    cout << inet_ntoa(client_addr.sin_addr) << endl;

    exchange_names();

    console_.set_other_color();
    cout << other_name;
    console_.set_default_color();
    cout << " подключился(ась)!" << endl;

    return true;
}


bool Connection_manager::start_client() {
    console_.set_system_color();
    cout << "\n--- Режим клиента ---" << endl;
    console_.set_default_color();

    string server_ip;
    cout << "Введите IP-адрес сервера (друга): ";
    getline(cin, server_ip);

    if (server_ip.empty()) {
        cout << "IP не введен. Выход..." << endl;
        return false;
    }

    if (!chat_socket.create()) {
        cout << "Ошибка создания сокета!" << endl;
        return false;
    }

    cout << "Подключение к " << server_ip << ":" << PORT << "..." << endl;

    if (!try_connect(server_ip)) {
        return false;
    }

    exchange_names();

    cout << "Подключение к ";
    console_.set_other_color();
    cout << other_name;
    console_.set_default_color();
    cout << "!" << endl;

    return true;
}


bool Connection_manager::exchange_names() {
    chat_socket.send(my_name);
    other_name = chat_socket.receive();
    return !other_name.empty();
}


string Connection_manager::get_other_name() {
    return other_name;
}


SOCKET Connection_manager::get_socket() {
    return chat_socket.get_socket();
}


void Connection_manager::cleanup() {
    WSACleanup();
}


void Connection_manager::set_my_name(const string& name) {
    my_name = name;
}


void Connection_manager::show_ip_addresses() {
    char hostname[256];
    gethostname(hostname, sizeof(hostname));
    struct hostent* host = gethostbyname(hostname);

    console_.set_system_color();
    cout << "\nВаши IP-адреса (скажите другу один из них):" << endl;
    console_.set_default_color();

    if (host != NULL) {
        for (int i = 0; host->h_addr_list[i] != NULL; i++) {
            struct in_addr addr;
            memcpy(&addr, host->h_addr_list[i], sizeof(addr));
            cout << "  " << inet_ntoa(addr) << endl;
        }
    }
}


bool Connection_manager::try_connect(const string& ip) {
    int attempts = 0;
    bool connected = false;

    while (attempts < MAX_CONNECTION_ATTEMPTS && !connected) {
        if (attempts > 0) {
            cout << "Попытка " << attempts + 1 << "... ждем 2 секунды" << endl;
            Sleep(2000);
        }
        connected = chat_socket.connect(ip, PORT);
        attempts++;
    }

    if (!connected) {
        cout << "Не удалось подключиться!" << endl;
        cout << "Проверьте:" << endl;
        cout << "1. Запущен ли сервер на другом компьютере" << endl;
        cout << "2. IP адрес правильный" << endl;
        cout << "3. Брандмауэр не блокирует порт" << endl;
        cout << "4. Подключение к одной сети" << endl;
        return false;
    }
    return true;
}


void User_interface::setup() {
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);
    setlocale(LC_ALL, "Russian");
}


void User_interface::show_welcome_screen() {
    console_.set_system_color();
    cout << "=====================================" << endl;
    cout << "              ЧАТ" << endl;
    cout << "=====================================\n" << endl;
    console_.set_default_color();
}


string User_interface::get_name() {
    string name;
    cout << "Введите ваше имя: ";
    getline(cin, name);
    return name.empty() ? "Пользователь" : name;
}


string User_interface::get_mode() {
    string mode;
    cout << "\nВыберите режим:" << endl;
    cout << "1 - Как сервер (ожидание друга)" << endl;
    cout << "2 - Как клиент (подключение к другу)" << endl;
    cout << "Ваш выбор (1 или 2): ";
    getline(cin, mode);
    return mode;
}


void User_interface::show_chat_start(const string& my_name, const string& other_name) {
    console_.set_system_color();
    cout << "\n=====================================" << endl;
    cout << "           НАЧАЛО ЧАТА" << endl;
    cout << "=====================================" << endl;
    console_.set_default_color();

    cout << "Вы: ";
    console_.set_my_color();
    cout << my_name << endl;
    console_.set_default_color();

    cout << "Собеседник: ";
    console_.set_other_color();
    cout << other_name << endl;
    console_.set_default_color();

    console_.set_time_color();
    cout << "\nВремя сообщений будет отображаться в формате [ЧЧ:ММ:СС]" << endl;
    console_.set_default_color();

    cout << "Для выхода введите '/exit'\n" << endl;
    cout << "Теперь просто печатайте - текст будет виден, но не продублируется\n" << endl;
}


Console_color& User_interface::get_console() {
    return console_;
}


bool initialize_application(Connection_manager& connection, const string& my_name) {
    connection.set_my_name(my_name);

    if (!connection.init_winsock()) {
        cout << "Ошибка инициализации Winsock!" << endl;
        return false;
    }

    return true;
}


bool establish_connection(const string& mode, Connection_manager& connection) {
    bool connected = false;

    if (mode == "1") {
        connected = connection.start_server();
    }
    else if (mode == "2") {
        connected = connection.start_client();
    }
    else {
        cout << "Неверный режим!" << endl;
        return false;
    }

    return connected;
}


void run_chat_session(const string& my_name, Connection_manager& connection, Console_color& console,
    Emoji_manager& emoji, Reminder_tracker& reminder, atomic<bool>& running, Chat_statistics& stats) {
    string other_name = connection.get_other_name();

    User_interface ui;
    ui.show_chat_start(my_name, other_name);
    emoji.show_help(console);

    Chat_session chat(connection.get_socket(), my_name, other_name, running, reminder, emoji, console, stats);
    chat.start();

    while (running) {
        string message = chat.read_input();

        if (!running) break;

        if (message == "/exit") {
            chat.send_message(message);
            break;
        }

        chat.send_message(message);
    }

    running = false;
    Sleep(1000);
    chat.close();

    cout << endl;
    stats.show_stats(console);
}


void shutdown_application(Connection_manager& connection) {
    connection.cleanup();

    cout << "\nНажмите Enter для выхода...";
    cin.get();
}


int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    User_interface ui;
    ui.setup();
    ui.show_welcome_screen();

    string my_name = ui.get_name();
    string mode = ui.get_mode();

    Connection_manager connection(ui.get_console());
    if (!initialize_application(connection, my_name)) {
        return 1;
    }

    if (!establish_connection(mode, connection)) {
        connection.cleanup();
        return 1;
    }

    Emoji_manager emoji;
    Reminder_tracker reminder;
    atomic<bool> running(true);
    Chat_statistics stats;

    run_chat_session(my_name, connection, ui.get_console(), emoji, reminder, running, stats);

    shutdown_application(connection);

    return 0;
}