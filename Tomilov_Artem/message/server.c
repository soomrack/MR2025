#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <signal.h>
#include <time.h>

#include <sys/utsname.h>
#include <sys/sysinfo.h>

#include "types.h"
#include "motor.h"

#define PORT 5000
#define BUF_SIZE 1024
#define MAX_CLIENTS 10
#define NAME_LEN 32

#define COLOR_RESET   "\033[0m"
#define COLOR_BOLD    "\033[1m"

static const char* user_colors[] = {
    "\033[1;31m",
    "\033[1;32m",
    "\033[1;33m",
    "\033[1;34m",
    "\033[1;35m",
    "\033[1;36m",
    "\033[1;91m",
    "\033[1;92m",
    "\033[1;93m",
    "\033[1;94m",
};

#define NUM_COLORS (sizeof(user_colors) / sizeof(user_colors[0]))

// ============================================================================
// КОНСТАНТЫ ЛОГИРОВАНИЯ
// ============================================================================

#define LOG_FILE         "server_stats.log"  // Файл лога (рядом с бинарником)
#define LOG_INTERVAL_SEC 10                  // Как часто писать в лог (секунды)

// ============================================================================
// СТРУКТУРЫ ДАННЫХ update: add types.h ServerCommand and Client is there
// ============================================================================

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
// ПРОТОТИПЫ: КОМАНДЫ ЛОГОВ
// ============================================================================

static void     cmd_log_last(Client clients[], int idx, const char *args);
static void     cmd_log_all(Client clients[], int idx, const char *args);
static void     cmd_log_clear(Client clients[], int idx, const char *args);

// ============================================================================
// ПРОТОТИПЫ: РАБОТА С ЛОГОМ И СИСТЕМНОЙ ИНФОРМАЦИЕЙ
// ============================================================================

static float    read_cpu_temp(void);
static void     log_write_entry(void);
static void     log_send_last(int sock, int n);
static void     log_send_all(int sock);

// ============================================================================
// ТАБЛИЦА КОМАНД
// ============================================================================

static const ServerCommand commands[] = {
    {"disconnect", "Disconnect from chat",          cmd_disconnect},
    {"help",       "Show available commands",       cmd_help},
    {"users",      "List active users",             cmd_users},
    {"OSinfo",     "Server information",            cmd_OSinfo},
    {"emoji",      "Show emoji shortcuts",          cmd_emoji},
    {"log_last",   "Show last 15 log entries",      cmd_log_last},
    {"log_all",    "Show entire log file",          cmd_log_all},
    {"log_clear",  "Clear the log file",            cmd_log_clear},
    {NULL, NULL, NULL}
};

// ============================================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ============================================================================

static volatile int g_server_running = 1;
static time_t       g_last_log_time  = 0;

static void signal_handler(int signum) {
    if (signum == SIGINT || signum == SIGTERM) {
        printf("\n[SIGNAL] Received shutdown signal\n");
        g_server_running = 0;
    }
}

// ============================================================================
// ГЛАВНАЯ ФУНКЦИЯ
// ============================================================================

int main(void) {
    ServerState state;

    printf("chat max10.0 STARTING\n\n");

    printf("Setting up signal handlers...\n");
    signal_handlers_setup();

    printf("Initializing server state...\n");
    server_state_init(&state);

    printf("Creating server socket on port %d...\n", PORT);
    state.server_fd = server_socket_create();
    if (state.server_fd < 0) {
        printf("[ERROR] Failed to create server socket\n");
        return 1;
    }

    printf("Initializing client array (max %d clients)...\n", MAX_CLIENTS);
    clients_array_init(state.clients);

    printf("Initializing motor module \n");
    if (motor_init() < 0) {
        printf("Motor module unavaliable \n");
    }

    printf("Log file: %s (interval: %ds)\n", LOG_FILE, LOG_INTERVAL_SEC);

    printf(" Server ready!\n\n");
    show_startup_message();

    printf("Starting main event loop...\n");
    printf("========================================\n\n");

    server_main_loop(&state);

    printf("\n Shutting down server...\n");

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (state.clients[i].sock > 0) {
            const char *msg = "[SERVER] Server is shutting down. Goodbye!\n";
            send(state.clients[i].sock, msg, strlen(msg), 0);
            client_disconnect(&state.clients[i]);
        }
    }

    close(state.server_fd);

    printf("Stopping motor module \n");
    motor_cleanup();

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
    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);
    signal(SIGPIPE, SIG_IGN);
}

