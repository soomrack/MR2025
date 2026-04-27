#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #pragma comment(lib, "ws2_32.lib")
    #define close(s) closesocket(s)
    #define sleep(sec) Sleep((sec)*1000)
    typedef int socklen_t;
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/select.h>
    #include <signal.h>
    #include <sys/utsname.h>
    #include <sys/sysinfo.h>
#endif

#include "types.h"
#include "motor.h"

#define PORT     5000
#define BUF_SIZE 1024

#define COLOR_RESET  "\033[0m"
#define COLOR_BOLD   "\033[1m"

static const char *user_colors[] = {
    "\033[1;31m", "\033[1;32m", "\033[1;33m", "\033[1;34m",
    "\033[1;35m", "\033[1;36m", "\033[1;91m", "\033[1;92m",
    "\033[1;93m", "\033[1;94m"
};
#define NUM_COLORS (sizeof(user_colors)/sizeof(user_colors[0]))

#define LOG_FILE         "server_stats.log"
#define LOG_INTERVAL_SEC 10

typedef struct {
    int    server_fd;
    Client clients[MAX_CLIENTS];
    int    running;
} ServerState;

static volatile int g_server_running = 1;
static time_t       g_last_log_time  = 0;

// ============================================================================
// КРОССПЛАТФОРМЕННЫЕ ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ
// ============================================================================

#ifdef _WIN32
    static BOOL WINAPI console_ctrl_handler(DWORD ctrl_type) {
        if (ctrl_type == CTRL_C_EVENT || ctrl_type == CTRL_BREAK_EVENT) {
            printf("\n[SIGNAL] Received shutdown signal\n");
            g_server_running = 0;
            return TRUE;
        }
        return FALSE;
    }
    static void signal_handlers_setup(void) {
        SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
    }
#else
    static void signal_handler(int signum) {
        if (signum == SIGINT || signum == SIGTERM) {
            printf("\n[SIGNAL] Received shutdown signal\n");
            g_server_running = 0;
        }
    }
    static void signal_handlers_setup(void) {
        signal(SIGINT, signal_handler);
        signal(SIGTERM, signal_handler);
        signal(SIGPIPE, SIG_IGN);
    }
#endif

// ============================================================================
// СИСТЕМНАЯ ИНФОРМАЦИЯ (CPU/RAM)
// ============================================================================

#ifdef _WIN32
    static float read_cpu_temp(void) {
        return -1.0f;
    }
    static void log_write_entry(void) {
        MEMORYSTATUSEX mem;
        mem.dwLength = sizeof(mem);
        GlobalMemoryStatusEx(&mem);
        unsigned long long free_mb  = mem.ullAvailPhys / 1024 / 1024;
        unsigned long long total_mb = mem.ullTotalPhys / 1024 / 1024;

        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

        FILE *f = fopen(LOG_FILE, "a");
        if (f) {
            fprintf(f, "[%s] [INFO] CPU temp: N/A | RAM free: %llu MB / %llu MB\n",
                    time_str, free_mb, total_mb);
            fclose(f);
        }
    }
#else
    static float read_cpu_temp(void) {
        FILE *f = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
        if (!f) return -1.0f;
        int raw = 0;
        fscanf(f, "%d", &raw);
        fclose(f);
        return raw / 1000.0f;
    }
    static void log_write_entry(void) {
        struct sysinfo si;
        if (sysinfo(&si) != 0) return;
        float temp = read_cpu_temp();
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char time_str[32];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        unsigned long free_mb  = si.freeram  / 1024 / 1024;
        unsigned long total_mb = si.totalram / 1024 / 1024;
        FILE *f = fopen(LOG_FILE, "a");
        if (!f) return;
        if (temp >= 0.0f) {
            if (temp >= 30.0f)
                fprintf(f, "[%s] [WARNING] CPU TEMP HIGH: %.1f C | RAM free: %lu MB / %lu MB\n",
                        time_str, temp, free_mb, total_mb);
            else
                fprintf(f, "[%s] [INFO] CPU: %.1f C | RAM free: %lu MB / %lu MB\n",
                        time_str, temp, free_mb, total_mb);
        } else {
            fprintf(f, "[%s] CPU: N/A | RAM free: %lu MB / %lu MB\n",
                    time_str, free_mb, total_mb);
        }
        fclose(f);
    }
