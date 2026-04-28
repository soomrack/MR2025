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
#include <unordered_map>
#include <memory>

// Linux headers
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/statvfs.h>

using namespace std;

// ========== ОБЪЯВЛЕНИЕ КЛАССОВ ВПЕРЁД ==========
class Logger;
class ArduinoComm;
class SystemMonitor;
class ClientHandlerContext;
class ICommand;
class CommandManager;
class ClientHandler;
class TcpServer;
class RobotServerApp;

// ========== КЛАСС LOGGER ==========
class Logger {
private:
    mutex log_mutex_;
    vector<string> log_history_;
    ofstream log_file_;

    string get_current_time() {
        auto now = chrono::system_clock::now();
        time_t now_time = chrono::system_clock::to_time_t(now);
        struct tm local_time;
        localtime_r(&now_time, &local_time);

        stringstream ss;
        ss << put_time(&local_time, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

public:
    Logger() {
        log_file_.open("server.log", ios::app);
    }

    ~Logger() {
        if (log_file_.is_open()) log_file_.close();
    }

    void log(const string& message, const string& type = "INFO") {
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

    void log_command(const string& client_ip, const string& command) {
        log("CMD from " + client_ip + ": " + command, "COMMAND");
    }

    string get_recent_logs(int count = 50) {
        lock_guard<mutex> lock(log_mutex_);
        stringstream ss;
        ss << "\n" << string(60, '=') << "\n";
        ss << "ПОСЛЕДНИЕ ЛОГИ (последние " << count << ")\n";
        ss << string(60, '=') << "\n";

        int start = log_history_.size() > count ? log_history_.size() - count : 0;
        for (size_t i = start; i < log_history_.size(); i++) {
            ss << log_history_[i] << "\n";
        }
        ss << string(60, '=') << "\n";
        return ss.str();
    }
};

// ========== КЛАСС ARDUINO COMMUNICATION ==========
class ArduinoComm {
private:
    int serial_port_;
    mutex serial_mutex_;
    bool running_;
    Logger& logger_;
    string read_buffer_;

    int open_serial(const char* port) {
        int fd = open(port, O_RDWR | O_NOCTTY | O_SYNC);
        if (fd < 0) return fd;

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

public:
    ArduinoComm(Logger& log_ref) : serial_port_(-1), running_(true), logger_(log_ref) {}

    ~ArduinoComm() {
        disconnect();
    }

    bool connect(const char* port) {
        serial_port_ = open_serial(port);
        if (serial_port_ >= 0) {
            logger_.log("Подключено к Arduino по порту " + string(port), "ARDUINO");
            return true;
        }
        logger_.log("Ошибка подключения к " + string(port), "ERROR");
        return false;
    }

    void disconnect() {
        if (serial_port_ >= 0) {
            close(serial_port_);
            serial_port_ = -1;
            logger_.log("Arduino отключена", "ARDUINO");
        }
    }

    bool send_command(const string& cmd) {
        if (serial_port_ < 0) return false;

        {
            lock_guard<mutex> lock(serial_mutex_);
            string to_send = cmd + "\n";
            write(serial_port_, to_send.c_str(), to_send.length());
        }

        return true;
    }

    void start_reader_thread(function<void(const string&)> data_callback) {
        thread([this, data_callback]() {
            char buffer[256];
            while (running_) {
                if (serial_port_ >= 0) {
                    int bytes = read(serial_port_, buffer, sizeof(buffer) - 1);
                    if (bytes > 0) {
                        buffer[bytes] = '\0';
                        read_buffer_ += buffer;

                        size_t pos;
                        while ((pos = read_buffer_.find('\n')) != string::npos) {
                            string line = read_buffer_.substr(0, pos);
                            line.erase(line.find_last_not_of(" \r\n\t") + 1);

                            if (!line.empty()) {
                                data_callback(line);
                            }

                            read_buffer_.erase(0, pos + 1);
                        }
                    }
                }
                this_thread::sleep_for(chrono::milliseconds(10));
            }
            }).detach();
    }

    void stop() {
        running_ = false;
    }

    bool is_connected() const {
        return serial_port_ >= 0;
    }
};

// ========== КЛАСС SYSTEM MONITOR ==========
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

    float min_temp_ = 999.0f;
    float max_temp_ = -999.0f;
    float max_cpu_load_ = 0.0f;
    float critical_temp_threshold_ = 55.0f;
    float critical_cpu_threshold_ = 80.0f;

    Logger& logger_;
    bool running_;

    string get_current_time() {
        auto now = chrono::system_clock::now();
        time_t now_time = chrono::system_clock::to_time_t(now);
        struct tm local_time;
        localtime_r(&now_time, &local_time);

        stringstream ss;
        ss << put_time(&local_time, "%Y-%m-%d %H:%M:%S");
        return ss.str();
    }

    float read_cpu_temperature() {
        float temp = 0.0f;
        FILE* fp = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
        if (fp) {
            int millideg;
            if (fscanf(fp, "%d", &millideg) == 1) {
                temp = millideg / 1000.0f;
            }
            fclose(fp);
        }
        return temp;
    }

    float read_cpu_load() {
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

    void check_critical_conditions() {
        float current_temp = read_cpu_temperature();
        float current_load = read_cpu_load();

        if (current_temp > critical_temp_threshold_) {
            string message = "Температура CPU " +
                to_string(current_temp) + "C превышает порог " +
                to_string(critical_temp_threshold_) + "C";
            log_critical_event(message);
            logger_.log(message, "CRITICAL");
        }

        if (current_load > critical_cpu_threshold_) {
            string message = "Загрузка CPU " +
                to_string(current_load) + "% превышает порог " +
                to_string(critical_cpu_threshold_) + "%";
            log_critical_event(message);
            logger_.log(message, "CRITICAL");
        }
    }

    void log_critical_event(const string& message) {
        lock_guard<mutex> lock(monitor_mutex_);

        ofstream file(critical_log_file_, ios::app);
        file << get_current_time() << " - " << message << "\n";
        file.close();
    }

public:
    SystemMonitor(Logger& log_ref) : logger_(log_ref), running_(true) {
        temp_log_file_ = "/home/vlad/rpi_server/temperature_history.csv";
        critical_log_file_ = "/home/vlad/rpi_server/critical_events.log";
        cpu_load_log_file_ = "/home/vlad/rpi_server/cpu_load_history.csv";

        system("mkdir -p /home/vlad/rpi_server");

        ifstream temp_check(temp_log_file_);
        if (!temp_check.good()) {
            ofstream temp_file(temp_log_file_);
            temp_file << "Timestamp,Temperature_C\n";
            temp_file.close();
        }

        ifstream load_check(cpu_load_log_file_);
        if (!load_check.good()) {
            ofstream load_file(cpu_load_log_file_);
            load_file << "Timestamp,CPU_Load_Percent\n";
            load_file.close();
        }
    }

    void log_temperature() {
        lock_guard<mutex> lock(monitor_mutex_);

        float current_temp = read_cpu_temperature();

        TempRecord record;
        record.timestamp = get_current_time();
        record.temperature = current_temp;

        temp_history_.push_back(record);
        if (temp_history_.size() > 1000) temp_history_.erase(temp_history_.begin());

        if (current_temp < min_temp_) min_temp_ = current_temp;
        if (current_temp > max_temp_) max_temp_ = current_temp;

        static int temp_counter = 0;
        if (++temp_counter % 6 == 0) {
            ofstream file(temp_log_file_, ios::app);
            file << record.timestamp << ","
                << fixed << setprecision(2) << current_temp << "\n";
            file.close();
        }
    }

    void log_cpu_load() {
        lock_guard<mutex> lock(monitor_mutex_);

        float current_load = read_cpu_load();

        CpuLoadRecord record;
        record.timestamp = get_current_time();
        record.load_percent = current_load;

        cpu_load_history_.push_back(record);
        if (cpu_load_history_.size() > 1000) cpu_load_history_.erase(cpu_load_history_.begin());

        if (current_load > max_cpu_load_) max_cpu_load_ = current_load;

        static int load_counter = 0;
        if (++load_counter % 6 == 0) {
            ofstream file(cpu_load_log_file_, ios::app);
            file << record.timestamp << ","
                << fixed << setprecision(2) << current_load << "\n";
            file.close();
        }
    }

    void process_arduino_data(const string& data) {
        if (data.find("TEMP_STATS") != string::npos) {
            size_t comma = data.find(",");
            if (comma != string::npos) {
                try {
                    float minT = stof(data.substr(11, comma - 11));
                    float maxT = stof(data.substr(comma + 1));
                    logger_.log("Температура от Arduino: мин=" + to_string(minT) + " макс=" + to_string(maxT), "TEMP");
                }
                catch (...) {
                    logger_.log("Ошибка парсинга TEMP_STATS: " + data, "ERROR");
                }
            }
        }
        else if (data.find("CMD:") != string::npos || data.find("SPEED:") != string::npos) {
            logger_.log("Arduino: " + data, "SERIAL");
        }
        else if (!data.empty()) {
            if (data != "OK" && data.length() > 1) {
                logger_.log("Arduino: " + data, "SERIAL");
            }
        }
    }

    string get_temperature_stats() {
        lock_guard<mutex> lock(monitor_mutex_);
        stringstream ss;

        float current = read_cpu_temperature();

        ss << "\n" << string(80, '=') << "\n";
        ss << "ТЕМПЕРАТУРА ПРОЦЕССОРА\n";
        ss << string(80, '=') << "\n";
        ss << "Текущая:     " << fixed << setprecision(2) << current << " °C\n";
        ss << "Минимальная: " << fixed << setprecision(2) << min_temp_ << " °C\n";
        ss << "Максимальная: " << fixed << setprecision(2) << max_temp_ << " °C\n";
        ss << string(80, '=') << "\n";

        return ss.str();
    }

    string get_cpu_load_stats() {
        lock_guard<mutex> lock(monitor_mutex_);
        stringstream ss;

        float current = read_cpu_load();

        ss << "\n" << string(80, '=') << "\n";
        ss << "ЗАГРУЗКА ПРОЦЕССОРА\n";
        ss << string(80, '=') << "\n";
        ss << "Текущая:     " << fixed << setprecision(2) << current << " %\n";
        ss << "Максимальная: " << fixed << setprecision(2) << max_cpu_load_ << " %\n";
        ss << "Количество ядер: " << sysconf(_SC_NPROCESSORS_ONLN) << "\n";
        ss << string(80, '=') << "\n";

        return ss.str();
    }

    string get_critical_events(int last_n = 10) {
        lock_guard<mutex> lock(monitor_mutex_);
        stringstream ss;

        ss << "\n" << string(80, '=') << "\n";
        ss << "КРИТИЧЕСКИЕ СОБЫТИЯ (последние " << last_n << ")\n";
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
            crit_file.close();

            if (all_events.empty()) {
                ss << "  Критических событий не случилось\n";
                ss << "  Робот работал в штатном режиме\n";
            }
            else {
                ss << "  Обнаружены следующие критические события:\n\n";
                int start = all_events.size() > last_n ? all_events.size() - last_n : 0;
                for (size_t i = start; i < all_events.size(); i++) {
                    ss << "  * " << all_events[i] << "\n";
                }
            }
        }
        else {
            ss << "  Критических событий не зарегистрировано\n";
        }
        ss << string(80, '=') << "\n";

        return ss.str();
    }

    string get_full_report() {
        lock_guard<mutex> lock(monitor_mutex_);

        stringstream ss;

        ss << "\n" << string(80, '#') << "\n";
        ss << "          ПОЛНЫЙ ОТЧЁТ О РАБОТЕ РОБОТА\n";
        ss << string(80, '#') << "\n";
        ss << "Сформирован: " << get_current_time() << "\n";
        ss << string(80, '#') << "\n";

        // Температура
        float current_temp = read_cpu_temperature();
        ss << "\n" << string(80, '=') << "\n";
        ss << "ТЕМПЕРАТУРА ПРОЦЕССОРА\n";
        ss << string(80, '=') << "\n";
        ss << "Текущая:     " << fixed << setprecision(2) << current_temp << " °C\n";
        ss << "Минимальная: " << fixed << setprecision(2) << min_temp_ << " °C\n";
        ss << "Максимальная: " << fixed << setprecision(2) << max_temp_ << " °C\n";

        // Загрузка CPU
        float current_load = read_cpu_load();
        ss << "\n" << string(80, '=') << "\n";
        ss << "ЗАГРУЗКА ПРОЦЕССОРА\n";
        ss << string(80, '=') << "\n";
        ss << "Текущая:     " << fixed << setprecision(2) << current_load << " %\n";
        ss << "Максимальная: " << fixed << setprecision(2) << max_cpu_load_ << " %\n";
        ss << "Количество ядер: " << sysconf(_SC_NPROCESSORS_ONLN) << "\n";

        // Критические события
        ss << "\n" << string(80, '=') << "\n";
        ss << "КРИТИЧЕСКИЕ СОБЫТИЯ (последние 10)\n";
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
            crit_file.close();

            if (all_events.empty()) {
                ss << "  Критических событий не случилось\n";
                ss << "  Робот работал в штатном режиме\n";
            }
            else {
                ss << "  Обнаружены следующие критические события:\n\n";
                int start = all_events.size() > 10 ? all_events.size() - 10 : 0;
                for (size_t i = start; i < all_events.size(); i++) {
                    ss << "  * " << all_events[i] << "\n";
                }
            }
        }
        else {
            ss << "  Критических событий не зарегистрировано\n";
        }

        // Файлы данных
        ss << "\n" << string(80, '=') << "\n";
        ss << "ФАЙЛЫ ДАННЫХ\n";
        ss << string(80, '=') << "\n";
        ss << "Данные сохраняются в следующих файлах:\n";
        ss << "  - " << temp_log_file_ << "\n";
        ss << "  - " << cpu_load_log_file_ << "\n";
        ss << "  - " << critical_log_file_ << "\n";

        ss << "\n" << string(80, '#') << "\n";
        ss << "КОНЕЦ ОТЧЁТА\n";
        ss << string(80, '#') << "\n";

        return ss.str();
    }

    void start_monitoring() {
        thread([this]() {
            while (running_) {
                log_temperature();
                log_cpu_load();
                check_critical_conditions();
                this_thread::sleep_for(chrono::seconds(10));
            }
            }).detach();
    }

    void stop() {
        running_ = false;
    }

    void set_critical_thresholds(float temp, float cpu) {
        critical_temp_threshold_ = temp;
        critical_cpu_threshold_ = cpu;
        logger_.log("Установлены новые пороги: температура=" + to_string(temp) +
            "C, CPU=" + to_string(cpu) + "%", "CONFIG");
    }
};

// ========== КОНТЕКСТ ДЛЯ ВЫПОЛНЕНИЯ КОМАНД ==========
class ClientHandlerContext {
public:
    string client_ip;
    Logger* logger;
    SystemMonitor* monitor;
    ArduinoComm* arduino;
    unordered_map<string, string> session_data;

    ClientHandlerContext(const string& ip, Logger& log, SystemMonitor& mon, ArduinoComm& ard)
        : client_ip(ip), logger(&log), monitor(&mon), arduino(&ard) {
    }
};

// ========== БАЗОВЫЙ ИНТЕРФЕЙС КОМАНДЫ ==========
class ICommand {
public:
    virtual ~ICommand() = default;
    virtual string execute(ClientHandlerContext& ctx, const string& args = "") = 0;
    virtual string get_name() const = 0;
    virtual string get_description() const = 0;
    virtual bool requires_authentication() const { return false; }
    virtual bool is_admin_only() const { return false; }
};

// ========== РЕАЛИЗАЦИИ КОМАНД ==========

// Команды движения
class MoveCommand : public ICommand {
private:
    char direction_;
    string description_;
public:
    MoveCommand(char dir, const string& desc) : direction_(dir), description_(desc) {}

    string execute(ClientHandlerContext& ctx, const string& args) override {
        string cmd(1, direction_);
        ctx.arduino->send_command(cmd);
        ctx.logger->log_command(ctx.client_ip, cmd);
        return "OK";
    }

    string get_name() const override { return string(1, direction_); }
    string get_description() const override { return description_; }
};

// Установка скорости
class SpeedCommand : public ICommand {
public:
    string execute(ClientHandlerContext& ctx, const string& args) override {
        if (args.empty()) return "ERROR: Speed value required (100-255)";
        int speed = stoi(args);
        if (speed < 100 || speed > 255) {
            return "ERROR: Speed must be between 100 and 255";
        }
        ctx.arduino->send_command("speed " + args);
        ctx.logger->log_command(ctx.client_ip, "speed " + args);
        return "OK";
    }

    string get_name() const override { return "speed"; }
    string get_description() const override { return "Установить скорость (speed 100-255)"; }
};

// Статистика температуры
class TemperatureStatsCommand : public ICommand {
public:
    string execute(ClientHandlerContext& ctx, const string& args) override {
        return ctx.monitor->get_temperature_stats();
    }

    string get_name() const override { return "GET_TEMP_STATS"; }
    string get_description() const override { return "Статистика температуры CPU"; }
};

// Загрузка CPU
class CpuLoadCommand : public ICommand {
public:
    string execute(ClientHandlerContext& ctx, const string& args) override {
        return ctx.monitor->get_cpu_load_stats();
    }

    string get_name() const override { return "GET_CPU_LOAD"; }
    string get_description() const override { return "Загрузка CPU"; }
};

// Критические события
class CriticalEventsCommand : public ICommand {
public:
    string execute(ClientHandlerContext& ctx, const string& args) override {
        int count = args.empty() ? 10 : stoi(args);
        if (count > 100) count = 100;
        return ctx.monitor->get_critical_events(count);
    }

    string get_name() const override { return "GET_CRITICAL_EVENTS"; }
    string get_description() const override { return "Критические события (GET_CRITICAL_EVENTS [N])"; }
};

// Полный отчёт
class FullReportCommand : public ICommand {
public:
    string execute(ClientHandlerContext& ctx, const string& args) override {
        return ctx.monitor->get_full_report();
    }

    string get_name() const override { return "GET_FULL_REPORT"; }
    string get_description() const override { return "Полный отчёт (все параметры)"; }
};

// Логи сервера
class LogsCommand : public ICommand {
public:
    string execute(ClientHandlerContext& ctx, const string& args) override {
        int count = args.empty() ? 50 : stoi(args);
        if (count > 200) count = 200;
        return ctx.logger->get_recent_logs(count);
    }

    string get_name() const override { return "GET_LOGS"; }
    string get_description() const override { return "Последние логи сервера (GET_LOGS [N])"; }
};

// Проверка связи
class PingCommand : public ICommand {
public:
    string execute(ClientHandlerContext& ctx, const string& args) override {
        return "PONG " + (args.empty() ? "" : args);
    }

    string get_name() const override { return "PING"; }
    string get_description() const override { return "Проверка связи (PING [text])"; }
};

// Эхо
class EchoCommand : public ICommand {
public:
    string execute(ClientHandlerContext& ctx, const string& args) override {
        return "ECHO: " + (args.empty() ? "(empty)" : args);
    }

    string get_name() const override { return "ECHO"; }
    string get_description() const override { return "Вернуть текст (ECHO <text>)"; }
};

// Статус системы
class StatusCommand : public ICommand {
public:
    string execute(ClientHandlerContext& ctx, const string& args) override {
        stringstream ss;
        ss << "\n" << string(50, '=') << "\n";
        ss << "СТАТУС СИСТЕМЫ\n";
        ss << string(50, '=') << "\n";
        ss << "Arduino: " << (ctx.arduino->is_connected() ? "ПОДКЛЮЧЕНА" : "ОТКЛЮЧЕНА") << "\n";
        ss << "IP клиента: " << ctx.client_ip << "\n";
        ss << "Данных сессии: " << ctx.session_data.size() << " параметров\n";
        ss << string(50, '=') << "\n";
        return ss.str();
    }

    string get_name() const override { return "STATUS"; }
    string get_description() const override { return "Текущий статус системы"; }
};

// Выключение сервера (только для администраторов)
class ShutdownCommand : public ICommand {
private:
    bool& running_;
public:
    ShutdownCommand(bool& running_flag) : running_(running_flag) {}

    string execute(ClientHandlerContext& ctx, const string& args) override {
        ctx.logger->log("Сервер выключается по команде от " + ctx.client_ip, "SHUTDOWN");
        running_ = false;
        return "SERVER_SHUTDOWN_INITIATED";
    }

    string get_name() const override { return "SHUTDOWN"; }
    string get_description() const override { return "Выключить сервер (только для администраторов)"; }
    bool is_admin_only() const override { return true; }
};

// Установка порогов
class SetThresholdsCommand : public ICommand {
public:
    string execute(ClientHandlerContext& ctx, const string& args) override {
        if (args.empty()) {
            return "ERROR: Usage: SET_THRESHOLDS <temp_C> <cpu_percent>";
        }

        stringstream ss(args);
        float temp, cpu;
        if (ss >> temp >> cpu) {
            ctx.monitor->set_critical_thresholds(temp, cpu);
            return "THRESHOLDS_UPDATED: temp=" + to_string(temp) + "C, cpu=" + to_string(cpu) + "%";
        }
        return "ERROR: Invalid arguments. Usage: SET_THRESHOLDS <temp_C> <cpu_percent>";
    }

    string get_name() const override { return "SET_THRESHOLDS"; }
    string get_description() const override { return "Установить пороги критичности (SET_THRESHOLDS 60 85)"; }
    bool is_admin_only() const override { return true; }
};

// ========== МЕНЕДЖЕР КОМАНД ==========
class CommandManager {
private:
    unordered_map<string, unique_ptr<ICommand>> commands_;
    unordered_map<string, string> aliases_;

public:
    void register_command(const string& name, unique_ptr<ICommand> cmd) {
        commands_[name] = move(cmd);
    }

    void register_alias(const string& alias, const string& command) {
        aliases_[alias] = command;
    }

    ICommand* get_command(const string& name) {
        // Проверка алиаса
        auto alias_it = aliases_.find(name);
        string real_name = (alias_it != aliases_.end()) ? alias_it->second : name;

        auto it = commands_.find(real_name);
        if (it != commands_.end()) {
            return it->second.get();
        }
        return nullptr;
    }

    string get_help() const {
        stringstream ss;
        ss << "\n" << string(60, '=') << "\n";
        ss << "ДОСТУПНЫЕ КОМАНДЫ\n";
        ss << string(60, '=') << "\n\n";

        vector<pair<string, string>> sorted;
        for (const auto& [name, cmd] : commands_) {
            sorted.emplace_back(name, cmd->get_description());
        }
        sort(sorted.begin(), sorted.end());

        for (const auto& [name, desc] : sorted) {
            ss << "  " << left << setw(22) << name << " - " << desc << "\n";
        }

        // Алиасы
        if (!aliases_.empty()) {
            ss << "\n" << string(60, '-') << "\n";
            ss << "Алиасы (сокращения):\n";
            for (const auto& [alias, cmd] : aliases_) {
                ss << "  " << left << setw(22) << alias << " -> " << cmd << "\n";
            }
        }

        ss << "\n" << string(60, '=') << "\n";
        ss << "Используйте: HELP, GET_TEMP_STATS, speed 150\n";
        ss << string(60, '=') << "\n";

        return ss.str();
    }

    vector<string> parse_command(const string& raw) {
        vector<string> result;
        stringstream ss(raw);
        string part;
        while (ss >> part) {
            result.push_back(part);
        }
        return result;
    }
};

// ========== КОМАНДА HELP (определяется после CommandManager) ==========
class HelpCommand : public ICommand {
private:
    CommandManager* cmd_manager_;
public:
    HelpCommand(CommandManager* mgr) : cmd_manager_(mgr) {}

    string execute(ClientHandlerContext& ctx, const string& args) override {
        return cmd_manager_->get_help();
    }

    string get_name() const override { return "HELP"; }
    string get_description() const override { return "Вывести список доступных команд"; }
};

// ========== КЛАСС CLIENT HANDLER ==========
class ClientHandler {
private:
    int client_socket_;
    string client_ip_;
    Logger& logger_;
    SystemMonitor& monitor_;
    ArduinoComm& arduino_;
    CommandManager& cmd_manager_;
    ClientHandlerContext context_;

    void send_response(const string& response) {
        if (response.empty()) return;

        const char* data = response.c_str();
        size_t total = response.length();
        size_t sent = 0;

        while (sent < total) {
            int result = send(client_socket_, data + sent, total - sent, 0);
            if (result <= 0) {
                logger_.log("Ошибка отправки данных клиенту", "ERROR");
                break;
            }
            sent += result;
            if (sent < total) usleep(5000);
        }
    }

public:
    ClientHandler(int socket, sockaddr_in addr, Logger& log, SystemMonitor& mon,
        ArduinoComm& ard, CommandManager& cmd_mgr)
        : client_socket_(socket), logger_(log), monitor_(mon), arduino_(ard),
        cmd_manager_(cmd_mgr), context_(inet_ntoa(addr.sin_addr), log, mon, ard) {
        client_ip_ = inet_ntoa(addr.sin_addr);
    }

    ~ClientHandler() {
        if (client_socket_ >= 0) {
            close(client_socket_);
        }
    }

    void handle() {
        logger_.log("Клиент подключился: " + client_ip_, "CONNECT");

        char buffer[4096];

        while (true) {
            memset(buffer, 0, sizeof(buffer));
            int bytes = recv(client_socket_, buffer, sizeof(buffer) - 1, 0);

            if (bytes <= 0) {
                if (bytes == 0) {
                    logger_.log("Клиент отключился: " + client_ip_, "DISCONNECT");
                }
                else {
                    logger_.log("Ошибка приёма от " + client_ip_ + ": " + strerror(errno), "ERROR");
                }
                break;
            }

            string raw_command(buffer);
            raw_command.erase(raw_command.find_last_not_of(" \n\r\t") + 1);

            if (raw_command.empty()) continue;

            logger_.log_command(client_ip_, raw_command);

            // Парсинг команды и аргументов
            vector<string> parts = cmd_manager_.parse_command(raw_command);
            string cmd_name = parts[0];
            string args;
            for (size_t i = 1; i < parts.size(); i++) {
                if (i > 1) args += " ";
                args += parts[i];
            }

            // Поиск команды
            ICommand* cmd = cmd_manager_.get_command(cmd_name);

            string response;
            if (cmd) {
                response = cmd->execute(context_, args);
            }
            else {
                response = "ERROR: Неизвестная команда '" + cmd_name + "'. Введите HELP для списка команд.";
            }

            if (!response.empty()) {
                send_response(response);
            }
        }

        close(client_socket_);
        client_socket_ = -1;
    }
};

// ========== КЛАСС TCP SERVER ==========
class TcpServer {
private:
    int server_socket_;
    int port_;
    bool running_;
    Logger& logger_;
    SystemMonitor& monitor_;
    ArduinoComm& arduino_;
    CommandManager& cmd_manager_;

public:
    TcpServer(int p, Logger& log, SystemMonitor& mon, ArduinoComm& ard, CommandManager& cmd_mgr)
        : server_socket_(-1), port_(p), running_(true), logger_(log),
        monitor_(mon), arduino_(ard), cmd_manager_(cmd_mgr) {
    }

    ~TcpServer() {
        stop();
    }

    bool start() {
        server_socket_ = socket(AF_INET, SOCK_STREAM, 0);
        if (server_socket_ < 0) {
            logger_.log("Ошибка создания сокета", "ERROR");
            return false;
        }

        int opt = 1;
        setsockopt(server_socket_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port_);
        server_addr.sin_addr.s_addr = INADDR_ANY;

        if (bind(server_socket_, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            logger_.log("Ошибка bind. Порт " + to_string(port_) + " возможно занят", "ERROR");
            close(server_socket_);
            return false;
        }

        if (listen(server_socket_, 5) < 0) {
            logger_.log("Ошибка listen", "ERROR");
            close(server_socket_);
            return false;
        }

        return true;
    }

    void run() {
        cout << "\n" << string(60, '=') << endl;
        cout << "СЕРВЕР ЗАПУЩЕН НА ПОРТУ " << port_ << endl;
        cout << string(60, '=') << endl;
        cout << "IP адреса:" << endl;
        system("hostname -I | awk '{print \"  - \" $1}'");
        cout << "\nОжидание клиентов..." << endl;

        while (running_) {
            struct sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);

            int client_socket = accept(server_socket_, (struct sockaddr*)&client_addr, &client_len);
            if (client_socket < 0) {
                if (running_) {
                    cerr << "Ошибка accept" << endl;
                }
                continue;
            }

            thread([this, client_socket, client_addr]() {
                ClientHandler handler(client_socket, client_addr, logger_, monitor_, arduino_, cmd_manager_);
                handler.handle();
                }).detach();
        }
    }

    void stop() {
        running_ = false;
        if (server_socket_ >= 0) {
            close(server_socket_);
            server_socket_ = -1;
        }
    }
};

// ========== КЛАСС ROBOT SERVER APP ==========
class RobotServerApp {
private:
    Logger logger_;
    SystemMonitor monitor_;
    ArduinoComm arduino_;
    CommandManager cmd_manager_;
    TcpServer server_;
    bool running_;

    void register_commands() {
        // Команды управления движением
        cmd_manager_.register_command("w", make_unique<MoveCommand>('w', "Вперёд"));
        cmd_manager_.register_command("a", make_unique<MoveCommand>('a', "Влево"));
        cmd_manager_.register_command("s", make_unique<MoveCommand>('s', "Назад"));
        cmd_manager_.register_command("d", make_unique<MoveCommand>('d', "Вправо"));
        cmd_manager_.register_command("x", make_unique<MoveCommand>('x', "Стоп"));
        cmd_manager_.register_command("speed", make_unique<SpeedCommand>());

        // Команды статистики
        cmd_manager_.register_command("GET_TEMP_STATS", make_unique<TemperatureStatsCommand>());
        cmd_manager_.register_command("GET_CPU_LOAD", make_unique<CpuLoadCommand>());
        cmd_manager_.register_command("GET_CRITICAL_EVENTS", make_unique<CriticalEventsCommand>());
        cmd_manager_.register_command("GET_FULL_REPORT", make_unique<FullReportCommand>());
        cmd_manager_.register_command("GET_LOGS", make_unique<LogsCommand>());

        // Системные команды
        cmd_manager_.register_command("ECHO", make_unique<EchoCommand>());
        cmd_manager_.register_command("PING", make_unique<PingCommand>());
        cmd_manager_.register_command("STATUS", make_unique<StatusCommand>());
        cmd_manager_.register_command("SHUTDOWN", make_unique<ShutdownCommand>(running_));
        cmd_manager_.register_command("SET_THRESHOLDS", make_unique<SetThresholdsCommand>());

        // Команда помощи (должна быть последней, чтобы видеть все команды)
        cmd_manager_.register_command("HELP", make_unique<HelpCommand>(&cmd_manager_));

        // Алиасы (сокращения)
        cmd_manager_.register_alias("?", "HELP");
        cmd_manager_.register_alias("temp", "GET_TEMP_STATS");
        cmd_manager_.register_alias("cpu", "GET_CPU_LOAD");
        cmd_manager_.register_alias("crit", "GET_CRITICAL_EVENTS");
        cmd_manager_.register_alias("report", "GET_FULL_REPORT");
        cmd_manager_.register_alias("logs", "GET_LOGS");
        cmd_manager_.register_alias("stop", "x");
        cmd_manager_.register_alias("exit", "SHUTDOWN");
    }

public:
    RobotServerApp(int tcp_port = 8888)
        : monitor_(logger_), arduino_(logger_),
        server_(tcp_port, logger_, monitor_, arduino_, cmd_manager_),
        running_(true) {
        register_commands();
    }

    ~RobotServerApp() {
        shutdown();
    }

    bool initialize(int argc, char* argv[]) {
        cout << "\n" << string(60, '=') << endl;
        cout << "РОБОТ-СЕРВЕР v6.0 (РАСШИРЯЕМЫЙ)" << endl;
        cout << string(60, '=') << endl;

        const char* port = "/dev/ttyACM0";
        if (argc > 1) {
            port = argv[1];
        }

        cout << "Подключение к " << port << "... ";

        if (!arduino_.connect(port)) {
            cerr << "\nОшибка открытия порта " << port << endl;
            cerr << "Проверьте доступные порты:" << endl;
            system("ls -la /dev/tty* 2>/dev/null | grep -E 'tty(ACM|USB)' || echo '  Ничего не найдено'");
            return false;
        }

        cout << "УСПЕШНО!" << endl;

        arduino_.start_reader_thread([this](const string& data) {
            monitor_.process_arduino_data(data);
            });

        this_thread::sleep_for(chrono::seconds(2));

        monitor_.start_monitoring();
        cout << "\nМониторинг запущен:" << endl;
        cout << "  - Температура CPU (каждые 10 сек)" << endl;
        cout << "  - Загрузка CPU (каждые 10 сек)" << endl;
        cout << "  - Критические события (порог 55°C / 80% CPU)" << endl;
        cout << "  - Данные сохраняются в /home/vlad/rpi_server/" << endl;

        if (!server_.start()) {
            return false;
        }

        // Вывод справки
        cout << cmd_manager_.get_help() << endl;

        return true;
    }

    void run() {
        server_.run();
    }

    void shutdown() {
        running_ = false;
        arduino_.stop();
        monitor_.stop();
        server_.stop();
        logger_.log("Приложение завершило работу", "SHUTDOWN");
    }
};

// ========== ГЛАВНАЯ ФУНКЦИЯ ==========
int main(int argc, char* argv[]) {
    RobotServerApp app;

    if (!app.initialize(argc, argv)) {
        cerr << "\nОшибка инициализации сервера!" << endl;
        return 1;
    }

    app.run();
    app.shutdown();

    cout << "\nДо свидания!" << endl;

    return 0;
}