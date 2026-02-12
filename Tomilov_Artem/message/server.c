#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <signal.h>

#include <sys/utsname.h>  // для uname()
#include <sys/sysinfo.h>  // для sysinfo()

#define PORT 5000
#define BUF_SIZE 1024
#define MAX_CLIENTS 10
#define NAME_LEN 32

// ANSI цветовые коды
#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"

// Массив доступных цветов для пользователей
static const char* user_colors[] = {
    "\033[1;31m",  // Ярко-красный
    "\033[1;32m",  // Ярко-зелёный
    "\033[1;33m",  // Ярко-жёлтый
    "\033[1;34m",  // Ярко-синий
    "\033[1;35m",  // Ярко-фиолетовый
    "\033[1;36m",  // Ярко-голубой
    "\033[1;91m",  // Светло-красный
    "\033[1;92m",  // Светло-зелёный
    "\033[1;93m",  // Светло-жёлтый
    "\033[1;94m",  // Светло-синий
};

#define NUM_COLORS (sizeof(user_colors) / sizeof(user_colors[0]))

// ============================================================================
// СТРУКТУРЫ ДАННЫХ
// ============================================================================

typedef struct {
    int sock;
    char name[NAME_LEN];
    int named;
    int color_index;  // Индекс цвета из массива user_colors
} Client;

typedef struct {
    const char *name;
    const char *description;
    void (*handler)(Client[], int, const char*);
} ServerCommand;

typedef struct {
    int server_fd;
    Client clients[MAX_CLIENTS];
    int running;
} ServerState;

// ============================================================================
// ПРОТОТИПЫ: ИНИЦИАЛИЗАЦИЯ И УПРАВЛЕНИЕ СЕРВЕРОМ
// ============================================================================

static void     server_state_init(ServerState *state);
static int      server_socket_create(void);
static void     server_socket_configure(int fd);
static void     clients_array_init(Client clients[]);
static void     signal_handlers_setup(void);
static void     show_startup_message(void);

// ============================================================================
// ПРОТОТИПЫ: ОСНОВНОЙ ЦИКЛ СЕРВЕРА
// ============================================================================

static void     server_main_loop(ServerState *state);
static void     fdset_build(int server_fd, Client clients[], fd_set *set, int *max_sd);
static void     handle_new_connection(ServerState *state);
static void     handle_client_activity(ServerState *state, fd_set *readfds);

// ============================================================================
// ПРОТОТИПЫ: ОБРАБОТКА КЛИЕНТОВ И СООБЩЕНИЙ
// ============================================================================

static void     accept_new_client(int server_fd, Client clients[]);
static void     process_client_message(Client clients[], int idx);
static int      is_command(const char *msg);
static void     process_command(Client clients[], int idx, const char *cmd);
static void     process_regular_message(Client clients[], int idx, const char *msg);
static void     process_name_setup(Client *client, const char *name);
static void     replace_emoji_shortcuts(char *text, size_t max_len);

// ============================================================================
// ПРОТОТИПЫ: УПРАВЛЕНИЕ КЛИЕНТАМИ
// ============================================================================

static void     announce_user_joined(Client clients[], Client *new_client);
static void     announce_user_left(Client clients[], Client *leaving_client);
static void     client_disconnect(Client *client);
static void     broadcast(Client clients[], const char *msg, int except);
static int      count_active_users(Client clients[]);
static int      assign_color_to_client(Client clients[], int client_idx);

// ============================================================================
// ПРОТОТИПЫ: КОМАНДЫ СЕРВЕРА
// ============================================================================

static void     cmd_disconnect(Client clients[], int idx, const char *args);
static void     cmd_help(Client clients[], int idx, const char *args);
static void     cmd_users(Client clients[], int idx, const char *args);
static void     cmd_OSinfo(Client clients[], int idx, const char *args);
static void     cmd_emoji(Client clients[], int idx, const char *args);

// ============================================================================
// ТАБЛИЦА КОМАНД
// ============================================================================

