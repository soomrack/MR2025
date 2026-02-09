#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 5000
#define BUF_SIZE 1024
#define MAX_CLIENTS 10
#define NAME_LEN 32

typedef struct {
    int sock;
    char name[NAME_LEN];
    int named;
} Client;

typedef struct {
    const char *name;
    const char *description;
    void (*handler)(Client[], int, const char*);
} ServerCommand;

// Прототипы основных функций
static void server_run(void);
static int  server_socket_create(void);
static void clients_init(Client clients[]);
static void server_loop(int server_fd, Client clients[]);
static void fdset_build(int server_fd, Client clients[], fd_set *set, int *max_sd);
static void accept_client(int server_fd, Client clients[]);
static void handle_clients(Client clients[], fd_set *set);

// Прототипы функций обработки сообщений
static void handle_client_message(Client clients[], int i);
static int  is_command(const char *msg);
static void process_command(Client clients[], int idx, const char *cmd);
static void process_regular_message(Client clients[], int idx, const char *msg);
static void process_name_setup(Client *client, const char *name);
static void announce_user_joined(Client clients[], Client *new_client);
static void announce_user_left(Client clients[], Client *leaving_client);

// Прототипы функций управления клиентами
static void client_disconnect(Client *client);
static void broadcast(Client clients[], const char *msg, int except);

// Прототипы команд
static void cmd_disconnect(Client clients[], int idx, const char *args);
static void cmd_help(Client clients[], int idx, const char *args);
static void cmd_users(Client clients[], int idx, const char *args);

// Таблица команд
static const ServerCommand commands[] = {
    {"disconnect", "Disconnect from chat", cmd_disconnect},
    {"help", "Show available commands", cmd_help},
    {"users", "List active users", cmd_users},
    {NULL, NULL, NULL}
};


int main(void) {
    server_run();
    return 0;
}


static void server_run(void) {
    int server_fd = server_socket_create();
    Client clients[MAX_CLIENTS];

    clients_init(clients);

    printf("Chat server started on port %d\n", PORT);
    printf("Available commands: \\help, \\users, \\disconnect\n");
    server_loop(server_fd, clients);
}

static int server_socket_create(void) {
    int fd;
    struct sockaddr_in addr;
    int opt = 1;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { 
        perror("socket"); 
        exit(1); 
    }

    // Разрешаем переиспользование адреса
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
        exit(1);
    }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); 
        exit(1);
    }

    if (listen(fd, 5) < 0) {
        perror("listen"); 
        exit(1);
    }

    return fd;
}

static void clients_init(Client clients[]) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].sock = 0;
        clients[i].named = 0;
        memset(clients[i].name, 0, NAME_LEN);
    }
}

static void server_loop(int server_fd, Client clients[]) {
    fd_set readfds;

    while (1) {
        int max_sd;
        FD_ZERO(&readfds);

        fdset_build(server_fd, clients, &readfds, &max_sd);
        
        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            continue;
        }

        if (FD_ISSET(server_fd, &readfds))
            accept_client(server_fd, clients);

        handle_clients(clients, &readfds);
    }
}

static void fdset_build(int server_fd, Client clients[], fd_set *set, int *max_sd) {
    FD_SET(server_fd, set);
    *max_sd = server_fd;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock > 0) {
            FD_SET(clients[i].sock, set);
            if (clients[i].sock > *max_sd)
                *max_sd = clients[i].sock;
        }
    }
}

static void accept_client(int server_fd, Client clients[]) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int fd = accept(server_fd, (struct sockaddr*)&addr, &len);

    if (fd < 0) {
        perror("accept");
        return;
    }

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock == 0) {
            clients[i].sock = fd;
            clients[i].named = 0;
            
            const char *welcome = "Welcome to the chat!\nEnter your name: ";
            send(fd, welcome, strlen(welcome), 0);
            
            printf("New connection from %s:%d (slot %d)\n", 
                   inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), i);
            return;
        }
    }

    // Нет свободных слотов
    const char *msg = "Server is full. Try again later.\n";
    send(fd, msg, strlen(msg), 0);
    close(fd);
    printf("Connection rejected: server full\n");
}

static void handle_clients(Client clients[], fd_set *set) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock > 0 && FD_ISSET(clients[i].sock, set))
            handle_client_message(clients, i);
    }
}

// ============================================================================
// ОБРАБОТКА СООБЩЕНИЙ (РЕФАКТОРИНГ)
// ============================================================================

static void handle_client_message(Client clients[], int i) {
    char buffer[BUF_SIZE];
    int n = read(clients[i].sock, buffer, BUF_SIZE - 1);

    if (n <= 0) {
        // Клиент отключился
        if (clients[i].named) {
            announce_user_left(clients, &clients[i]);
        }
        client_disconnect(&clients[i]);
        return;
    }

    buffer[n] = '\0';
    // Убираем перевод строки
    buffer[strcspn(buffer, "\n\r")] = '\0';

    // Пропускаем пустые сообщения
    if (strlen(buffer) == 0) {
        return;
    }

    if (!clients[i].named) {
        // Клиент еще не представился
        process_name_setup(&clients[i], buffer);
        announce_user_joined(clients, &clients[i]);
    } else if (is_command(buffer)) {
        // Обработка команды
        process_command(clients, i, buffer);
    } else {
        // Обычное сообщение
        process_regular_message(clients, i, buffer);
    }
}