#endif

// ============================================================================
// ЛОГИ: ОТПРАВКА КЛИЕНТУ
// ============================================================================

static void log_send_last(int sock, int n) {
    FILE *f = fopen(LOG_FILE, "r");
    if (!f) {
        const char *msg = "[LOG] Log file is empty or does not exist yet.\n";
        send(sock, msg, (int)strlen(msg), 0);
        return;
    }
    char lines[500][BUF_SIZE];
    int count = 0;
    while (fgets(lines[count % 500], BUF_SIZE, f)) count++;
    fclose(f);
    if (count == 0) {
        const char *msg = "[LOG] Log file is empty.\n";
        send(sock, msg, (int)strlen(msg), 0);
        return;
    }
    int show = (count < n) ? count : n;
    char header[BUF_SIZE];
    snprintf(header, sizeof(header), "[LOG] === Last %d entries (of %d total) ===\n", show, count);
    send(sock, header, (int)strlen(header), 0);
    int stored = (count < 500) ? count : 500;
    int head   = count % 500;
    int start  = (head - show + stored) % stored;
    for (int i = 0; i < show; i++) {
        int idx = (start + i) % stored;
        send(sock, lines[idx], (int)strlen(lines[idx]), 0);
    }
    const char *footer = "[LOG] === End ===\n";
    send(sock, footer, (int)strlen(footer), 0);
}

static void log_send_all(int sock) {
    FILE *f = fopen(LOG_FILE, "r");
    if (!f) {
        const char *msg = "[LOG] Log file is empty or does not exist yet.\n";
        send(sock, msg, (int)strlen(msg), 0);
        return;
    }
    const char *header = "[LOG] === Full log ===\n";
    send(sock, header, (int)strlen(header), 0);
    char line[BUF_SIZE];
    while (fgets(line, sizeof(line), f))
        send(sock, line, (int)strlen(line), 0);
    fclose(f);
    const char *footer = "[LOG] === End ===\n";
    send(sock, footer, (int)strlen(footer), 0);
}

// ============================================================================
// ОСТАЛЬНЫЕ ФУНКЦИИ СЕРВЕРА (ОБЩИЕ ДЛЯ ВСЕХ ОС)
// ============================================================================

static void server_state_init(ServerState *state) {
    state->server_fd = -1;
    state->running   = 1;
    memset(state->clients, 0, sizeof(state->clients));
}

static int server_socket_create(void) {
    int fd;
    struct sockaddr_in addr;

#ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("[ERROR] WSAStartup failed\n");
        return -1;
    }
#endif
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) {
        perror("socket");
        return -1;
    }
    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));

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

static void clients_array_init(Client clients[]) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].sock = 0;
        clients[i].named = 0;
        clients[i].color_index = -1;
        memset(clients[i].name, 0, NAME_LEN);
    }
}

static void show_startup_message(void) {
    printf("========================================\n");
    printf("  Chat MAX 228.0 is Running!\n");
    printf("========================================\n");
    printf("Port: %d\n", PORT);
    printf("Max clients: %d\n", MAX_CLIENTS);
    printf("Available commands:\n");
    printf("  \\help, \\users, \\OSinfo, \\log_last, \\log_all, \\log_clear\n");
    printf("  \\drive, \\drive_key, \\drive_speed, \\emoji\n");
    printf("========================================\n");
    printf("Press Ctrl+C to stop the server\n");
}

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
            const char *welcome = "Welcome to the chat!\nEnter your name: ";
            send(fd, welcome, (int)strlen(welcome), 0);
            printf("[CONNECTION] New client from %s:%d (slot %d)\n",
                   inet_ntoa(addr.sin_addr), ntohs(addr.sin_port), i);
            return;
        }
    }
    const char *msg = "Server is full. Try again later.\n";
    send(fd, msg, (int)strlen(msg), 0);
    close(fd);
    printf("[CONNECTION] Connection rejected: server full\n");
}

