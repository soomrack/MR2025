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
#include <vector>
#include <algorithm>
#include <functional>
#include <memory>
#include <unordered_map>

#include <sys/select.h>
#include <sys/statvfs.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using namespace std;

// ========== ¬—œŒÃŒ√¿“≈À‹Õ€≈ ‘”Õ ÷»» (Œ¡⁄ﬂ¬À≈Õ»ﬂ) ==========
string get_current_time();
void print_server_banner();

// ========== »Õ“≈–‘≈…—  ŒÃ¿Õƒ€ ==========
class ICommandHandler {
public:
    virtual ~ICommandHandler() = default;
    virtual string execute(const string& command, class SystemMonitor& monitor,
        class ArduinoComm& arduino, class Logger& logger) = 0;
    virtual string get_command_name() const = 0;
};

// ==========  À¿—— ÀŒ√√≈– ==========
class Logger {
private:
    mutex log_mutex_;
    vector<string> log_history_;
    ofstream log_file_;
    string get_current_time();

public:
    Logger();
    ~Logger();
    void log(const string& message, const string& type = "INFO");
    void log_command(const string& client_ip, const string& command);
    string get_recent_logs(int count);
};

// ==========  À¿—— ARDUINO_COMM ==========
class ArduinoComm {
private:
    int serial_port_;
    mutex serial_mutex_;
    bool running_;
    class Logger& logger_;
    string read_buffer_;

    int open_serial(const char* port);

public:
    ArduinoComm(Logger& log_ref);
    ~ArduinoComm();
    bool connect(const char* port);
    void disconnect();
    bool send_command(const string& cmd);
    void start_reader_thread(function<void(const string&)> data_callback);
    void stop();
    bool is_connected() const;
};

// ==========  À¿—— SYSTEM_MONITOR ==========
class SystemMonitor {
private:
    mutex monitor_mutex_;
    string temp_log_file_;
    string critical_log_file_;
    string cpu_load_log_file_;

    struct TempRecord {
        string timestamp;
        float temperature;
    };

    struct CpuLoadRecord {
        string timestamp;
        float load_percent;
    };

    vector<TempRecord> temp_history_;
    vector<CpuLoadRecord> cpu_load_history_;

    float min_temp_;
    float max_temp_;
    float max_cpu_load_;
    float critical_temp_threshold_;
    float critical_cpu_threshold_;

    class Logger& logger_;
    bool running_;

    string get_current_time();
    float read_cpu_temperature();
    float read_cpu_load();
    void check_critical_conditions();
    void log_critical_event(const string& message);

public:
    SystemMonitor(Logger& log_ref);
    ~SystemMonitor();
    void log_temperature();
    void log_cpu_load();
    void process_arduino_data(const string& data);
    string get_temperature_stats();
    string get_cpu_load_stats();
    string get_critical_events(int last_n = 10);
    string get_full_report();
    void start_monitoring();
    void stop();
};

// ==========  ŒÃ¿Õƒ€-Œ¡–¿¡Œ“◊» » ==========
class MoveCommand : public ICommandHandler {
public:
    string execute(const string& command, SystemMonitor& monitor,
        ArduinoComm& arduino, Logger& logger) override;
    string get_command_name() const override { return "move"; }
};

class SpeedCommand : public ICommandHandler {
public:
    string execute(const string& command, SystemMonitor& monitor,
        ArduinoComm& arduino, Logger& logger) override;
    string get_command_name() const override { return "speed"; }
};

class GetLogsCommand : public ICommandHandler {
public:
    string execute(const string& command, SystemMonitor& monitor,
        ArduinoComm& arduino, Logger& logger) override;
    string get_command_name() const override { return "GET_LOGS"; }
};

class GetTempStatsCommand : public ICommandHandler {
public:
    string execute(const string& command, SystemMonitor& monitor,
        ArduinoComm& arduino, Logger& logger) override;
    string get_command_name() const override { return "GET_TEMP_STATS"; }
};