static int is_command(const char *msg) {
    return msg[0] == '\\';
}

static void process_name_setup(Client *client, const char *name) {
    strncpy(client->name, name, NAME_LEN - 1);
    client->name[NAME_LEN - 1] = '\0';
    client->named = 1;
    
    char welcome[BUF_SIZE];
    snprintf(welcome, BUF_SIZE, 
             "[SERVER] Welcome, %s! Type \\help for available commands.\n", 
             client->name);
    send(client->sock, welcome, strlen(welcome), 0);
}

static void announce_user_joined(Client clients[], Client *new_client) {
    char msg[BUF_SIZE];
    snprintf(msg, BUF_SIZE, "[SERVER] %s joined the chat\n", new_client->name);
    broadcast(clients, msg, -1);
    printf("User joined: %s\n", new_client->name);
}

static void announce_user_left(Client clients[], Client *leaving_client) {
    char msg[BUF_SIZE];
    snprintf(msg, BUF_SIZE, "[SERVER] %s left the chat\n", leaving_client->name);
    broadcast(clients, msg, -1);
    printf("User left: %s\n", leaving_client->name);
}

static void process_command(Client clients[], int idx, const char *cmd) {
    // Убираем '\' в начале
    const char *command = cmd + 1;
    
    // Найти пробел (аргументы команды)
    const char *args = strchr(command, ' ');
    char cmd_name[32];
    
    if (args) {
        size_t len = args - command;
        if (len >= sizeof(cmd_name)) len = sizeof(cmd_name) - 1;
        strncpy(cmd_name, command, len);
        cmd_name[len] = '\0';
        args++; // Пропустить пробел
    } else {
        strncpy(cmd_name, command, sizeof(cmd_name) - 1);
        cmd_name[sizeof(cmd_name) - 1] = '\0';
        args = "";
    }
    
    // Найти и выполнить команду
    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(cmd_name, commands[i].name) == 0) {
            commands[i].handler(clients, idx, args);
            return;
        }
    }
    
    // Неизвестная команда
    char error[BUF_SIZE];
    snprintf(error, BUF_SIZE, 
             "[SERVER] Unknown command: \\%s (type \\help for available commands)\n", 
             cmd_name);
    send(clients[idx].sock, error, strlen(error), 0);
}

static void process_regular_message(Client clients[], int idx, const char *msg) {
    char buffer[BUF_SIZE];
    snprintf(buffer, BUF_SIZE, "[%s] %s\n", clients[idx].name, msg);
    broadcast(clients, buffer, idx);
}

// ============================================================================
// КОМАНДЫ
// ============================================================================

static void cmd_disconnect(Client clients[], int idx, const char *args) {
    const char *goodbye = "[SERVER] Goodbye!\n";
    send(clients[idx].sock, goodbye, strlen(goodbye), 0);
    
    if (clients[idx].named) {
        announce_user_left(clients, &clients[idx]);
    }
    
    client_disconnect(&clients[idx]);
}

static void cmd_help(Client clients[], int idx, const char *args) {
    char help[BUF_SIZE * 2];
    int offset = 0;
    
    offset += snprintf(help + offset, sizeof(help) - offset, 
                      "[SERVER] Available commands:\n");
    
    for (int i = 0; commands[i].name != NULL; i++) {
        offset += snprintf(help + offset, sizeof(help) - offset,
                          "  \\%-12s - %s\n", 
                          commands[i].name, 
                          commands[i].description);
    }
    
    send(clients[idx].sock, help, strlen(help), 0);
}

static void cmd_users(Client clients[], int idx, const char *args) {
    char msg[BUF_SIZE];
    int offset = 0;
    int count = 0;
    
    offset += snprintf(msg + offset, sizeof(msg) - offset,
                      "[SERVER] Active users:\n");
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock > 0 && clients[i].named) {
            offset += snprintf(msg + offset, sizeof(msg) - offset,
                              "  - %s%s\n", 
                              clients[i].name,
                              (i == idx) ? " (you)" : "");
            count++;
        }
    }
    
    if (count == 0) {
        snprintf(msg, sizeof(msg), "[SERVER] No users online\n");
    } else {
        offset += snprintf(msg + offset, sizeof(msg) - offset,
                          "Total: %d user%s\n", count, count > 1 ? "s" : "");
    }
    
    send(clients[idx].sock, msg, strlen(msg), 0);
}

// ============================================================================
// УПРАВЛЕНИЕ КЛИЕНТАМИ
// ============================================================================

static void client_disconnect(Client *client) {
    printf("Client disconnected: socket %d\n", client->sock);
    close(client->sock);
    client->sock = 0;
    client->named = 0;
    memset(client->name, 0, NAME_LEN);
}

static void broadcast(Client clients[], const char *msg, int except) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock > 0 && clients[i].named && i != except) {
            if (send(clients[i].sock, msg, strlen(msg), 0) < 0) {
                perror("broadcast send");
            }
        }
    }
}