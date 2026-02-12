#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

// ============================================================================
// Класс Client
// ============================================================================

class Client {
public:
    Client(const std::string &ip, int server_port);
    ~Client();
    int connect_and_run();

private:
    int client_socket;
    std::string server_ip;
    int port;
    std::string name;
    struct sockaddr_in6 server_addr;

private:
    int initialize_socket();
    void initialize_address();
    int validate_server_address();
    int connect_to_server();
    void request_name();
    bool reconnect();
    void receive_messages(); 
    void send_messages();
};

// ============================================================================
// Главная функция
// ============================================================================

int main(int argc, char* argv[]) {
    std::string server_ip = "2a03:d000:105:7010:92de:80ff:febe:590b"; // localhost для IPv6
    int port = 8080;
    
    if (argc > 1) {
        server_ip = argv[1];
    }
    if (argc > 2) {
        port = std::stoi(argv[2]);
    }
    
    Client client(server_ip, port);
    return client.connect_and_run();
}

Client::Client(const std::string &ip, int server_port) 
    : client_socket(-1), server_ip(ip), port(server_port) {
        memset(&server_addr, 0, sizeof(server_addr));
    }


Client::~Client() {
    if (client_socket >= 0) {
        close(client_socket);
    }
}


int Client::initialize_socket() {
    // Создаем сокет для IPv6
    client_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (client_socket < 0) {
        std::cerr << "Ошибка создания сокета" << std::endl;
        return 1;
    }
    return 0;
}


void Client::initialize_address() {
    // Настраиваем адрес
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(port);
}


int Client::validate_server_address() {
    if (inet_pton(AF_INET6, server_ip.c_str(), &server_addr.sin6_addr) <= 0) {
        std::cerr << "Неверный адрес сервера" << std::endl;
        close(client_socket);
        return 1;
    }
    return 0;
}


int Client::connect_to_server() {
    // Подключаемся к серверу
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Ошибка подключения к серверу" << std::endl;
        close(client_socket);
        return 1;
    }
    return 0;
}


void Client::request_name() {
    // Запрашиваем имя
    std::cout << "Введите ваше имя: ";
    std::getline(std::cin, name);
    
    // Отправляем имя серверу
    send(client_socket, name.c_str(), name.length(), 0);
    
    std::cout << "\nВы вошли в чат как: " << name << std::endl;
    std::cout << "Начните вводить сообщения (Ctrl+C для выхода):\n" << std::endl;
}


bool Client::reconnect() {
    auto start_time = std::chrono::steady_clock::now();
    int attempt = 1;

    std::cout << "\n[Попытка переподключения к серверу]" << std::endl;

    while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(30)) {
        shutdown(client_socket, SHUT_RDWR);
        close(client_socket);
        
        if (initialize_socket() != 0) return false;
        
        initialize_address();
        if (validate_server_address() != 0) return false;

        std::cout << "[Попытка #" << attempt++ << "]" << std::endl;
        
        if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0) {
            std::cout << "[Переподключение успешно]" << std::endl;
            return true;
        }
        
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    std::cout << "[Не удалось переподключиться за 30 секунд]" << std::endl;
    return false;
}


void Client::receive_messages() {
    char buffer[1024];
    
    while (true) {
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received <= 0) {
            std::cout << "\n[Соединение с сервером потеряно]" << std::endl;
            
            if (reconnect()) {
                send(client_socket, name.c_str(), name.length(), 0);
                std::cout << "[Вы снова в чате как: " << name << " ]" << std::endl;
            } else {
                exit(1);
            }
            continue;
        }
        
        buffer[bytes_received] = '\0';
        std::cout << "\r\033[K";
        std::cout << buffer;
        // Очищаем строку снова перед выводом приглашения
        std::cout << "\033[K";          
        std::cout << "[Введите сообщение:] ";
        std::cout.flush();
    }
}


void Client::send_messages() {
    // Запускаем поток для получения сообщений
    std::thread receive_thread(&Client::receive_messages, this);
    receive_thread.detach();
    
    // Отправляем сообщения
    std::string message;
    while (true) {
        std::cout << "[Введите сообщение:] ";
        std::cout.flush();

        if(!std::getline(std::cin, message)){
            break;
        }

        if (!message.empty()) {
            message += "\n";
            if (send(client_socket, message.c_str(), message.length(), 0) < 0) {
                std::cerr << "Ошибка отправки сообщения" << std::endl;
                break;
            }
        }
    }
}

int Client::connect_and_run() {
    // Создаем сокет для IPv6
    if (initialize_socket() != 0) return 1;
    
    // Настраиваем адрес сервера
    initialize_address();

    if (validate_server_address() != 0) return 1;
    if (connect_to_server() != 0) return 1;
    
    std::cout << "=== КЛИЕНТ ЧАТА ===" << std::endl;
    std::cout << "Подключено к серверу!" << std::endl;

    request_name();
    send_messages();
    
    return 0;
}
