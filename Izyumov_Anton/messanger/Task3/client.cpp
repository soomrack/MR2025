#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <functional>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <fstream>
#include <ctime>
#include <netinet/tcp.h>

namespace ClientThresholds {
    constexpr double CPU_LOAD_WARN    = 4.0;   // load average 1min
    constexpr double CPU_TEMP_WARN    = 30.0;  // °C
    constexpr double RAM_USED_WARN    = 90.0;  // процент использования
    constexpr double ROM_USED_WARN    = 1.0;  // процент использования
    constexpr int JOYSTICK_X_MAX_WARN = 700;   // верхний порог для оси X
    constexpr int JOYSTICK_X_MIN_WARN = 10;    // нижний порог для оси X
    constexpr int JOYSTICK_Y_MAX_WARN = 500;   // верхний порог для оси Y
    constexpr int JOYSTICK_Y_MIN_WARN = 20;    // нижний порог для оси Y
}

// ==================== Класс Socket ====================
class Socket {
private:
    int fd_;
    
public:
    Socket();
    explicit Socket(int fd);
    ~Socket();
    
    Socket(const Socket&) = delete;
    Socket& operator=(const Socket&) = delete;
    
    Socket(Socket&& other) noexcept;
    Socket& operator=(Socket&& other) noexcept;
    
    bool create();
    bool connect(const std::string& ip, int port);
    int send(const std::string& data) const;
    std::string receive();
    void close();
    
    int getFd() const;
    bool isValid() const;
};

Socket::Socket() : fd_(-1) {}

Socket::Socket(int fd) : fd_(fd) {}

Socket::~Socket() {
    close();
}

Socket::Socket(Socket&& other) noexcept : fd_(other.fd_) {
    other.fd_ = -1;
}

Socket& Socket::operator=(Socket&& other) noexcept {
    if (this != &other) {
        close();
        fd_ = other.fd_;
        other.fd_ = -1;
    }
    return *this;
}

bool Socket::create() {
    fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
    return fd_ >= 0;
}

bool Socket::connect(const std::string& ip, int port) {
    if (fd_ < 0) return false;
    
    int flag = 1;
    setsockopt(fd_, IPPROTO_TCP, TCP_NODELAY, &flag, sizeof(flag));
    
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    
    return ::connect(fd_, (struct sockaddr*)&addr, sizeof(addr)) == 0;
}

int Socket::send(const std::string& data) const {
    if (fd_ < 0) return -1;
    return ::send(fd_, data.c_str(), data.length(), 0);
}

std::string Socket::receive() {
    if (fd_ < 0) return "ERROR";
    
    char buffer[4096];
    int bytes = ::recv(fd_, buffer, sizeof(buffer) - 1, 0);
    
    if (bytes <= 0) {
        return (bytes == 0) ? "DISCONNECT" : "ERROR";
    }
    
    buffer[bytes] = '\0';
    return std::string(buffer, bytes);
}

void Socket::close() {
    if (fd_ >= 0) {
        ::close(fd_);
        fd_ = -1;
    }
}

int Socket::getFd() const { return fd_; }
bool Socket::isValid() const { return fd_ >= 0; }

// ==================== Класс Message ====================
class Message {
private:
    std::string content_;
    std::string sender_;
    std::chrono::system_clock::time_point timestamp_;
    
public:
    Message() = default;
    Message(const std::string& sender, const std::string& content);
    
    std::string format() const;
    std::string toString() const;
    std::string getContent() const;
    std::string getSender() const;
};

Message::Message(const std::string& sender, const std::string& content)
    : sender_(sender), content_(content), timestamp_(std::chrono::system_clock::now()) {}

std::string Message::format() const {
    return "[" + sender_ + "]: " + content_;
}

std::string Message::toString() const {
    auto time_t = std::chrono::system_clock::to_time_t(timestamp_);
    char time_str[26];
    ctime_r(&time_t, time_str);
    time_str[24] = '\0'; // Убираем перевод строки
    
    return "[" + std::string(time_str) + "] " + format();
}

std::string Message::getContent() const { return content_; }
std::string Message::getSender() const { return sender_; }

// ==================== Класс MessageReceiver ====================
class MessageReceiver {
private:
    Socket& socket_;
    std::atomic<bool> running_;
    std::thread receiver_thread_;
    std::function<void(const Message&)> message_callback_;
    std::function<void(const std::string&)> error_callback_;
    
public:
    explicit MessageReceiver(Socket& socket);
    ~MessageReceiver();
    
