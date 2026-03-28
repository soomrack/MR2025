#include <iostream>
#include <string>
#include <cstring>
#include <thread>
#include <chrono>
#include <mutex>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <vector>
#include <queue>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

// Класс для логирования температуры
class TemperatureLogger {
private:
    mutex log_mutex;
    ofstream log_file;
    vector<string> recent_logs;
    const size_t MAX_RECENT_LOGS = 1000;
    bool running;
    thread monitor_thread;

    string get_current_time();
    float read_temperature();
    int read_cpu_freq();
    float read_cpu_load();
    void monitor_loop();

public:
    TemperatureLogger(const string& filename = "temperature.log");
    ~TemperatureLogger();

    void start();
    void stop();
    void log(const string& message, const string& level = "INFO");
    void log_warning(const string& msg);
    string get_temperature_stats(int amount, char unit);
    string get_recent_logs(int count = 100);
    string get_current_temperature();
};

// Реализация методов TemperatureLogger
string TemperatureLogger::get_current_time() {
    auto now = chrono::system_clock::now();
    time_t now_time = chrono::system_clock::to_time_t(now);
    struct tm local_time;
    localtime_r(&now_time, &local_time);
    stringstream ss;
    ss << put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

float TemperatureLogger::read_temperature() {
    FILE* fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!fp) return -1.0f;

    int temp_raw;
    fscanf(fp, "%d", &temp_raw);
    fclose(fp);

    return temp_raw / 1000.0f;
}

int TemperatureLogger::read_cpu_freq() {
    FILE* fp = fopen("/sys/devices/system/cpu/cpu0/cpufreq/scaling_cur_freq", "r");
    if (!fp) return -1;

    int freq;
    fscanf(fp, "%d", &freq);
    fclose(fp);

    return freq / 1000;
}

float TemperatureLogger::read_cpu_load() {
    FILE* fp = fopen("/proc/loadavg", "r");
    if (!fp) return -1.0f;

    float load1, load5, load15;
    fscanf(fp, "%f %f %f", &load1, &load5, &load15);
    fclose(fp);

    return load1;
}

void TemperatureLogger::monitor_loop() {
    while (running) {
        float temp = read_temperature();
        int freq = read_cpu_freq();
        float load = read_cpu_load();

        if (temp >= 0) {
            stringstream ss;
            ss << fixed << setprecision(2);
            ss << "TEMP:" << temp << " FREQ:" << freq << "MHz LOAD:" << load;
            log(ss.str(), "METRIC");

            if (temp > 80.0f) {
                log_warning("HIGH TEMPERATURE: " + to_string(temp) + "°C");
            }
        }

        this_thread::sleep_for(chrono::seconds(10));
    }
}

TemperatureLogger::TemperatureLogger(const string& filename) : running(false) {
    log_file.open(filename, ios::app);
    log("=== Temperature monitor started ===");
}

TemperatureLogger::~TemperatureLogger() {
    stop();
    log("=== Temperature monitor stopped ===");
    if (log_file.is_open()) {
        log_file.close();
    }
}

void TemperatureLogger::start() {
    if (!running) {
        running = true;
        monitor_thread = thread(&TemperatureLogger::monitor_loop, this);
        log("Monitoring thread started");
    }
}

void TemperatureLogger::stop() {
    if (running) {
        running = false;
        if (monitor_thread.joinable()) {
            monitor_thread.join();
        }
        log("Monitoring thread stopped");
    }
}

