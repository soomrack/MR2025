#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #include <windows.h>
    #include <conio.h>
    #pragma comment(lib, "ws2_32.lib")
    #define close(s) closesocket(s)
    #define sleep(sec) Sleep((sec)*1000)
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/select.h>
    #include <termios.h>
#endif

#define PORT              5000
#define BUF_SIZE          1024
#define RECONNECT_TIMEOUT 60
#define RECONNECT_INTERVAL 3

#define DRIVE_MODE_START_MARKER  "DRIVE_MODE_START"
#define DRIVE_MODE_END_MARKER    "DRIVE_MODE_END"

// ============================================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ============================================================================

typedef struct {
    int sockfd;
    int connected;
    time_t disconnect_time;
} ClientState;

static int      g_drive_mode = 0;
#ifdef _WIN32
    static HANDLE g_stdin_handle = NULL;
    static DWORD  g_old_console_mode = 0;
#else
    static struct termios g_old_termios;
#endif

// ============================================================================
// ПРОТОТИПЫ
// ============================================================================

static void client_state_init(ClientState *state);
static int  connect_to_server(void);
static int  try_reconnect(ClientState *state);
static void show_welcome_message(void);
static void drive_mode_enter(void);
static void drive_mode_exit(void);
static int  chat_loop(ClientState *state);
static int  handle_server_message(ClientState *state, char *buffer);
static int  handle_user_input(ClientState *state);
static int  is_client_command(const char *input);
static int  process_client_command(ClientState *state, const char *input);
static int  send_to_server(int sockfd, const char *message);

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2,2), &wsaData) != 0) {
        printf("[ERROR] WSAStartup failed\n");
        return 1;
    }
#endif

    ClientState state;
    int running = 1;
    printf("=== CHAT CLIENT STARTING ===\n\n");
    client_state_init(&state);

    printf("Connecting to server at 127.0.0.1:%d...\n", PORT);
    state.sockfd = connect_to_server();
    if (state.sockfd < 0) {
        printf("[ERROR] Failed to connect to server\n");
#ifdef _WIN32
        WSACleanup();
#endif
        return 1;
    }
    state.connected = 1;
    printf("[OK] Connected to chat server\n\n");
    show_welcome_message();
    printf("Starting chat...\n----------------------------------------\n\n");

    while (running) {
        int result = chat_loop(&state);
        if (result == 0) {
            running = 0;
        } else if (result == -1 && state.connected == 0) {
            printf("\n[RECONNECT] Connection lost. Attempting to reconnect...\n");
            if (g_drive_mode) drive_mode_exit();
            if (try_reconnect(&state)) {
                printf("[OK] Reconnected to server!\n");
                show_welcome_message();
            } else {
                printf("[ERROR] Failed to reconnect. Exiting...\n");
                running = 0;
            }
        }
    }

    printf("\nCleaning up...\n");
    if (g_drive_mode) drive_mode_exit();
    if (state.connected && state.sockfd > 0) close(state.sockfd);
#ifdef _WIN32
    WSACleanup();
#endif
    printf("[EXIT] Goodbye!\n");
    return 0;
}

// ============================================================================
// ИНИЦИАЛИЗАЦИЯ И ПОДКЛЮЧЕНИЕ
// ============================================================================

static void client_state_init(ClientState *state) {
    state->sockfd = -1;
    state->connected = 0;
    state->disconnect_time = 0;
}

static int connect_to_server(void) {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) return -1;
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);
    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sockfd);
        return -1;
    }
    return sockfd;
}

static int try_reconnect(ClientState *state) {
    time_t start_time = time(NULL);
    int attempt = 0;
    state->disconnect_time = start_time;
    while (difftime(time(NULL), start_time) < RECONNECT_TIMEOUT) {
        attempt++;
        printf("[RECONNECT] Attempt #%d...\n", attempt);
        int new_sockfd = connect_to_server();
        if (new_sockfd >= 0) {
            state->sockfd = new_sockfd;
            state->connected = 1;
            state->disconnect_time = 0;
            return 1;
        }
        printf("[RECONNECT] Failed. Retrying in %d seconds...\n", RECONNECT_INTERVAL);
        sleep(RECONNECT_INTERVAL);
    }
    printf("[RECONNECT] Timeout reached (%d seconds)\n", RECONNECT_TIMEOUT);
    return 0;
}

