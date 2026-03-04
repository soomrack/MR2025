#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <vector>
#include <sys/select.h>

const int MAX_CLIENTS = 10;

static int client_fd[MAX_CLIENTS];

void send_to_client(std::string line, int this_client_fd) {
    if (this_client_fd >= 0) {
        send(this_client_fd, line.c_str(), line.size(), 0);
    }
}

bool handle_client_command(std::string msg, int this_client_fd) {
    if (msg == "/users") {
        int count = 0;
        for (int i=0; i<MAX_CLIENTS; i++) {
            if (client_fd[i]>=0) count++;
        }

        std::string toSend = "Всего клиентов: " + std::to_string(count) + "\n";
        send_to_client(toSend, this_client_fd);
    }
    return 0;
}

void handle_chat_message(std::string msg, const int client_index) {
    std::string msg_cropped=msg;
    while (msg_cropped.back() == '/') {
        msg_cropped.pop_back();
    } 

    if (size(msg_cropped)>0 && msg_cropped.at(0) == '/') {
        // команда с клиента
        if ( !handle_client_command(msg_cropped, client_fd[client_index]) ) return; // skip sending only if returned 0
    }

    std::cout << "Client " << client_index << ": " << msg << std::endl;

    // рассылаем сообщение всем клиентам
    std::string toSend = "Client " + std::to_string(client_index) + ": " + msg + "\n";
    for (int k = 0; k < MAX_CLIENTS; ++k) {
        send_to_client(toSend, client_fd[k]);
    }
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return 1;
    }

    // чтобы быстро перезапускать сервер
    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
        perror("bind");
        return 1;
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        return 1;
    }

    std::cout << "Server is listening on port 8080...\n";

    // массив клиентов и буфер текущего сообщения для каждого
    std::string clientBuffer[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        client_fd[i] = -1;
    }

    fd_set readfds;

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        int max_fd = server_fd;
        if (STDIN_FILENO > max_fd) max_fd = STDIN_FILENO;

        // добавить клиентов в набор
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            int fd = client_fd[i];
            if (fd >= 0) {
                FD_SET(fd, &readfds);
                if (fd > max_fd) max_fd = fd;
            }
        }

        int activity = select(max_fd + 1, &readfds, nullptr, nullptr, nullptr);
        if (activity < 0) {
            perror("select");
            break;
        }
        
        // Завершение работы по Ctrl+D
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            char c;
            if (read(STDIN_FILENO, &c, 1) > 0) {
                // if (c == 'q') {  // 'q' + Enter
                //     std::cout << "Сервер завершается...\n";
                //     break;
                // }
            } else {
                std::cout << "Сервер завершается (Ctrl+D)...\n";
                break;
            }
        }

        // новое подключение
        if (FD_ISSET(server_fd, &readfds)) {
            int new_fd = accept(server_fd, nullptr, nullptr);
            if (new_fd < 0) {
                perror("accept");
            } else {
                bool added = false;
                for (int i = 0; i < MAX_CLIENTS; ++i) {
                    if (client_fd[i] < 0) {
                        client_fd[i] = new_fd;
                        clientBuffer[i].clear();
                        std::cout << "New client connected, slot " << i << ", fd=" << new_fd << "\n";
                        added = true;
                        break;
                    }
                }
                if (!added) {
                    std::cout << "Too many clients, rejecting connection\n";
                    close(new_fd);
                }
            }
        }

        // данные от клиентов
        for (int i = 0; i < MAX_CLIENTS; ++i) {
            int fd = client_fd[i];
            if (fd < 0) continue;

            if (FD_ISSET(fd, &readfds)) {
                char buffer[1024];
                ssize_t bytes = recv(fd, buffer, sizeof(buffer), 0);
                if (bytes <= 0) {
                    if (bytes < 0) perror("recv");
                    std::cout << "Client " << i << " disconnected\n";
                    close(fd);
                    client_fd[i] = -1;
                    clientBuffer[i].clear();
                    continue;
                }

                // накапливаем и разбиваем по '\n'
                for (ssize_t j = 0; j < bytes; ++j) {
                    char c = buffer[j];
                    if (c == '\n') {
                        std::string msg = clientBuffer[i];
                        clientBuffer[i].clear();

                        handle_chat_message(msg, i);
                    } else {
                        clientBuffer[i].push_back(c);
                    }
                }
            }
        }
    }

    // закрываем всё
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (client_fd[i] >= 0) close(client_fd[i]);
    }
    close(server_fd);
    return 0;
}