static int assign_color_to_client(Client clients[], int client_idx) {
    int color_usage[NUM_COLORS] = {0};
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (i != client_idx && clients[i].sock > 0 && clients[i].color_index >= 0)
            color_usage[clients[i].color_index]++;
    }
    int min_usage = color_usage[0], best_color = 0;
    for (int i = 1; i < NUM_COLORS; i++) {
        if (color_usage[i] < min_usage) {
            min_usage = color_usage[i];
            best_color = i;
        }
    }
    clients[client_idx].color_index = best_color;
    return best_color;
}

static void replace_emoji_shortcuts(char *text, size_t max_len) {
    struct { const char *shortcut; const char *emoji; } emoji_map[] = {
        {":)", "😊"}, {":smile:", "😊"}, {":D", "😂"}, {":laugh:", "😂"},
        {":(", "😢"}, {":sad:", "😢"}, {":love:", "😍"}, {":cool:", "😎"},
        {":heart:", "❤️"}, {":fire:", "🔥"}, {NULL, NULL}
    };
    char result[BUF_SIZE * 2] = {0};
    const char *src = text;
    char *dst = result;
    size_t result_len = 0;
    while (*src && result_len < max_len - 1) {
        int replaced = 0;
        for (int i = 0; emoji_map[i].shortcut; i++) {
            size_t slen = strlen(emoji_map[i].shortcut);
            if (strncmp(src, emoji_map[i].shortcut, slen) == 0) {
                size_t elen = strlen(emoji_map[i].emoji);
                if (result_len + elen < max_len - 1) {
                    strcpy(dst, emoji_map[i].emoji);
                    dst += elen;
                    src += slen;
                    result_len += elen;
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

static void broadcast(Client clients[], const char *msg, int except) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock > 0 && clients[i].named && i != except) {
            send(clients[i].sock, msg, (int)strlen(msg), 0);
        }
    }
}

static void client_disconnect(Client *client) {
    close(client->sock);
    client->sock = 0;
    client->named = 0;
    client->color_index = -1;
    memset(client->name, 0, NAME_LEN);
}

static int count_active_users(Client clients[]) {
    int cnt = 0;
    for (int i = 0; i < MAX_CLIENTS; i++)
        if (clients[i].sock > 0 && clients[i].named) cnt++;
    return cnt;
}

// ============================================================================
// ОБРАБОТЧИКИ КОМАНД
// ============================================================================

static void cmd_disconnect(Client clients[], int idx, const char *args) {
    (void)args;
    const char *goodbye = "[SERVER] Goodbye! See you soon!\n";
    send(clients[idx].sock, goodbye, (int)strlen(goodbye), 0);
    printf("[COMMAND] User '%s' disconnected\n", clients[idx].name);
    if (clients[idx].named) {
        char msg[BUF_SIZE];
        snprintf(msg, BUF_SIZE, "[SERVER] %s left the chat\n", clients[idx].name);
        broadcast(clients, msg, -1);
    }
    client_disconnect(&clients[idx]);
}

static void cmd_help(Client clients[], int idx, const char *args) {
    (void)args;
    char help[BUF_SIZE * 2];
    int off = snprintf(help, sizeof(help), "[SERVER] Available commands:\n");
    off += snprintf(help+off, sizeof(help)-off, "  \\help         - this help\n");
    off += snprintf(help+off, sizeof(help)-off, "  \\users        - list active users\n");
    off += snprintf(help+off, sizeof(help)-off, "  \\OSinfo       - server system info\n");
    off += snprintf(help+off, sizeof(help)-off, "  \\log_last     - last 15 log entries\n");
    off += snprintf(help+off, sizeof(help)-off, "  \\log_all      - full log\n");
    off += snprintf(help+off, sizeof(help)-off, "  \\log_clear    - clear log\n");
    off += snprintf(help+off, sizeof(help)-off, "  \\drive        - enter robot control\n");
    off += snprintf(help+off, sizeof(help)-off, "  \\drive_speed N- set speed 0-100\n");
    off += snprintf(help+off, sizeof(help)-off, "  \\emoji        - show emoji shortcuts\n");
    off += snprintf(help+off, sizeof(help)-off, "  \\disconnect   - leave chat\n");
    send(clients[idx].sock, help, off, 0);
}

static void cmd_users(Client clients[], int idx, const char *args) {
    (void)args;
    char msg[BUF_SIZE * 2];
    int off = snprintf(msg, sizeof(msg), "[SERVER] Active users:\n");
    int cnt = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock > 0 && clients[i].named) {
            const char *color = user_colors[clients[i].color_index];
            off += snprintf(msg+off, sizeof(msg)-off, "  - %s%s%s%s\n",
                            color, clients[i].name, COLOR_RESET,
                            (i==idx) ? " (you)" : "");
            cnt++;
        }
    }
    if (cnt == 0)
        snprintf(msg, sizeof(msg), "[SERVER] No users online\n");
    else
        snprintf(msg+off, sizeof(msg)-off, "Total: %d user%s\n", cnt, cnt>1?"s":"");
    send(clients[idx].sock, msg, (int)strlen(msg), 0);
}

