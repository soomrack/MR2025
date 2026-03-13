#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 5000
#define BUFFER_SIZE 1024

int main() {
    std::cout << "=== ТЕСТОВЫЙ СЕРВЕР ===" << std::endl;
    std::cout << "Порт: " << PORT << std::endl;
    std::cout << "=====================================" << std::endl;

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        std::cerr << "Ошибка создания сокета" << std::endl;
        return 1;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
        std::cerr << "Ошибка привязки к порту" << std::endl;
        close(server_fd);
        return 1;
    }

    if (listen(server_fd, 3) < 0) {
        std::cerr << "Ошибка прослушивания" << std::endl;
        close(server_fd);
        return 1;
    }
    
    std::cout << "Сервер запущен. Ожидание подключений..." << std::endl;
    
    int client_count = 0;
    
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_len);
        if (client_fd < 0) {
            std::cerr << "Ошибка принятия подключения" << std::endl;
            continue;
        }
        
        client_count++;
        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        std::cout << "\n[" << client_count << "] Подключен клиент: " << client_ip << std::endl;
        
        char buffer[BUFFER_SIZE] = {0};
        int request_count = 0;

        while (true) {
            memset(buffer, 0, BUFFER_SIZE);
            int bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
            
            if (bytes_read <= 0) {
                break;
            }
            
            std::string command(buffer);

            command.erase(std::remove(command.begin(), command.end(), '\n'), command.end());
            command.erase(std::remove(command.begin(), command.end(), '\r'), command.end());
            
            std::cout << "  Команда: " << command << std::endl;
            
            std::string response;
            
            if (command == "GET_DATA") {
                request_count++;
                // Отправляем "1"
                response = "1\n";
                std::cout << "  Отправлено: 1 (запрос #" << request_count << ")" << std::endl;
            }
            else if (command == "EXIT") {
                response = "До свидания!\n";
                write(client_fd, response.c_str(), response.length());
                std::cout << "  Клиент завершил работу. Запросов: " << request_count << std::endl;
                break;
            }
            else if (command == "HELP") {
                response = "Команды: GET_DATA, EXIT, HELP\n";
            }
            else {
                response = "Неизвестная команда. Используйте HELP\n";
            }
            
            write(client_fd, response.c_str(), response.length());
        }
        
        close(client_fd);
        std::cout << "  Соединение закрыто" << std::endl;
    }
    
    close(server_fd);
    return 0;
}