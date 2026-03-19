#include <iostream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <poll.h>
#include <vector>
#include <map>
#include <algorithm>
#include <memory>
#include <signal.h>
#include <ctime>
#include <sstream>
#include <fstream>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <mntent.h>
#include <iomanip>

// ==================== Класс Client ====================
class Client {
private:
    int fd_;
    struct sockaddr_in addr_;
    
public:
    Client(int fd, const struct sockaddr_in& addr);
    ~Client();
    
    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&& other) noexcept;
    
    int getFd() const;
    std::string getAddress() const;
    int getPort() const;
    
    ssize_t send(const std::string& message) const;
    std::string receive();
};

Client::Client(int fd, const struct sockaddr_in& addr) : fd_(fd), addr_(addr) {
    std::cout << "[+] Новый клиент: fd=" << fd_ 
              << " [" << getAddress() << ":" << getPort() << "]" << std::endl;
}

Client::~Client() {
    if (fd_ >= 0) {
        std::cout << "[-] Клиент отключен: fd=" << fd_ 
                  << " [" << getAddress() << ":" << getPort() << "]" << std::endl;
        close(fd_);
    }
}

Client::Client(Client&& other) noexcept : fd_(other.fd_), addr_(other.addr_) {
    other.fd_ = -1;
}

int Client::getFd() const { return fd_; }
std::string Client::getAddress() const { return inet_ntoa(addr_.sin_addr); }
int Client::getPort() const { return ntohs(addr_.sin_port); }

ssize_t Client::send(const std::string& message) const {
    return ::send(fd_, message.c_str(), message.length(), MSG_NOSIGNAL);
}

std::string Client::receive() {
    char buf[4096];
    int bytes_read = recv(fd_, buf, sizeof(buf) - 1, 0);
    
    if (bytes_read <= 0) {
        return (bytes_read == 0) ? "DISCONNECT" : "ERROR";
    }
    
    buf[bytes_read] = '\0';
    return std::string(buf, bytes_read);
}

// ==================== Класс TCPServer ====================
class TCPServer {
private:
    int server_fd_;
    int port_;
    bool listening_;
    
public:
    TCPServer(int port);
    ~TCPServer();
    
    TCPServer(const TCPServer&) = delete;
    TCPServer& operator=(const TCPServer&) = delete;
    
    bool start();
    std::unique_ptr<Client> accept();
    
    int getFd() const;
    int getPort() const;
};

TCPServer::TCPServer(int port) : server_fd_(-1), port_(port), listening_(false) {}

TCPServer::~TCPServer() {
    if (server_fd_ >= 0) {
        close(server_fd_);
        std::cout << "[*] Сервер на порту " << port_ << " остановлен" << std::endl;
    }
}

bool TCPServer::start() {
    server_fd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd_ < 0) {
        perror("socket");
        return false;
    }
    
    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
    }
    
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(server_fd_, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }
    
    if (listen(server_fd_, 10) < 0) {
        perror("listen");
        close(server_fd_);
        server_fd_ = -1;
        return false;
    }
    
    std::cout << "[*] Сервер запущен на порту " << port_ << " (fd=" << server_fd_ << ")" << std::endl;
    listening_ = true;
    return true;
}

std::unique_ptr<Client> TCPServer::accept() {
    sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    int client_fd = ::accept(server_fd_, (struct sockaddr*)&client_addr, &client_len);
    if (client_fd < 0) {
        return nullptr;
    }
    
    return std::make_unique<Client>(client_fd, client_addr);
}

int TCPServer::getFd() const { return server_fd_; }
int TCPServer::getPort() const { return port_; }

// ==================== Класс ClientManager ====================
class ClientManager {
private:
    std::map<int, std::vector<std::unique_ptr<Client>>> clients_by_port_;
    
public:
    void addClient(int port, std::unique_ptr<Client> client);
    void removeClient(int client_fd);
    Client* findClient(int client_fd);
    int findClientPort(int client_fd);
    size_t getClientCount() const;
    std::vector<Client*> getClientsOnPort(int port);
};

void ClientManager::addClient(int port, std::unique_ptr<Client> client) {
    clients_by_port_[port].push_back(std::move(client));
}

