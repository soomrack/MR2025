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
    
    running_ = true;
    receiver_thread_ = std::thread(&MessageReceiver::receiveLoop, this);
    return true;
}

void MessageReceiver::stop() {
    if (running_) {
        running_ = false;
        if (receiver_thread_.joinable()) {
            receiver_thread_.join();
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
    
public:
    TCPClient();
    ~TCPClient();
    
    bool initialize(const std::string& name);
    bool connectTo(const std::string& ip, int port);
    void disconnect();
    bool sendMessage(const std::string& text);
    bool sendRaw(const std::string& data);
    bool isConnected() const;
    void run();
    
private:
    void onMessageReceived(const Message& msg);
    void onError(const std::string& error);
    bool handleLocalCommand(const std::string& input);
};

TCPClient::TCPClient() : receiver_(socket_), connected_(false) {}

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
    if (connected_) {
        ui_.printInfo("Уже подключен. Сначала выполните /disconnect");
        return false;
    }
    
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
    
    receiver_.start();
    
    return true;
}

void TCPClient::disconnect() {
    if (connected_) {
        connected_ = false;
        // Принудительно завершаем ожидание в recv
        if (socket_.isValid()) {
            ::shutdown(socket_.getFd(), SHUT_RDWR);
        }
        // Сначала закрываем сокет, чтобы прервать возможный блокирующий recv в потоке приёма
        socket_.close();
        // Затем останавливаем поток (join)
        receiver_.stop();
        ui_.printSystem("Отключен от сервера");
    }
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
        return false;
    }
    
    ui_.printInfo("Сообщение отправлено (" + std::to_string(bytes_sent) + " байт)");
    return true;
}

bool TCPClient::sendRaw(const std::string& data) {
    if (!connected_ || !socket_.isValid()) {
        ui_.printError("Нет подключения к серверу");
        return false;
    }
    
    int bytes_sent = socket_.send(data);
    if (bytes_sent <= 0) {
        ui_.printError("Ошибка отправки команды");
        return false;
    }
    
    ui_.printInfo("Команда отправлена (" + std::to_string(bytes_sent) + " байт)");
    return true;
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
    ui_.printMessage(msg);
}

void TCPClient::onError(const std::string& error) {
    ui_.printError(error);
    connected_ = false;
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
        ui_.printSystem("  /sticker <название> - отправить стикер (smile, sad, thumbs_up, thumbs_down)");
        ui_.printSystem("  /quit - выход из программы");
        ui_.printSystem("  /help - показать эту справку");
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
        if (subcmd == "time") {
            if (!isConnected()) {
                ui_.printError("Нет подключения к серверу");
                return true;
            }
            sendRaw("/server time");
            return true;
        } else {
            ui_.printError("Неизвестная подкоманда /server. Используйте /server time");
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

