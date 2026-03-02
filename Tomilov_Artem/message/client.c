#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <termios.h>
#include <time.h>

#define PORT              5000
#define BUF_SIZE          1024
#define RECONNECT_TIMEOUT 60
#define RECONNECT_INTERVAL 3

// Маркер от сервера: сервер посылает эту строку когда обрабатывает \drive
#define DRIVE_MODE_START_MARKER  "DRIVE_MODE_START"
// Маркер от сервера: выход из режима (после нажатия q)
#define DRIVE_MODE_END_MARKER    "DRIVE_MODE_END"

// ============================================================================
// СТРУКТУРЫ И ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ============================================================================

typedef struct {
    int sockfd;
    int connected;
    time_t disconnect_time;
} ClientState;

// Состояние WASD-режима
static int             g_drive_mode = 0;
static struct termios  g_old_termios;

// ============================================================================
// ПРОТОТИПЫ: ИНИЦИАЛИЗАЦИЯ И ПОДКЛЮЧЕНИЕ
// ============================================================================

static void client_state_init(ClientState *state);
static int  connect_to_server(void);
static int  try_reconnect(ClientState *state);
static void show_welcome_message(void);

// ============================================================================
// ПРОТОТИПЫ: WASD DRIVE MODE
// ============================================================================

static void drive_mode_enter(void);
static void drive_mode_exit(void);

// ============================================================================
// ПРОТОТИПЫ: ОСНОВНОЙ ЦИКЛ ЧАТА
// ============================================================================

static int  chat_loop(ClientState *state);
static void fdset_prepare(int sockfd, fd_set *readfds, int *max_sd);
static int  handle_server_message(ClientState *state, char *buffer);
static int  handle_user_input(ClientState *state, char *buffer);

// ============================================================================
// ПРОТОТИПЫ: ОБРАБОТКА КОМАНД И СООБЩЕНИЙ
// ============================================================================

static int is_client_command(const char *input);
static int process_client_command(ClientState *state, const char *input);
static int send_to_server(int sockfd, const char *message);

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
    ClientState state;
    int running = 1;

    printf("=== CHAT CLIENT STARTING ===\n\n");

    client_state_init(&state);

    printf("Connecting to server at 127.0.0.1:%d...\n", PORT);
    state.sockfd = connect_to_server();
    if (state.sockfd < 0) {
        printf("[ERROR] Failed to connect to server\n");
        return 1;
    }
    state.connected = 1;
    printf("[OK] Connected to chat server\n\n");

    show_welcome_message();

    printf("Starting chat...\n");
    printf("----------------------------------------\n\n");

    while (running) {
        int result = chat_loop(&state);

        if (result == 0) {
            // Пользователь вышел намеренно
            running = 0;
        } else if (result == -1 && state.connected == 0) {
            // Потеряно соединение — пытаемся переподключиться
            printf("\n[RECONNECT] Connection lost. Attempting to reconnect...\n");

            // Если были в drive-режиме — восстанавливаем терминал
            if (g_drive_mode) {
                drive_mode_exit();
            }

            if (try_reconnect(&state)) {
                printf("[OK] Reconnected to server!\n");
                show_welcome_message();
            } else {
                printf("[ERROR] Failed to reconnect. Exiting...\n");
                running = 0;
            }
        }
    }

    printf("\nCleaning up and exiting...\n");

    // На случай если терминал остался в raw-режиме
    if (g_drive_mode) {
        drive_mode_exit();
    }

    if (state.connected && state.sockfd > 0) {
        close(state.sockfd);
    }

    printf("[EXIT] Goodbye!\n");
    return 0;
}

// ============================================================================
// РЕАЛИЗАЦИЯ: ИНИЦИАЛИЗАЦИЯ И ПОДКЛЮЧЕНИЕ
// ============================================================================

static void client_state_init(ClientState *state) {
    state->sockfd         = -1;
    state->connected      = 0;
    state->disconnect_time = 0;
}

static int connect_to_server(void) {
    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        return -1;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(PORT);

    // Замените на IP сервера если нужно
    if (inet_pton(AF_INET, "192.168.0.117", &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sockfd);
        return -1;
    }

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sockfd);
        return -1;
    }

    return sockfd;
}