    void setMessageCallback(std::function<void(const Message&)> cb);
    void setErrorCallback(std::function<void(const std::string&)> cb);
    bool start();
    void stop();
    
private:
    void receiveLoop();
};

MessageReceiver::MessageReceiver(Socket& socket) 
    : socket_(socket), running_(false) {}

MessageReceiver::~MessageReceiver() {
    stop();
}

void MessageReceiver::setMessageCallback(std::function<void(const Message&)> cb) {
    message_callback_ = cb;
}

void MessageReceiver::setErrorCallback(std::function<void(const std::string&)> cb) {
    error_callback_ = cb;
}

bool MessageReceiver::start() {
    if (running_) return false;

    if (receiver_thread_.joinable()) {
        receiver_thread_.join();
    }
    
    running_ = true;
    receiver_thread_ = std::thread(&MessageReceiver::receiveLoop, this);
    return true;
}

void MessageReceiver::stop() {
    if (running_) {
        running_ = false;
        if (std::this_thread::get_id() != receiver_thread_.get_id()) {
            if (receiver_thread_.joinable()) {
                receiver_thread_.join();
            }
        }
    }
}

void MessageReceiver::receiveLoop() {
    while (running_) {
        std::string data = socket_.receive();
        
        if (data == "DISCONNECT") {
            if (error_callback_) {
                error_callback_("Соединение с сервером разорвано");
            }
            break;
        }
        else if (data == "ERROR") {
            if (error_callback_) {
                error_callback_("Ошибка получения данных");
            }
            break;
        }
        else if (!data.empty()) {
            Message msg("Server", data);
            if (message_callback_) {
                message_callback_(msg);
            }
        }
        
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// ==================== Класс ConsoleUI ====================
class ConsoleUI {
private:
    std::mutex cout_mutex_;
    
public:
    void printMessage(const Message& msg);
    void printError(const std::string& error);
    void printInfo(const std::string& info);
    void printSystem(const std::string& msg);
    void printPrompt();
    void clearLine();
    void printWarning(const std::string& warning);
    void printRaw(const std::string& text);
};

void ConsoleUI::printMessage(const Message& msg) {
    std::lock_guard<std::mutex> lock(cout_mutex_);
    std::cout << "[Получено] " << msg.toString() << std::endl;
    std::cout << "Введите сообщение: ";
    std::cout.flush();
}

void ConsoleUI::printError(const std::string& error) {
    std::lock_guard<std::mutex> lock(cout_mutex_);
    std::cout << "[Ошибка] " << error << std::endl;
}

void ConsoleUI::printInfo(const std::string& info) {
    std::lock_guard<std::mutex> lock(cout_mutex_);
    std::cout << "[Инфо] " << info << std::endl;
}

void ConsoleUI::printSystem(const std::string& msg) {
    std::lock_guard<std::mutex> lock(cout_mutex_);
    std::cout << "[Система] " << msg << std::endl;
}

void ConsoleUI::printPrompt() {
    std::lock_guard<std::mutex> lock(cout_mutex_);
    std::cout << "Введите сообщение: ";
    std::cout.flush();
}

void ConsoleUI::clearLine() {
    std::cout << "\r\033[K";
    std::cout.flush();
}

void ConsoleUI::printWarning(const std::string& warning) {
    std::lock_guard<std::mutex> lock(cout_mutex_);
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time_t);
    std::cout << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] [ПРЕДУПРЕЖДЕНИЕ] " << warning << std::endl;
}

void ConsoleUI::printRaw(const std::string& text) {
    std::lock_guard<std::mutex> lock(cout_mutex_);
    std::cout << text << std::endl;
}

// ==================== Класс ClientConfig ====================
class ClientConfig {
private:
    std::string client_name_;
    
public:
    ClientConfig() = default;
    
    static ClientConfig readFromCommandLine(int argc, char* argv[]);
    
    std::string getClientName() const;
    void print() const;
};

ClientConfig ClientConfig::readFromCommandLine(int argc, char* argv[]) {
    ClientConfig config;
    
    if (argc >= 2) {
        config.client_name_ = argv[1];
    } else {
        std::cout << "Введите ваше имя: ";
        std::getline(std::cin, config.client_name_);
        if (config.client_name_.empty()) {
            config.client_name_ = "Anonymous";
        }
    }
    
    return config;
}

std::string ClientConfig::getClientName() const { return client_name_; }

void ClientConfig::print() const {
    std::cout << "      КОНФИГУРАЦИЯ КЛИЕНТА     " << std::endl;
    std::cout << " Имя клиента: " << client_name_ << std::endl;
}

