// 1. Определения препроцессора
#define _WIN32_WINNT 0x0600  // Требуем версию Windows Vista или новее для некоторых функций Winsock
#define _CRT_SECURE_NO_WARNINGS // Отключаем предупреждения о небезопасных функциях типа strcpy
#define _WINSOCK_DEPRECATED_NO_WARNINGS // Отключаем предупреждения об устаревших функциях Winsock

// 2. Подключаемые заголовочные файлы
#include <iostream>   // Стандартный ввод/вывод
#include <string>     // Для работы со строками
#include <thread>     // Для работы с потоками
#include <chrono>     // Для работы со временем (Sleep, таймауты)
#include <conio.h>    // Для _kbhit() и _getch() (неблокирующий ввод с клавиатуры)
#include <fstream>    // Для работы с файлами
#include <sstream>    // Для форматирования строк
#include <iomanip>    // Для манипуляторов вывода (setw, setprecision)
#include <ctime>      // Для работы с датой/временем
#include <vector>     // Для динамических массивов
#include <memory>     // Для умных указателей (unique_ptr)
#include <unordered_map> // Для хеш-таблиц
#include <functional> // Для std::function

// 3. Заголовки для работы с сетью Windows
#include <winsock2.h> // Основной заголовок Windows Sockets
#include <windows.h>  // Общие функции Windows API
#include <ws2tcpip.h> // Дополнительные функции для TCP/IP

#pragma comment(lib, "ws2_32.lib") // Линковка библиотеки Winsock

using namespace std; // Используем стандартное пространство имен

// ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ (ОБЪЯВЛЕНИЯ) ==========
string utf8_to_cp1251(const string& utf8_str); // Конвертация строки из UTF-8 в кодировку Windows-1251 (для консоли)
void set_console_encoding();                   // Установка кодировки консоли
void clear_screen();                           // Очистка экрана (вызов system("cls"))
void print_header();                           // Печать шапки программы
string get_current_filename();                 // Генерация имени файла для отчета на основе текущей даты/времени
void save_to_file(const string& content);      // Сохранение строки в файл

// ========== ИНТЕРФЕЙС РЕЖИМА РАБОТЫ ==========
class IClientMode {
public:
    virtual ~IClientMode() = default; // Виртуальный деструктор
    // Чисто виртуальная функция выполнения режима. Принимает ссылку на клиента.
    virtual void execute(class RobotClient& client) = 0;
    // Чисто виртуальная функция для получения имени режима (для меню)
    virtual string get_name() const = 0;
};

// ========== КЛАСС RobotClient (ОБЪЯВЛЕНИЕ) ==========
class RobotClient {
private:
    SOCKET sock_;                // Сокет для соединения
    string server_ip_;           // IP-адрес сервера
    int port_;                   // Порт сервера
    bool connected_;             // Флаг состояния подключения
    bool receive_mode_;          // Флаг режима приема данных
    static const int recv_timeout_ms_ = 5000; // Таймаут приема по умолчанию (5 сек)

public:
    // Конструктор: принимает IP и порт
    RobotClient(const string& ip, int p);
    // Деструктор: вызывает disconnect()
    ~RobotClient();
    bool connect_to_server();    // Установка соединения с сервером
    void disconnect();           // Разрыв соединения
    string send_command(const string& cmd); // Отправка команды без ожидания ответа
    string send_request(const string& cmd); // Отправка команды и прием ответа (для статистики)
    string send_monitoring_request(const string& cmd); // Отправка для мониторинга (непрерывный прием)
    bool is_connected() const;   // Проверка соединения
};

