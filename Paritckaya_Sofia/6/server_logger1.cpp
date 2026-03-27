#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <chrono>
#include <mutex>
#include <queue>
#include <condition_variable>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>

// Linux headers
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

class Logger {
private:
    mutex log_mutex; 
    ofstream log_file;
    vector<string> recent_logs; // хранит последние логи в памяти
    const size_t MAX_RECENT_LOGS = 100; // до 100 логов

    string get_current_time() { //форматирует текущее время в строку ГГГГ.ММ.ДД ЧЧ:ММ:СС
        auto now = chrono::system_clock::now();
        time_t now_time = chrono::system_clock::to_time_t(now);
        struct tm local_time;
        localtime_r(&now_time, &local_time);
        stringstream ss;
        ss << put_time(&local_time, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

public: //конструктор 
    Logger(const string& filename = "robot.log") {
        log_file.open(filename, ios::app);
        log("=== Robot server started ===");
    }

    ~Logger() { //деструктор
        log("=== Robot server stopped ===");
        if (log_file.is_open()) {
            log_file.close();
        }
    }

    // основной метод логгирования
    void log(const string& message, const string& level = "INFO") {
        lock_guard<mutex> lock(log_mutex);
        string formatted = "[" + get_current_time() + "] [" + level + "] " + message;
        cout << formatted << endl;
        if (log_file.is_open()) {
            log_file << formatted << endl;
            log_file.flush();
        }
        recent_logs.push_back(formatted);
        if (recent_logs.size() > MAX_RECENT_LOGS) {
            recent_logs.erase(recent_logs.begin());
        }
    }

    // специализированные методы логгирования 
    void log_error(const string& msg) { log(msg, "ERROR"); }
    void log_command(const string& client, const string& cmd) { log("Client " + client + " -> " + cmd, "CMD"); }
    void log_arduino(const string& data) { log("Arduino -> " + data, "ARDUINO"); }
    void log_usb(const string& data) { log("-> Arduino: " + data, "USB"); }

    // метод для получения последних логов
    string get_recent_logs(int count = 50) {
        lock_guard<mutex> lock(log_mutex);
        stringstream ss;
        int start = recent_logs.size() > count ? recent_logs.size() - count : 0;
        for (size_t i = start; i < recent_logs.size(); i++) {
            ss << recent_logs[i] << "\n";
        }
        return ss.str();
    }
};

// глобальные переменные
int serial_port = -1;
int server_socket = -1;
mutex ack_mutex;
condition_variable ack_cv;
queue<string> ack_queue;
bool running = true;
Logger logger;
thread arduino_thread;

// открываем и настраиваем порт
int open_serial(const char* port) {
    int fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) return -1;

    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) != 0) {
        close(fd);
        return -1;
    }

    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);

    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;
    tty.c_oflag = 0;
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 5;

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_cflag |= (CLOCAL | CREAD);
    tty.c_cflag &= ~(PARENB | PARODD);
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CRTSCTS;

    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        close(fd);
        return -1;
    }

    return fd;
}

// отправка команды в ардуино и ожидание подтверждения
bool send_command_with_ack(const string& cmd, int timeout_ms = 1000) {
    if (serial_port < 0) {
        logger.log_error("Port not open");
        return false;
    }

    string cmd_with_newline = cmd + "\n";
    write(serial_port, cmd_with_newline.c_str(), cmd_with_newline.length());
    logger.log_usb(cmd);

    auto start = chrono::steady_clock::now();

    while (chrono::duration_cast<chrono::milliseconds>(
        chrono::steady_clock::now() - start).count() < timeout_ms) {

        unique_lock<mutex> lock(ack_mutex);
        if (!ack_queue.empty()) {
            string ack = ack_queue.front();
            ack_queue.pop();

            if (ack.find("CMD:" + cmd + " OK") != string::npos) {
                return true;
            }
            else if (ack.find("CMD:") != string::npos) {
                ack_queue.push(ack);
            }
            else if (ack.find("ERROR") != string::npos) {
                logger.log_error("Arduino error: " + ack);
                return false;
            }
        }
        ack_cv.wait_for(lock, chrono::milliseconds(50));
    }

    logger.log_error("Timeout waiting for Arduino response to command " + cmd);
    return false;
}

