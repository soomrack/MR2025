#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <atomic>
#include <thread>
#include <chrono>
#include <unordered_map>
#include <functional>
#include <fstream>
#include <termios.h>
#include <fcntl.h>
#include <errno.h>
#include <mutex>
#include <ctime>
#include <sys/types.h>
#include <sys/stat.h>

using namespace std::chrono_literals;

class Server {
private:
    int server_socket;
    int client_socket;
    int socket_port;
    std::atomic<bool> running{false};
    std::atomic<bool> connected{false};
    sockaddr_in address{};
    std::unordered_map<std::string, std::function<void()>> action_map;
    std::thread receiver;
    std::thread sender;
    std::thread connection_checker;
    std::thread logger;
    std::atomic<bool> sent;

    // UART / Arduino / logger
    std::string serial_device{"/dev/ttyACM0"};
    int serial_baud{9600};
    int serial_fd{-1};
    std::mutex log_mutex;
    std::ofstream log_file;
    std::ofstream warn_log_file;
    int WARN_VALUE = 95;
    std::string last_timestamp;
    long long last_log_stamp{0};

public:
    Server() {
        init_action_map();
    }
    ~Server() {
        running = false;
        close(client_socket);
        close(server_socket);
        close_serial_port();
    }

    void tcp_init(int port) {
        socket_port = port;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(socket_port);
    }

    void set_serial(const std::string &device, int baud) {
        serial_device = device;
        serial_baud = baud;
    }


    int connect_to_client(){
        server_socket = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        if (bind(server_socket, (sockaddr*)&address, sizeof(address)) < 0) {
            perror("bind");
            return 1;
        }

        if (listen(server_socket, 1) < 0) {
            perror("listen");
            return 1;
        }

        std::cout << "Server is listening on port " << socket_port << "...\n";

        client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket < 0) {
            perror("accept");
            return 1;
        }

        set_recv_timeout(client_socket, 1); 
        connected = true;
        std::cout << "Connnected \n";
        return 0;
    }


    void start(){
        running = true;

        receiver = std::thread(&Server::receive_message, this);
        sender = std::thread(&Server::send_message, this);
        connection_checker = std::thread(&Server::check_connection, this);

        // start UART logger thread
        logger = std::thread(&Server::serial_logging, this);

        if (connection_checker.joinable()) connection_checker.join();
        if (receiver.joinable()) receiver.join();
        if (sender.joinable()) sender.join();
        if (logger.joinable()) logger.join();
    }

