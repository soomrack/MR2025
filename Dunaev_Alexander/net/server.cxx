#include <iostream>
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


struct Client {
    int socket;
    std::string name;
};

std::vector<Client> clients;
std::mutex clients_mutex;


std::string receive_client_name(int &client_socket){
    std::string client_name;
    char buffer[1024];
    // Получаем имя клиента
    int bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if (bytes_received <= 0) {
        close(client_socket);
        return "error";
    }
    
    buffer[bytes_received] = '\0';
    client_name = buffer;
    return client_name;
}

void broadcast_message(const std::string& message, int sender_socket) {
    std::lock_guard<std::mutex> lock(clients_mutex);
    for (const auto& client : clients) {
        if (client.socket != sender_socket) {
            send(client.socket, message.c_str(), message.length(), 0);
        }
    }
}

void add_client_to_list(int &client_socket, std::string client_name){
    // Добавляем клиента в список
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.push_back({client_socket, client_name});
}


void notify_client_joined(const int &client_socket, const std::string &client_name){
    std::cout << "[СЕРВЕР] " << client_name << " присоединился к чату" << std::endl;
    
    std::string join_msg = "[СИСТЕМА] " + client_name + " присоединился к чату\n";
    broadcast_message(join_msg, client_socket);
}


void notify_client_left(const int &client_socket, const std::string &client_name){
    std::cout << "[СЕРВЕР] " << client_name << " отключился" << std::endl;
    
    std::string leave_msg = "[СИСТЕМА] " + client_name + " покинул чат\n";
    broadcast_message(leave_msg, -1);
    
    close(client_socket);
}

void remove_client_from_list(int &client_socket){
    // Удаляем клиента из списка
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(
            std::remove_if(clients.begin(), clients.end(),
                [client_socket](const Client& c) { return c.socket == client_socket; }),
            clients.end()
        );
}

std::string trim(const std::string &str){
    size_t first = str.find_first_not_of("\t\n\r");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of("\t\n\r");
    return str.substr(first, last-first+1);
}

void process_client_messages(int &client_socket, std::string &client_name){
    char buffer[1024];
    // Обрабатываем сообщения
    int bytes_received;
    while (true) {
        bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes_received <= 0) {
            break;
        }
        
        buffer[bytes_received] = '\0';
        std::string pre_message = trim(std::string(buffer)) + '\n';

        if(pre_message[0] != '/'){
            std::string message = "[" + client_name + "] " + pre_message;
            
            std::cout << message;
            broadcast_message(message, client_socket);
        } else if (pre_message == "/hello\n"){
            std::cout << "всем общий привет\n" << std::endl;
            broadcast_message("всем общий привет\n", client_socket);
        } else if (pre_message == "/megahello\n"){
            std::cout << "ВСЕМ ОБЩИЙ МЕГАПРИВЕТ\n" << std::endl;
            broadcast_message("ВСЕМ ОБЩИЙ МЕГАПРИВЕТ\n", client_socket);
        } else {
            std::cout << pre_message;
        }

        }
}



void handle_client(int client_socket) {
    std::string client_name = receive_client_name(client_socket);

    add_client_to_list(client_socket, client_name);
    notify_client_joined(client_socket, client_name);
    
    process_client_messages(client_socket, client_name);
 
    remove_client_from_list(client_socket);
    notify_client_left(client_socket, client_name);
    
}


int socket_init(int &server_socket){
    // Создаем сокет для IPv6
    server_socket = socket(AF_INET6, SOCK_STREAM, 0);
    if (server_socket < 0) {
        std::cerr << "Ошибка создания сокета" << std::endl;
        return 1;
    }
    
    // Настраиваем сокет
    int opt = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    return 0;

}


void address_init(sockaddr_in6 &server_addr, int port){
    // Настраиваем адрес
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_addr = in6addr_any;
    server_addr.sin6_port = htons(port);
}


void accept_connections(const int &server_socket){
    // Принимаем подключения
    while (true) {
        struct sockaddr_in6 client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
        
        if (client_socket < 0) {
            std::cerr << "Ошибка подключения клиента" << std::endl;
            continue;
        }
        
        std::cout << "[СЕРВЕР] Новое подключение" << std::endl;
        
        std::thread client_thread(handle_client, client_socket);
        client_thread.detach();
    }
}


int bind_socket(const int& server_socket, const sockaddr_in6 &server_addr){
    // Привязываем сокет
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Ошибка привязки сокета" << std::endl;
        close(server_socket);
        return 1;
    }

    return 0;

}


int start_listening(const int server_socket, int backlog = 10){
    // Начинаем слушать
    if (listen(server_socket, backlog) < 0) {
        std::cerr << "Ошибка прослушивания" << std::endl;
        close(server_socket);
        return 1;
    }
    return 0;
}

    
void print_server_info(const int &port){
    std::cout << "=== СЕРВЕР ЧАТА ===" << std::endl;
    std::cout << "Сервер запущен на порту " << port << std::endl;
    std::cout << "Ожидание подключений..." << std::endl;
}


int main(int argc, char* argv[]) {
    int port = 8080;
    int server_socket;
    struct sockaddr_in6 server_addr;
    
    if (argc > 1) {
        port = std::stoi(argv[1]);
    }
    
    socket_init(server_socket);
    address_init(server_addr, port);

    if (bind_socket(server_socket, server_addr) != 0) return 1;
    if (start_listening(server_socket) != 0) return 1;
    
    
    print_server_info(port);   
   
    accept_connections(server_socket);
    
    close(server_socket);
    return 0;
}


