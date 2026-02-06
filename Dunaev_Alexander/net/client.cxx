#include <iostream>
#include <string>
#include <thread>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>


int socket_init(int &client_socket){

    client_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (client_socket < 0) {
        std::cerr << "Ошибка создания сокета" << std::endl;
        return 1;
    }
    
    return 0;
}


void address_init(sockaddr_in6 &server_addr, int port){

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

    if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Ошибка подключения к серверу" << std::endl;
        close(client_socket);
        return 1;
    }
    return 0;
}


std::string name;
void get_client_name(int &client_socket){

    std::cout << "Введите ваше имя: ";
    std::getline(std::cin, name);
    

    send(client_socket, name.c_str(), name.length(), 0);
    
    std::cout << "\nВы вошли в чат как: " << name << std::endl;
    std::cout << "Начните вводить сообщения (Ctrl+C для выхода):\n" << std::endl;

}


bool reconnect_to_server(int &sock, const std::string &server_ip, int port, sockaddr_in6 &server_addr){
    auto start_time = std::chrono::steady_clock::now();
    int attempt = 1;

    std::cout << "\n[Попытка переподключения к серверу]" << std::endl;

    while (std::chrono::steady_clock::now() - start_time < std::chrono::seconds(30)){
        close(sock);

        socket_init(sock);
        address_init(server_addr, port);

        std::cout << "[Попытка #" << attempt++ << "]" << std::endl;
        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == 0){
            std::cout << "[Переподключение успешно]" << std::endl;
            return true;

        }           
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    std::cout << "[Не удалось переподключиться за 30 секунд]" << std::endl;
    return false;


}
void receive_messages(int &sock, const std::string &server_ip, int &port, sockaddr_in6 &server_addr) {
    char buffer[1024];
    
    while (true) {
        int bytes_received = recv(sock, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received <= 0) {
            std::cout << "\n[Соединение с сервером потеряно]" << std::endl;
            if(reconnect_to_server(sock, server_ip, port, server_addr)){
                send(sock, name.c_str(), name.length(), 0);
                std::cout << "[Вы снова в чате как: " << name << " ]"<< std::endl;

            } else {
                exit(0);
            }
            continue;
        }
        
        buffer[bytes_received] = '\0';
        std::cout << buffer;
        std::cout.flush();
    }

}


void send_message(int &client_socket, std::string &server_ip, int &port, sockaddr_in6 &server_addr){

    std::thread receive_thread(receive_messages, std::ref(client_socket), std::ref(server_ip), std::ref(port), std::ref(server_addr));
    receive_thread.detach();
    

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
    

    int client_socket;
    socket_init(client_socket);
    

    struct sockaddr_in6 server_addr;
    address_init(server_addr, port);

    if (test_serv_addr(server_ip, server_addr, client_socket) != 0) return 1;
    if (test_serv_connection(client_socket, server_addr) != 0) return 1;
    
    
    
    std::cout << "=== КЛИЕНТ ЧАТА ===" << std::endl;
    std::cout << "Подключено к серверу!" << std::endl;

    get_client_name(client_socket);

    send_message(client_socket, server_ip, port, server_addr);
    
    
    
    close(client_socket);
    return 0;
}
