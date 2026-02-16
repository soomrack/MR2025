#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <csignal>


const std::string   UART_PORT = "/dev/ttyS1";
const std::string   LOG_FILE  = "/home/qwerty/arduino_sensor.log";
const int           BAUD_RATE = 9600;
const int           WARN_THRESHOLD = 700;


int uart_fd = -1;
bool running = true;


void signal_handler(int signal);

std::string get_timestamp();

std::string get_status(int value);

void write_log(const std::string &line);

class UART {
    public:
        UART(const std::string &uart_port);
        ~UART();
        int open_port();
        int get_fd() const;
        std::string read_line();

    private:
        std::string port;
        int fd;
        int setup_port();
};


class Logger {
    public:
        Logger(const std::string &file);
        void log(const std::string &raw_value);

    private:
        std::string log_file;

};


int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    std::cout << "=== LOGGER ===" << std::endl;
    std::cout << "Порт:       " << UART_PORT << std::endl;
    std::cout << "Лог-файл:   " << LOG_FILE  << std::endl;
    std::cout << "Порог WARNING: " << WARN_THRESHOLD << std::endl;
    std::cout << "Ожидание данных... " << std::endl;

    UART uart(UART_PORT);
    Logger logger(LOG_FILE);

    if (uart.open_port() != 0) return 1;

    while (running) {
        std::string line = uart.read_line();

        if(line.empty()) continue;

        logger.log(line);
    }
    return 0;


}


void signal_handler(int signal) {
    std::cout << "\n[Демон остановлен]" << std::endl;
    running = false;
    if (uart_fd >= 0) close (uart_fd);
}


std::string get_timestamp() {
    time_t now = time(nullptr);
    char buf[32];
    strftime(buf, sizeof(buf), "%b %d %H:%M:%S", localtime(&now));
    return std::string(buf);
}


std::string get_status(int value) {
    return value >= WARN_THRESHOLD ? "WARNING" : "INFO";
}


void write_log(const std::string &line) {
    std::ofstream log_file(LOG_FILE, std::ios::app);
    if (!log_file.is_open()) {
        std::cerr << "Ошибка открытия лог-файла: " << LOG_FILE << std::endl;
        return;
    }
    log_file << line << std::endl;
    std::cout << line << std::endl;
}


int UART::setup_port(){
    struct termios options;
    tcgetattr(fd, &options);

    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);

    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    options.c_cc[VMIN] = 1;
    options.c_cc[VTIME] = 0;

    tcsetattr(fd, TCSANOW, &options);
    return 0;
}


int UART::open_port() {
    fd = open(port.c_str(), O_RDWR | O_NOCTTY);
    if (fd < 0) {
        std::cerr << "Ошибка открытия порта: " << port << std::endl;
        return 1;
    }
    setup_port();
    return 0;
}


std::string UART::read_line() {
    std::string line;
    char c;

    while (running) {
        int n = read(fd, &c, 1);
        if (n < 0) return "";
        if (n == 0) continue;

        if (c == '\n') break;
        if (c == '\r') continue;
        line += c;
    }
    return line;
}


int UART::get_fd() const { return fd; }


UART::~UART() {
    if (fd >= 0) close(fd);
}


UART::UART(const std::string &uart_port) : port(uart_port), fd(-1) {}


void Logger::log(const std::string &raw_value) {
    int value = 0;
    try {
        value = std::stoi(raw_value);
    } catch (...) {
        std::cerr << "Не удалось распарсить: " << raw_value << std::endl;
        return;
    }

    std::string timestamp = get_timestamp();
    std::string status = get_status(value);
    std::string line = timestamp + " [" + status + "] " + std::to_string(value);

    write_log(line);
}


Logger::Logger(const std::string &file) : log_file(file) {}