private:
    void receive_message(){
        char buffer[1024] = {};
        while (running && connected) {
            ssize_t bytes = read(client_socket, buffer, sizeof(buffer) - 1);

            if (bytes > 0) {
                buffer[bytes] = '\0';
                processing_mesage(buffer);
            }
        }
    }


    void send_message(){
        std::string message;
        fd_set readfds;
        struct timeval tv;

        while (running && connected) {

            FD_ZERO(&readfds);
            FD_SET(STDIN_FILENO, &readfds);
            tv.tv_sec = 1;
            tv.tv_usec = 0;

            int sel = select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &tv);
            if (sel < 0) {
                perror("select");
                break;
            }
            if (sel == 0) {
                continue;
            }

            std::getline(std::cin, message);
            send(client_socket, message.c_str(), message.length(), 0);
            if (message == "/exit") {
                running = false;
            }
        }
    }


    void check_connection() {
        while (running) {
            sent = false;
            send(client_socket, "/y", 3, 0);
            
            auto start = std::chrono::steady_clock::now(); 

            while (!sent) {
                auto now = std::chrono::steady_clock::now(); 
                if (now-start > 1s) {
                    break;
                }
            }

            if (!running) return;
            if (!sent) {
                reconnect_to_client();
            }
            std::this_thread::sleep_for(0.5s);
        }
    }


    void processing_mesage(const char* message){
        std::string command = message;
        if (auto it = action_map.find(command); it != action_map.end()) {
            it->second();  // вызов без дополнительных параметров
        } else {
            print_message(message);
        }
    }

    
    void reconnect_to_client() {
        connected = false;
        std::this_thread::sleep_for(1s);
        std::cout << "Disconnect. Trying to reconnecting...\n";
        
        close(client_socket);
        client_socket = accept(server_socket, nullptr, nullptr);
        if (client_socket >= 0) {
            std::cout << "Reconnected\n";
            sent = true;
            set_recv_timeout(client_socket, 1);
            connected = true;

            // if we still have previous threads, detach them before overwriting
            if (receiver.joinable()) receiver.detach();
            if (sender.joinable()) sender.detach();

            receiver = std::thread(&Server::receive_message, this);
            sender = std::thread(&Server::send_message, this);
            receiver.detach();
            sender.detach();
            return;
        }
        std::cout << "Видно не судьба видно не судьба";
    }


    void set_recv_timeout(int socket_fd, int seconds) {
        struct timeval tv;
        tv.tv_sec = seconds;
        tv.tv_usec = 0;
        if (setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) < 0) {
            perror("setsockopt SO_RCVTIMEO");
        }
    }

    // ---------- Serial / Arduino helpers ----------
    static speed_t baud_to_speed(int baud) {
        switch (baud) {
            case 9600: return B9600;
            case 19200: return B19200;
            case 38400: return B38400;
            case 57600: return B57600;
            case 115200: return B115200;
            default: return B9600;
        }
    }

    int open_serial_port() {
        serial_fd = open(serial_device.c_str(), O_RDONLY | O_NOCTTY);
        if (serial_fd < 0) {
            std::string err = "open "; err += serial_device;
            perror(err.c_str());
            return -1;
        }

        struct termios tty{};
        if (tcgetattr(serial_fd, &tty) != 0) {
            perror("tcgetattr");
            ::close(serial_fd);
            serial_fd = -1;
            return -1;
        }

        cfsetospeed(&tty, baud_to_speed(serial_baud));
        cfsetispeed(&tty, baud_to_speed(serial_baud));

        tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
        tty.c_cflag |= (CLOCAL | CREAD);
        tty.c_cflag &= ~(PARENB | PARODD);
        tty.c_cflag &= ~CSTOPB;
        tty.c_cflag &= ~CRTSCTS;

        tty.c_iflag = 0;
        tty.c_oflag = 0;
        tty.c_lflag = 0;

        tty.c_cc[VMIN] = 0;
        tty.c_cc[VTIME] = 10; // 1 second read timeout

        tcflush(serial_fd, TCIFLUSH);
        if (tcsetattr(serial_fd, TCSANOW, &tty) != 0) {
            perror("tcsetattr");
            ::close(serial_fd);
            serial_fd = -1;
            return -1;
        }

        return 0;
    }

    void close_serial_port() {
        if (serial_fd >= 0) {
            ::close(serial_fd);
            serial_fd = -1;
        }
        if (log_file.is_open()) log_file.close();
        if (warn_log_file.is_open()) warn_log_file.close();
    }

    std::string current_time_str() {
        std::time_t t = std::time(nullptr);
        char buf[64];
        if (std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&t))) {
            return std::string(buf);
        }
        return {};
    }

    void log_arduino_line(const std::string &line) {
        std::lock_guard<std::mutex> lock(log_mutex);
        // std::cout << "Arduino: \033[34m" << line << "\033[0m\n";
        if (!log_file.is_open()) log_file.open("arduino.log", std::ios::app);
        if (log_file.is_open()) {
            log_file << current_time_str() << " " << line << std::endl;
            log_file.flush();
        }

        // If line is long, write to separate warnings log and print as warning
        if (std::stoi(line) > WARN_VALUE) {
            if (!warn_log_file.is_open()) warn_log_file.open("arduino_warnings.log", std::ios::app);
            if (warn_log_file.is_open()) {
                warn_log_file << current_time_str() << " " << line << std::endl;
                warn_log_file.flush();
            }
            
            std::string warn_msg = "Warning: Arduino value " + line + " exceeds threshold " + std::to_string(WARN_VALUE);
            send(client_socket, warn_msg.c_str(), warn_msg.length(), 0);
        }
    }

    void serial_logging() {
        if (serial_device.empty()) return;
        if (open_serial_port() != 0) {
            std::cerr << "Failed to open serial port " << serial_device << "\n";
            return;
        }

        char buf[256];
        std::string line;
        while (running) {
            ssize_t n = read(serial_fd, buf, sizeof(buf));
            if (n < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    std::this_thread::sleep_for(100ms);
                    continue;
                }
                perror("read serial");
                break;
            } else if (n == 0) {
                std::this_thread::sleep_for(100ms);
                continue;
            } else {
                for (ssize_t i = 0; i < n; ++i) {
                    char c = buf[i];
                    if (c == '\r') continue;
                    if (c == '\n') {
                        if (!line.empty()) {
                            log_arduino_line(line);
                            line.clear();
                        }
                    } else {
                        line.push_back(c);
                    }
                }
            }
        }

        close_serial_port();
    }


    void print_message(const char* message) {
        std::cout << "Received message: \033[32m" << message << "\n\033[0m";   
    }


    void init_action_map() {
        action_map = {
            {"/shutdown", [this]() {this->shut_server_down();}},
            {"/exit", [this]() {this->exit_from_chat();}},
            {"/y", [this]() {this->confirm_sended_message();}},
            {"/get_os_info", [this]() {this->send_OS_info();}},
            {"/get_all_logs", [this]() {this->send_all_logs();}},
            {"/get_last_logs", [this]() {this->send_last_logs();}},
            {"/clear_logs", [this]() {this->clear_logs();}}
        };
    }


    void confirm_sended_message(){
        sent = true;
    }    


    void exit_from_chat() {
        reconnect_to_client();
    }

    void shut_server_down() {
        running = false;
        std::cout << "Сервер выключен";
    }


    void send_OS_info() {
        std::ifstream file("/etc/os-release");
        std::string line;
        std::string os_info;

        while (std::getline(file, line)) {
            if (line.find("PRETTY_NAME=") == 0) {
                size_t start = line.find('"') + 1;
                size_t end = line.rfind('"');
                os_info = line.substr(start, end - start) + '\n';
                break;
            }
        }
        send(client_socket, os_info.c_str(), os_info.length(), 0);
    }


    void send_all_logs() {
        std::ifstream log("arduino.log");
        std::string content((std::istreambuf_iterator<char>(log)), std::istreambuf_iterator<char>());
        send(client_socket, content.c_str(), content.length(), 0);
    }


    void send_last_logs() {
        std::ifstream log("arduino.log");
        std::string line;
        std::string last_logs;
        bool flag = false;
        long long current_log_stamp = 0;
        while (std::getline(log, line)) {
            current_log_stamp++;
            if (current_log_stamp <= last_log_stamp) {
                continue;
            }
            
            last_logs += line + "\n";
        
        }
        last_log_stamp = current_log_stamp;
        send(client_socket, last_logs.c_str(), last_logs.length(), 0);
    }


    void clear_logs() {
        std::lock_guard<std::mutex> lock(log_mutex);
        if (log_file.is_open()) {
            log_file.close();
        }
        if (warn_log_file.is_open()) {
            warn_log_file.close();
        }

        // Truncate the log file
        std::ofstream ofs("arduino.log", std::ios::trunc);
        ofs.close();
        // Truncate warnings log
        std::ofstream ofs_warn("arduino_warnings.log", std::ios::trunc);
        ofs_warn.close();

        // Reopen for append so logger can continue
        log_file.open("arduino.log", std::ios::app);
        warn_log_file.open("arduino_warnings.log", std::ios::app);

        std::string resp = "Logs cleared\n";
        if (connected) {
            send(client_socket, resp.c_str(), resp.length(), 0);
        }
        std::cout << "Logs cleared\n";
    }
};


int main(int argc, char *argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <port> [serial_device] [baud]\n";
        return 1;
    }

    Server server;
    int port = std::stoi(argv[1]);

    // optional serial args: device and baud
    if (argc > 2) {
        std::string dev = "/dev/tty" + std::string(argv[2]);
        int baud = 9600;
        if (argc > 3) {
            baud = std::stoi(argv[3]);
        }
        server.set_serial(dev, baud);
    }

    server.tcp_init(port);
    if (server.connect_to_client()) {
        std::cout << "error";
        return 1;
    }
    server.start();
}  