static int try_reconnect(ClientState *state) {
    time_t start_time = time(NULL);
    int    attempt    = 0;

    state->disconnect_time = start_time;

    while (difftime(time(NULL), start_time) < RECONNECT_TIMEOUT) {
        attempt++;
        printf("[RECONNECT] Attempt #%d...\n", attempt);

        int new_sockfd = connect_to_server();
        if (new_sockfd >= 0) {
            state->sockfd          = new_sockfd;
            state->connected       = 1;
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
    printf(" Welcome to MAX 10.0!\n");
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
// РЕАЛИЗАЦИЯ: WASD DRIVE MODE
// ============================================================================

// Переключаем терминал в raw-режим:
// - без буферизации строк (ICANON выключен)
// - без эха символов (ECHO выключен)
// Это позволяет читать каждое нажатие клавиши мгновенно.
static void drive_mode_enter(void) {
    struct termios raw;
    tcgetattr(STDIN_FILENO, &g_old_termios);
    raw = g_old_termios;

    raw.c_lflag &= ~(ICANON | ECHO);   // убираем буферизацию и эхо
    raw.c_cc[VMIN]  = 1;               // читаем по одному символу
    raw.c_cc[VTIME] = 0;               // без таймаута

    tcsetattr(STDIN_FILENO, TCSANOW, &raw);
    g_drive_mode = 1;

    printf("\n[DRIVE] Entering WASD mode\n");
    printf("[DRIVE] w=forward  s=back  a=left  d=right  space=stop  q=exit\n\n");
}

// Восстанавливаем оригинальные настройки терминала.
static void drive_mode_exit(void) {
    tcsetattr(STDIN_FILENO, TCSANOW, &g_old_termios);
    g_drive_mode = 0;
    printf("\n[DRIVE] Exited WASD mode\n");
}

// ============================================================================
// РЕАЛИЗАЦИЯ: ОСНОВНОЙ ЦИКЛ ЧАТА
// ============================================================================

static int chat_loop(ClientState *state) {
    fd_set readfds;
    char   buffer[BUF_SIZE];

    if (!state->connected || state->sockfd < 0) {
        return -1;
    }

    while (state->connected) {
        int max_sd;
        fdset_prepare(state->sockfd, &readfds, &max_sd);

        struct timeval timeout;
        timeout.tv_sec  = 1;
        timeout.tv_usec = 0;

        int activity = select(max_sd + 1, &readfds, NULL, NULL, &timeout);

        if (activity < 0) {
            perror("select");
            state->connected = 0;
            return -1;
        }

        if (activity == 0) {
            continue;
        }

        // Сообщения от сервера
        if (FD_ISSET(state->sockfd, &readfds)) {
            int result = handle_server_message(state, buffer);
            if (result <= 0) {
                return result;
            }
        }

        // Ввод пользователя
        if (state->connected && FD_ISSET(STDIN_FILENO, &readfds)) {
            int result = handle_user_input(state, buffer);
            if (result == 0) {
                return 0;
            }
        }
    }

    return -1;
}

static void fdset_prepare(int sockfd, fd_set *readfds, int *max_sd) {
    FD_ZERO(readfds);
    FD_SET(sockfd, readfds);
    FD_SET(STDIN_FILENO, readfds);
    *max_sd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;
}

// Обрабатывает входящее сообщение от сервера.
// Проверяет маркеры DRIVE_MODE_START и DRIVE_MODE_END
// чтобы синхронно переключать режим терминала.
static int handle_server_message(ClientState *state, char *buffer) {
    int n = read(state->sockfd, buffer, BUF_SIZE - 1);

    if (n <= 0) {
        if (n < 0) perror("read");
        printf("\nConnection to server lost\n");
        state->connected = 0;
        close(state->sockfd);
        state->sockfd = -1;
        return -1;
    }

    buffer[n] = '\0';

    // Проверяем маркер входа в drive-режим
    if (strstr(buffer, DRIVE_MODE_START_MARKER)) {
        // Выводим сообщение сервера (справку) без маркера
        // Маркер нужен только нам, пользователю его показывать не надо
        char *marker = strstr(buffer, DRIVE_MODE_START_MARKER);
        *marker = '\0';
        printf("%s", buffer);
        fflush(stdout);
        drive_mode_enter();
        return 1;
    }

    // Проверяем маркер выхода из drive-режима
    if (strstr(buffer, DRIVE_MODE_END_MARKER)) {
        char *marker = strstr(buffer, DRIVE_MODE_END_MARKER);
        *marker = '\0';
        printf("%s", buffer);
        fflush(stdout);
        drive_mode_exit();
        return 1;
    }

    printf("%s", buffer);
    fflush(stdout);
    return 1;
}

// Обрабатывает нажатие клавиши или ввод строки от пользователя.
// В drive-режиме: каждое нажатие сразу шлётся как \drive_key X.
// В обычном режиме: читаем целую строку и обрабатываем как команду/сообщение.
static int handle_user_input(ClientState *state, char *buffer) {
    // -------------------------------------------------------
    // WASD DRIVE MODE — читаем по одному символу без \n
    // -------------------------------------------------------
    if (g_drive_mode) {
        char key;
        int  n = read(STDIN_FILENO, &key, 1);
        if (n <= 0) return 1;

        if (key == 'q') {
            // Отправляем q на сервер, сервер остановит моторы
            // и пришлёт DRIVE_MODE_END — тогда мы выйдем из режима
            char msg[32];
            snprintf(msg, sizeof(msg), "\\drive_key q\n");
            if (!send_to_server(state->sockfd, msg)) {
                state->connected = 0;
                return -1;
            }
            return 1;
        }

        // Отправляем нажатую клавишу на сервер
        char msg[32];
        // Пробел передаём как отдельный символ
        snprintf(msg, sizeof(msg), "\\drive_key %c\n", key);
        if (!send_to_server(state->sockfd, msg)) {
            state->connected = 0;
            return -1;
        }
        return 1;
    }

    // -------------------------------------------------------
    // ОБЫЧНЫЙ РЕЖИМ — читаем строку целиком
    // -------------------------------------------------------
    if (!fgets(buffer, BUF_SIZE, stdin)) {
        return 1;  // EOF или ошибка — продолжаем
    }

    // Убираем \n для проверки команд
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
        len--;
    }

    // Пропускаем пустые строки
    if (len == 0) return 1;

    // Проверяем на клиентскую команду
    if (is_client_command(buffer)) {
        return process_client_command(state, buffer);
    }

    // Обычное сообщение — восстанавливаем \n и шлём
    strcat(buffer, "\n");
    if (!send_to_server(state->sockfd, buffer)) {
        state->connected = 0;
        return -1;
    }

    return 1;
}

// ============================================================================
// РЕАЛИЗАЦИЯ: ОБРАБОТКА КОМАНД И СООБЩЕНИЙ
// ============================================================================

static int is_client_command(const char *input) {
    return input[0] == '\\';
}

static int process_client_command(ClientState *state, const char *input) {
    const char *cmd = input + 1;  // пропускаем '\'

    // Локальная команда disconnect
    if (strcmp(cmd, "disconnect") == 0) {
        printf("[CLIENT] Disconnecting from server...\n");

        const char *disconnect_msg = "\\disconnect\n";
        send_to_server(state->sockfd, disconnect_msg);

        sleep(100000);  // 100ms — дать серверу обработать

        state->connected = 0;
        close(state->sockfd);
        state->sockfd = -1;
        return 0;  // сигнал к выходу
    }

    // Команда \drive — сервер обработает и пришлёт DRIVE_MODE_START
    // handle_server_message поймает маркер и вызовет drive_mode_enter()
    // Здесь просто отправляем команду серверу как обычно

    // Все остальные команды шлём на сервер
    char send_buf[BUF_SIZE];
    snprintf(send_buf, BUF_SIZE, "%s\n", input);

    if (!send_to_server(state->sockfd, send_buf)) {
        state->connected = 0;
        return -1;
    }

    return 1;
}

static int send_to_server(int sockfd, const char *message) {
    if (send(sockfd, message, strlen(message), 0) < 0) {
        perror("send");
        return 0;
    }
    return 1;
}