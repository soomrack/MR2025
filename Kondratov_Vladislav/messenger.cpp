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

// Класс для работы со временем
class time_utils {
public:
    static string get_current_time() {
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
};

// Класс для управления цветом в консоли
class console_color {
private:
    HANDLE h_console;
    static const int MY_COLOR = 10;      // зеленый
    static const int OTHER_COLOR = 9;    // синий
    static const int SYSTEM_COLOR = 14;  // желтый
    static const int DEFAULT_COLOR = 7;  // белый
    static const int TIME_COLOR = 8;     // серый для времени
    static const int INPUT_COLOR = 15;   // ярко-белый для ввода

public:
    console_color() {
        h_console = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    void set_color(int color) const {
        SetConsoleTextAttribute(h_console, color);
    }

    void set_my_color() const { set_color(MY_COLOR); }
    void set_other_color() const { set_color(OTHER_COLOR); }
    void set_system_color() const { set_color(SYSTEM_COLOR); }
    void set_default_color() const { set_color(DEFAULT_COLOR); }
    void set_time_color() const { set_color(TIME_COLOR); }
    void set_input_color() const { set_color(INPUT_COLOR); }
};

// Класс для сбора статистики
class chat_statistics {
private:
    int messages_sent;
    int messages_received;
    chrono::steady_clock::time_point start_time;

public:
    chat_statistics() : messages_sent(0), messages_received(0) {
        start_time = chrono::steady_clock::now();
    }

    void increment_sent() { messages_sent++; }
    void increment_received() { messages_received++; }

    int get_messages_sent() const { return messages_sent; }
    int get_messages_received() const { return messages_received; }
    int get_total_messages() const { return messages_sent + messages_received; }

    double get_session_duration() const {
        auto now = chrono::steady_clock::now();
        return chrono::duration_cast<chrono::seconds>(now - start_time).count();
    }

    void show_stats(const console_color& console) const {
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
};

// Класс для управления эмодзи
class emoji_manager {
private:
    map<string, string> emoji_map;

    void init_emoji_map() {
        emoji_map["/smile"] = ":-)";
        emoji_map["/love"] = "<3";
        emoji_map["/cool"] = "B-)";
    }

public:
    emoji_manager() {
        init_emoji_map();
    }

    string replace_commands(const string& message) const {
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

    void show_help(const console_color& console) const {
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
};

// Класс для отслеживания напоминаний
class reminder_tracker {
private:
    chrono::steady_clock::time_point last_message_time;
    chrono::steady_clock::time_point last_reminder_time;
    bool waiting_for_my_response;

public:
    reminder_tracker()
        : last_message_time(chrono::steady_clock::now())
        , last_reminder_time(chrono::steady_clock::now())
        , waiting_for_my_response(false) {
    }

    void message_received() {
        last_message_time = chrono::steady_clock::now();
        waiting_for_my_response = true;
    }

    void message_sent() {
        last_message_time = chrono::steady_clock::now();
        waiting_for_my_response = false;
    }

    bool need_reminder() {
        if (!waiting_for_my_response) return false;

        auto now = chrono::steady_clock::now();
        auto seconds_since_last_msg = chrono::duration_cast<chrono::seconds>(now - last_message_time).count();
        auto seconds_since_last_reminder = chrono::duration_cast<chrono::seconds>(now - last_reminder_time).count();

        if (seconds_since_last_msg >= 60 && seconds_since_last_reminder >= 60) {
            last_reminder_time = now;
            return true;
        }
        return false;
    }

    void reset() {
        waiting_for_my_response = false;
    }
};

// Класс для управления сокетом
class socket_manager {
private:
    SOCKET socket_;

public:
    socket_manager() : socket_(INVALID_SOCKET) {}

    ~socket_manager() {
        close();
    }

    bool create() {
        socket_ = ::socket(AF_INET, SOCK_STREAM, 0);
        return socket_ != INVALID_SOCKET;
    }

    bool bind(int port) {
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        return ::bind(socket_, (sockaddr*)&addr, sizeof(addr)) != SOCKET_ERROR;
    }

    bool listen(int backlog) {
        return ::listen(socket_, backlog) != SOCKET_ERROR;
    }

    SOCKET accept(sockaddr_in& client_addr) {
        int client_size = sizeof(client_addr);
        return ::accept(socket_, (sockaddr*)&client_addr, &client_size);
    }

    bool connect(const string& ip, int port) {
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip.c_str());

        if (addr.sin_addr.s_addr == INADDR_NONE) {
            return false;
        }

        return ::connect(socket_, (sockaddr*)&addr, sizeof(addr)) != SOCKET_ERROR;
    }

    int send(const string& data) const {
        return ::send(socket_, data.c_str(), data.length(), 0);
    }

    string receive() const {
        char buffer[1024] = { 0 };
        int bytes = recv(socket_, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) return "";
        return string(buffer);
    }

    void close() {
        if (socket_ != INVALID_SOCKET) {
            closesocket(socket_);
            socket_ = INVALID_SOCKET;
        }
    }

    SOCKET get_socket() const { return socket_; }
    void set_socket(SOCKET s) { socket_ = s; }
};

// Класс для управления чат-сессией
class chat_session {
private:
    socket_manager socket_manager_;
    string my_name;
    string other_name;
    atomic<bool>& running_;
    reminder_tracker& reminder_;
    emoji_manager& emoji_;
    console_color& console_;
    chat_statistics& stats_;