static void show_startup_message(void) {
    printf("========================================\n");
    printf("  Chat MAX 10.0 is Running! 💬\n");
    printf("========================================\n");
    printf("Port: %d\n", PORT);
    printf("Max clients: %d\n", MAX_CLIENTS);
    printf("Features: Colors + Emoji + Stats Log!\n");
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

        struct timeval timeout;
        timeout.tv_sec  = 1;
        timeout.tv_usec = 0;

        int activity = select(max_sd + 1, &readfds, NULL, NULL, &timeout);

        if (activity < 0) {
            if (g_server_running) perror("select");
            break;
        }

        // -----------------------------------------------------------------
        // Периодическое логирование температуры и памяти в файл
        // -----------------------------------------------------------------
        time_t now = time(NULL);
        if (now - g_last_log_time >= LOG_INTERVAL_SEC) {
            log_write_entry();
            g_last_log_time = now;
        }

        if (activity == 0) continue;

        if (FD_ISSET(state->server_fd, &readfds)) {
            handle_new_connection(state);
        }

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

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock == 0) {
            clients[i].sock = fd;
            clients[i].named = 0;
            assign_color_to_client(clients, i);

            const char *welcome = " Welcome to the chat! \nEnter your name: ";
            send(fd, welcome, strlen(welcome), 0);

            printf("[CONNECTION] New client from %s:%d (slot %d, color index %d)\n",
                   inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), i, clients[i].color_index);
            return;
        }
    }

    const char *msg = "Server is full. Try again later.\n";
    send(fd, msg, strlen(msg), 0);
    close(fd);
    printf("[CONNECTION] Connection rejected: server full\n");
}

static void process_client_message(Client clients[], int idx) {
    char buffer[BUF_SIZE];
    int n = read(clients[idx].sock, buffer, BUF_SIZE - 1);

    if (n <= 0) {
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
    buffer[strcspn(buffer, "\n\r")] = '\0';

    if (strlen(buffer) == 0) return;

    if (!clients[idx].named) {
        process_name_setup(&clients[idx], buffer);
        announce_user_joined(clients, &clients[idx]);
    } else if (is_command(buffer)) {
        printf("[COMMAND] User '%s' executed: %s\n", clients[idx].name, buffer);
        process_command(clients, idx, buffer);
    } else {
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
    const char *command = cmd + 1;
    const char *args = strchr(command, ' ');
    char cmd_name[32];

    if (args) {
        size_t len = args - command;
        if (len >= sizeof(cmd_name)) len = sizeof(cmd_name) - 1;
        strncpy(cmd_name, command, len);
        cmd_name[len] = '\0';
        args++;
    } else {
        strncpy(cmd_name, command, sizeof(cmd_name) - 1);
        cmd_name[sizeof(cmd_name) - 1] = '\0';
        args = "";
    }

    for (int i = 0; commands[i].name != NULL; i++) {
        if (strcmp(cmd_name, commands[i].name) == 0) {
            commands[i].handler(clients, idx, args);
            return;
        }
    }

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
        {":)",      "😊"},
        {":smile:", "😊"},
        {":D",      "😂"},
        {":laugh:", "😂"},
        {":(",      "😢"},
        {":sad:",   "😢"},
        {":love:",  "😍"},
        {":cool:",  "😎"},
        {":heart:", "❤️"},
        {":fire:",  "🔥"},
        {NULL, NULL}
    };

    char result[BUF_SIZE * 2] = {0};
    const char *src = text;
    char *dst = result;
    size_t result_len = 0;

    while (*src && result_len < max_len - 1) {
        int replaced = 0;

        for (int i = 0; emoji_map[i].shortcut != NULL; i++) {
            size_t shortcut_len = strlen(emoji_map[i].shortcut);
            if (strncmp(src, emoji_map[i].shortcut, shortcut_len) == 0) {
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

        if (!replaced) {
            *dst++ = *src++;
            result_len++;
        }
    }

    *dst = '\0';
    strncpy(text, result, max_len - 1);
    text[max_len - 1] = '\0';
}

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
        if (clients[i].sock > 0 && clients[i].named) count++;
    }
    return count;
}

static int assign_color_to_client(Client clients[], int client_idx) {
    int color_usage[NUM_COLORS] = {0};

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (i != client_idx && clients[i].sock > 0 && clients[i].color_index >= 0) {
            color_usage[clients[i].color_index]++;
        }
    }

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
// РЕАЛИЗАЦИЯ: ЛОГИРОВАНИЕ СТАТИСТИКИ В ФАЙЛ
// ============================================================================

// Читает температуру CPU из sysfs (Raspberry Pi и большинство Linux).
// Возвращает температуру в градусах Цельсия или -1.0 при ошибке.
static float read_cpu_temp(void) {
    FILE *f = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!f) return -1.0f;
    int raw = 0;
    fscanf(f, "%d", &raw);
    fclose(f);
    return raw / 1000.0f;  // значение хранится в милли-градусах
}

