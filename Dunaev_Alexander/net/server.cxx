#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <termios.h>

// ============================================================================
// Структуры данных
// ============================================================================

struct Client {
    int socket;
    std::string name;
};

// ============================================================================
// Глобальные переменные
// ============================================================================

std::vector<Client> clients;
std::mutex clients_mutex;

// ============================================================================
// Вспомогательные функции
// ============================================================================


std::string arduino_led_control(bool state) {
    int uart = open("/dev/ttyS1", O_RDWR, O_NOCTTY);

    if (uart == -1) {
        return "Ошибка открытия UART\n";
    }

    struct termios options;
    tcgetattr(uart, &options);
    cfsetispeed(&options, B9600);
    cfsetospeed(&options, B9600);
    options.c_cflag |= (CLOCAL | CREAD);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    tcsetattr(uart, TCSANOW, &options);

    char cmd = state ? '1' : '0';
    write(uart, &cmd, 1);

    close(uart);

    return state ? "Arduino LED включен\n" : \
        "Arduino LED выключен";

}



std::string led_control(bool state) {
    // PA8 = (0 * 32) + 8 = 8
    const int gpio_pin = 8;
    std::string gpio_path = "/sys/class/gpio/gpio" + std::to_string(gpio_pin);
    
    // Проверяем, экспортирован ли пин
    std::ifstream check(gpio_path + "/value");
    if (!check.is_open()) {
        // Экспортируем пин
        std::ofstream export_file("/sys/class/gpio/export");
        if (!export_file.is_open()) {
            return "Ошибка доступа к GPIO\n";
        }
        export_file << gpio_pin;
        export_file.close();
        
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        
        // Устанавливаем направление (output)
        std::ofstream direction(gpio_path + "/direction");
        if (direction.is_open()) {
            direction << "out";
            direction.close();
        } else {
            return "Ошибка установки направления GPIO\n";
        }
    }
    
    // Записываем значение
    std::ofstream value_file(gpio_path + "/value");
    if (!value_file.is_open()) {
        return "Не удалось записать значение на PA8\n";
    }
    
    value_file << (state ? "1" : "0");
    value_file.close();
    
    return state ? "Светодиод включен\n" : "Светодиод выключен\n";
}


std::string trim(const std::string &str) {
    size_t first = str.find_first_not_of("\t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of("\t\n\r");
    return str.substr(first, last - first + 1);
}

void print_server_info(const int &port) {
    std::cout << "=== СЕРВЕР ЧАТА ===" << std::endl;
    std::cout << "Сервер запущен на порту " << port << std::endl;
    std::cout << "Ожидание подключений..." << std::endl;
}

std::string get_OS_info() {
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
    return os_info;
}

std::string get_temp_info() {
    std::ifstream file("/sys/class/thermal/thermal_zone0/temp");
    std::string temp_str;
    
    if (!file.is_open()) {
        return "Не удалось получить температуру процессора\n";
    }
    
    std::getline(file, temp_str);
    
    if (temp_str.empty()) {
        return "Не удалось прочитать температуру\n";
    }
    
    // Температура в милиградусах, делим на 1000
    double temp = std::stod(temp_str) / 1000.0;
    
    return "Температура процессора: " + std::to_string(temp) + "°C\n";
}

// ============================================================================
// Класс Server
// ============================================================================

class Server {
private: 
    int server_socket;
    int port;
    struct sockaddr_in6 server_addr;

public:
    Server() : server_socket(-1), port(8080) {
        memset(&server_addr, 0, sizeof(server_addr));
    }

    ~Server() {
        if (server_socket >= 0) {
            close(server_socket);
        }
    }

    void set_port(int server_port) {
        port = server_port;
    }

    int get_port() const {
        return port;
    }

    int get_socket() const {
        return server_socket;
    }

    int initialize() {
        // Создаем сокет для IPv6
        server_socket = socket(AF_INET6, SOCK_STREAM, 0);
        if (server_socket < 0) {
            std::cerr << "Ошибка создания сокета" << std::endl;
            return 1;
        }
        
        // Настраиваем сокет
        int opt = 1;
        setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        // Настраиваем адрес
        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin6_family = AF_INET6;
        server_addr.sin6_addr = in6addr_any;
        server_addr.sin6_port = htons(port);
        
        return 0;
    }

    int bind_and_listen(int backlog = 10) {
        // Привязываем сокет
        if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Ошибка привязки сокета" << std::endl;
            close(server_socket);
            return 1;
        }
    
        // Начинаем слушать
        if (listen(server_socket, backlog) < 0) {
            std::cerr << "Ошибка прослушивания" << std::endl;
            close(server_socket);
            return 1;
        }
        
        return 0;
    }

    int accept_client() {
        struct sockaddr_in6 client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_socket < 0) {
            std::cerr << "Ошибка подключения клиента" << std::endl;
        } else {
            std::cout << "[СЕРВЕР] Новое подключение" << std::endl;
        }
        
        return client_socket;
    }
};

// ============================================================================
// Класс MessageHandler
// ============================================================================