    void clear_line() {
        cout << '\r' << string(80, ' ') << '\r' << flush;
    }

    void show_input(const string& input) {
        console_.set_input_color();
        cout << input << flush;
        console_.set_default_color();
    }

public:
    chat_session(SOCKET socket, const string& my_name, const string& other_name,
        atomic<bool>& run, reminder_tracker& rem, emoji_manager& em,
        console_color& cons, chat_statistics& stat)
        : my_name(my_name), other_name(other_name), running_(run), reminder_(rem),
        emoji_(em), console_(cons), stats_(stat) {
        socket_manager_.set_socket(socket);
    }

    void start() {
        thread receiver(&chat_session::receive_messages, this);
        receiver.detach();

        thread reminder_thread(&chat_session::reminder_loop, this);
        reminder_thread.detach();
    }

    void receive_messages() {
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

            // Очищаем строку с вводом
            clear_line();

            // Выводим полученное сообщение
            console_.set_time_color();
            cout << time_utils::get_current_time() << " ";
            console_.set_other_color();
            cout << other_name;
            console_.set_default_color();
            cout << ": " << message << endl;
        }
    }

    void reminder_loop() {
        while (running_) {
            this_thread::sleep_for(chrono::seconds(5));

            if (reminder_.need_reminder() && running_) {
                clear_line();

                console_.set_system_color();
                cout << "НАПОМИНАНИЕ: ";
                console_.set_time_color();
                cout << time_utils::get_current_time() << " ";
                console_.set_other_color();
                cout << other_name;
                console_.set_default_color();
                cout << " ждет ответа!" << endl;
            }
        }
    }

    string read_input() {
        string input;
        char ch;

        while (running_) {
            if (_kbhit()) {
                ch = _getch();

                if (ch == '\r') { // Enter
                    // Очищаем строку с вводом перед отправкой
                    clear_line();
                    return input;
                }
                else if (ch == '\b') { // Backspace
                    if (!input.empty()) {
                        input.pop_back();
                        // Перерисовываем строку ввода
                        cout << '\r' << string(80, ' ') << '\r';
                        show_input(input);
                    }
                }
                else {
                    input += ch;
                    cout << ch << flush; // Показываем символ
                }
            }
            this_thread::sleep_for(chrono::milliseconds(10));
        }
        return "";
    }

    void send_message(const string& msg) {
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

            // Выводим отправленное сообщение
            console_.set_time_color();
            cout << time_utils::get_current_time() << " ";
            console_.set_my_color();
            cout << my_name;
            console_.set_default_color();
            cout << ": " << processed_message << endl;
        }
    }