static void cmd_emoji(Client clients[], int idx, const char *args) {
    (void)args;
    const char *msg = 
        "[SERVER] Available emoji shortcuts:\n"
        "  :)  or :smile: -> 😊\n"
        "  :D  or :laugh: -> 😂\n"
        "  :(  or :sad:   -> 😢\n"
        "  :love:         -> 😍\n"
        "  :cool:         -> 😎\n"
        "  :heart:        -> ❤️\n"
        "  :fire:         -> 🔥\n"
        "Type them in your messages to see emojis.\n";
    send(clients[idx].sock, msg, (int)strlen(msg), 0);
}

#ifdef _WIN32
static void cmd_OSinfo(Client clients[], int idx, const char *args) {
    (void)args;
    char msg[BUF_SIZE];
    int off = 0;
    off += snprintf(msg+off, sizeof(msg)-off, "[SERVER] === OS Information ===\n");
    off += snprintf(msg+off, sizeof(msg)-off, "OS: Windows (MinGW)\n");
    off += snprintf(msg+off, sizeof(msg)-off, "Architecture: %s\n",
                    sizeof(void*)==8 ? "64-bit" : "32-bit");
    MEMORYSTATUSEX mem;
    mem.dwLength = sizeof(mem);
    GlobalMemoryStatusEx(&mem);
    off += snprintf(msg+off, sizeof(msg)-off, "Total RAM: %llu MB\n", mem.ullTotalPhys / 1048576);
    off += snprintf(msg+off, sizeof(msg)-off, "Free RAM:  %llu MB\n", mem.ullAvailPhys / 1048576);
    off += snprintf(msg+off, sizeof(msg)-off, "CPU temp:  N/A\n");
    off += snprintf(msg+off, sizeof(msg)-off, "Active users: %d\n", count_active_users(clients));
    send(clients[idx].sock, msg, off, 0);
}
#else
static void cmd_OSinfo(Client clients[], int idx, const char *args) {
    (void)args;
    char msg[BUF_SIZE];
    int off = 0;
    struct utsname uts;
    uname(&uts);
    struct sysinfo si;
    sysinfo(&si);
    float temp = read_cpu_temp();
    off += snprintf(msg+off, sizeof(msg)-off, "[SERVER] === OS Information ===\n");
    off += snprintf(msg+off, sizeof(msg)-off, "OS: %s\n", uts.sysname);
    off += snprintf(msg+off, sizeof(msg)-off, "Hostname: %s\n", uts.nodename);
    off += snprintf(msg+off, sizeof(msg)-off, "Kernel: %s\n", uts.release);
    off += snprintf(msg+off, sizeof(msg)-off, "Architecture: %s\n", uts.machine);
    off += snprintf(msg+off, sizeof(msg)-off, "Uptime: %ld s\n", si.uptime);
    off += snprintf(msg+off, sizeof(msg)-off, "Total RAM: %lu MB\n", si.totalram / 1048576);
    off += snprintf(msg+off, sizeof(msg)-off, "Free RAM:  %lu MB\n", si.freeram / 1048576);
    if (temp >= 0)
        off += snprintf(msg+off, sizeof(msg)-off, "CPU temp: %.1f C\n", temp);
    else
        off += snprintf(msg+off, sizeof(msg)-off, "CPU temp: N/A\n");
    off += snprintf(msg+off, sizeof(msg)-off, "Active users: %d\n", count_active_users(clients));
    send(clients[idx].sock, msg, off, 0);
}
#endif

