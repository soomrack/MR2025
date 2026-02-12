#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <time.h>

#define PORT 5000
#define BUF_SIZE 1024
#define RECONNECT_TIMEOUT 60
#define RECONNECT_INTERVAL 3

// ============================================================================
// СТРУКТУРЫ И ГЛОБАЛЬНЫЕ КОНСТАНТЫ
// ============================================================================

typedef struct {
    int sockfd;
    int connected;
    time_t disconnect_time;
} ClientState;

// ============================================================================
// ПРОТОТИПЫ ФУНКЦИЙ: ИНИЦИАЛИЗАЦИЯ И УПРАВЛЕНИЕ ПОДКЛЮЧЕНИЕМ
// ============================================================================

static void     client_state_init(ClientState *state);
static int      connect_to_server(void);
static int      try_reconnect(ClientState *state);
static void     show_welcome_message(void);

// ============================================================================
// ПРОТОТИПЫ ФУНКЦИЙ: ОСНОВНОЙ ЦИКЛ ЧАТА
// ============================================================================

static int      chat_loop(ClientState *state);
static void     fdset_prepare(int sockfd, fd_set *readfds, int *max_sd);
static int      handle_server_message(ClientState *state, char *buffer);
static int      handle_user_input(ClientState *state, char *buffer);

// ============================================================================
// ПРОТОТИПЫ ФУНКЦИЙ: ОБРАБОТКА КОМАНД И СООБЩЕНИЙ
// ============================================================================

static int      is_client_command(const char *input);
static int      process_client_command(ClientState *state, const char *input);
static int      send_to_server(int sockfd, const char *message);

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
    ClientState state;
    int running = 1;
    
    printf("=== CHAT CLIENT STARTING ===\n\n");

    printf("Initializing client state...\n");
    client_state_init(&state);
    
    printf("Connecting to server at 127.0.0.1:%d...\n", PORT);
    state.sockfd = connect_to_server();
    if (state.sockfd < 0) {
        printf("[ERROR] Failed to connect to server\n");
        return 1;
    }
    state.connected = 1;
    printf("[SUCCESS] Connected to chat server\n\n");
    
    show_welcome_message();
    
    printf("Starting main chat loop...\n");
    printf("----------------------------------------\n\n");
    
    while (running) {
        // Основной цикл чата
        int result = chat_loop(&state);
        
        if (result == 0) {
            // Пользователь вышел намеренно
            running = 0;
        } else if (result == -1 && state.connected == 0) {
            // Потеряно соединение - пытаемся переподключиться
            printf("\nRECONNECT Connection lost. Attempting to reconnect...\n");
            
            if (try_reconnect(&state)) {
                printf("SUCCESS Reconnected to server!\n");
                show_welcome_message();
                // Продолжаем цикл
            } else {
                printf("ERROR Failed to reconnect. Exiting...\n");
                running = 0;
            }
        }
    }
    
    // ШАГ 5: Очистка ресурсов и выход
    printf("\nCleaning up and exiting...\n");
    if (state.connected && state.sockfd > 0) {
        close(state.sockfd);
    }
    printf("EXIT Goodbye!\n");
    
    return 0;
}

// ============================================================================
// РЕАЛИЗАЦИЯ: ИНИЦИАЛИЗАЦИЯ И УПРАВЛЕНИЕ ПОДКЛЮЧЕНИЕМ
// ============================================================================

static void client_state_init(ClientState *state) {
    state->sockfd = -1;
    state->connected = 0;
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
    server_addr.sin_port = htons(PORT);

    // Подключение к локальному серверу
    // Замените "127.0.0.1" на IP сервера в локальной сети если нужно
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
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
    int attempt = 0;
    
    state->disconnect_time = start_time;
    
    while (difftime(time(NULL), start_time) < RECONNECT_TIMEOUT) {
        attempt++;
        printf("RECONNECT Attempt #%d...\n", attempt);
        
        int new_sockfd = connect_to_server();
        if (new_sockfd >= 0) {
            state->sockfd = new_sockfd;
            state->connected = 1;
            state->disconnect_time = 0;
            return 1;
        }
        
        printf("RECONNECT Failed. Retrying in %d seconds...\n", RECONNECT_INTERVAL);
        sleep(RECONNECT_INTERVAL);
    }
    
    printf("RECONNECT Timeout reached (%d seconds)\n", RECONNECT_TIMEOUT);
    return 0;
}

static void show_welcome_message(void) {
    printf("========================================\n");
    printf(" Welcome to MAX 10.0!\n");
    printf("========================================\n");
    printf("Available commands:\n");
    printf("  \\help        - Show server commands\n");
    printf("  \\users       - List online users\n");
    printf("  \\emoji       - Show emoji shortcuts\n");
    printf("  \\OSinfo      - Show server OS info\n");
    printf("  \\disconnect  - Disconnect from server\n");
    printf("========================================\n");
    printf("Use emoji in your messages\n");
    printf("Type :) :D :heart: :fire: and more\n");
    printf("========================================\n\n");
}