void TemperatureLogger::log(const string& message, const string& level) {
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

void TemperatureLogger::log_warning(const string& msg) {
    log(msg, "WARNING");
}

string TemperatureLogger::get_temperature_stats(int amount, char unit) {
    int seconds = 0;
    switch (unit) {
    case 'm': seconds = amount * 60; break;
    case 'h': seconds = amount * 60 * 60; break;
    case 'd': seconds = amount * 24 * 60 * 60; break;
    default: return "ERROR: Invalid unit";
    }

    auto threshold = chrono::system_clock::now() - chrono::seconds(seconds);
    time_t threshold_time = chrono::system_clock::to_time_t(threshold);

    ifstream file("temperature.log");
    if (!file.is_open()) {
        return "ERROR: Cannot open log file";
    }

    string line;
    vector<float> temperatures;
    float max_temp = -1000, min_temp = 1000, avg_temp = 0;
    int warning_count = 0;
    stringstream result;

    while (getline(file, line)) {
        if (line.find("[METRIC] TEMP:") == string::npos) continue;

        if (line.length() < 20) continue;
        string dt = line.substr(1, 19);
        struct tm tm = {};
        stringstream(dt) >> get_time(&tm, "%Y-%m-%d %H:%M:%S");
        time_t line_time = mktime(&tm);

        if (difftime(line_time, threshold_time) >= 0) {
            size_t temp_pos = line.find("TEMP:");
            if (temp_pos != string::npos) {
                float temp = stof(line.substr(temp_pos + 5));
                temperatures.push_back(temp);

                if (temp > max_temp) max_temp = temp;
                if (temp < min_temp) min_temp = temp;
                avg_temp += temp;

                if (temp > 80.0f) warning_count++;
            }
        }
    }

    file.close();

    if (temperatures.empty()) {
        return "No temperature data for this period";
    }

    avg_temp /= temperatures.size();

    string unit_text;
    switch (unit) {
    case 'm': unit_text = "minutes"; break;
    case 'h': unit_text = "hours"; break;
    case 'd': unit_text = "days"; break;
    }

    result << "=== Temperature Statistics for last " << amount << " " << unit_text << " ===\n";
    result << "Period: " << temperatures.size() << " measurements\n";
    result << "Average temperature: " << fixed << setprecision(2) << avg_temp << "°C\n";
    result << "Maximum temperature: " << max_temp << "°C\n";
    result << "Minimum temperature: " << min_temp << "°C\n";
    result << "Overheat warnings (>80°C): " << warning_count << "\n";
    result << "\nDetailed logs:\n\n";

    file.open("temperature.log");
    vector<string> last_logs;
    while (getline(file, line)) {
        if (line.find("[METRIC] TEMP:") != string::npos) {
            last_logs.push_back(line);
        }
    }
    file.close();

    int start = last_logs.size() > 50 ? last_logs.size() - 50 : 0;
    for (size_t i = start; i < last_logs.size(); i++) {
        result << last_logs[i] << "\n";
    }

    return result.str();
}

string TemperatureLogger::get_recent_logs(int count) {
    lock_guard<mutex> lock(log_mutex);
    stringstream ss;
    int start = recent_logs.size() > count ? recent_logs.size() - count : 0;
    for (size_t i = start; i < recent_logs.size(); i++) {
        ss << recent_logs[i] << "\n";
    }
    return ss.str();
}

string TemperatureLogger::get_current_temperature() {
    float temp = read_temperature();
    int freq = read_cpu_freq();
    float load = read_cpu_load();

    if (temp < 0) return "ERROR: Cannot read temperature";

    stringstream ss;
    ss << fixed << setprecision(2);
    ss << "=== Current System Status ===\n";
    ss << "Temperature: " << temp << "°C\n";
    ss << "CPU Frequency: " << freq << " MHz\n";
    ss << "CPU Load (1min): " << load << "\n";

    if (temp > 80.0f) {
        ss << "WARNING: High temperature!\n";
    }

    return ss.str();
}

// Класс для обработки клиентских подключений
class ClientHandler {
private:
    int client_socket;
    string client_ip;
    TemperatureLogger& logger;

    string process_command(const string& cmd);

public:
    ClientHandler(int socket, struct sockaddr_in addr, TemperatureLogger& log_ref);
    void handle();
};

ClientHandler::ClientHandler(int socket, struct sockaddr_in addr, TemperatureLogger& log_ref)
    : client_socket(socket), logger(log_ref) {
    char ip_str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &addr.sin_addr, ip_str, INET_ADDRSTRLEN);
    client_ip = string(ip_str);
}