static void cmd_log_last(Client clients[], int idx, const char *args) {
    (void)args;
    log_send_last(clients[idx].sock, 15);
}
static void cmd_log_all(Client clients[], int idx, const char *args) {
    (void)args;
    log_send_all(clients[idx].sock);
}
static void cmd_log_clear(Client clients[], int idx, const char *args) {
    (void)args;
    FILE *f = fopen(LOG_FILE, "w");
    if (f) {
        fclose(f);
        const char *ok = "[LOG] Log file cleared.\n";
        send(clients[idx].sock, ok, (int)strlen(ok), 0);
        printf("[LOG] Cleared by user '%s'\n", clients[idx].name);
    } else {
        const char *err = "[LOG] Failed to clear log.\n";
        send(clients[idx].sock, err, (int)strlen(err), 0);
    }
}

// ============================================================================
// ТАБЛИЦА КОМАНД
// ============================================================================

static const ServerCommand commands[] = {
    {"disconnect",   "Disconnect",   cmd_disconnect},
    {"help",         "Help",         cmd_help},
    {"users",        "List users",   cmd_users},
    {"OSinfo",       "System info",  cmd_OSinfo},
    {"log_last",     "Last log",     cmd_log_last},
    {"log_all",      "All log",      cmd_log_all},
    {"log_clear",    "Clear log",    cmd_log_clear},
    {"drive",        "Drive mode",   cmd_drive},
    {"drive_key",    "Drive key",    cmd_drive_key},
    {"drive_speed",  "Set speed",    cmd_drive_speed},
    {"emoji",        "Show emojis",  cmd_emoji},
    {NULL, NULL, NULL}
};

// ============================================================================
// ОБРАБОТКА СООБЩЕНИЙ КЛИЕНТОВ
// ============================================================================

static int is_command(const char *msg) { return msg[0] == '\\'; }

static void process_command(Client clients[], int idx, const char *cmd) {
    const char *command = cmd + 1;
    const char *args = strchr(command, ' ');
    char cmd_name[32];
    if (args) {
        size_t len = args - command;
        if (len >= sizeof(cmd_name)) len = sizeof(cmd_name)-1;
        strncpy(cmd_name, command, len);
        cmd_name[len] = '\0';
        args++;
    } else {
        strncpy(cmd_name, command, sizeof(cmd_name)-1);
        cmd_name[sizeof(cmd_name)-1] = '\0';
        args = "";
    }
    for (int i = 0; commands[i].name; i++) {
        if (strcmp(cmd_name, commands[i].name) == 0) {
            commands[i].handler(clients, idx, args);
            return;
        }
    }
    char err[BUF_SIZE];
    snprintf(err, sizeof(err), "[SERVER] Unknown command: \\%s\n", cmd_name);
    send(clients[idx].sock, err, (int)strlen(err), 0);
}

static void process_regular_message(Client clients[], int idx, const char *msg) {
    char buffer[BUF_SIZE];
    char processed[BUF_SIZE];
    strncpy(processed, msg, BUF_SIZE-1);
    processed[BUF_SIZE-1] = '\0';
    replace_emoji_shortcuts(processed, BUF_SIZE);
    const char *color = user_colors[clients[idx].color_index];
    snprintf(buffer, BUF_SIZE, "%s[%s]%s %s\n", color, clients[idx].name, COLOR_RESET, processed);
    broadcast(clients, buffer, idx);
}