class GetCpuLoadCommand : public ICommandHandler {
public:
    string execute(const string& command, SystemMonitor& monitor,
        ArduinoComm& arduino, Logger& logger) override;
    string get_command_name() const override { return "GET_CPU_LOAD"; }
};

class GetCriticalEventsCommand : public ICommandHandler {
public:
    string execute(const string& command, SystemMonitor& monitor,
        ArduinoComm& arduino, Logger& logger) override;
    string get_command_name() const override { return "GET_CRITICAL_EVENTS"; }
};

class GetFullReportCommand : public ICommandHandler {
public:
    string execute(const string& command, SystemMonitor& monitor,
        ArduinoComm& arduino, Logger& logger) override;
    string get_command_name() const override { return "GET_FULL_REPORT"; }
};

// ========== Ã≈Õ≈ƒ∆≈–  ŒÃ¿Õƒ ==========
class CommandManager {
private:
    unordered_map<string, unique_ptr<ICommandHandler>> handlers_;

public:
    CommandManager() {
        register_handler(make_unique<MoveCommand>());
        register_handler(make_unique<SpeedCommand>());
        register_handler(make_unique<GetLogsCommand>());
        register_handler(make_unique<GetTempStatsCommand>());
        register_handler(make_unique<GetCpuLoadCommand>());
        register_handler(make_unique<GetCriticalEventsCommand>());
        register_handler(make_unique<GetFullReportCommand>());
    }

    void register_handler(unique_ptr<ICommandHandler> handler) {
        handlers_[handler->get_command_name()] = move(handler);
    }

    string process(const string& command, SystemMonitor& monitor,
        ArduinoComm& arduino, Logger& logger);
    void print_available_commands() const;
};

// ==========  À¿—— CLIENT_HANDLER ==========
class ClientHandler {
private:
    int client_socket_;
    string client_ip_;
    class Logger& logger_;
    class SystemMonitor& monitor_;
    class ArduinoComm& arduino_;
    class CommandManager& cmd_manager_;
    bool monitoring_mode_;

    void send_full_response(const string& response);

public:
    ClientHandler(int socket, sockaddr_in addr, Logger& log_ref,
        SystemMonitor& mon_ref, ArduinoComm& ard_ref, CommandManager& cmd_ref);
    void handle();
};

// ==========  À¿—— TCP_SERVER ==========
class TcpServer {
private:
    int server_socket_;
    int port_;
    bool running_;
    class Logger& logger_;
    class SystemMonitor& monitor_;
    class ArduinoComm& arduino_;
    class CommandManager& cmd_manager_;

public:
    TcpServer(int p, Logger& log_ref, SystemMonitor& mon_ref,
        ArduinoComm& ard_ref, CommandManager& cmd_ref);
    ~TcpServer();
    bool start();
    void run();
    void stop();
};

// ========== √À¿¬ÕŒ≈ œ–»ÀŒ∆≈Õ»≈ —≈–¬≈–¿ ==========
class RobotServerApp {
private:
    Logger logger_;
    SystemMonitor monitor_;
    ArduinoComm arduino_;
    CommandManager cmd_manager_;
    TcpServer server_;

public:
    RobotServerApp(int tcp_port = 8888);
    bool initialize(int argc, char* argv[]);
    void run();
    void shutdown();
};