void ClientManager::removeClient(int client_fd) {
    for (auto& pair : clients_by_port_) {
        auto& clients = pair.second;
        auto it = std::find_if(clients.begin(), clients.end(),
            [client_fd](const auto& c) { return c->getFd() == client_fd; });
        
        if (it != clients.end()) {
            clients.erase(it);
            if (clients.empty()) {
                clients_by_port_.erase(pair.first);
            }
            return;
        }
    }
}

Client* ClientManager::findClient(int client_fd) {
    for (auto& pair : clients_by_port_) {
        for (auto& client : pair.second) {
            if (client->getFd() == client_fd) {
                return client.get();
            }
        }
    }
    return nullptr;
}

int ClientManager::findClientPort(int client_fd) {
    for (const auto& pair : clients_by_port_) {
        for (const auto& client : pair.second) {
            if (client->getFd() == client_fd) {
                return pair.first;
            }
        }
    }
    return -1;
}

size_t ClientManager::getClientCount() const {
    size_t count = 0;
    for (const auto& pair : clients_by_port_) {
        count += pair.second.size();
    }
    return count;
}

std::vector<Client*> ClientManager::getClientsOnPort(int port) {
    std::vector<Client*> result;
    auto it = clients_by_port_.find(port);
    if (it != clients_by_port_.end()) {
        for (const auto& client : it->second) {
            result.push_back(client.get());
        }
    }
    return result;
}

// ==================== Класс MessageForwarder ====================
class MessageForwarder {
private:
    ClientManager& client_manager_;
    int port1_;
    int port2_;
    
public:
    MessageForwarder(ClientManager& cm, int p1, int p2);
    void forwardMessage(int sender_fd, const std::string& message);
};

MessageForwarder::MessageForwarder(ClientManager& cm, int p1, int p2) 
    : client_manager_(cm), port1_(p1), port2_(p2) {}

void MessageForwarder::forwardMessage(int sender_fd, const std::string& message) {
    int sender_port = client_manager_.findClientPort(sender_fd);
    if (sender_port == -1) return;
    
    int target_port = (sender_port == port1_) ? port2_ : port1_;
    auto target_clients = client_manager_.getClientsOnPort(target_port);
    
    if (target_clients.empty()) {
        return;
    }
    
    std::cout << "\nПересылка " << message.length() << " байт: "
              << "порт " << sender_port << " → порт " << target_port << std::endl;
    
    for (auto* client : target_clients) {
        ssize_t sent = client->send(message);
        if (sent > 0) {
            std::cout << "    Отправлено клиенту fd=" << client->getFd() 
                     << " (" << sent << " байт)" << std::endl;
        }
    }
}

// ==================== Класс SystemInfo ====================
class SystemInfo {
public:
    static std::string getTime();
    static std::string getCpuLoad();
    static std::string getCpuTemp();
    static std::string getRamInfo();
    static std::string getRomInfo();
};

std::string SystemInfo::getTime() {
    time_t now = time(nullptr);
    std::string time_str = ctime(&now);
    time_str.pop_back(); // убираем перевод строки
    return "Server time: " + time_str;
}

// Получение загрузки CPU (load average)
std::string SystemInfo::getCpuLoad() {
    std::ifstream file("/proc/loadavg");
    if (!file.is_open()) return "Ошибка";
    double load1, load5, load15;
    file >> load1 >> load5 >> load15;
    std::ostringstream oss;
    oss << "Load average: 1min=" << load1 << ", 5min=" << load5 << ", 15min=" << load15;
    return oss.str();
}

// Получение температуры CPU (для Raspberry Pi)
std::string SystemInfo::getCpuTemp() {
    std::ifstream file("/sys/class/thermal/thermal_zone0/temp");
    if (!file.is_open()) {
        return "Ошибка: температура недоступна (не Raspberry Pi?)";
    }
    int temp_raw;
    file >> temp_raw;
    file.close();
    double temp_celsius = temp_raw / 1000.0;
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << temp_celsius << " °C";
    return oss.str();
}

// Получение информации о RAM
std::string SystemInfo::getRamInfo() {
    struct sysinfo si;
    if (sysinfo(&si) != 0) {
        return "Ошибка: sysinfo failed";
    }
    // Конвертация в мегабайты
    long total_mb = si.totalram * si.mem_unit / (1024 * 1024);
    long free_mb = si.freeram * si.mem_unit / (1024 * 1024);
    long used_mb = total_mb - free_mb;
    std::ostringstream oss;
    oss << "RAM: всего " << total_mb << " MB, используется " << used_mb
        << " MB, свободно " << free_mb << " MB";
    return oss.str();
}