static void process_client_message(Client clients[], int idx) {
    char buffer[BUF_SIZE];
    int n = recv(clients[idx].sock, buffer, BUF_SIZE-1, 0);
    if (n <= 0) {
        if (clients[idx].named) {
            printf("[DISCONNECT] User '%s' disconnected\n", clients[idx].name);
            char leave_msg[BUF_SIZE];
            snprintf(leave_msg, BUF_SIZE, "[SERVER] %s left the chat\n", clients[idx].name);
            broadcast(clients, leave_msg, -1);
        } else {
            printf("[DISCONNECT] Unnamed client disconnected\n");
        }
        client_disconnect(&clients[idx]);
        return;
    }
    buffer[n] = '\0';
    buffer[strcspn(buffer, "\n\r")] = '\0';
    if (strlen(buffer) == 0) return;

    if (!clients[idx].named) {
        // первое сообщение – имя
        strncpy(clients[idx].name, buffer, NAME_LEN-1);
        clients[idx].name[NAME_LEN-1] = '\0';
        clients[idx].named = 1;
        assign_color_to_client(clients, idx);
        char welcome[BUF_SIZE];
        snprintf(welcome, BUF_SIZE, "[SERVER] Welcome, %s! Type \\help for commands.\n", clients[idx].name);
        send(clients[idx].sock, welcome, (int)strlen(welcome), 0);
        char join_msg[BUF_SIZE];
        snprintf(join_msg, BUF_SIZE, "[SERVER] %s joined the chat\n", clients[idx].name);
        broadcast(clients, join_msg, -1);
        printf("[USER] New user: %s (color %d)\n", clients[idx].name, clients[idx].color_index);
    } else if (is_command(buffer)) {
        printf("[COMMAND] %s: %s\n", clients[idx].name, buffer);
        process_command(clients, idx, buffer);
    } else {
        process_regular_message(clients, idx, buffer);
    }
}

// ============================================================================
// ГЛАВНЫЙ ЦИКЛ
// ============================================================================

static void server_main_loop(ServerState *state) {
    fd_set readfds;
    while (g_server_running && state->running) {
        int max_sd = state->server_fd;
        FD_ZERO(&readfds);
        FD_SET(state->server_fd, &readfds);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (state->clients[i].sock > 0) {
                FD_SET(state->clients[i].sock, &readfds);
                if (state->clients[i].sock > max_sd)
                    max_sd = state->clients[i].sock;
            }
        }
        struct timeval tv = {1, 0};
        int activity = select(max_sd + 1, &readfds, NULL, NULL, &tv);
        if (activity < 0) {
            if (g_server_running) perror("select");
            break;
        }
        time_t now = time(NULL);
        if (now - g_last_log_time >= LOG_INTERVAL_SEC) {
            log_write_entry();
            g_last_log_time = now;
        }
        uart_read_arduino(); // чтение данных от Arduino

        if (FD_ISSET(state->server_fd, &readfds))
            accept_new_client(state->server_fd, state->clients);
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (state->clients[i].sock > 0 && FD_ISSET(state->clients[i].sock, &readfds))
                process_client_message(state->clients, i);
        }
    }
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
    ServerState state;
    printf("Chat MAX 228.0 STARTING\n\n");
    signal_handlers_setup();
    server_state_init(&state);
    state.server_fd = server_socket_create();
    if (state.server_fd < 0) {
        printf("[ERROR] Failed to create socket\n");
        return 1;
    }
    clients_array_init(state.clients);
    if (motor_init() < 0)
        printf("[WARN] Motor module unavailable (UART)\n");
    show_startup_message();
    printf("Starting main loop...\n========================================\n");
    server_main_loop(&state);

    printf("\nShutting down...\n");
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (state.clients[i].sock > 0) {
            const char *msg = "[SERVER] Server shutting down. Goodbye!\n";
            send(state.clients[i].sock, msg, (int)strlen(msg), 0);
            client_disconnect(&state.clients[i]);
        }
    }
    close(state.server_fd);
    motor_cleanup();
#ifdef _WIN32
    WSACleanup();
#endif
    printf("[EXIT] Server stopped.\n");
    return 0;
}