// ========== –≈¿À»«¿÷»ﬂ ¬—œŒÃŒ√¿“≈À‹Õ€’ ‘”Õ ÷»… ==========
string get_current_time() {
    auto now = chrono::system_clock::now();
    time_t now_time = chrono::system_clock::to_time_t(now);
    struct tm local_time;
    localtime_r(&now_time, &local_time);

    stringstream ss;
    ss << put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

void print_server_banner() {
    cout << "\n" << string(60, '=') << endl;
    cout << "–Œ¡Œ“-—≈–¬≈– — ÃŒÕ»“Œ–»Õ√ŒÃ CPU" << endl;
    cout << string(60, '=') << endl;
}

// ========== –≈¿À»«¿÷»ﬂ LOGGER ==========
string Logger::get_current_time() {
    auto now = chrono::system_clock::now();
    time_t now_time = chrono::system_clock::to_time_t(now);
    struct tm local_time;
    localtime_r(&now_time, &local_time);

    stringstream ss;
    ss << put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

Logger::Logger() {
    log_file_.open("server.log", ios::app);
}

Logger::~Logger() {
    if (log_file_.is_open()) log_file_.close();
}

void Logger::log(const string& message, const string& type) {
    lock_guard<mutex> lock(log_mutex_);
    string timestamp = get_current_time();
    string entry = "[" + timestamp + "] [" + type + "] " + message;

    cout << entry << endl;
    log_history_.push_back(entry);
    if (log_history_.size() > 1000) log_history_.erase(log_history_.begin());

    if (log_file_.is_open()) {
        log_file_ << entry << endl;
        log_file_.flush();
    }
}

void Logger::log_command(const string& client_ip, const string& command) {
    log("CMD from " + client_ip + ": " + command, "COMMAND");
}

string Logger::get_recent_logs(int count) {
    lock_guard<mutex> lock(log_mutex_);
    stringstream ss;
    ss << "=== œŒ—À≈ƒÕ»≈ ÀŒ√» (ÔÓÒÎÂ‰ÌËÂ " << count << ") ===\n";
    ss << string(60, '=') << "\n";

    int start = log_history_.size() > count ? log_history_.size() - count : 0;
    for (size_t i = start; i < log_history_.size(); i++) {
        ss << log_history_[i] << "\n";
    }
    return ss.str();
}

// ========== –≈¿À»«¿÷»ﬂ ARDUINO_COMM ==========
ArduinoComm::ArduinoComm(Logger& log_ref) : serial_port_(-1), running_(true), logger_(log_ref) {}

ArduinoComm::~ArduinoComm() { disconnect(); }

int ArduinoComm::open_serial(const char* port) {
    int fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
    if (fd < 0) return fd;

    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    if (tcgetattr(fd, &tty) != 0) { close(fd); return -1; }

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

    if (tcsetattr(fd, TCSANOW, &tty) != 0) { close(fd); return -1; }
    return fd;
}

bool ArduinoComm::connect(const char* port) {
    serial_port_ = open_serial(port);
    return serial_port_ >= 0;
}

void ArduinoComm::disconnect() {
    if (serial_port_ >= 0) {
        close(serial_port_);
        serial_port_ = -1;
    }
}

bool ArduinoComm::send_command(const string& cmd) {
    if (serial_port_ < 0) return false;
    lock_guard<mutex> lock(serial_mutex_);
    string to_send = cmd + "\n";
    write(serial_port_, to_send.c_str(), to_send.length());
    return true;
}

void ArduinoComm::start_reader_thread(function<void(const string&)> data_callback) {
    thread([this, data_callback]() {
        char buffer[256];
        while (running_) {
            int bytes = read(serial_port_, buffer, sizeof(buffer) - 1);
            if (bytes > 0) {
                buffer[bytes] = '\0';
                read_buffer_ += buffer;

                size_t pos;
                while ((pos = read_buffer_.find('\n')) != string::npos) {
                    string line = read_buffer_.substr(0, pos);
                    line.erase(line.find_last_not_of(" \r\n\t") + 1);
                    if (!line.empty()) data_callback(line);
                    read_buffer_.erase(0, pos + 1);
                }
            }
            this_thread::sleep_for(chrono::milliseconds(10));
        }
        }).detach();
}

void ArduinoComm::stop() { running_ = false; }
bool ArduinoComm::is_connected() const { return serial_port_ >= 0; }

// ========== –≈¿À»«¿÷»ﬂ SYSTEM_MONITOR ==========
SystemMonitor::SystemMonitor(Logger& log_ref)
    : logger_(log_ref), running_(true), min_temp_(999.0f), max_temp_(-999.0f),
    max_cpu_load_(0.0f), critical_temp_threshold_(55.0f), critical_cpu_threshold_(80.0f) {

    temp_log_file_ = "/home/vlad/rpi_server/temperature_history.csv";
    critical_log_file_ = "/home/vlad/rpi_server/critical_events.log";
    cpu_load_log_file_ = "/home/vlad/rpi_server/cpu_load_history.csv";

    system("mkdir -p /home/vlad/rpi_server");

    ifstream temp_check(temp_log_file_);
    if (!temp_check.good()) {
        ofstream temp_file(temp_log_file_);
        temp_file << "Timestamp,Temperature_C\n";
    }

    ifstream load_check(cpu_load_log_file_);
    if (!load_check.good()) {
        ofstream load_file(cpu_load_log_file_);
        load_file << "Timestamp,CPU_Load_Percent\n";
    }
}

SystemMonitor::~SystemMonitor() { stop(); }

string SystemMonitor::get_current_time() {
    auto now = chrono::system_clock::now();
    time_t now_time = chrono::system_clock::to_time_t(now);
    struct tm local_time;
    localtime_r(&now_time, &local_time);

    stringstream ss;
    ss << put_time(&local_time, "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

float SystemMonitor::read_cpu_temperature() {
    float temp = 0.0f;
    FILE* fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (fp) {
        int millideg;
        if (fscanf(fp, "%d", &millideg) == 1) temp = millideg / 1000.0f;
        fclose(fp);
    }
    return temp;
}

float SystemMonitor::read_cpu_load() {
    float load = 0.0f;
    FILE* fp = fopen("/proc/loadavg", "r");
    if (fp) {
        double load1, load5, load15;
        int processes;
        if (fscanf(fp, "%lf %lf %lf %d/%*d %*d", &load1, &load5, &load15, &processes) == 4) {
            load = load1 * 100.0 / sysconf(_SC_NPROCESSORS_ONLN);
        }
        fclose(fp);
    }
    return min(load, 100.0f);
}

void SystemMonitor::check_critical_conditions() {
    float current_temp = read_cpu_temperature();
    float current_load = read_cpu_load();

    if (current_temp > critical_temp_threshold_) {
        string message = "ÚÂÏÔÂ‡ÚÛ‡ CPU " + to_string(current_temp) +
            "C ÔÂ‚˚¯‡ÂÚ ÔÓÓ„ " + to_string(critical_temp_threshold_) + "C";
        log_critical_event(message);
        logger_.log(message, "CRITICAL");
    }

    if (current_load > critical_cpu_threshold_) {
        string message = "Á‡„ÛÁÍ‡ CPU " + to_string(current_load) +
            "% ÔÂ‚˚¯‡ÂÚ ÔÓÓ„ " + to_string(critical_cpu_threshold_) + "%";
        log_critical_event(message);
        logger_.log(message, "CRITICAL");
    }
}

void SystemMonitor::log_critical_event(const string& message) {
    lock_guard<mutex> lock(monitor_mutex_);
    ofstream file(critical_log_file_, ios::app);
    file << get_current_time() << " - " << message << "\n";
}

void SystemMonitor::log_temperature() {
    lock_guard<mutex> lock(monitor_mutex_);
    float current_temp = read_cpu_temperature();

    TempRecord record{ get_current_time(), current_temp };
    temp_history_.push_back(record);

    if (current_temp < min_temp_) min_temp_ = current_temp;
    if (current_temp > max_temp_) max_temp_ = current_temp;

    static int temp_counter = 0;
    if (++temp_counter % 6 == 0) {
        ofstream file(temp_log_file_, ios::app);
        file << record.timestamp << "," << fixed << setprecision(2) << current_temp << "\n";
    }
}

void SystemMonitor::log_cpu_load() {
    lock_guard<mutex> lock(monitor_mutex_);
    float current_load = read_cpu_load();

    CpuLoadRecord record{ get_current_time(), current_load };
    cpu_load_history_.push_back(record);

    if (current_load > max_cpu_load_) max_cpu_load_ = current_load;

    static int load_counter = 0;
    if (++load_counter % 6 == 0) {
        ofstream file(cpu_load_log_file_, ios::app);
        file << record.timestamp << "," << fixed << setprecision(2) << current_load << "\n";
    }
}

void SystemMonitor::process_arduino_data(const string& data) {
    if (data.find("TEMP_STATS") != string::npos) {
        size_t comma = data.find(",");
        if (comma != string::npos) {
            try {
                float minT = stof(data.substr(11, comma - 11));
                float maxT = stof(data.substr(comma + 1));
                logger_.log("“ÂÏÔÂ‡ÚÛ‡ ÓÚ Arduino: ÏËÌ=" + to_string(minT) +
                    " Ï‡ÍÒ=" + to_string(maxT), "TEMP");
            }
            catch (...) {
                logger_.log("Œ¯Ë·Í‡ Ô‡ÒËÌ„‡ TEMP_STATS: " + data, "ERROR");
            }
        }
    }
    else if (data.find("CMD:") != string::npos || data.find("SPEED:") != string::npos) {
        logger_.log("Arduino: " + data, "SERIAL");
    }
    else if (!data.empty() && data != "OK" && data.length() > 1) {
        logger_.log("Arduino: " + data, "SERIAL");
    }
}

string SystemMonitor::get_temperature_stats() {
    lock_guard<mutex> lock(monitor_mutex_);
    stringstream ss;
    float current = read_cpu_temperature();

    ss << "\n" << string(80, '=') << "\n";
    ss << "“≈Ãœ≈–¿“”–¿ œ–Œ÷≈——Œ–¿\n";
    ss << string(80, '=') << "\n";
    ss << "“ÂÍÛ˘‡ˇ:     " << fixed << setprecision(2) << current << " ∞C\n";
    ss << "ÃËÌËÏ‡Î¸Ì‡ˇ: " << fixed << setprecision(2) << min_temp_ << " ∞C\n";
    ss << "Ã‡ÍÒËÏ‡Î¸Ì‡ˇ: " << fixed << setprecision(2) << max_temp_ << " ∞C\n";
    return ss.str();
}

string SystemMonitor::get_cpu_load_stats() {
    lock_guard<mutex> lock(monitor_mutex_);
    stringstream ss;
    float current = read_cpu_load();

    ss << "\n" << string(80, '=') << "\n";
    ss << "«¿√–”« ¿ œ–Œ÷≈——Œ–¿\n";
    ss << string(80, '=') << "\n";
    ss << "“ÂÍÛ˘‡ˇ:     " << fixed << setprecision(2) << current << " %\n";
    ss << "Ã‡ÍÒËÏ‡Î¸Ì‡ˇ: " << fixed << setprecision(2) << max_cpu_load_ << " %\n";
    ss << " ÓÎË˜ÂÒÚ‚Ó ˇ‰Â: " << sysconf(_SC_NPROCESSORS_ONLN) << "\n";
    return ss.str();
}

string SystemMonitor::get_critical_events(int last_n) {
    lock_guard<mutex> lock(monitor_mutex_);
    stringstream ss;

    ss << "\n" << string(80, '=') << "\n";
    ss << " –»“»◊≈— »≈ —Œ¡€“»ﬂ (ÔÓÒÎÂ‰ÌËÂ " << last_n << ")\n";
    ss << string(80, '=') << "\n";

    ifstream crit_file(critical_log_file_);
    if (crit_file.is_open()) {
        vector<string> all_events;
        string line;
        while (getline(crit_file, line)) {
            if (!line.empty() && line.back() == '\r') line.pop_back();
            if (line.empty()) continue;
            all_events.push_back(line);
            if (all_events.size() > 1000) all_events.erase(all_events.begin());
        }

        if (all_events.empty()) {
            ss << "   ËÚË˜ÂÒÍËı ÒÓ·˚ÚËÈ ÌÂ ÒÎÛ˜ËÎÓÒ¸\n";
        }
        else {
            ss << "  Œ·Ì‡ÛÊÂÌ˚ ÒÎÂ‰Û˛˘ËÂ ÍËÚË˜ÂÒÍËÂ ÒÓ·˚ÚËˇ:\n\n";
            int start = all_events.size() > last_n ? all_events.size() - last_n : 0;
            for (size_t i = start; i < all_events.size(); i++) {
                ss << "  * " << all_events[i] << "\n";
            }
        }
    }
    else {
        ss << "   ËÚË˜ÂÒÍËı ÒÓ·˚ÚËÈ ÌÂ Á‡Â„ËÒÚËÓ‚‡ÌÓ\n";
    }
    return ss.str();
}

string SystemMonitor::get_full_report() {
    lock_guard<mutex> lock(monitor_mutex_);
    stringstream ss;

    ss << "\n" << string(80, '#') << "\n";
    ss << "          œŒÀÕ€… Œ“◊®“ Œ –¿¡Œ“≈ –Œ¡Œ“¿\n";
    ss << string(80, '#') << "\n";
    ss << "—ÙÓÏËÓ‚‡Ì: " << get_current_time() << "\n";
    ss << string(80, '#') << "\n";

    ss << get_temperature_stats();
    ss << get_cpu_load_stats();
    ss << get_critical_events(10);

    ss << "\n" << string(80, '=') << "\n";
    ss << "‘¿…À€ ƒ¿ÕÕ€’\n";
    ss << string(80, '=') << "\n";
    ss << "  - " << temp_log_file_ << "\n";
    ss << "  - " << cpu_load_log_file_ << "\n";
    ss << "  - " << critical_log_file_ << "\n";
    ss << "\n" << string(80, '#') << "\n ŒÕ≈÷ Œ“◊®“¿\n" << string(80, '#') << "\n";

    return ss.str();
}

void SystemMonitor::start_monitoring() {
    thread([this]() {
        while (running_) {
            log_temperature();
            log_cpu_load();
            check_critical_conditions();
            this_thread::sleep_for(chrono::seconds(10));
        }
        }).detach();
}

void SystemMonitor::stop() { running_ = false; }

// ========== –≈¿À»«¿÷»ﬂ  ŒÃ¿Õƒ ==========
string MoveCommand::execute(const string& command, SystemMonitor& monitor,
    ArduinoComm& arduino, Logger& logger) {
    arduino.send_command(command);
    return "OK";
}

string SpeedCommand::execute(const string& command, SystemMonitor& monitor,
    ArduinoComm& arduino, Logger& logger) {
    arduino.send_command(command);
    return "OK";
}

string GetLogsCommand::execute(const string& command, SystemMonitor& monitor,
    ArduinoComm& arduino, Logger& logger) {
    return logger.get_recent_logs(50);
}

string GetTempStatsCommand::execute(const string& command, SystemMonitor& monitor,
    ArduinoComm& arduino, Logger& logger) {
    return monitor.get_temperature_stats();
}

string GetCpuLoadCommand::execute(const string& command, SystemMonitor& monitor,
    ArduinoComm& arduino, Logger& logger) {
    return monitor.get_cpu_load_stats();
}

string GetCriticalEventsCommand::execute(const string& command, SystemMonitor& monitor,
    ArduinoComm& arduino, Logger& logger) {
    return monitor.get_critical_events(10);
}

string GetFullReportCommand::execute(const string& command, SystemMonitor& monitor,
    ArduinoComm& arduino, Logger& logger) {
    return monitor.get_full_report();
}

// ========== –≈¿À»«¿÷»ﬂ COMMAND_MANAGER ==========
string CommandManager::process(const string& command, SystemMonitor& monitor,
    ArduinoComm& arduino, Logger& logger) {
    string cmd_name = command;
    size_t space_pos = command.find(' ');
    if (space_pos != string::npos) {
        cmd_name = command.substr(0, space_pos);
    }

    auto it = handlers_.find(cmd_name);
    if (it != handlers_.end()) {
        return it->second->execute(command, monitor, arduino, logger);
    }

    if (command.length() == 1 && (command == "w" || command == "a" ||
        command == "s" || command == "d" || command == "x")) {
        return handlers_["move"]->execute(command, monitor, arduino, logger);
    }

    if (command.substr(0, 5) == "speed") {
        return handlers_["speed"]->execute(command, monitor, arduino, logger);
    }

    return "ERROR: Unknown command";
}

void CommandManager::print_available_commands() const {
    cout << "\nƒÓÒÚÛÔÌ˚Â ÍÓÏ‡Ì‰˚:" << endl;
    cout << "  w,a,s,d,x,speed N - ÛÔ‡‚ÎÂÌËÂ ‰‚ËÊÂÌËÂÏ" << endl;
    cout << "  GET_LOGS - ÔÓÒÎÂ‰ÌËÂ ÎÓ„Ë" << endl;
    cout << "  GET_TEMP_STATS - ÒÚ‡ÚËÒÚËÍ‡ ÚÂÏÔÂ‡ÚÛ˚" << endl;
    cout << "  GET_CPU_LOAD - Á‡„ÛÁÍ‡ CPU" << endl;
    cout << "  GET_CRITICAL_EVENTS - ÍËÚË˜ÂÒÍËÂ ÒÓ·˚ÚËˇ" << endl;
    cout << "  GET_FULL_REPORT - ÔÓÎÌ˚È ÓÚ˜ÂÚ" << endl;
}

// ========== –≈¿À»«¿÷»ﬂ CLIENT_HANDLER ==========
ClientHandler::ClientHandler(int socket, sockaddr_in addr, Logger& log_ref,
    SystemMonitor& mon_ref, ArduinoComm& ard_ref, CommandManager& cmd_ref)
    : client_socket_(socket), logger_(log_ref), monitor_(mon_ref),
    arduino_(ard_ref), cmd_manager_(cmd_ref), monitoring_mode_(false) {
    client_ip_ = inet_ntoa(addr.sin_addr);
}

void ClientHandler::send_full_response(const string& response) {
    if (response.empty()) return;
    const char* data = response.c_str();
    size_t total = response.length();
    size_t sent = 0;

    while (sent < total) {
        int result = send(client_socket_, data + sent, total - sent, 0);
        if (result <= 0) {
            logger_.log("Œ¯Ë·Í‡ ÓÚÔ‡‚ÍË ‰‡ÌÌ˚ı ÍÎËÂÌÚÛ", "ERROR");
            break;
        }
        sent += result;
        if (sent < total) usleep(5000);
    }
}

void ClientHandler::handle() {
    logger_.log(" ÎËÂÌÚ ÔÓ‰ÍÎ˛˜ËÎÒˇ: " + client_ip_, "CONNECT");
    char buffer[4096];

    while (true) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(client_socket_, buffer, sizeof(buffer) - 1, 0);

        if (bytes <= 0) {
            logger_.log(" ÎËÂÌÚ ÓÚÍÎ˛˜ËÎÒˇ: " + client_ip_, "DISCONNECT");
            break;
        }

        string command(buffer);
        command.erase(command.find_last_not_of(" \n\r\t") + 1);
        logger_.log_command(client_ip_, command);

        string response = cmd_manager_.process(command, monitor_, arduino_, logger_);
        if (!response.empty()) send_full_response(response);
    }
    close(client_socket_);
}

// ========== –≈¿À»«¿÷»ﬂ TCP_SERVER ==========
TcpServer::TcpServer(int p, Logger& log_ref, SystemMonitor& mon_ref,
    ArduinoComm& ard_ref, CommandManager& cmd_ref)
    : server_socket_(-1), port_(p), running_(true), logger_(log_ref),
    monitor_(mon_ref), arduino_(ard_ref), cmd_manager_(cmd_ref) {
}

TcpServer::~TcpServer() { stop(); }

bool TcpServer::start() {
    server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket_ < 0) return false;

    int opt = 1;
    setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port_);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(server_socket_);
        return false;
    }

    if (listen(server_socket_, 5) < 0) {
        close(server_socket_);
        return false;
    }
    return true;
}