// ==================== Класс TCPClient ====================
class TCPClient {
private:
    Socket socket_;
    MessageReceiver receiver_;
    ConsoleUI ui_;
    std::string client_name_;
    std::atomic<bool> connected_;
    
    std::string stickerToEmoji(const std::string& sticker);
    void replaceStickers(std::string& text);

    std::ofstream log_file_;
    bool openLogFile();
    void closeLogFile();
    void checkAndWarn(const std::string& response);
    
    static constexpr int METRICS_INTERVAL_SECONDS = 30; // интервал опроса метрик
    std::thread metrics_thread_;
    std::atomic<bool> metrics_running_;
    std::atomic<int> background_responses_left_{0};
    std::atomic<bool> background_active_{false};
    std::mutex send_mutex_;
    std::string last_metrics_filename_;
    std::ofstream metrics_file_;
    std::mutex bg_mutex_;
    std::condition_variable bg_cv_;

    void startMetricsLoop();
    void stopMetricsLoop();
    void metricsLoop();
    bool readMetricsFile(std::function<bool(const std::string&)> processor);
    void showAllMetrics();
    void showWarnings();
    void showMetricsSince(const std::string& datetime);
    void showMetricsRange(const std::string& startStr, const std::string& endStr);
    bool sendRawInternal(const std::string& data, bool silent, bool already_locked);
    void openMetricsFile();
    void closeMetricsFile();

public:
    TCPClient();
    ~TCPClient();
    
    bool initialize(const std::string& name);
    bool connectTo(const std::string& ip, int port);
    void disconnect();
    bool sendMessage(const std::string& text);
    bool sendRaw(const std::string& data, bool silent = false);
    bool isConnected() const;
    void run();
    
private:
    void onMessageReceived(const Message& msg);
    void onError(const std::string& error);
    bool handleLocalCommand(const std::string& input);
};

TCPClient::TCPClient() : receiver_(socket_), connected_(false), metrics_running_(false) {}

TCPClient::~TCPClient() {
    disconnect();
}

bool TCPClient::initialize(const std::string& name) {
    client_name_ = name;
    
    ui_.printSystem("Инициализация клиента " + client_name_);
    
    receiver_.setMessageCallback([this](const Message& msg) {
        onMessageReceived(msg);
    });
    
    receiver_.setErrorCallback([this](const std::string& err) {
        onError(err);
    });
    
    return true;
}

bool TCPClient::connectTo(const std::string& ip, int port) {
    disconnect();

    if (!socket_.create()) {
        ui_.printError("Не удалось создать сокет");
        return false;
    }
    
    ui_.printSystem("Подключение к " + ip + ":" + std::to_string(port) + "...");
    
    if (!socket_.connect(ip, port)) {
        ui_.printError("Не удалось подключиться к серверу");
        socket_.close();
        return false;
    }
    
    connected_ = true;
    ui_.printInfo("Успешное подключение к серверу!");
    
    // Открываем лог-файл для истории чата
    if (!openLogFile()) {
        ui_.printError("Не удалось создать лог-файл");
    }
    // Открываем файл для метрик
    openMetricsFile();
    
    receiver_.start();
    
    // Запускаем поток периодического опроса метрик
    startMetricsLoop();
    
    return true;
}

void TCPClient::disconnect() {
    stopMetricsLoop();
    if (!connected_ && !socket_.isValid()) return;
    connected_ = false;
    {
        std::lock_guard<std::mutex> lock(bg_mutex_);
        background_active_ = false;
    }
    bg_cv_.notify_all();

    if (socket_.isValid()) {
        ::shutdown(socket_.getFd(), SHUT_RDWR);
        socket_.close();
    }
    receiver_.stop();
    closeLogFile();
    closeMetricsFile();
    ui_.printSystem("Отключен от сервера");
}

bool TCPClient::sendMessage(const std::string& text) {
    if (!connected_ || !socket_.isValid()) {
        ui_.printError("Нет подключения к серверу");
        return false;
    }
    
    if (text.empty()) {
        return false;
    }
    
    // Заменяем стикеры в тексте
    std::string processedText = text;
    replaceStickers(processedText);
    
    Message msg(client_name_, processedText);
    std::string formatted = msg.format();
    
    int bytes_sent = socket_.send(formatted);
    if (bytes_sent <= 0) {
        ui_.printError("Ошибка отправки сообщения");
        disconnect();
        return false;
    }
    
    ui_.printInfo("Сообщение отправлено (" + std::to_string(bytes_sent) + " байт)");
    return true;
}