static const ServerCommand commands[] = {
    {"disconnect", "Disconnect from chat", cmd_disconnect},
    {"help", "Show available commands", cmd_help},
    {"users", "List active users", cmd_users},
    {"OSinfo", "Server information", cmd_OSinfo},
    {"emoji", "Show emoji shortcuts", cmd_emoji},
    {NULL, NULL, NULL}
};

// ============================================================================
// ГЛОБАЛЬНАЯ ПЕРЕМЕННАЯ ДЛЯ ОБРАБОТКИ СИГНАЛОВ
// ============================================================================

static volatile int g_server_running = 1;

static void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        printf("\n[SIGNAL] Received shutdown signal\n");
        g_server_running = 0;
    }
}

// ============================================================================
// ГЛАВНАЯ ФУНКЦИЯ - ПОЭТАПНАЯ РАБОТА СЕРВЕРА
// ============================================================================

int main(void) {
    ServerState state;
    
    printf("chat max10.0 STARTING\n\n");
    
    printf("Setting up signal handlers...\n");
    signal_handlers_setup();
    
    
    printf("nitializing server state...\n");
    server_state_init(&state);
    
    
    printf("Creating server socket on port %d...\n", PORT);
    state.server_fd = server_socket_create();
    if (state.server_fd < 0) {
        printf("[ERROR] Failed to create server socket\n");
        return 1;
    }
    
    printf("Initializing client array (max %d clients)...\n", MAX_CLIENTS);
    clients_array_init(state.clients);
    
    printf(" Server ready!\n\n");
    show_startup_message();

    printf("Starting main event loop...\n");
    printf("========================================\n\n");
    
    server_main_loop(&state);
    
    printf("\n Shutting down server...\n");
    
    // Отключаем всех клиентов
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (state.clients[i].sock > 0) {
            const char *msg = "[SERVER] Server is shutting down. Goodbye!\n";
            send(state.clients[i].sock, msg, strlen(msg), 0);
            client_disconnect(&state.clients[i]);
        }
    }
    
    // Закрываем серверный сокет
    close(state.server_fd);
    
    printf("[EXIT] Server stopped. Goodbye!\n");
    
    return 0;
}

// ============================================================================
// РЕАЛИЗАЦИЯ: ИНИЦИАЛИЗАЦИЯ И УПРАВЛЕНИЕ СЕРВЕРОМ
// ============================================================================

static void server_state_init(ServerState *state) {
    state->server_fd = -1;
    state->running = 1;
    memset(state->clients, 0, sizeof(state->clients));
}

static int server_socket_create(void) {
    int fd;
    struct sockaddr_in addr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }

    server_socket_configure(fd);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(fd);
        return -1;
    }

    if (listen(fd, 5) < 0) {
        perror("listen");
        close(fd);
        return -1;
    }

    return fd;
}

static void server_socket_configure(int fd) {
    int opt = 1;
    
    // Разрешаем переиспользование адреса
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt");
    }
}

static void clients_array_init(Client clients[]) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].sock = 0;
        clients[i].named = 0;
        clients[i].color_index = -1;
        memset(clients[i].name, 0, NAME_LEN);
    }
}

static void signal_handlers_setup(void) {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN); // Игнорируем SIGPIPE при записи в закрытый сокет
}

static void show_startup_message(void) {
    printf("========================================\n");
    printf("  Chat MAX 10.0 is Running! 💬\n");
    printf("========================================\n");
    printf("Port: %d\n", PORT);
    printf("Max clients: %d\n", MAX_CLIENTS);
    printf("Features: Colors + Emoji support!\n");
    printf("Available commands:\n");
    for (int i = 0; commands[i].name != NULL; i++) {
        printf("  \\%-12s - %s\n", commands[i].name, commands[i].description);
    }
    printf("========================================\n");
    printf("Press Ctrl+C to stop the server\n");
}

// ============================================================================
// РЕАЛИЗАЦИЯ: ОСНОВНОЙ ЦИКЛ СЕРВЕРА
// ============================================================================