// читаем данные с ардуино
void arduino_reader_thread() {
    char buffer[256];
    string partial;

    logger.log("Arduino reader thread started");

    while (running) {
        if (serial_port >= 0) {
            int n = read(serial_port, buffer, sizeof(buffer) - 1);
            if (n > 0) {
                buffer[n] = '\0';
                string data(buffer);

                for (char c : data) {
                    if (c == '\n') {
                        if (!partial.empty()) {
                            if (partial.back() == '\r') partial.pop_back();

                            logger.log_arduino(partial);

                            if (partial.find("CMD:") != string::npos ||
                                partial.find("SPEED:") != string::npos ||
                                partial.find("ERROR") != string::npos) {
                                    {
                                        lock_guard<mutex> lock(ack_mutex);
                                        ack_queue.push(partial);
                                    }
                                    ack_cv.notify_one();
                            }
                            partial.clear();
                        }
                    }
                    else {
                        partial += c;
                    }
                }
            }
        }
        this_thread::sleep_for(chrono::milliseconds(10));
    }

    logger.log("Arduino reader thread finished");
}

// получаем логи за указнный клиентом период
string get_logs_by_time(const string& filename, int amount, char unit) {
    ifstream file(filename);
    if (!file.is_open()) {
        return "ERROR: Cannot open log file";
    }

    int seconds = 0;
    string unit_text;
    switch (unit) {
    case 'm': seconds = amount * 60; unit_text = "min"; break;
    case 'h': seconds = amount * 60 * 60; unit_text = "hours"; break;
    case 'd': seconds = amount * 24 * 60 * 60; unit_text = "days"; break;
    default:
        return "ERROR: Invalid unit";
    }

    auto threshold = chrono::system_clock::now() - chrono::seconds(seconds);
    time_t threshold_time = chrono::system_clock::to_time_t(threshold);

    string line;
    int count = 0;
    stringstream result;

    while (getline(file, line)) {
        if (line.length() < 20 || line[0] != '[') continue;
        try {
            string dt = line.substr(1, 19);
            struct tm tm = {};
            stringstream(dt) >> get_time(&tm, "%Y-%m-%d %H:%M:%S");
            time_t line_time = mktime(&tm);
            if (difftime(line_time, threshold_time) >= 0) {
                result << line << '\n';
                count++;
            }
        }
        catch (...) {}
    }

    file.close();

    return "=== Log for last " + to_string(amount) + " " + unit_text +
        " (" + to_string(count) + " records) ===\n\n" + result.str();
}

// получение айпи клиента
string get_client_ip(struct sockaddr_in client_addr) {
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    return string(client_ip);
}