bool TCPClient::sendRawInternal(const std::string& data, bool silent, bool already_locked) {
    if (!already_locked) {
        send_mutex_.lock();
    }
    
    bool result = false;
    if (!connected_ || !socket_.isValid()) {
        if (!silent) ui_.printError("Нет подключения к серверу");
    } else {
        int bytes_sent = socket_.send(data);
        if (bytes_sent <= 0) {
            if (!silent) ui_.printError("Ошибка отправки команды");
            disconnect();
        } else {
            if (!silent) ui_.printInfo("Команда отправлена (" + std::to_string(bytes_sent) + " байт)");
            result = true;
        }
    }
    
    if (!already_locked) {
        send_mutex_.unlock();
    }
    return result;
}

bool TCPClient::sendRaw(const std::string& data, bool silent) {
    {
        std::unique_lock<std::mutex> lock(bg_mutex_);
        while (background_active_ && connected_) {
            bg_cv_.wait(lock);
        }
        if (!connected_) {
            if (!silent) ui_.printError("Нет подключения к серверу");
            return false;
        }
    }
    return sendRawInternal(data, silent, false);
}

bool TCPClient::isConnected() const {
    return connected_ && socket_.isValid();
}

void TCPClient::run() {
    ui_.printSystem("Клиент готов к работе. Введите /help для списка команд");
    
    std::string input;
    while (true) {
        ui_.printPrompt();
        std::getline(std::cin, input);
        
        if (std::cin.eof()) {
            break;
        }
        
        if (input.empty()) {
            continue;
        }
        
        if (handleLocalCommand(input)) {
            continue;
        }
        
        if (isConnected()) {
            sendMessage(input);
        } else {
            ui_.printError("Не подключен к серверу. Используйте /connect <ip> <port>");
        }
    }
    
    disconnect();
}

void TCPClient::onMessageReceived(const Message& msg) {
    // Если сообщение от сервера
    if (msg.getSender() == "Server") {
        std::string content = msg.getContent();
        
        // Если это фоновый опрос
        if (background_active_) {
            // Записываем в файл метрик
            if (metrics_file_.is_open()) {
                auto now = std::chrono::system_clock::now();
                auto time_t = std::chrono::system_clock::to_time_t(now);
                std::tm tm = *std::localtime(&time_t);
                metrics_file_ << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] " 
                              << content << std::endl;
            }
            // Проверяем пороги
            checkAndWarn(content);
            
            // Уменьшаем счетчик оставшихся ответов
            if (--background_responses_left_ == 0) {
                background_active_ = false;
            }
            // Не выводим в консоль
        } else {
            // Обычный ответ сервера (на ручную команду)
            ui_.printMessage(msg);               // вывод в консоль
            // Записываем в общий лог-файл
            if (log_file_.is_open()) {
                auto now = std::chrono::system_clock::now();
                auto time_t = std::chrono::system_clock::to_time_t(now);
                std::tm tm = *std::localtime(&time_t);
                log_file_ << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] [Server] " 
                          << content << std::endl;
            }
            // Проверяем пороги (предупреждения выведутся, если нужно)
            checkAndWarn(content);
        }
    } else {
        // Сообщение от другого клиента (переслано сервером)
        ui_.printMessage(msg);                   // вывод в консоль
        // Записываем в общий лог-файл
        if (log_file_.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            std::tm tm = *std::localtime(&time_t);
            log_file_ << "[" << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << "] " 
                      << msg.toString() << std::endl;
        }
    }
}

void TCPClient::onError(const std::string& error) {
    ui_.printError(error);
    disconnect();
}