static void server_main_loop(ServerState *state) {
    fd_set readfds;

    while (g_server_running && state->running) {
        int max_sd;
        FD_ZERO(&readfds);

        fdset_build(state->server_fd, state->clients, &readfds, &max_sd);
        
        // Используем timeout для периодической проверки флага g_server_running
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int activity = select(max_sd + 1, &readfds, NULL, NULL, &timeout);
        
        if (activity < 0) {
            if (g_server_running) {
                perror("select");
            }
            break;
        }
        
        if (activity == 0) {
            // Timeout - просто продолжаем
            continue;
        }

        // Обработка новых подключений
        if (FD_ISSET(state->server_fd, &readfds)) {
            handle_new_connection(state);
        }

        // Обработка активности клиентов
        handle_client_activity(state, &readfds);
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

static void handle_new_connection(ServerState *state) {
    accept_new_client(state->server_fd, state->clients);
}

static void handle_client_activity(ServerState *state, fd_set *readfds) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (state->clients[i].sock > 0 && FD_ISSET(state->clients[i].sock, readfds)) {
            process_client_message(state->clients, i);
        }
    }
}

// ============================================================================
// РЕАЛИЗАЦИЯ: ОБРАБОТКА КЛИЕНТОВ И СООБЩЕНИЙ
// ============================================================================

static void accept_new_client(int server_fd, Client clients[]) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int fd = accept(server_fd, (struct sockaddr*)&addr, &len);

    if (fd < 0) {
        perror("accept");
        return;
    }

    // Ищем свободный слот
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock == 0) {
            clients[i].sock = fd;
            clients[i].named = 0;
            
            // Назначаем цвет клиенту
            assign_color_to_client(clients, i);
            
            const char *welcome = " Welcome to the chat! \nEnter your name: ";
            send(fd, welcome, strlen(welcome), 0);
            
            printf("[CONNECTION] New client from %s:%d (slot %d, color index %d)\n", 
                   inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), i, clients[i].color_index);
            return;
        }
    }

    // Нет свободных слотов
    const char *msg = "Server is full. Try again later.\n";
    send(fd, msg, strlen(msg), 0);
    close(fd);
    printf("[CONNECTION] Connection rejected: server full\n");
}

static void process_client_message(Client clients[], int idx) {
    char buffer[BUF_SIZE];
    int n = read(clients[idx].sock, buffer, BUF_SIZE - 1);

    if (n <= 0) {
        // Клиент отключился
        if (clients[idx].named) {
            printf("[DISCONNECT] User '%s' disconnected\n", clients[idx].name);
            announce_user_left(clients, &clients[idx]);
        } else {
            printf("[DISCONNECT] Unnamed client disconnected (slot %d)\n", idx);
        }
        client_disconnect(&clients[idx]);
        return;
    }

    buffer[n] = '\0';
    // Убираем перевод строки
    buffer[strcspn(buffer, "\n\r")] = '\0';

    // Пропускаем пустые сообщения
    if (strlen(buffer) == 0) {
        return;
    }

    if (!clients[idx].named) {
        // Клиент еще не представился
        process_name_setup(&clients[idx], buffer);
        announce_user_joined(clients, &clients[idx]);
    } else if (is_command(buffer)) {
        // Обработка команды
        printf("[COMMAND] User '%s' executed: %s\n", clients[idx].name, buffer);
        process_command(clients, idx, buffer);
    } else {
        // Обычное сообщение (НЕ логируем на сервере)
        process_regular_message(clients, idx, buffer);
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
             "[SERVER] Welcome, %s! Type \\help or \\emoji to get started. 💬\n", 
             client->name);
    send(client->sock, welcome, strlen(welcome), 0);
    
    printf("[USER] New user joined: %s\n", client->name);
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
    char processed_msg[BUF_SIZE];
    const char *color = user_colors[clients[idx].color_index];
    
    // Копируем сообщение и заменяем смайлики
    strncpy(processed_msg, msg, BUF_SIZE - 1);
    processed_msg[BUF_SIZE - 1] = '\0';
    replace_emoji_shortcuts(processed_msg, BUF_SIZE);
    
    snprintf(buffer, BUF_SIZE, "%s[%s]%s %s\n", 
             color, clients[idx].name, COLOR_RESET, processed_msg);
    broadcast(clients, buffer, idx);
}