// Получение информации о дисковом пространстве корневой ФС (ROM/SD карта)
std::string SystemInfo::getRomInfo() {
    struct statvfs stat;
    if (statvfs("/", &stat) != 0) {
        return "Ошибка: statvfs для /";
    }
    unsigned long total = stat.f_blocks * stat.f_frsize;
    unsigned long free = stat.f_bfree * stat.f_frsize;
    unsigned long avail = stat.f_bavail * stat.f_frsize;
    unsigned long used = total - free;

    // Перевод в гигабайты для наглядности
    double total_gb = total / (1024.0 * 1024.0 * 1024.0);
    double used_gb = used / (1024.0 * 1024.0 * 1024.0);
    double avail_gb = avail / (1024.0 * 1024.0 * 1024.0);

    std::ostringstream oss;
    oss << "Корневая ФС: всего " << std::fixed << std::setprecision(2) << total_gb
        << " GB, используется " << used_gb << " GB, доступно " << avail_gb << " GB";
    return oss.str();
}

// ==================== Класс PollLoop ====================
class PollLoop {
private:
    std::vector<struct pollfd> poll_fds_;
    std::map<int, std::unique_ptr<TCPServer>> servers_;
    ClientManager& client_manager_;
    MessageForwarder& forwarder_;
    bool running_;
    
    void handleInput(int fd);
    void handleNewConnection(TCPServer* server);
    void handleClientMessage(int client_fd);
    void handleError(int fd);
    void handleServerCommand(Client* client, const std::string& command);
    
public:
    PollLoop(ClientManager& cm, MessageForwarder& fwd);
    void addServer(std::unique_ptr<TCPServer> server);
    void addClient(int client_fd);
    void removeFd(int fd);
    void run();
    void stop();
};

PollLoop::PollLoop(ClientManager& cm, MessageForwarder& fwd) 
    : client_manager_(cm), forwarder_(fwd), running_(false) {}

void PollLoop::addServer(std::unique_ptr<TCPServer> server) {
    int fd = server->getFd();
    servers_[fd] = std::move(server);
    
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN;
    poll_fds_.push_back(pfd);
}

void PollLoop::addClient(int client_fd) {
    struct pollfd pfd;
    pfd.fd = client_fd;
    pfd.events = POLLIN;
    poll_fds_.push_back(pfd);
}

void PollLoop::removeFd(int fd) {
    auto it = std::find_if(poll_fds_.begin(), poll_fds_.end(),
        [fd](const auto& pfd) { return pfd.fd == fd; });
    if (it != poll_fds_.end()) {
        poll_fds_.erase(it);
    }
}

void PollLoop::run() {
    running_ = true;
    
    while (running_) {
        int ret = poll(poll_fds_.data(), poll_fds_.size(), -1);
        
        if (ret < 0) {
            if (errno == EINTR) continue;
            perror("poll");
            break;
        }
        
        for (size_t i = 0; i < poll_fds_.size(); i++) {
            if (poll_fds_[i].revents & POLLIN) {
                handleInput(poll_fds_[i].fd);
            }
            else if (poll_fds_[i].revents & (POLLERR | POLLHUP | POLLNVAL)) {
                handleError(poll_fds_[i].fd);
            }
        }
    }
}

void PollLoop::stop() {
    running_ = false;
}

void PollLoop::handleInput(int fd) {
    auto server_it = servers_.find(fd);
    if (server_it != servers_.end()) {
        handleNewConnection(server_it->second.get());
    } else {
        handleClientMessage(fd);
    }
}

void PollLoop::handleNewConnection(TCPServer* server) {
    auto client = server->accept();
    if (client) {
        int client_fd = client->getFd();
        client_manager_.addClient(server->getPort(), std::move(client));
        addClient(client_fd);
    }
}

