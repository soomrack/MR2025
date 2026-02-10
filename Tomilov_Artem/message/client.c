#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 5000
#define BUF_SIZE 1024

// Прототипы основных функций
static void client_run(void);
static int  connect_to_server(void);
static void chat_loop(int sockfd);

// Прототипы вспомогательных функций
static void fdset_prepare(int sockfd, fd_set *readfds, int *max_sd);
static int  handle_server_message(int sockfd, char *buffer);
static int  handle_user_input(int sockfd, char *buffer);
static int  is_client_command(const char *input);
static int  process_client_command(int sockfd, const char *input);

int main(void) {
    client_run();
    return 0;
}

static void client_run(void) {
    int sockfd = connect_to_server();
    printf("Connected to chat server.\n");
    printf("Commands: \\OSinfo, \\help, \\users, \\disconnect\n\n");
    chat_loop(sockfd);
}

static int connect_to_server(void) {
    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // Подключение к локальному серверу
    // Замените "127.0.0.1" на IP сервера в локальной сети если нужно
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("inet_pton"); 
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect"); 
        exit(1);
    }

    return sockfd; 
}

// ============================================================================
// ГЛАВНЫЙ ЦИКЛ ЧАТА (РЕФАКТОРИНГ)
// ============================================================================

static void chat_loop(int sockfd) {
    fd_set readfds;
    char buffer[BUF_SIZE];
    int running = 1;

    while (running) {
        int max_sd;
        fdset_prepare(sockfd, &readfds, &max_sd);
        
        if (select(max_sd + 1, &readfds, NULL, NULL, NULL) < 0) {
            perror("select");
            break;
        }

        // Проверяем сообщения от сервера
        if (FD_ISSET(sockfd, &readfds)) {
            if (!handle_server_message(sockfd, buffer)) {
                running = 0; // Сервер отключился
            }
        }

        // Проверяем ввод пользователя
        if (running && FD_ISSET(STDIN_FILENO, &readfds)) {
            if (!handle_user_input(sockfd, buffer)) {
                running = 0; // Пользователь вышел
            }
        }
    }

    close(sockfd);
    printf("Disconnected.\n");
}

static void fdset_prepare(int sockfd, fd_set *readfds, int *max_sd) {
    FD_ZERO(readfds);
    FD_SET(sockfd, readfds);
    FD_SET(STDIN_FILENO, readfds);
    *max_sd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;
}

// ============================================================================
// ОБРАБОТКА СООБЩЕНИЙ ОТ СЕРВЕРА
// ============================================================================

static int handle_server_message(int sockfd, char *buffer) {
    int n = read(sockfd, buffer, BUF_SIZE - 1);
    
    if (n <= 0) {
        if (n < 0) {
            perror("read");
        }
        printf("\n[DISCONNECTED] Connection to server lost\n");
        return 0; // Сигнал к выходу
    }
    
    buffer[n] = '\0';
    printf("%s", buffer);
    fflush(stdout);
    
    return 1; // Продолжаем
}

// ============================================================================
// ОБРАБОТКА ВВОДА ПОЛЬЗОВАТЕЛЯ
// ============================================================================

static int handle_user_input(int sockfd, char *buffer) {
    if (!fgets(buffer, BUF_SIZE, stdin)) {
        // EOF или ошибка чтения
        return 1; // Продолжаем
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
        return process_client_command(sockfd, buffer);
    }
    
    // Обычное сообщение - восстанавливаем перевод строки
    strcat(buffer, "\n");
    
    if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
        perror("send");
        return 0; // Выходим при ошибке отправки
    }
    
    return 1; // Продолжаем
}

static int is_client_command(const char *input) {
    return input[0] == '\\';
}

// ============================================================================
// ОБРАБОТКА КОМАНД КЛИЕНТА
// ============================================================================

static int process_client_command(int sockfd, const char *input) {
    const char *cmd = input + 1; // Пропускаем '\'
    
    // Локальная команда disconnect
    if (strcmp(cmd, "disconnect") == 0) {
        printf("[CLIENT] Disconnecting from server...\n");
        
        // Отправляем команду на сервер для корректного отключения
        const char *disconnect_msg = "\\disconnect\n";
        send(sockfd, disconnect_msg, strlen(disconnect_msg), 0);
        
        // Небольшая задержка, чтобы сервер успел обработать
        usleep(100000);
        
        return 0; // Сигнал к выходу
    }
    
    // Остальные команды отправляем на сервер
    char buffer[BUF_SIZE];
    snprintf(buffer, BUF_SIZE, "%s\n", input);
    
    if (send(sockfd, buffer, strlen(buffer), 0) < 0) {
        perror("send");
        return 0; // Выходим при ошибке
    }
    
    return 1; // Продолжаем
}