// ========== РЕЖИМЫ РАБОТЫ (ОБЪЯВЛЕНИЯ) ==========
// Режим ручного управления роботом (клавиши W, A, S, D, X)
class ControlMode : public IClientMode {
public:
    void execute(RobotClient& client) override;
    string get_name() const override { return "УПРАВЛЕНИЕ РОБОТОМ"; } 

// Режим запроса статистики (температура, CPU, логи и т.д.)
class StatsMode : public IClientMode {
public:
    void execute(RobotClient& client) override;
    string get_name() const override { return "ЗАПРОС СТАТИСТИКИ"; } 
};

// Режим мониторинга в реальном времени
class MonitoringMode : public IClientMode {
public:
    void execute(RobotClient& client) override;
    string get_name() const override { return "МОНИТОРИНГ"; } 
};

// Режим настройки подключения (смена IP)
class ConnectionSetupMode : public IClientMode {
private:
    string& ip_; // Ссылка на строку с IP для изменения
public:
    ConnectionSetupMode(string& ip) : ip_(ip) {}
    void execute(RobotClient& client) override;
    string get_name() const override { return "НАСТРОЙКА ПОДКЛЮЧЕНИЯ"; } 
};

// ========== МЕНЕДЖЕР РЕЖИМОВ ==========
class ModeManager {
private:
    // Карта для хранения всех режимов. Ключ - строка (цифра меню), значение - умный указатель на объект режима.
    unordered_map<string, unique_ptr<IClientMode>> modes_;
    string& current_ip_; // Ссылка на IP для передачи в ConnectionSetupMode
public:
    ModeManager(string& ip) : current_ip_(ip) {
        // Регистрируем все режимы при создании
        register_mode("1", make_unique<ControlMode>());
        register_mode("2", make_unique<StatsMode>());
        register_mode("3", make_unique<MonitoringMode>());
        register_mode("4", make_unique<ConnectionSetupMode>(current_ip_));
    }
    void register_mode(const string& key, unique_ptr<IClientMode> mode) {
        modes_[key] = move(mode); // Перемещаем умный указатель в карту
    }
    // Выполнить режим по ключу (цифре меню)
    bool execute_mode(const string& key, RobotClient& client) {
        auto it = modes_.find(key);
        if (it != modes_.end()) {
            it->second->execute(client);
            return true;
        }
        return false;
    }
    void show_menu() const; // Показать главное меню
};

// ========== ГЛАВНОЕ ПРИЛОЖЕНИЕ КЛИЕНТА ==========
class ClientApplication {
private:
    string server_ip_;
    unique_ptr<RobotClient> client_;
    unique_ptr<ModeManager> mode_manager_;
    void initialize_console();                // Настройка консоли
    string get_server_ip_from_user();        // Запрос IP у пользователя
    void wait_for_exit();                    // Ожидание нажатия клавиши перед выходом
    void test_connection();                  // Тестирование соединения
public:
    ClientApplication();                     // Конструктор
    ~ClientApplication();                    // Деструктор
    void run();                              // Запуск основного цикла программы
    void shutdown();                         // Корректное завершение работы
};

// ========== РЕАЛИЗАЦИЯ ВСПОМОГАТЕЛЬНЫХ ФУНКЦИЙ ==========
string utf8_to_cp1251(const string& utf8_str) {
    if (utf8_str.empty()) return ""; // Если строка пуста, возвращаем пустую строку

    // 1. Получаем размер буфера в широких символах (UTF-16) для UTF-8 строки
    int wlen = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, NULL, 0);
    if (wlen <= 0) return utf8_str; // Если ошибка, возвращаем исходную строку

    // 2. Выделяем буфер и конвертируем UTF-8 -> UTF-16
    wchar_t* wbuf = new wchar_t[wlen];
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, wbuf, wlen);

    // 3. Получаем размер буфера для кодировки Windows-1251 из UTF-16 строки
    int clen = WideCharToMultiByte(1251, 0, wbuf, -1, NULL, 0, NULL, NULL);
    if (clen <= 0) { delete[] wbuf; return utf8_str; }

    // 4. Выделяем буфер и конвертируем UTF-16 -> Windows-1251
    char* cbuf = new char[clen];
    WideCharToMultiByte(1251, 0, wbuf, -1, cbuf, clen, NULL, NULL);

    // 5. Копируем результат в string и очищаем память
    string result(cbuf);
    delete[] wbuf;
    delete[] cbuf;
    return result;
}

void set_console_encoding() {
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);
}

void clear_screen() {
    system("cls");
}

void print_header() {
    cout << "====================================" << endl;
    cout << "    УПРАВЛЕНИЕ РОБОТОМ v5.0        " << endl;
    cout << "====================================" << endl;
}

string get_current_filename() {
    auto now = chrono::system_clock::now();
    time_t now_time = chrono::system_clock::to_time_t(now);
    struct tm timeinfo;
    localtime_s(&timeinfo, &now_time);

    // Формируем имя файла по шаблону: robot_report_ГГГГ-ММ-ДД_ЧЧ-ММ-СС.txt
    stringstream ss;
    ss << "robot_report_"
        << setw(2) << setfill('0') << timeinfo.tm_year + 1900 << "-"
        << setw(2) << setfill('0') << timeinfo.tm_mon + 1 << "-"
        << setw(2) << setfill('0') << timeinfo.tm_mday << "_"
        << setw(2) << setfill('0') << timeinfo.tm_hour << "-"
        << setw(2) << setfill('0') << timeinfo.tm_min << "-"
        << setw(2) << setfill('0') << timeinfo.tm_sec << ".txt";
    return ss.str();
}