// Дописывает одну строку статистики в конец файла лога.
static void log_write_entry(void) {
    struct sysinfo si;
    if (sysinfo(&si) != 0) {
        printf("[LOG] sysinfo() failed\n");
        return;
    }

    float temp = read_cpu_temp();

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

    unsigned long free_mb  = si.freeram  / 1024 / 1024;
    unsigned long total_mb = si.totalram / 1024 / 1024;

    FILE *f = fopen(LOG_FILE, "a");
    if (!f) {
        printf("[LOG] Cannot open log file: %s\n", LOG_FILE);
        return;
    }

    if (temp >= 0.0f) {
        fprintf(f, "[%s] CPU: %.1f C | RAM free: %lu MB / %lu MB\n",
                time_str, temp, free_mb, total_mb);
    } else {
        fprintf(f, "[%s] CPU: N/A | RAM free: %lu MB / %lu MB\n",
                time_str, free_mb, total_mb);
    }

    fclose(f);
    printf("[LOG] Written: CPU %.1f C, RAM free %lu MB\n", temp, free_mb);
}

// Считывает файл лога и отправляет клиенту последние n строк.
static void log_send_last(int sock, int n) {
    FILE *f = fopen(LOG_FILE, "r");
    if (!f) {
        const char *msg = "[LOG] Log file is empty or does not exist yet.\n";
        send(sock, msg, strlen(msg), 0);
        return;
    }

    // Читаем все строки в кольцевой буфер на 500 строк
    char  lines[500][BUF_SIZE];
    int   count = 0;

    while (fgets(lines[count % 500], BUF_SIZE, f)) {
        count++;
    }
    fclose(f);

    if (count == 0) {
        const char *msg = "[LOG] Log file is empty.\n";
        send(sock, msg, strlen(msg), 0);
        return;
    }

    int show = (count < n) ? count : n;

    char header[BUF_SIZE];
    snprintf(header, sizeof(header),
             "[LOG] === Last %d entries (of %d total) ===\n", show, count);
    send(sock, header, strlen(header), 0);

    // Вычисляем начало нужного нам диапазона в кольцевом буфере
    int stored = (count < 500) ? count : 500;
    int head   = count % 500;  // следующая позиция для записи
    int start  = (head - show + stored) % stored;

    for (int i = 0; i < show; i++) {
        int idx = (start + i) % stored;
        send(sock, lines[idx], strlen(lines[idx]), 0);
    }

    const char *footer = "[LOG] === End ===\n";
    send(sock, footer, strlen(footer), 0);
}

// Отправляет весь файл лога клиенту построчно.
static void log_send_all(int sock) {
    FILE *f = fopen(LOG_FILE, "r");
    if (!f) {
        const char *msg = "[LOG] Log file is empty or does not exist yet.\n";
        send(sock, msg, strlen(msg), 0);
        return;
    }

    const char *header = "[LOG] === Full log ===\n";
    send(sock, header, strlen(header), 0);

    char line[BUF_SIZE];
    while (fgets(line, sizeof(line), f)) {
        send(sock, line, strlen(line), 0);
    }
    fclose(f);

    const char *footer = "[LOG] === End ===\n";
    send(sock, footer, strlen(footer), 0);
}

// ============================================================================
// РЕАЛИЗАЦИЯ: КОМАНДЫ СЕРВЕРА (оригинальные)
// ============================================================================

static void cmd_disconnect(Client clients[], int idx, const char *args) {
    (void)args;
    const char *goodbye = "[SERVER] Goodbye! See you soon!\n";
    send(clients[idx].sock, goodbye, strlen(goodbye), 0);
    printf("[COMMAND] User '%s' disconnected via \\disconnect\n", clients[idx].name);
    if (clients[idx].named) {
        announce_user_left(clients, &clients[idx]);
    }
    client_disconnect(&clients[idx]);
}

static void cmd_help(Client clients[], int idx, const char *args) {
    (void)args;
    char help[BUF_SIZE * 2];
    int offset = 0;

    offset += snprintf(help + offset, sizeof(help) - offset,
                       "[SERVER] Available commands:\n");
    for (int i = 0; commands[i].name != NULL; i++) {
        offset += snprintf(help + offset, sizeof(help) - offset,
                           "  \\%-12s - %s\n",
                           commands[i].name, commands[i].description);
    }
    send(clients[idx].sock, help, strlen(help), 0);
}