static void show_welcome_message(void) {
    printf("========================================\n");
    printf(" Welcome to MAX 228.0!\n");
    printf("========================================\n");
    printf("Available commands:\n");
    printf("  \\help         - Show server commands\n");
    printf("  \\users        - List online users\n");
    printf("  \\emoji        - Show emoji shortcuts\n");
    printf("  \\OSinfo       - Show server OS info\n");
    printf("  \\drive        - Enter WASD robot control\n");
    printf("  \\drive_speed N- Set motor speed 0-100\n");
    printf("  \\disconnect   - Disconnect from server\n");
    printf("========================================\n\n");
}

// ============================================================================
// RAW-ТЕРМИНАЛ (WASD РЕЖИМ)
// ============================================================================

#ifdef _WIN32
static void drive_mode_enter(void) {
    HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
    GetConsoleMode(hStdin, &g_old_console_mode);
    DWORD mode = g_old_console_mode;
    mode &= ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT);
    SetConsoleMode(hStdin, mode);
    g_stdin_handle = hStdin;
    g_drive_mode = 1;
    printf("\n[DRIVE] Entering WASD mode (raw)\n");
    printf("[DRIVE] w=forward  s=back  a=left  d=right  space=stop  q=exit\n\n");
}

static void drive_mode_exit(void) {
    if (g_stdin_handle) {
        SetConsoleMode(g_stdin_handle, g_old_console_mode);
        g_stdin_handle = NULL;
    }
    g_drive_mode = 0;
    printf("\n[DRIVE] Exited WASD mode\n");
}
#else
static void drive_mode_enter(void) {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &g_old_termios);
    raw = g_old_termios;
    raw.c_lflag &= ~(ICANON | ECHO);
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    g_drive_mode = 1;
    printf("\n[DRIVE] Entering WASD mode\n");
    printf("[DRIVE] w=forward  s=back  a=left  d=right  space=stop  q=exit\n\n");
}

static void drive_mode_exit(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &g_old_termios);
    g_drive_mode = 0;
    printf("\n[DRIVE] Exited WASD mode\n");
}
#endif

// ============================================================================
// ОСНОВНОЙ ЦИКЛ (Windows и Linux версии)
// ============================================================================

#ifdef _WIN32
static int chat_loop(ClientState *state) {
    char buffer[BUF_SIZE];
    if (!state->connected || state->sockfd < 0) return -1;

    // Настраиваем сокет на неблокирующий режим (для возможности таймаута)
    u_long mode = 1;
    ioctlsocket(state->sockfd, FIONBIO, &mode);

    while (state->connected) {
        // Проверяем данные от сервера (неблокирующий recv)
        int n = recv(state->sockfd, buffer, BUF_SIZE - 1, 0);
        if (n > 0) {
            buffer[n] = '\0';
            // Обрабатываем сообщение
            if (strstr(buffer, DRIVE_MODE_START_MARKER)) {
                char *marker = strstr(buffer, DRIVE_MODE_START_MARKER);
                *marker = '\0';
                printf("%s", buffer);
                fflush(stdout);
                drive_mode_enter();
            } else if (strstr(buffer, DRIVE_MODE_END_MARKER)) {
                char *marker = strstr(buffer, DRIVE_MODE_END_MARKER);
                *marker = '\0';
                printf("%s", buffer);
                fflush(stdout);
                drive_mode_exit();
            } else {
                printf("%s", buffer);
                fflush(stdout);
            }
        } else if (n == 0) {
            // Сервер закрыл соединение
            printf("\nConnection closed by server\n");
            state->connected = 0;
            close(state->sockfd);
            state->sockfd = -1;
            return -1;
        } else {
            int err = WSAGetLastError();
            if (err != WSAEWOULDBLOCK) {
                printf("\nrecv error: %d\n", err);
                state->connected = 0;
                close(state->sockfd);
                state->sockfd = -1;
                return -1;
            }
        }

        // Обработка ввода пользователя
        int result = handle_user_input(state);
        if (result == 0) return 0;
        if (result == -1) {
            state->connected = 0;
            return -1;
        }

        // Небольшая задержка, чтобы не грузить CPU
        Sleep(50);
    }
    return -1;
}
#else
static int chat_loop(ClientState *state) {
    fd_set readfds;
    char buffer[BUF_SIZE];
    if (!state->connected || state->sockfd < 0) return -1;

    while (state->connected) {
        FD_ZERO(&readfds);
        FD_SET(state->sockfd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);
        int max_sd = (state->sockfd > STDIN_FILENO) ? state->sockfd : STDIN_FILENO;
        struct timeval tv = {1, 0};
        int activity = select(max_sd + 1, &readfds, NULL, NULL, &tv);
        if (activity < 0) {
            perror("select");
            break;
        }
        if (activity == 0) continue;

        if (FD_ISSET(state->sockfd, &readfds)) {
            int n = recv(state->sockfd, buffer, BUF_SIZE - 1, 0);
            if (n <= 0) {
                if (n < 0) perror("recv");
                printf("\nConnection to server lost\n");
                state->connected = 0;
                close(state->sockfd);
                state->sockfd = -1;
                return -1;
            }
            buffer[n] = '\0';
            if (strstr(buffer, DRIVE_MODE_START_MARKER)) {
                char *marker = strstr(buffer, DRIVE_MODE_START_MARKER);
                *marker = '\0';
                printf("%s", buffer);
                fflush(stdout);
                drive_mode_enter();
            } else if (strstr(buffer, DRIVE_MODE_END_MARKER)) {
                char *marker = strstr(buffer, DRIVE_MODE_END_MARKER);
                *marker = '\0';
                printf("%s", buffer);
                fflush(stdout);
                drive_mode_exit();
            } else {
                printf("%s", buffer);
                fflush(stdout);
            }
        }
        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            int result = handle_user_input(state);
            if (result == 0) return 0;
            if (result == -1) {
                state->connected = 0;
                return -1;
            }
        }
    }
    return -1;
}
#endif