// Сохраняем переданный текст в файл с автогенерируемым именем
void save_to_file(const string& content) {
    string filename = get_current_filename();
    ofstream file(filename);
    file << content;
    file.close();
    cout << "\nОтчёт сохранён в файл: " << filename << endl;
}

// ========== РЕАЛИЗАЦИЯ RobotClient ==========
// Конструктор: инициализируем переменные, сокет пока недействителен
RobotClient::RobotClient(const string& ip, int p)
    : server_ip_(ip), port_(p), connected_(false), receive_mode_(false) {
    sock_ = INVALID_SOCKET; // В Windows невалидный сокет обозначается так
}

RobotClient::~RobotClient() {
    disconnect();
}

bool RobotClient::connect_to_server() {
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        cout << "Ошибка WSAStartup" << endl;
        return false;
    }

    sock_ = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_ == INVALID_SOCKET) {
        cout << "Ошибка создания сокета" << endl;
        WSACleanup();
        return false;
    }

    int timeout = recv_timeout_ms_;
    setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));
    setsockopt(sock_, SOL_SOCKET, SO_SNDTIMEO, (const char*)&timeout, sizeof(timeout));

    sockaddr_in server_addr;  //Настройка адреса сервера
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    server_addr.sin_addr.s_addr = inet_addr(server_ip_.c_str());

    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        cout << "Неверный IP адрес" << endl;
        closesocket(sock_);
        WSACleanup();
        return false;
    }

    cout << "Подключение к " << server_ip_ << ":" << port_ << "...";

    if (::connect(sock_, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cout << " ОШИБКА" << endl;
        closesocket(sock_);
        WSACleanup();
        return false;
    }

    cout << " УСПЕШНО!" << endl;
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
    receive_mode_ = false;
    send(sock_, cmd.c_str(), cmd.length(), 0);
    return "OK";
}

string RobotClient::send_request(const string& cmd) {
    if (!connected_) return "ERROR: Not connected";

    receive_mode_ = true;

    if (send(sock_, cmd.c_str(), cmd.length(), 0) == SOCKET_ERROR) {
        return "ERROR: Send failed";
    }

    int timeout = 30000;
    setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    string full_response;
    char buffer[4096];
    int bytes_received;

    Sleep(500);

    while (true) {
        bytes_received = recv(sock_, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            full_response += buffer;

            if (bytes_received < (int)(sizeof(buffer) - 1)) {
                Sleep(50);
                u_long available = 0;
                ioctlsocket(sock_, FIONREAD, &available);
                if (available == 0) break;
            }
        }
        else if (bytes_received == 0) {
            break;
        }
        else {
            int error = WSAGetLastError();
            if (error != WSAETIMEDOUT) {
                full_response += "\n[Ошибка приёма данных]";
            }
            break;
        }
    }

    timeout = recv_timeout_ms_;
    setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    if (full_response.empty()) return "ERROR: No response";
    return utf8_to_cp1251(full_response);
}

string RobotClient::send_monitoring_request(const string& cmd) {
    if (!connected_) return "ERROR: Not connected";

    if (send(sock_, cmd.c_str(), cmd.length(), 0) == SOCKET_ERROR) {
        return "ERROR: Send failed";
    }

    int timeout = 30000;
    setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    string full_response;
    char buffer[4096];
    int bytes_received;

    Sleep(500);

    while (true) {
        bytes_received = recv(sock_, buffer, sizeof(buffer) - 1, 0);

        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            full_response += buffer;
            if (bytes_received < (int)(sizeof(buffer) - 1)) break;
        }
        else if (bytes_received == 0) {
            break;
        }
        else {
            int error = WSAGetLastError();
            if (error != WSAETIMEDOUT) {
                full_response += "\n[Ошибка приёма данных]";
            }
            break;
        }
    }

    timeout = recv_timeout_ms_;
    setsockopt(sock_, SOL_SOCKET, SO_RCVTIMEO, (const char*)&timeout, sizeof(timeout));

    if (full_response.empty()) return "ERROR: No response";
    return utf8_to_cp1251(full_response);
}

bool RobotClient::is_connected() const { return connected_; }