bool TCPClient::handleLocalCommand(const std::string& input) {
    if (input.empty() || input[0] != '/') {
        return false;
    }
    
    std::istringstream iss(input);
    std::string cmd;
    iss >> cmd;
    
    if (cmd == "/help") {
        ui_.printSystem("Доступные команды:");
        ui_.printSystem("  /connect <ip> <port> - подключиться к серверу");
        ui_.printSystem("  /disconnect - отключиться от сервера");
        ui_.printSystem("  /server time - запросить время на сервере");
        ui_.printSystem("  /server cpu - загрузка процессора");
        ui_.printSystem("  /server cpu_temp - температура CPU");
        ui_.printSystem("  /server ram - использование оперативной памяти");
        ui_.printSystem("  /server rom - использование SD карты");
        ui_.printSystem("  /server joystick - получить значение джойстика с Arduino");
        ui_.printSystem("  /sticker <название> - отправить стикер (smile, sad, thumbs_up, thumbs_down)");
        ui_.printSystem("  /quit - выход из программы");
        ui_.printSystem("  /help - показать эту справку");
        ui_.printSystem("  /logs show - показать все метрики из файла");
        ui_.printSystem("  /logs warnings - показать предупреждения");
        ui_.printSystem("  /logs since YYYY-MM-DD HH:MM:SS - показать метрики с указанного времени");
        ui_.printSystem("  /logs from YYYY-MM-DD HH:MM:SS to YYYY-MM-DD HH:MM:SS - показать метрики за диапазон");
        return true;
    }
    
    if (cmd == "/quit" || cmd == "/exit" || cmd == "/q") {
        ui_.printSystem("Завершение работы...");
        disconnect();
        exit(0);
    }
    
    if (cmd == "/connect") {
        std::string ip;
        int port;
        if (!(iss >> ip >> port)) {
            ui_.printError("Использование: /connect <ip> <port>");
            return true;
        }
        connectTo(ip, port);
        return true;
    }
    
    if (cmd == "/disconnect") {
        disconnect();
        return true;
    }
    
    if (cmd == "/server") {
        std::string subcmd;
        iss >> subcmd;
        if (subcmd == "time" || subcmd == "cpu" || subcmd == "cpu_temp" || subcmd == "ram" || subcmd == "rom" || subcmd == "joystick") {
            if (!isConnected()) {
                ui_.printError("Нет подключения к серверу");
                return true;
            }
            sendRaw("/server " + subcmd);
            return true;
        } else {
            ui_.printError("Неизвестная подкоманда /server. Доступны: time, cpu, cpu_temp, ram, rom, joystick");
            return true;
        }
    }
    
    if (cmd == "/sticker") {
        std::string sticker;
        iss >> sticker;
        std::string emoji = stickerToEmoji(sticker);
        if (emoji.empty()) {
            ui_.printError("Неизвестный стикер. Доступны: smile, sad, thumbs_up, thumbs_down");
            return true;
        }
        if (!isConnected()) {
            ui_.printError("Нет подключения к серверу");
            return true;
        }
        // Отправляем стикер как отдельное сообщение (для совместимости)
        Message msg(client_name_, emoji);
        socket_.send(msg.format());
        ui_.printInfo("Стикер отправлен");
        return true;
    }

    if (cmd == "/logs") {
        std::string sub;
        iss >> sub;
        if (sub == "show" || sub.empty()) {
            showAllMetrics();
        } else if (sub == "warnings") {
            showWarnings();
        } else if (sub == "since") {
            std::string datetime;
            std::getline(iss, datetime);
            size_t first = datetime.find_first_not_of(" \t");
            if (first != std::string::npos) {
                datetime = datetime.substr(first);
            } else {
                ui_.printError("Укажите дату и время в формате: YYYY-MM-DD HH:MM:SS");
                return true;
            }
            showMetricsSince(datetime);
        } else if (sub == "from") {
            std::string rest;
            std::getline(iss, rest); // берём всю оставшуюся часть строки после "from"
            
            // Ищем разделитель " to "
            size_t to_pos = rest.find(" to ");
            if (to_pos == std::string::npos) {
                ui_.printError("Не найден разделитель 'to' между датами");
                return true;
            }
            
            // Извлекаем начальную и конечную даты
            std::string start = rest.substr(0, to_pos);
            std::string end = rest.substr(to_pos + 4); // длина " to " = 4
            
            // Убираем лишние пробелы в начале и конце
            auto trim = [](std::string& s) {
                s.erase(0, s.find_first_not_of(" \t"));
                s.erase(s.find_last_not_of(" \t") + 1);
            };
            trim(start);
            trim(end);
            
            if (start.empty() || end.empty()) {
                ui_.printError("Одна из дат не указана");
                return true;
            }
            
            showMetricsRange(start, end);
            return true;
        } else {
            ui_.printError("Неизвестная подкоманда /logs. Доступны: show, warnings, since <datetime>, from <start> to <end>");
        }
        return true;
    }
    
    ui_.printError("Неизвестная команда. Введите /help");
    return true;
}

std::string TCPClient::stickerToEmoji(const std::string& sticker) {
    if (sticker == "smile") return "😊";
    if (sticker == "sad") return "😢";
    if (sticker == "thumbs_up") return "👍";
    if (sticker == "thumbs_down") return "👎";
    return "";
}