// ============================================================================
// ОБРАБОТКА ВВОДА ПОЛЬЗОВАТЕЛЯ (общая для обеих ОС)
// ============================================================================

static int handle_user_input(ClientState *state) {
    if (g_drive_mode) {
#ifdef _WIN32
        if (_kbhit()) {
            int key = _getch();
            if (key == 224 || key == 0) { // стрелки – игнорируем
                _getch();
                return 1;
            }
            char ch = (char)key;
#else
        char ch;
        if (read(STDIN_FILENO, &ch, 1) <= 0) return 1;
#endif
            if (ch == 'q') {
                send_to_server(state->sockfd, "\\drive_key q\n");
                return 1;
            }
            char msg[32];
            snprintf(msg, sizeof(msg), "\\drive_key %c\n", ch);
            if (!send_to_server(state->sockfd, msg)) {
                return -1;
            }
        }
        return 1;
    }

    // Обычный режим – ввод строки
#ifdef _WIN32
    // На Windows используем fgets, но только если есть данные
    if (_kbhit()) {
        char buffer[BUF_SIZE];
        if (!fgets(buffer, BUF_SIZE, stdin)) return 1;
        size_t len = strlen(buffer);
        if (len > 0 && buffer[len-1] == '\n') buffer[--len] = '\0';
        if (len == 0) return 1;

        if (is_client_command(buffer)) {
            return process_client_command(state, buffer);
        }
        strcat(buffer, "\n");
        if (!send_to_server(state->sockfd, buffer)) return -1;
    }
#else
    // На Linux используем fgets блокирующе, но select уже показал, что stdin готов
    char buffer[BUF_SIZE];
    if (!fgets(buffer, BUF_SIZE, stdin)) return 1;
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len-1] == '\n') buffer[--len] = '\0';
    if (len == 0) return 1;

    if (is_client_command(buffer)) {
        return process_client_command(state, buffer);
    }
    strcat(buffer, "\n");
    if (!send_to_server(state->sockfd, buffer)) return -1;
#endif
    return 1;
}

static int is_client_command(const char *input) {
    return input[0] == '\\';
}

static int process_client_command(ClientState *state, const char *input) {
    const char *cmd = input + 1;
    if (strcmp(cmd, "disconnect") == 0) {
        printf("[CLIENT] Disconnecting...\n");
        send_to_server(state->sockfd, "\\disconnect\n");
        state->connected = 0;
        close(state->sockfd);
        state->sockfd = -1;
        return 0;
    }
    char send_buf[BUF_SIZE];
    snprintf(send_buf, BUF_SIZE, "%s\n", input);
    if (!send_to_server(state->sockfd, send_buf)) {
        state->connected = 0;
        return -1;
    }
    return 1;
}

static int send_to_server(int sockfd, const char *message) {
    if (send(sockfd, message, (int)strlen(message), 0) < 0) {
        perror("send");
        return 0;
    }
    return 1;
}