string ClientHandler::process_command(const string& cmd) {
    if (cmd == "GET_CURRENT") {
        return logger.get_current_temperature();
    }
    else if (cmd == "GET_RECENT") {
        return logger.get_recent_logs(50);
    }
    else if (cmd.rfind("GET_STATS", 0) == 0) {
        string params = cmd.substr(9);
        params.erase(0, params.find_first_not_of(" \t"));

        if (params.empty()) {
            return "ERROR: No parameters. Use: GET_STATS <number><m/h/d>";
        }

        size_t i = 0;
        while (i < params.length() && isdigit(params[i])) i++;

        if (i > 0 && i < params.length()) {
            int amount = stoi(params.substr(0, i));
            char unit = tolower(params[i]);

            if (unit == 'm' || unit == 'h' || unit == 'd') {
                return logger.get_temperature_stats(amount, unit);
            }
            return "ERROR: Invalid unit. Use m (minutes), h (hours), or d (days)";
        }
        return "ERROR: Invalid format. Use: GET_STATS 10m, GET_STATS 2h, GET_STATS 1d";
    }
    else if (cmd == "HELP") {
        return "Available commands:\n"
            "  GET_CURRENT - Get current temperature\n"
            "  GET_RECENT - Get recent temperature logs\n"
            "  GET_STATS <n><m/h/d> - Get statistics for last n minutes/hours/days\n"
            "  HELP - Show this help\n";
    }

    return "ERROR: Unknown command. Type HELP for available commands";
}

void ClientHandler::handle() {
    logger.log("Client connected: " + client_ip, "CONNECT");

    char buffer[4096];

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        if (bytes <= 0) {
            logger.log("Client disconnected: " + client_ip, "DISCONNECT");
            break;
        }

        string command(buffer);
        command.erase(command.find_last_not_of(" \n\r\t") + 1);
        logger.log("Command from " + client_ip + ": " + command, "CMD");

        string response = process_command(command);
        send(client_socket, response.c_str(), response.length(), 0);
    }

    close(client_socket);
}

// Класс сервера
class TemperatureServer {
private:
    int server_socket;
    bool running;
    TemperatureLogger logger;

    bool create_socket();
    bool bind_socket();
    bool start_listening();
    void accept_clients();

public:
    TemperatureServer();
    ~TemperatureServer();

    bool initialize();
    void run();
    void shutdown();
};

TemperatureServer::TemperatureServer() : server_socket(-1), running(false) {}

TemperatureServer::~TemperatureServer() {
    shutdown();
}

bool TemperatureServer::create_socket() {
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        cerr << "Error creating socket" << endl;
        return false;
    }

    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    return true;
}

bool TemperatureServer::bind_socket() {
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8889);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Bind error. Port 8889 may be in use." << endl;
        return false;
    }
    return true;
}

bool TemperatureServer::start_listening() {
    if (listen(server_socket, 5) < 0) {
        cerr << "Listen error" << endl;
        return false;
    }
    return true;
}

void TemperatureServer::accept_clients() {
    while (running) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) {
            if (running) {
                cerr << "Accept error" << endl;
            }
            continue;
        }

        ClientHandler* handler = new ClientHandler(client_socket, client_addr, logger);
        thread client_thread([handler]() {
            handler->handle();
            delete handler;
            });
        client_thread.detach();
    }
}

bool TemperatureServer::initialize() {
    cout << "=== Raspberry Pi Temperature Monitor Server ===" << endl;

    logger.start();

    if (!create_socket()) return false;
    if (!bind_socket()) return false;
    if (!start_listening()) return false;

    cout << "Temperature monitor server started on port 8889" << endl;
    cout << "IP addresses:" << endl;
    system("hostname -I | awk '{print \"  - \" $1}'");
    cout << "Waiting for clients..." << endl;

    return true;
}

void TemperatureServer::run() {
    running = true;
    accept_clients();
}

void TemperatureServer::shutdown() {
    running = false;
    logger.stop();
    if (server_socket >= 0) {
        close(server_socket);
    }
}

// Чистый main()
int main() {
    TemperatureServer server;

    if (!server.initialize()) {
        return 1;
    }

    server.run();

    return 0;
}