void TcpServer::run() {
    cout << "\n—≈–¬≈– «¿œ”Ÿ≈Õ Õ¿ œŒ–“” " << port_ << endl;
    cout << string(60, '=') << endl;
    cout << "IP ‡‰ÂÒ‡:" << endl;
    system("hostname -I | awk '{print \"  - \" $1}'");
    cout << "\nŒÊË‰‡ÌËÂ ÍÎËÂÌÚÓ‚..." << endl;

    while (running_) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);
        if (client_socket < 0) continue;

        thread([this, client_socket, client_addr]() {
            ClientHandler handler(client_socket, client_addr, logger_, monitor_, arduino_, cmd_manager_);
            handler.handle();
            }).detach();
    }
}

void TcpServer::stop() {
    running_ = false;
    if (server_socket_ >= 0) {
        close(server_socket_);
        server_socket_ = -1;
    }
}

// ========== –≈¿À»«¿÷»ﬂ ROBOT_SERVER_APP ==========
RobotServerApp::RobotServerApp(int tcp_port)
    : monitor_(logger_), arduino_(logger_), server_(tcp_port, logger_, monitor_, arduino_, cmd_manager_) {
}

bool RobotServerApp::initialize(int argc, char* argv[]) {
    print_server_banner();

    const char* port = "/dev/ttyACM0";
    if (argc > 1) port = argv[1];

    cout << "œÓ‰ÍÎ˛˜ÂÌËÂ Í " << port << "... ";
    if (!arduino_.connect(port)) {
        cerr << "\nŒ¯Ë·Í‡ ÓÚÍ˚ÚËˇ ÔÓÚ‡ " << port << endl;
        system("ls -la /dev/tty* 2>/dev/null | grep -E 'tty(ACM|USB)'");
        return false;
    }
    cout << "”—œ≈ÿÕŒ!" << endl;

    arduino_.start_reader_thread([this](const string& data) {
        monitor_.process_arduino_data(data);
        });

    this_thread::sleep_for(chrono::seconds(2));
    monitor_.start_monitoring();

    cout << "\nÃÓÌËÚÓËÌ„ Á‡ÔÛ˘ÂÌ:" << endl;
    cout << "  - “ÂÏÔÂ‡ÚÛ‡ CPU (Í‡Ê‰˚Â 10 ÒÂÍ)" << endl;
    cout << "  - «‡„ÛÁÍ‡ CPU (Í‡Ê‰˚Â 10 ÒÂÍ)" << endl;
    cout << "  -  ËÚË˜ÂÒÍËÂ ÒÓ·˚ÚËˇ (ÔÓÓ„ 55∞C / 80% CPU)" << endl;

    if (!server_.start()) return false;

    cmd_manager_.print_available_commands();
    return true;
}

void RobotServerApp::run() {
    server_.run();
}

void RobotServerApp::shutdown() {
    arduino_.stop();
    monitor_.stop();
    server_.stop();
}

// ========== √À¿¬Õ¿ﬂ ‘”Õ ÷»ﬂ ==========
int main(int argc, char* argv[]) {
    RobotServerApp app;

    if (!app.initialize(argc, argv)) {
        return 1;
    }

    app.run();
    app.shutdown();

    return 0;
}