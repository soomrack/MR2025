#include <iostream>
#include <string>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

void receive_messages(int sock) {
    char buffer[1024];
    
    while (true) {
        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received <= 0) {
            std::cout << "\n[Соединение с сервером потеряно]" << std::endl;
            exit(0);
        }
        
        buffer[bytes_received] = '\0';
        std::cout << buffer;
        std::cout.flush();
    }
}

int socket_init(int &client_socket){
    // Создаем сокет для IPv6
    client_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (client_socket < 0) {
        std::cerr << "Ошибка создания сокета" << std::endl;
        return 1;
    }
    
    return 0;
}


void address_init(sockaddr_in6 &server_addr, int port){
    // Настраиваем адрес
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(port);
}


int test_serv_addr(std::string server_ip, sockaddr_in6 &server_addr, int client_socket){
    if (inet_pton(AF_INET6, server_ip.c_str(), &server_addr.sin6_addr) <= 0) {
        std::cerr << "Неверный адрес сервера" << std::endl;
        close(client_socket);
        return 1;
    }
    return 0;
}


int test_serv_connection(int & client_socket, sockaddr_in6 &server_addr){
    // Подключаемся к серверу
    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Ошибка подключения к серверу" << std::endl;
        close(client_socket);
        return 1;
    }
    return 0;
}


void get_client_name(int &client_socket){
    // Запрашиваем имя
    std::string name;
    std::cout << "Введите ваше имя: ";
    std::getline(std::cin, name);
    
    // Отправляем имя серверу
    send(client_socket, name.c_str(), name.length(), 0);
    
    std::cout << "\nВы вошли в чат как: " << name << std::endl;
    std::cout << "Начните вводить сообщения (Ctrl+C для выхода):\n" << std::endl;

}


void send_message(int &client_socket){
    // Запускаем поток для получения сообщений
    std::thread receive_thread(receive_messages, client_socket);
    receive_thread.detach();
    
    // Отправляем сообщения
    std::string message;
    while (std::getline(std::cin, message)) {
        if (!message.empty()) {
            message += "\n";
            if (send(client_socket, message.c_str(), message.length(), 0) < 0) {
                std::cerr << "Ошибка отправки сообщения" << std::endl;
                break;
            }
        }
    }
}


int main(int argc, char* argv[]) {
    std::string server_ip = "::1"; // localhost для IPv6
    int port = 8080;
    
    if (argc > 1) {
        server_ip = argv[1];
    }
    if (argc > 2) {
        port = std::stoi(argv[2]);
    }
    
    // Создаем сокет для IPv6
    int client_socket;
    socket_init(client_socket);
    
    // Настраиваем адрес сервера
    struct sockaddr_in6 server_addr;
    address_init(server_addr, port);

    if (test_serv_addr(server_ip, server_addr, client_socket) != 0) return 1;
    if (test_serv_connection(client_socket, server_addr) != 0) return 1;
    
    
    
    std::cout << "=== КЛИЕНТ ЧАТА ===" << std::endl;
    std::cout << "Подключено к серверу!" << std::endl;

    get_client_name(client_socket);

    send_message(client_socket);
    
    
    
    close(client_socket);
    return 0;
}