// ========== РЕАЛИЗАЦИЯ ModeManager ==========
void ModeManager::show_menu() const {
    clear_screen();
    print_header();
    cout << "\nГЛАВНОЕ МЕНЮ" << endl;
    cout << "-----------------------------" << endl;
    cout << "1 - " << modes_.at("1")->get_name() << endl;
    cout << "2 - " << modes_.at("2")->get_name() << endl;
    cout << "3 - " << modes_.at("3")->get_name() << endl;
    cout << "4 - " << modes_.at("4")->get_name() << endl;
    cout << "0 - Выход" << endl;
    cout << "-----------------------------" << endl;
    cout << "Выберите режим: ";
}

// ========== РЕАЛИЗАЦИЯ РЕЖИМОВ ==========
void ControlMode::execute(RobotClient& client) {
    clear_screen();
    print_header();
    cout << "\nРЕЖИМ УПРАВЛЕНИЯ" << endl;
    cout << "-----------------------------" << endl;
    cout << "W - вперёд    S - назад" << endl;
    cout << "A - влево     D - вправо" << endl;
    cout << "X - стоп      Q - выход из режима" << endl;
    cout << "-----------------------------" << endl;
    cout << "Текущая скорость: 150" << endl;
    cout << "\nКоманды:" << endl;
    cout << "  speed 200 - установить скорость" << endl;
    cout << "-----------------------------\n" << endl;

    bool in_control = true;
    while (in_control) {
        if (_kbhit()) {
            char key = _getch();
            key = tolower(key);

            if (key == 'q') {
                in_control = false;
                cout << "\nВыход из режима управления..." << endl;
            }
            else if (key == 'w' || key == 'a' || key == 's' || key == 'd') {
                string cmd(1, key);
                client.send_command(cmd);
                cout << "Команда: " << cmd << endl;
            }
            else if (key == 'x') {
                client.send_command("x");
                cout << "СТОП" << endl;
            }
        }
        Sleep(50);
    }
}

void StatsMode::execute(RobotClient& client) {
    while (true) {
        clear_screen();
        print_header();

        cout << "\nРЕЖИМ ЗАПРОСА СТАТИСТИКИ" << endl;
        cout << "-----------------------------" << endl;
        cout << "1 - Статистика температуры" << endl;
        cout << "2 - Критические события" << endl;
        cout << "3 - Загрузка CPU" << endl;
        cout << "4 - Полный отчёт" << endl;
        cout << "5 - Последние логи сервера" << endl;
        cout << "0 - Вернуться в главное меню" << endl;
        cout << "-----------------------------" << endl;
        cout << "Ваш выбор: ";

        string choice;
        getline(cin, choice);

        if (choice == "0") return;

        string command, report_type;
        if (choice == "1") { command = "GET_TEMP_STATS"; report_type = "Статистика температуры"; }
        else if (choice == "2") { command = "GET_CRITICAL_EVENTS"; report_type = "Критические события"; }
        else if (choice == "3") { command = "GET_CPU_LOAD"; report_type = "Загрузка CPU"; }
        else if (choice == "4") { command = "GET_FULL_REPORT"; report_type = "Полный отчёт"; }
        else if (choice == "5") { command = "GET_LOGS"; report_type = "Последние логи"; }
        else {
            cout << "\nНеверный выбор! Нажмите любую клавишу..." << endl;
            _getch();
            continue;
        }

        cout << "\nЗапрос " << report_type << "..." << endl;
        cout << "Ожидание ответа";
        for (int i = 0; i < 3; i++) { Sleep(500); cout << "."; }
        cout << endl;

        string response = client.send_request(command);
        cout << "\n" << response << endl;

        cout << "\nСохранить отчёт в файл? (y/n): ";
        string save;
        getline(cin, save);

        if (save == "y" || save == "Y" || save == "д" || save == "Д") {
            save_to_file(response);
        }

        cout << "\nНажмите Enter для продолжения...";
        cin.get();
    }
}