void TCPClient::replaceStickers(std::string& text) {
    const std::string marker = "/sticker ";
    size_t pos = 0;
    
    while ((pos = text.find(marker, pos)) != std::string::npos) {
        // Начало имени сразу после маркера
        size_t name_start = pos + marker.length();
        if (name_start >= text.length()) break;
        
        // Собираем имя (буквы, цифры, подчёркивание)
        size_t name_end = name_start;
        while (name_end < text.length() && 
               (std::isalnum(text[name_end]) || text[name_end] == '_')) {
            ++name_end;
        }
        
        std::string sticker_name = text.substr(name_start, name_end - name_start);
        std::string emoji = stickerToEmoji(sticker_name);
        
        if (!emoji.empty()) {
            // Заменяем "/sticker имя" на emoji
            text.replace(pos, name_end - pos, emoji);
            pos += emoji.length(); // Продолжаем поиск после вставленного emoji
        } else {
            // Если имя не распознано, пропускаем этот маркер
            pos = name_end;
        }
    }
}

bool TCPClient::openLogFile() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time_t);
    std::ostringstream filename;
    filename << "client_log_" << client_name_ << "_"
             << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".log";
    log_file_.open(filename.str(), std::ios::out | std::ios::app);
    if (log_file_.is_open()) {
        log_file_ << "=== Лог клиента " << client_name_ << " ===" << std::endl;
        log_file_ << "Подключение к серверу в "
                  << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << std::endl;
        return true;
    }
    return false;
}

void TCPClient::closeLogFile() {
    if (log_file_.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&time_t);
        log_file_ << "Отключение от сервера в "
                  << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << std::endl;
        log_file_.close();
    }
}

void TCPClient::checkAndWarn(const std::string& response) {
    // Лямбда для извлечения первого числа из строки
    auto extractNumber = [](const std::string& str, size_t start_pos = 0) -> double {
        size_t start = str.find_first_of("0123456789-", start_pos);
        if (start == std::string::npos) return -1.0;
        size_t end = start;
        while (end < str.size() && (isdigit(str[end]) || str[end] == '.' || str[end] == '-')) {
            ++end;
        }
        try {
            return std::stod(str.substr(start, end - start));
        } catch (...) {
            return -1.0;
        }
    };

    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time_t);
    std::ostringstream time_oss;
    time_oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    std::string timeStr = time_oss.str();

    // Лямбда для записи предупреждения в соответствующий лог с временем
    auto logWarning = [&](const std::string& warnMsg) {
        if (background_active_.load()) {
            if (metrics_file_.is_open())
                metrics_file_ << "[" << timeStr << "] [WARN] " << warnMsg << std::endl;
        } else {
            if (log_file_.is_open())
                log_file_ << "[" << timeStr << "] [WARN] " << warnMsg << std::endl;
        }
    };

    // Проверка загрузки CPU
    if (response.find("Load average:") != std::string::npos) {
        size_t pos = response.find("1min=");
        if (pos != std::string::npos) {
            double val = extractNumber(response, pos + 5);
            if (val > 0 && val > ClientThresholds::CPU_LOAD_WARN) {
                std::ostringstream warn;
                warn << "Превышение порога загрузки CPU: " << val
                     << " (порог " << ClientThresholds::CPU_LOAD_WARN << ")";
                ui_.printWarning(warn.str());
                logWarning(warn.str());
            }
        }
    }
    // Проверка температуры CPU
    else if (response.find("°C") != std::string::npos) {
        double val = extractNumber(response);
        if (val > 0 && val > ClientThresholds::CPU_TEMP_WARN) {
            std::ostringstream warn;
            warn << "Превышение порога температуры CPU: " << val
                 << " °C (порог " << ClientThresholds::CPU_TEMP_WARN << " °C)";
            ui_.printWarning(warn.str());
            logWarning(warn.str());
        }
    }
    // Проверка RAM
    else if (response.find("RAM:") != std::string::npos) {
        size_t pos_total = response.find("всего ");
        size_t pos_used = response.find("используется ");
        if (pos_total != std::string::npos && pos_used != std::string::npos) {
            double total = extractNumber(response, pos_total + 6);
            double used = extractNumber(response, pos_used + 12);
            if (total > 0 && used >= 0) {
                double percent = (used / total) * 100.0;
                if (percent > ClientThresholds::RAM_USED_WARN) {
                    std::ostringstream warn;
                    warn << "Превышение порога использования RAM: " << std::fixed << std::setprecision(1)
                         << percent << "% (порог " << ClientThresholds::RAM_USED_WARN << "%)";
                    ui_.printWarning(warn.str());
                    logWarning(warn.str());
                }
            }
        }
    }
    // Проверка ROM
    else if (response.find("Корневая ФС:") != std::string::npos) {
        size_t pos_total = response.find("всего ");
        size_t pos_used = response.find("используется ");
        if (pos_total != std::string::npos && pos_used != std::string::npos) {
            double total = extractNumber(response, pos_total + 6);
            double used = extractNumber(response, pos_used + 12);
            if (total > 0 && used >= 0) {
                double percent = (used / total) * 100.0;
                if (percent > ClientThresholds::ROM_USED_WARN) {
                    std::ostringstream warn;
                    warn << "Превышение порога использования ROM: " << std::fixed << std::setprecision(1)
                         << percent << "% (порог " << ClientThresholds::ROM_USED_WARN << "%)";
                    ui_.printWarning(warn.str());
                    logWarning(warn.str());
                }
            }
        }
    }
    // Проверка джойстика
    else if (response.find("Джойстик X:") != std::string::npos) {
        // Извлекаем оба числа
        std::vector<double> values;
        size_t pos = 0;
        while ((pos = response.find_first_of("0123456789-", pos)) != std::string::npos) {
            size_t end = pos;
            while (end < response.size() && (isdigit(response[end]) || response[end] == '.' || response[end] == '-')) {
                ++end;
            }
            try {
                double val = std::stod(response.substr(pos, end - pos));
                values.push_back(val);
            } catch (...) {}
            pos = end;
        }
        if (values.size() >= 2) {
            double x = values[0];
            double y = values[1];
            
            // Проверка оси X
            if (x > ClientThresholds::JOYSTICK_X_MAX_WARN) {
                std::ostringstream warn;
                warn << "Превышение верхнего порога оси X: " << x
                    << " (порог " << ClientThresholds::JOYSTICK_X_MAX_WARN << ")";
                ui_.printWarning(warn.str());
                logWarning(warn.str());
            } else if (x < ClientThresholds::JOYSTICK_X_MIN_WARN) {
                std::ostringstream warn;
                warn << "Преодоление нижнего порога оси X: " << x
                    << " (порог " << ClientThresholds::JOYSTICK_X_MIN_WARN << ")";
                ui_.printWarning(warn.str());
                logWarning(warn.str());
            }
            
            // Проверка оси Y
            if (y > ClientThresholds::JOYSTICK_Y_MAX_WARN) {
                std::ostringstream warn;
                warn << "Превышение верхнего порога оси Y: " << y
                    << " (порог " << ClientThresholds::JOYSTICK_Y_MAX_WARN << ")";
                ui_.printWarning(warn.str());
                logWarning(warn.str());
            } else if (y < ClientThresholds::JOYSTICK_Y_MIN_WARN) {
                std::ostringstream warn;
                warn << "Преодоление нижнего порога оси Y: " << y
                    << " (порог " << ClientThresholds::JOYSTICK_Y_MIN_WARN << ")";
                ui_.printWarning(warn.str());
                logWarning(warn.str());
            }
        }
    }
}