static void cmd_users(Client clients[], int idx, const char *args) {
    (void)args;
    char msg[BUF_SIZE * 2];
    int offset = 0;
    int count  = 0;

    offset += snprintf(msg + offset, sizeof(msg) - offset,
                       "[SERVER] Active users:\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock > 0 && clients[i].named) {
            const char *color = user_colors[clients[i].color_index];
            offset += snprintf(msg + offset, sizeof(msg) - offset,
                               "  - %s%s%s%s\n",
                               color, clients[i].name, COLOR_RESET,
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
    (void)args;
    char msg[BUF_SIZE];
    int offset = 0;

    struct utsname sys_info;
    if (uname(&sys_info) != 0) {
        const char *err = "[SERVER] Failed to get OS info\n";
        send(clients[idx].sock, err, strlen(err), 0);
        return;
    }
    struct sysinfo s_info;
    if (sysinfo(&s_info) != 0) {
        const char *err = "[SERVER] Failed to get system info\n";
        send(clients[idx].sock, err, strlen(err), 0);
        return;
    }

    float temp = read_cpu_temp();

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
                       "Total RAM: %lu MB\n", s_info.totalram / 1024 / 1024);
    offset += snprintf(msg + offset, sizeof(msg) - offset,
                       "Free RAM: %lu MB\n",  s_info.freeram  / 1024 / 1024);
    offset += snprintf(msg + offset, sizeof(msg) - offset,
                       "1 min load avg: %.2f\n",
                       (float)s_info.loads[0] / 65536.0f);
    if (temp >= 0.0f) {
        offset += snprintf(msg + offset, sizeof(msg) - offset,
                           "CPU temp: %.1f C\n", temp);
    } else {
        offset += snprintf(msg + offset, sizeof(msg) - offset,
                           "CPU temp: N/A\n");
    }
    offset += snprintf(msg + offset, sizeof(msg) - offset,
                       "Active users: %d\n", count_active_users(clients));
    send(clients[idx].sock, msg, strlen(msg), 0);
}

static void cmd_emoji(Client clients[], int idx, const char *args) {
    (void)args;
    char msg[BUF_SIZE];
    int offset = 0;

    offset += snprintf(msg + offset, sizeof(msg) - offset,
                       "[SERVER] Available Emoji Shortcuts\n\n");
    offset += snprintf(msg + offset, sizeof(msg) - offset, "  :)  or  :smile:  -> smile\n");
    offset += snprintf(msg + offset, sizeof(msg) - offset, "  :D  or  :laugh:  -> laugh\n");
    offset += snprintf(msg + offset, sizeof(msg) - offset, "  :(  or  :sad:    -> sad\n");
    offset += snprintf(msg + offset, sizeof(msg) - offset, "  :love:           -> love\n");
    offset += snprintf(msg + offset, sizeof(msg) - offset, "  :cool:           -> cool\n");
    offset += snprintf(msg + offset, sizeof(msg) - offset, "  :heart:          -> heart\n");
    offset += snprintf(msg + offset, sizeof(msg) - offset, "  :fire:           -> fire\n");
    offset += snprintf(msg + offset, sizeof(msg) - offset,
                       "\nExample: Hello :) I am happy :love:\n");
    send(clients[idx].sock, msg, strlen(msg), 0);
}

// ============================================================================
// РЕАЛИЗАЦИЯ: КОМАНДЫ ЛОГОВ
// ============================================================================

// \log_last — показать 15 последних записей
static void cmd_log_last(Client clients[], int idx, const char *args) {
    (void)args;
    log_send_last(clients[idx].sock, 15);
}

// \log_all — показать весь файл лога
static void cmd_log_all(Client clients[], int idx, const char *args) {
    (void)args;
    log_send_all(clients[idx].sock);
}

// \log_clear — очистить файл лога
static void cmd_log_clear(Client clients[], int idx, const char *args) {
    (void)args;

    // Открыть файл в режиме "w" — это его обнуляет
    FILE *f = fopen(LOG_FILE, "w");
    if (f) {
        fclose(f);
        const char *ok = "[LOG] Log file cleared.\n";
        send(clients[idx].sock, ok, strlen(ok), 0);
        printf("[LOG] Log cleared by user '%s'\n", clients[idx].name);
    } else {
        const char *err = "[LOG] Failed to clear log file.\n";
        send(clients[idx].sock, err, strlen(err), 0);
    }
}