void MonitoringMode::execute(RobotClient& client) {
    clear_screen();
    print_header();

    cout << "\nРЕЖИМ МОНИТОРИНГА (реальное время)" << endl;
    cout << "-----------------------------" << endl;
    cout << "1 - Мониторинг температуры" << endl;
    cout << "2 - Мониторинг загрузки CPU" << endl;
    cout << "3 - Мониторинг критических событий" << endl;
    cout << "4 - Полный мониторинг (все параметры)" << endl;
    cout << "0 - Вернуться в главное меню" << endl;
    cout << "-----------------------------" << endl;
    cout << "Ваш выбор: ";

    string choice;
    getline(cin, choice);

    string command, monitor_title;

    if (choice == "1") { command = "GET_TEMP_STATS"; monitor_title = "МОНИТОРИНГ ТЕМПЕРАТУРЫ"; }
    else if (choice == "2") { command = "GET_CPU_LOAD"; monitor_title = "МОНИТОРИНГ ЗАГРУЗКИ CPU"; }
    else if (choice == "3") { command = "GET_CRITICAL_EVENTS"; monitor_title = "МОНИТОРИНГ КРИТИЧЕСКИХ СОБЫТИЙ"; }
    else if (choice == "4") { command = "GET_FULL_REPORT"; monitor_title = "ПОЛНЫЙ МОНИТОРИНГ"; }
    else if (choice == "0") { return; }
    else {
        cout << "\nНеверный выбор!" << endl;
        Sleep(1500);
        return;
    }

    bool monitoring = true;
    int update_count = 0;

    cout << "\n" << string(60, '=') << endl;
    cout << monitor_title << " (обновление каждые 5 секунд)" << endl;
    cout << string(60, '=') << endl;
    cout << "Для выхода нажмите 'q'" << endl;
    cout << string(60, '=') << "\n" << endl;

    while (monitoring) {
        if (_kbhit()) {
            char key = _getch();
            if (key == 'q' || key == 'Q') break;
        }

        update_count++;
        string response = client.send_monitoring_request(command);

        if (update_count > 1) cout << "\033[2J\033[1;1H";

        cout << string(60, '=') << endl;
        cout << monitor_title << " (обновление каждые 5 секунд)" << endl;
        cout << "Обновление #" << update_count << " | Нажмите 'q' для выхода" << endl;
        cout << string(60, '=') << "\n" << endl;
        cout << response << endl;
        cout << "\n" << string(60, '-') << endl;
        cout << "Следующее обновление через 5 секунд..." << endl;

        for (int i = 0; i < 5 && monitoring; i++) {
            if (_kbhit()) {
                char key = _getch();
                if (key == 'q' || key == 'Q') { monitoring = false; break; }
            }
            Sleep(1000);
        }
    }

    cout << "\nВыход из режима мониторинга..." << endl;
    Sleep(1000);
}

void ConnectionSetupMode::execute(RobotClient& client) {
    cout << "\nВведите новый IP: ";
    getline(cin, ip_);

    client.disconnect();
    if (!client.connect_to_server()) {
        cout << "\nНе удалось подключиться к " << ip_ << endl;
        cout << "Нажмите любую клавишу...";
        _getch();
    }
    cout << "\nПодключено к " << ip_ << endl;
}

// ========== РЕАЛИЗАЦИЯ ClientApplication ==========
ClientApplication::ClientApplication() : server_ip_("") {}

ClientApplication::~ClientApplication() {
    shutdown();
}

void ClientApplication::initialize_console() {
    set_console_encoding();
}

string ClientApplication::get_server_ip_from_user() {
    string ip;
    cout << "=== ОБЪЕДИНЁННЫЙ КЛИЕНТ УПРАВЛЕНИЯ РОБОТОМ ===" << endl;
    cout << "Введите IP Raspberry Pi: ";
    getline(cin, ip);
    return ip;
}

void ClientApplication::wait_for_exit() {
    cout << "\nНажмите любую клавишу для выхода...";
    _getch();
}

void ClientApplication::test_connection() {
    cout << "\nПроверка соединения..." << endl;
    string response = client_->send_request("GET_TEMP_STATS");
    if (response.find("ERROR") == string::npos) {
        cout << "Соединение работает" << endl;
    }
    else {
        cout << "Проблема с соединением" << endl;
    }
    Sleep(1500);
}

void ClientApplication::run() {
    initialize_console();

    server_ip_ = get_server_ip_from_user();
    client_ = make_unique<RobotClient>(server_ip_, 8888);

    if (!client_->connect_to_server()) {
        cout << "\nНе удалось подключиться к серверу!" << endl;
        wait_for_exit();
        return;
    }

    cout << "\nПодключение установлено!" << endl;
    test_connection();

    mode_manager_ = make_unique<ModeManager>(server_ip_);

    bool running = true;
    while (running) {
        mode_manager_->show_menu();
        string choice;
        getline(cin, choice);

        if (choice == "0") {
            running = false;
        }
        else if (!mode_manager_->execute_mode(choice, *client_)) {
            cout << "\nНеверный выбор! Нажмите любую клавишу...";
            _getch();
        }
    }

    shutdown();
}

void ClientApplication::shutdown() {
    if (client_) {
        client_->disconnect();
    }
    cout << "\nДо свидания!" << endl;
    Sleep(1000);
}

// ========== ГЛАВНАЯ ФУНКЦИЯ ==========
int main() {
    ClientApplication app;
    app.run();
    return 0;
}