// ============================================================================
// РЕАЛИЗАЦИЯ: ОСНОВНОЙ ЦИКЛ ЧАТА
// ============================================================================

static int chat_loop(ClientState *state) {
    fd_set readfds;
    char buffer[BUF_SIZE];
    
    if (!state->connected || state->sockfd < 0) {
        return -1;
    }
    
    while (state->connected) {
        int max_sd;
        fdset_prepare(state->sockfd, &readfds, &max_sd);
        
        // Используем timeout для проверки состояния
        struct timeval timeout;
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        
        int activity = select(max_sd + 1, &readfds, NULL, NULL, &timeout);
        
        if (activity < 0) {
            perror("select");
            state->connected = 0;
            return -1;
        }
        
        if (activity == 0) {
            // Timeout - просто продолжаем
            continue;
        }

        // Проверяем сообщения от сервера
        if (FD_ISSET(state->sockfd, &readfds)) {
            int result = handle_server_message(state, buffer);
            if (result <= 0) {
                return result; // -1 = потеря связи, 0 = выход пользователя
            }
        }

        // Проверяем ввод пользователя
        if (state->connected && FD_ISSET(STDIN_FILENO, &readfds)) {
            int result = handle_user_input(state, buffer);
            if (result == 0) {
                return 0; // Пользователь вышел
            }
        }
    }
    
    return -1; // Потеряно соединение
}

static void fdset_prepare(int sockfd, fd_set *readfds, int *max_sd) {
    FD_ZERO(readfds);
    FD_SET(sockfd, readfds);
    FD_SET(STDIN_FILENO, readfds);
    *max_sd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;
}

static int handle_server_message(ClientState *state, char *buffer) {
    int n = read(state->sockfd, buffer, BUF_SIZE - 1);
    
    if (n <= 0) {
        if (n < 0) {
            perror("read");
        }
        printf("\nConnection to server lost\n");
        state->connected = 0;
        close(state->sockfd);
        state->sockfd = -1;
        return -1; // Сигнал о потере связи
    }
    
    buffer[n] = '\0';
    printf("%s", buffer);
    fflush(stdout);
    
    return 1; // Продолжаем
}

static int handle_user_input(ClientState *state, char *buffer) {
    if (!fgets(buffer, BUF_SIZE, stdin)) {
        return 1; // EOF или ошибка чтения - продолжаем
    }
    
    // Убираем перевод строки для проверки команд
    size_t len = strlen(buffer);
    if (len > 0 && buffer[len - 1] == '\n') {
        buffer[len - 1] = '\0';
    }
    
    // Пропускаем пустые строки
    if (strlen(buffer) == 0) {
        return 1;
    }
    
    // Проверяем, является ли это командой
    if (is_client_command(buffer)) {
        return process_client_command(state, buffer);
    }
    
    // Обычное сообщение - восстанавливаем перевод строки
    strcat(buffer, "\n");
    
    if (!send_to_server(state->sockfd, buffer)) {
        state->connected = 0;
        return -1; // Ошибка отправки
    }
    
    return 1; // Продолжаем
}

// ============================================================================
// РЕАЛИЗАЦИЯ: ОБРАБОТКА КОМАНД И СООБЩЕНИЙ
// ============================================================================

static int is_client_command(const char *input) {
    return input[0] == '\\';
}

static int process_client_command(ClientState *state, const char *input) {
    const char *cmd = input + 1; // Пропускаем '\'
    
    // Локальная команда disconnect
    if (strcmp(cmd, "disconnect") == 0) {
        printf("[CLIENT] Disconnecting from server...\n");
        
        // Отправляем команду на сервер для корректного отключения
        const char *disconnect_msg = "\\disconnect\n";
        send_to_server(state->sockfd, disconnect_msg);
        
        // Небольшая задержка, чтобы сервер успел обработать
        sleep(100000);
        
        state->connected = 0;
        close(state->sockfd);
        state->sockfd = -1;
        
        return 0; // Сигнал к выходу
    }
    
    // Остальные команды отправляем на сервер
    char buffer[BUF_SIZE];
    snprintf(buffer, BUF_SIZE, "%s\n", input);
    
    if (!send_to_server(state->sockfd, buffer)) {
        state->connected = 0;
        return -1;
    }
    
    return 1; // Продолжаем
}

static int send_to_server(int sockfd, const char *message) {
    if (send(sockfd, message, strlen(message), 0) < 0) {
        perror("send");
        return 0;
    }
    return 1;
}