void TCPClient::startMetricsLoop() {
    if (metrics_running_) return;
    metrics_running_ = true;
    metrics_thread_ = std::thread(&TCPClient::metricsLoop, this);
}

void TCPClient::stopMetricsLoop() {
    if (metrics_running_) {
        metrics_running_ = false;
        if (metrics_thread_.joinable()) {
            metrics_thread_.join();
        }
    }
}

void TCPClient::metricsLoop() {
    while (metrics_running_ && connected_) {
        // Захватываем мьютекс отправки только на время отправки команд
        {
            std::lock_guard<std::mutex> lock(send_mutex_);
            
            background_active_ = true;
            background_responses_left_ = 6;   // количество запросов
            
            // Отправляем все команды с увеличенной задержкой
            sendRawInternal("/server time", true, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            sendRawInternal("/server cpu", true, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            sendRawInternal("/server cpu_temp", true, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            sendRawInternal("/server ram", true, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            sendRawInternal("/server rom", true, true);
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            sendRawInternal("/server joystick", true, true);
        } // send_mutex_ отпущен, background_active_ = true
        
        // Ожидаем получения всех ответов с таймаутом
        auto start_time = std::chrono::steady_clock::now();
        const auto timeout = std::chrono::seconds(5);
        while (background_responses_left_ > 0 && connected_) {
            if (std::chrono::steady_clock::now() - start_time > timeout) {
                if (metrics_file_.is_open()) {
                    metrics_file_ << "[ERROR] Timeout waiting for " 
                                  << background_responses_left_ << " responses" << std::endl;
                }
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        
        // Завершаем фоновый опрос
        {
            std::lock_guard<std::mutex> lock(bg_mutex_);
            background_active_ = false;
        }
        bg_cv_.notify_all();   // пробуждаем ожидающие ручные команды
        
        // Интервал до следующего опроса
        for (int i = 0; i < METRICS_INTERVAL_SECONDS && metrics_running_ && connected_; ++i) {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}

void TCPClient::openMetricsFile() {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&time_t);
    std::ostringstream filename;
    filename << "metrics_" << client_name_ << "_"
             << std::put_time(&tm, "%Y%m%d_%H%M%S") << ".log";
    metrics_file_.open(filename.str(), std::ios::out | std::ios::app);
    last_metrics_filename_ = filename.str();
    if (metrics_file_.is_open()) {
        metrics_file_ << "=== Лог метрик сервера ===" << std::endl;
        metrics_file_ << "Подключение к серверу в "
                      << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << std::endl;
    }
}

void TCPClient::closeMetricsFile() {
    if (metrics_file_.is_open()) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        std::tm tm = *std::localtime(&time_t);
        metrics_file_ << "Отключение от сервера в "
                      << std::put_time(&tm, "%Y-%m-%d %H:%M:%S") << std::endl;
        metrics_file_.close();
    }
}

bool TCPClient::readMetricsFile(std::function<bool(const std::string&)> processor) {
    if (last_metrics_filename_.empty()) {
        ui_.printError("Нет файла метрик");
        return false;
    }
    std::ifstream file(last_metrics_filename_);
    if (!file.is_open()) {
        ui_.printError("Не удалось открыть файл метрик");
        return false;
    }
    std::string line;
    while (std::getline(file, line)) {
        if (!processor(line)) break;
    }
    return true;
}

void TCPClient::showAllMetrics() {
    readMetricsFile([this](const std::string& line) {
        ui_.printRaw(line);
        return true;
    });
}

void TCPClient::showWarnings() {
    readMetricsFile([this](const std::string& line) {
        if (line.find("[WARN]") != std::string::npos)
            ui_.printRaw(line);
        return true;
    });
}

void TCPClient::showMetricsSince(const std::string& datetime) {
    std::tm tm = {};
    std::istringstream ss(datetime);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) {
        ui_.printError("Неверный формат даты/времени. Используйте: YYYY-MM-DD HH:MM:SS");
        return;
    }
    time_t since = std::mktime(&tm);
    readMetricsFile([this, since](const std::string& line) {
        if (line.size() < 21 || line[0] != '[') return true;
        std::string time_str = line.substr(1, 19);
        std::tm line_tm = {};
        std::istringstream ss_time(time_str);
        ss_time >> std::get_time(&line_tm, "%Y-%m-%d %H:%M:%S");
        if (ss_time.fail()) return true;
        time_t line_time = std::mktime(&line_tm);
        if (line_time >= since) ui_.printRaw(line);
        return true;
    });
}

void TCPClient::showMetricsRange(const std::string& startStr, const std::string& endStr) {
    std::tm start_tm = {}, end_tm = {};
    std::istringstream ss_start(startStr);
    ss_start >> std::get_time(&start_tm, "%Y-%m-%d %H:%M:%S");
    std::istringstream ss_end(endStr);
    ss_end >> std::get_time(&end_tm, "%Y-%m-%d %H:%M:%S");
    if (ss_start.fail() || ss_end.fail()) {
        ui_.printError("Неверный формат даты/времени. Используйте: YYYY-MM-DD HH:MM:SS");
        return;
    }
    time_t start = std::mktime(&start_tm);
    time_t end = std::mktime(&end_tm);
    if (end < start) {
        ui_.printError("Конечная дата не может быть раньше начальной");
        return;
    }
    readMetricsFile([this, start, end](const std::string& line) {
        if (line.size() < 21 || line[0] != '[') return true;
        std::string time_str = line.substr(1, 19);
        std::tm line_tm = {};
        std::istringstream ss_time(time_str);
        ss_time >> std::get_time(&line_tm, "%Y-%m-%d %H:%M:%S");
        if (ss_time.fail()) return true;
        time_t line_time = std::mktime(&line_tm);
        if (line_time >= start && line_time <= end) {
            ui_.printRaw(line);
        }
        return true;
    });
}

// ==================== Класс Application ====================
class Application {
private:
    TCPClient client_;
    
public:
    int run(int argc, char* argv[]);
};

int Application::run(int argc, char* argv[]) {
    try {
        ClientConfig config = ClientConfig::readFromCommandLine(argc, argv);
        config.print();
        
        if (!client_.initialize(config.getClientName())) {
            std::cerr << "Ошибка инициализации клиента" << std::endl;
            return 1;
        }
        
        client_.run();
        
    } catch (const std::exception& e) {
        std::cerr << "Критическая ошибка: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}

// ==================== Функция main ====================
int main(int argc, char* argv[]) {
    Application app;
    return app.run(argc, argv);
}