    string get_name() const { return my_name; }
    void close() {
        socket_manager_.close();
    }
};

// Класс для управления подключением
class connection_manager {
private:
    static const int PORT = 8080;
    static const int MAX_ATTEMPTS = 3;

    socket_manager chat_socket;
    string my_name;
    string other_name;
    console_color& console_;

public:
    connection_manager(console_color& cons) : console_(cons) {}

    bool init_winsock() {
        WSADATA wsaData;
        return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
    }

    bool start_server() {
        console_.set_system_color();
        cout << "\n--- Режим сервера ---" << endl;
        console_.set_default_color();

        socket_manager server_socket;
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

    bool start_client() {
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

    bool exchange_names() {
        chat_socket.send(my_name);
        other_name = chat_socket.receive();
        return !other_name.empty();
    }

    string get_other_name() const { return other_name; }
    SOCKET get_socket() const { return chat_socket.get_socket(); }

    void cleanup() {
        WSACleanup();
    }

    void set_my_name(const string& name) { my_name = name; }

private:
    void show_ip_addresses() {
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

    bool try_connect(const string& ip) {
        int attempts = 0;
        bool connected = false;

        while (attempts < MAX_ATTEMPTS && !connected) {
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
};

// Класс для управления пользовательским интерфейсом
class user_interface {
private:
    console_color console_;

public:
    void setup() {
        SetConsoleOutputCP(1251);
        SetConsoleCP(1251);
        setlocale(LC_ALL, "Russian");
    }

    void show_welcome_screen() {
        console_.set_system_color();
        cout << "=====================================" << endl;
        cout << "              ЧАТ" << endl;
        cout << "=====================================\n" << endl;
        console_.set_default_color();
    }

    string get_name() {
        string name;
        cout << "Введите ваше имя: ";
        getline(cin, name);
        return name.empty() ? "Пользователь" : name;
    }

    string get_mode() {
        string mode;
        cout << "\nВыберите режим:" << endl;
        cout << "1 - Как сервер (ожидание друга)" << endl;
        cout << "2 - Как клиент (подключение к другу)" << endl;
        cout << "Ваш выбор (1 или 2): ";
        getline(cin, mode);
        return mode;
    }

    void show_chat_start(const string& my_name, const string& other_name) {
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

    console_color& get_console() { return console_; }
};

// Вспомогательные функции для main
bool initialize_application(connection_manager& connection, const string& my_name) {
    connection.set_my_name(my_name);

    if (!connection.init_winsock()) {
        cout << "Ошибка инициализации Winsock!" << endl;
        return false;
    }

    return true;
}

bool establish_connection(const string& mode, connection_manager& connection) {
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

void run_chat_session(const string& my_name, connection_manager& connection, console_color& console, emoji_manager& emoji, reminder_tracker& reminder, atomic<bool>& running, chat_statistics& stats) {
    string other_name = connection.get_other_name();

    user_interface ui;
    ui.show_chat_start(my_name, other_name);
    emoji.show_help(console);

    chat_session chat(connection.get_socket(), my_name, other_name, running, reminder, emoji, console, stats);
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

void shutdown_application(connection_manager& connection) {
    connection.cleanup();

    cout << "\nНажмите Enter для выхода...";
    cin.get();
}

// Точка входа
int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // инициализация
    user_interface ui;
    ui.setup();
    ui.show_welcome_screen();

    // получение данных от пользователей
    string my_name = ui.get_name();
    string mode = ui.get_mode();

    // инициализация Winsock
    connection_manager connection(ui.get_console());
    if (!initialize_application(connection, my_name)) {
        return 1;
    }

    // установка соединения
    if (!establish_connection(mode, connection)) {
        connection.cleanup();
        return 1;
    }

    // запуск чата
    emoji_manager emoji;
    reminder_tracker reminder;
    atomic<bool> running(true);
    chat_statistics stats;

    run_chat_session(my_name, connection, ui.get_console(), emoji, reminder, running, stats);

    // завершение работы
    shutdown_application(connection);

    return 0;
}