void PollLoop::handleClientMessage(int client_fd) {
    Client* client = client_manager_.findClient(client_fd);
    if (!client) return;
    
    std::string message = client->receive();
    
    if (message == "DISCONNECT") {
        removeFd(client_fd);
        client_manager_.removeClient(client_fd);
    }
    else if (message == "ERROR") {
        std::cerr << "[!] Ошибка чтения от клиента fd=" << client_fd << std::endl;
        removeFd(client_fd);
        client_manager_.removeClient(client_fd);
    }
    else {
        std::cout << "Получено от fd=" << client_fd 
                  << " (" << message.length() << " байт)" << std::endl;
        std::cout << "    Содержимое: " << message << std::endl;
        
        // Проверяем, является ли сообщение командой серверу
        if (message.rfind("/server", 0) == 0) {
            handleServerCommand(client, message);
        } else {
            forwarder_.forwardMessage(client_fd, message);
        }
    }
}

void PollLoop::handleError(int fd) {
    if (servers_.find(fd) == servers_.end()) {
        removeFd(fd);
        client_manager_.removeClient(fd);
    }
}

void PollLoop::handleServerCommand(Client* client, const std::string& command) {
    std::cout << "    [Команда серверу] " << command << std::endl;

    // Разбиваем команду на части
    std::istringstream iss(command);
    std::string token;
    std::vector<std::string> tokens;
    while (iss >> token) {
        tokens.push_back(token);
    }

    if (tokens.size() < 2 || tokens[0] != "/server") {
        client->send("Неверный формат команды");
        return;
    }

    std::string subcmd = tokens[1];
    std::string response;

    if (subcmd == "time") {
        response = SystemInfo::getTime();
    }
    else if (subcmd == "cpu") {
        response = SystemInfo::getCpuLoad();
    }
    else if (subcmd == "cpu_temp") {
        response = SystemInfo::getCpuTemp();
    }
    else if (subcmd == "ram") {
        response = SystemInfo::getRamInfo();
    }
    else if (subcmd == "rom") {
        response = SystemInfo::getRomInfo();
    }
    else {
        response = "Неизвестная команда серверу. Доступные: time, cpu, cpu_temp, ram, rom";
    }

    client->send(response);
    std::cout << "    Отправлен ответ клиенту fd=" << client->getFd() << std::endl;
}

// ==================== Глобальные переменные для сигналов ====================
static PollLoop* g_poll_loop = nullptr;

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        std::cout << "\n\nПолучен сигнал завершения..." << std::endl;
        if (g_poll_loop) {
            g_poll_loop->stop();
        }
    }
}

// ==================== Класс Application ====================
class Application {
private:
    static constexpr int PORT_A = 7000;
    static constexpr int PORT_B = 7021;
    
    ClientManager client_manager_;
    MessageForwarder forwarder_;
    PollLoop poll_loop_;
    
public:
    Application();
    ~Application();
    
    bool initialize();
    void run();
    void shutdown();
};

Application::Application() 
    : forwarder_(client_manager_, PORT_A, PORT_B)
    , poll_loop_(client_manager_, forwarder_) {
    
    g_poll_loop = &poll_loop_;
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
}

Application::~Application() {
    g_poll_loop = nullptr;
}

bool Application::initialize() {
    std::cout << "TCP FORWARD SERVER" << std::endl;
    
    auto server1 = std::make_unique<TCPServer>(PORT_A);
    auto server2 = std::make_unique<TCPServer>(PORT_B);
    
    if (!server1->start() || !server2->start()) {
        std::cerr << "Ошибка инициализации серверов" << std::endl;
        return false;
    }
    
    poll_loop_.addServer(std::move(server1));
    poll_loop_.addServer(std::move(server2));
    
    return true;
}

void Application::run() {
    std::cout << "\nСервер готов к работе" << std::endl;
    std::cout << "Порт A: " << PORT_A << std::endl;
    std::cout << "Порт B: " << PORT_B << std::endl;
    std::cout << "Режим: пересылка A ↔ B" << std::endl;
    std::cout << "Для выхода нажмите Ctrl+C\n" << std::endl;
    
    poll_loop_.run();
}

void Application::shutdown() {
    std::cout << "\nЗавершение работы..." << std::endl;
    poll_loop_.stop();
    std::cout << "Сервер остановлен" << std::endl;
}

// ==================== Функция main ====================
int main() {
    Application app;
    
    if (app.initialize()) {
        app.run();
        app.shutdown();
    }
    
    return 0;
}