// ============================================================================
// РЕАЛИЗАЦИЯ: УПРАВЛЕНИЕ КЛИЕНТАМИ
// ============================================================================

static void replace_emoji_shortcuts(char *text, size_t max_len) {
    struct {
        const char *shortcut;
        const char *emoji;
    } emoji_map[] = {
        // Смайлики
        {":)", "😊"},
        {":smile:", "😊"},
        {":D", "😂"},
        {":laugh:", "😂"},
        {":(", "😢"},
        {":sad:", "😢"},
        {":love:", "😍"},
        {":cool:", "😎"},
        {":heart:", "❤️"},
        {":fire:", "🔥"},
        {NULL, NULL}
    };
    
    char result[BUF_SIZE * 2] = {0};
    const char *src = text;
    char *dst = result;
    size_t result_len = 0;
    
    while (*src && result_len < max_len - 1) {
        int replaced = 0;
        
        // Проверяем все возможные шорткаты
        for (int i = 0; emoji_map[i].shortcut != NULL; i++) {
            size_t shortcut_len = strlen(emoji_map[i].shortcut);
            
            if (strncmp(src, emoji_map[i].shortcut, shortcut_len) == 0) {
                // Нашли совпадение - заменяем на эмодзи
                size_t emoji_len = strlen(emoji_map[i].emoji);
                if (result_len + emoji_len < max_len - 1) {
                    strcpy(dst, emoji_map[i].emoji);
                    dst += emoji_len;
                    src += shortcut_len;
                    result_len += emoji_len;
                    replaced = 1;
                    break;
                }
            }
        }
        
        // Если не нашли замену, копируем символ как есть
        if (!replaced) {
            *dst++ = *src++;
            result_len++;
        }
    }
    
    *dst = '\0';
    
    // Копируем результат обратно
    strncpy(text, result, max_len - 1);
    text[max_len - 1] = '\0';
}

// ============================================================================
// РЕАЛИЗАЦИЯ: УПРАВЛЕНИЕ КЛИЕНТАМИ
// ============================================================================

static void announce_user_joined(Client clients[], Client *new_client) {
    char msg[BUF_SIZE];
    snprintf(msg, BUF_SIZE, "[SERVER] %s joined the chat\n", new_client->name);
    broadcast(clients, msg, -1);
}

static void announce_user_left(Client clients[], Client *leaving_client) {
    char msg[BUF_SIZE];
    snprintf(msg, BUF_SIZE, "[SERVER] %s left the chat\n", leaving_client->name);
    broadcast(clients, msg, -1);
}

static void client_disconnect(Client *client) {
    close(client->sock);
    client->sock = 0;
    client->named = 0;
    client->color_index = -1;
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

static int count_active_users(Client clients[]) {
    int count = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock > 0 && clients[i].named) {
            count++;
        }
    }
    return count;
}

static int assign_color_to_client(Client clients[], int client_idx) {
    int color_usage[NUM_COLORS] = {0};
    
    // Подсчитываем использование каждого цвета
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (i != client_idx && clients[i].sock > 0 && clients[i].color_index >= 0) {
            color_usage[clients[i].color_index]++;
        }
    }
    
    // Находим наименее используемый цвет
    int min_usage = color_usage[0];
    int best_color = 0;
    
    for (int i = 1; i < (int)NUM_COLORS; i++) {
        if (color_usage[i] < min_usage) {
            min_usage = color_usage[i];
            best_color = i;
        }
    }
    
    clients[client_idx].color_index = best_color;
    return best_color;
}

// ============================================================================
// РЕАЛИЗАЦИЯ: КОМАНДЫ СЕРВЕРА
// ============================================================================