class MessageHandler {
private:
    std::string get_color_for_name(const std::string &name) {
        int hash = 0;
        for (char c : name) {
            hash = hash * 31 + c;
        }

        const std::string colors[] = {
            "\033[31m", // красный
            "\033[32m", // зеленый
            "\033[33m", // желтый
            "\033[34m", // синий
            "\033[35m", // пурпурный
            "\033[36m", // голубой
        };

        return colors[abs(hash) % 6];
    }


    void broadcast_message(const std::string &message, int sender_socket) {
        std::lock_guard<std::mutex> lock(clients_mutex);
        for (const auto& client : clients) {
            if (client.socket != sender_socket) {
                send(client.socket, message.c_str(), message.length(), 0);
            }
        }
    }
    
    void send_to_client(const std::string &message, int client_socket) {
        send(client_socket, message.c_str(), message.length(), 0);
    }
    
    std::string receive_client_name(int client_socket) {
        char buffer[1024];
        int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received <= 0) {
            close(client_socket);
            return "";
        }
        
        buffer[bytes_received] = '\0';
        return std::string(buffer);
    }
    
    void add_client(int client_socket, const std::string &client_name) {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.push_back({client_socket, client_name});
    }
    
    void remove_client(int client_socket) {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(
            std::remove_if(clients.begin(), clients.end(),
                [client_socket](const Client& c) { return c.socket == client_socket; }),
            clients.end()
        );
    }
    
    void notify_join(int client_socket, const std::string &client_name) {
        std::cout << "[СЕРВЕР] " << client_name << " присоединился к чату" << std::endl;
        std::string join_msg = "[СИСТЕМА] " + client_name + " присоединился к чату\n";
        broadcast_message(join_msg, client_socket);
    }
    
    void notify_leave(int client_socket, const std::string &client_name) {
        std::cout << "[СЕРВЕР] " << client_name << " отключился" << std::endl;
        std::string leave_msg = "[СИСТЕМА] " + client_name + " покинул чат\n";
        broadcast_message(leave_msg, -1);
        close(client_socket);
    }
    
    void process_command(const std::string &command, int client_socket, const std::string &pre_message) {
        if (command == "/hello\n") {
            std::cout << "всем общий привет\n" << std::endl;
            broadcast_message("всем общий привет\n", client_socket);
        } else if (command == "/megahello\n") {
            std::cout << "ВСЕМ ОБЩИЙ МЕГАПРИВЕТ\n" << std::endl;
            broadcast_message("ВСЕМ ОБЩИЙ МЕГАПРИВЕТ\n", client_socket);
        } else if (command == "/smile\n") {
            std::string message = pre_message + ":-)\n";
            std::cout << message;
            broadcast_message(message, client_socket);
        } else if (command == "/os_info\n") {
            send_to_client(get_OS_info(), client_socket);
        } else if (command == "/temp_info\n") {
            send_to_client(get_temp_info(), client_socket);
        } else if (command == "/led_on\n") {
            send_to_client(led_control(true), client_socket);
        } else if (command == "/led_off\n") {
            send_to_client(led_control(false), client_socket);
        } else if (command == "/Aled_on\n") {
            send_to_client(arduino_led_control(true), client_socket);
        } else if (command == "/Aled_off\n") {
            send_to_client(arduino_led_control(false), client_socket);
        } else {
            std::cout << command;
        }
    }
    
    void process_messages(int client_socket, const std::string &client_name) {
        char buffer[1024];
        int bytes_received;
        std::string color = get_color_for_name(client_name);
        std::string reset = "\033[0m";
        
        while (true) {
            bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
            
            if (bytes_received <= 0) {
                break;
            }
            
            buffer[bytes_received] = '\0';
            std::string message = trim(std::string(buffer)) + '\n';

            std::string pre_message = color + "[" + client_name + "]" + reset;
    
            if (message[0] != '/') {
                std::string formatted_msg = pre_message + message;
                std::cout << formatted_msg;
                broadcast_message(formatted_msg, client_socket);
            } else {
                process_command(message, client_socket, pre_message);
            }
        }
    }

public:
    void handle_client(int client_socket) {
        std::string client_name = receive_client_name(client_socket);
        
        if (client_name.empty()) {
            return;
        }
    
        add_client(client_socket, client_name);
        notify_join(client_socket, client_name);
        
        process_messages(client_socket, client_name);
     
        remove_client(client_socket);
        notify_leave(client_socket, client_name);
    }
};

// ============================================================================
// Главная функция
// ============================================================================

int main(int argc, char* argv[]) {
    int port = 8080;
    
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }

    Server server;
    MessageHandler message_handler;
    
    server.set_port(port);
    
    if (server.initialize() != 0) return 1;
    if (server.bind_and_listen() != 0) return 1;
    
    print_server_info(server.get_port());   
   
    while (true) {
        int client_socket = server.accept_client();
        
        if (client_socket < 0) {
            continue;
        }
        
        std::thread client_thread(&MessageHandler::handle_client, &message_handler, client_socket);
        client_thread.detach();
    }
    
    return 0;
}