// обработка подключения
void handle_client(int client_socket, struct sockaddr_in client_addr) {
    string client_ip = get_client_ip(client_addr);
    logger.log("Client connected: " + client_ip, "CONNECT");

    char buffer[1024];

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes <= 0) {
            logger.log("Client disconnected: " + client_ip, "DISCONNECT");
            break;
        }

        string command(buffer);
        command.erase(command.find_last_not_of(" \n\r\t") + 1);
        logger.log_command(client_ip, command);

        string response;
        // обработка команд 
        if (command == "w" || command == "a" || command == "s" || command == "d" || command == "x") {
            response = send_command_with_ack(command) ? "OK" : "ERROR: Arduino not responding";
        }
        else if (command.substr(0, 5) == "speed") {
            response = send_command_with_ack(command) ? "OK" : "ERROR";
        }
        else if (command == "GET_LOGS") {
            response = logger.get_recent_logs(50);
        }
        else if (command.rfind("GET_LOGS_BY_TIME", 0) == 0) {
            string params = command.substr(16);

            size_t first = params.find_first_not_of(" \t");
            if (first == string::npos) {
                response = "ERROR: Empty parameter";
            }
            else {
                params = params.substr(first);

                size_t space_pos = params.find(' ');
                string amount_str, unit_str;

                if (space_pos != string::npos) {
                    amount_str = params.substr(0, space_pos);
                    unit_str = params.substr(space_pos + 1);
                }
                else {
                    size_t i = 0;
                    while (i < params.length() && isdigit(params[i])) i++;
                    if (i > 0 && i < params.length()) {
                        amount_str = params.substr(0, i);
                        unit_str = params.substr(i);
                    }
                    else {
                        amount_str = params;
                        unit_str = "";
                    }
                }

                amount_str.erase(0, amount_str.find_first_not_of(" \t"));
                amount_str.erase(amount_str.find_last_not_of(" \t") + 1);
                unit_str.erase(0, unit_str.find_first_not_of(" \t"));
                unit_str.erase(unit_str.find_last_not_of(" \t") + 1);

                int amount = 0;
                char unit = 0;

                try {
                    if (!amount_str.empty()) {
                        amount = stoi(amount_str);
                    }
                }
                catch (...) {
                    amount = 0;
                }

                if (!unit_str.empty()) {
                    unit = tolower(unit_str[0]);
                }

                if (amount > 0 && (unit == 'm' || unit == 'h' || unit == 'd')) {
                    logger.log("History request for " + to_string(amount) + unit + " from " + client_ip, "HISTORY");
                    response = get_logs_by_time("robot.log", amount, unit);
                }
                else {
                    response = "ERROR: Invalid time parameter. Use format: number + m/h/d (e.g., 10m, 2h, 3d)";
                    logger.log("Invalid time parameter from " + client_ip + ": " + params, "ERROR");
                }
            }
        }
        else {
            response = "ERROR: Unknown command";
        }

        send(client_socket, response.c_str(), response.length(), 0);
    }

    close(client_socket);
}

// инициализация подключения к ардуино
bool init_arduino(int argc, char* argv[]) {
    cout << "=== Robot Server ===" << endl;

    const char* port = "/dev/ttyACM0";
    if (argc > 1) {
        port = argv[1];
    }

    cout << "Connecting to " << port << "..." << endl;
    serial_port = open_serial(port);

    if (serial_port < 0) {
        cerr << "Error opening port " << port << endl;
        return false;
    }

    cout << "Port opened successfully" << endl;
    arduino_thread = thread(arduino_reader_thread);
    this_thread::sleep_for(chrono::seconds(2));

    return true;
}

// инициализация сервера
bool init_server() {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        cerr << "Error creating socket" << endl;
        return false;
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8888);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Bind error. Port 8888 may be in use." << endl;
        close(server_socket);
        return false;
    }

    if (listen(server_socket, 5) < 0) {
        cerr << "Listen error" << endl;
        close(server_socket);
        return false;
    }

    cout << "Server started on port 8888" << endl;
    cout << "IP addresses:" << endl;
    system("hostname -I | awk '{print \"  - \" $1}'");
    cout << "Waiting for clients..." << endl;

    return true;
}

// запуск сервера
void run_server() {
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            cerr << "Accept error" << endl;
            continue;
        }

        thread client_thread(handle_client, client_socket, client_addr);
        client_thread.detach();
    }
}

// очистка
void cleanup() {
    running = false;

    if (arduino_thread.joinable()) {
        arduino_thread.join();
    }

    if (serial_port >= 0) {
        close(serial_port);
    }

    if (server_socket >= 0) {
        close(server_socket);
    }
}

int main(int argc, char* argv[]) {
    if (!init_arduino(argc, argv)) { // инициализация ардуино
        cleanup();
        return 1;
    }

    if (!init_server()) { // инициализация сервера
        cleanup();
        return 1;
    }
    run_server(); // запуск основного цикла сервера

    cleanup(); //очистка
    return 0;
}