static void cmd_disconnect(Client clients[], int idx, const char *args) {
    const char *goodbye = "[SERVER] Goodbye! See you soon!\n";
    send(clients[idx].sock, goodbye, strlen(goodbye), 0);
    
    printf("[COMMAND] User '%s' disconnected via \\disconnect\n", clients[idx].name);
    
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
    char msg[BUF_SIZE * 2];
    int offset = 0;
    int count = 0;
    
    offset += snprintf(msg + offset, sizeof(msg) - offset,
                      "[SERVER] Active users:\n");
    
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock > 0 && clients[i].named) {
            const char *color = user_colors[clients[i].color_index];
            offset += snprintf(msg + offset, sizeof(msg) - offset,
                              "  - %s%s%s%s\n", 
                              color,
                              clients[i].name,
                              COLOR_RESET,
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

static void cmd_OSinfo(Client clients[], int idx, const char *args) {
    char msg[BUF_SIZE];
    int offset = 0;
    
    struct utsname sys_info;
    if (uname(&sys_info) != 0) {
        const char *error_msg = "[SERVER] Failed to get OS info\n";
        send(clients[idx].sock, error_msg, strlen(error_msg), 0);
        return;
    }

    struct sysinfo s_info;
    if (sysinfo(&s_info) != 0) {
        const char *error_msg = "[SERVER] Failed to get system info\n";
        send(clients[idx].sock, error_msg, strlen(error_msg), 0);
        return;
    }

    offset += snprintf(msg + offset, sizeof(msg) - offset,
                      "[SERVER] === OS Information ===\n");

    offset += snprintf(msg + offset, sizeof(msg) - offset,
                      "OS: %s\n", sys_info.sysname);
    offset += snprintf(msg + offset, sizeof(msg) - offset,
                      "Hostname: %s\n", sys_info.nodename);
    offset += snprintf(msg + offset, sizeof(msg) - offset,
                      "Kernel: %s\n", sys_info.release);
    offset += snprintf(msg + offset, sizeof(msg) - offset,
                      "Architecture: %s\n", sys_info.machine);

    offset += snprintf(msg + offset, sizeof(msg) - offset,
                      "\n[SERVER] === System Information ===\n");                  

    offset += snprintf(msg + offset, sizeof(msg) - offset,
                      "Uptime: %ld seconds\n", s_info.uptime);
    offset += snprintf(msg + offset, sizeof(msg) - offset,
                      "Total RAM: %lu bytes\n", s_info.totalram);
    offset += snprintf(msg + offset, sizeof(msg) - offset,
                      "Free RAM: %lu bytes\n", s_info.freeram);
    offset += snprintf(msg + offset, sizeof(msg) - offset,
                      "1 min load avg: %lu\n", s_info.loads[0]);
    offset += snprintf(msg + offset, sizeof(msg) - offset,
                      "Active users: %d\n", count_active_users(clients));

    send(clients[idx].sock, msg, strlen(msg), 0);
}

static void cmd_emoji(Client clients[], int idx, const char *args) {
    char msg[BUF_SIZE];
    int offset = 0;

    offset += snprintf(msg + offset, sizeof(msg) - offset,
                       "[SERVER] Available Emoji Shortcuts\n\n");

    offset += snprintf(msg + offset, sizeof(msg) - offset,
                       "😊 :)  or  :smile:\n");

    offset += snprintf(msg + offset, sizeof(msg) - offset,
                       "😂 :D  or  :laugh:\n");

    offset += snprintf(msg + offset, sizeof(msg) - offset,
                       "😢 :(  or  :sad:\n");

    offset += snprintf(msg + offset, sizeof(msg) - offset,
                       "😍 :love:\n");

    offset += snprintf(msg + offset, sizeof(msg) - offset,
                       "😎 :cool:\n");

    offset += snprintf(msg + offset, sizeof(msg) - offset,
                       "❤️  :heart:\n");

    offset += snprintf(msg + offset, sizeof(msg) - offset,
                       "🔥 :fire:\n");

    offset += snprintf(msg + offset, sizeof(msg) - offset,
                       "\nExample: Hello :) I am happy :love:\n");

    send(clients[idx].sock, msg, strlen(msg), 0);
}
