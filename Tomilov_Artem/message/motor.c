#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <time.h>
#include <sys/socket.h>

#include "motor.h"

// ============================================================================
// ВНУТРЕННИЕ ПЕРЕМЕННЫЕ МОДУЛЯ
// ============================================================================

static int g_uart_fd    = -1;   // Файловый дескриптор UART порта
static int g_speed      = MOTOR_SPEED_DEFAULT;  // Текущая скорость 0-100
static int g_initialized = 0;  // Флаг: инициализирован ли модуль

// ============================================================================
// ВНУТРЕННИЕ ПРОТОТИПЫ
// ============================================================================

static int  uart_open(void);
static void uart_send(const char *cmd);
static void motor_log(const char *user, const char *event);
static void motor_reply(int sock, const char *msg);

// ============================================================================
// РЕАЛИЗАЦИЯ: ИНИЦИАЛИЗАЦИЯ
// ============================================================================

static int uart_open(void) {
    int fd = open(MOTOR_UART_PORT, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        perror("[MOTOR] open UART");
        return -1;
    }

    struct termios options;
    tcgetattr(fd, &options);

    // Устанавливаем скорость
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    // Режим 8N1: 8 бит, без чётности, 1 стоп-бит
    options.c_cflag &= ~PARENB;         // без чётности
    options.c_cflag &= ~CSTOPB;         // 1 стоп-бит
    options.c_cflag &= ~CSIZE;
    options.c_cflag |=  CS8;            // 8 бит данных
    options.c_cflag |=  CLOCAL | CREAD; // включаем приём

    // Сырой режим — без обработки символов
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_oflag &= ~OPOST;

    tcsetattr(fd, TCSANOW, &options);
    return fd;
}

int motor_init(void) {
    g_uart_fd = uart_open();
    if (g_uart_fd < 0) {
        printf("[MOTOR] Failed to open UART port: %s\n", MOTOR_UART_PORT);
        return -1;
    }

    g_speed       = MOTOR_SPEED_DEFAULT;
    g_initialized = 1;

    printf("[MOTOR] UART opened: %s  %d \n", MOTOR_UART_PORT, MOTOR_BAUD_RATE);
    printf("[MOTOR] Default speed: %d%%\n", g_speed);
    printf("[MOTOR] Log file: %s\n", MOTOR_LOG_FILE);

    // Отправляем STOP при старте
    uart_send("STOP\n");
    motor_log("SERVER", "INIT — motors stopped at startup");

    return 0;
}

void motor_cleanup(void) {
    if (g_uart_fd >= 0) {
        uart_send("STOP\n");
        motor_log("SERVER", "SHUTDOWN — motors stopped");
        close(g_uart_fd);
        g_uart_fd = -1;
    }
    g_initialized = 0;
}

// ============================================================================
// UART ОТПРАВКА
// ============================================================================

// Отправляет строку-команду на STM32 через UART.
static void uart_send(const char *cmd) {
    int len  = strlen(cmd);
    int sent = write(g_uart_fd, cmd, len);
    if (sent < 0) {
        perror("[MOTOR] UART write");
    } else {
        printf("[MOTOR] Sent to STM32: %s", cmd);
    }
}

// ============================================================================
// СКОРОСТЬ
// ============================================================================

void motor_set_speed(int speed) {
    if (speed < MOTOR_SPEED_MIN) speed = MOTOR_SPEED_MIN;
    if (speed > MOTOR_SPEED_MAX) speed = MOTOR_SPEED_MAX;
    g_speed = speed;

    // Формируем команду: "SPEED:75\n"
    char cmd[32];
    snprintf(cmd, sizeof(cmd), "SPEED:%d\n", g_speed);
    uart_send(cmd);
}

int motor_get_speed(void) {
    return g_speed;
}

// ============================================================================
// РЕАЛИЗАЦИЯ: ЛОГИРОВАНИЕ
// ============================================================================

// Дописывает запись в motor.log.
// Формат: [2026-02-24 14:05:00] USER: СОБЫТИЕ
static void motor_log(const char *user, const char *event) {
    FILE *f = fopen(MOTOR_LOG_FILE, "a");
    if (!f) {
        printf("[MOTOR] Cannot open log file: %s\n", MOTOR_LOG_FILE);
        return;
    }

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(f, "[%s] %s: %s\n", time_str, user, event);
    fclose(f);
}

// Отправляет короткое сообщение клиенту чата.
static void motor_reply(int sock, const char *msg) {
    send(sock, msg, strlen(msg), 0);
}

// ============================================================================
// РЕАЛИЗАЦИЯ: ОБРАБОТЧИКИ КОМАНД ЧАТА
// ============================================================================

// \drive_forward — двигаться вперёд
void cmd_drive_forward(Client clients[], int idx, const char *args) {
    (void)args;

    if (!g_initialized) {
        motor_reply(clients[idx].sock,
                    "[MOTOR] Error: motor module not initialized.\n");
        return;
    }

    // Сначала отправляем текущую скорость, потом направление
    char speed_cmd[32];
    snprintf(speed_cmd, sizeof(speed_cmd), "SPEED:%d\n", g_speed);
    uart_send(speed_cmd);
    uart_send("FORWARD\n");

    // Логируем: кто и что сделал
    char log_entry[128];
    snprintf(log_entry, sizeof(log_entry),
             "DRIVE FORWARD — speed %d%%", g_speed);
    motor_log(clients[idx].name, log_entry);

    // Отвечаем клиенту
    char reply[128];
    snprintf(reply, sizeof(reply),
             "[MOTOR] Driving FORWARD at speed %d%%\n", g_speed);
    motor_reply(clients[idx].sock, reply);
}

// \drive_back — двигаться назад
void cmd_drive_back(Client clients[], int idx, const char *args) {
    (void)args;

    if (!g_initialized) {
        motor_reply(clients[idx].sock,
                    "[MOTOR] Error: motor module not initialized.\n");
        return;
    }

    char speed_cmd[32];
    snprintf(speed_cmd, sizeof(speed_cmd), "SPEED:%d\n", g_speed);
    uart_send(speed_cmd);
    uart_send("BACK\n");

    char log_entry[128];
    snprintf(log_entry, sizeof(log_entry),
             "DRIVE BACK — speed %d%%", g_speed);
    motor_log(clients[idx].name, log_entry);

    char reply[128];
    snprintf(reply, sizeof(reply),
             "[MOTOR] Driving BACK at speed %d%%\n", g_speed);
    motor_reply(clients[idx].sock, reply);
}

// \drive_stop — остановить моторы
void cmd_drive_stop(Client clients[], int idx, const char *args) {
    (void)args;

    if (!g_initialized) {
        motor_reply(clients[idx].sock,
                    "[MOTOR] Error: motor module not initialized.\n");
        return;
    }

    uart_send("STOP\n");

    motor_log(clients[idx].name, "DRIVE STOP");

    motor_reply(clients[idx].sock, "[MOTOR] Motors STOPPED\n");
}

// \drive_speed N — установить скорость от 0 до 100
// Пример: \drive_speed 75
void cmd_drive_speed(Client clients[], int idx, const char *args) {
    if (!g_initialized) {
        motor_reply(clients[idx].sock,
                    "[MOTOR] Error: motor module not initialized.\n");
        return;
    }

    // Если аргументов нет — показываем текущую скорость
    if (args == NULL || strlen(args) == 0) {
        char reply[64];
        snprintf(reply, sizeof(reply),
                 "[MOTOR] Current speed: %d%% (usage: \\drive_speed 0-100)\n",
                 g_speed);
        motor_reply(clients[idx].sock, reply);
        return;
    }

    // Парсим число из аргумента
    int new_speed = atoi(args);

    if (new_speed < MOTOR_SPEED_MIN || new_speed > MOTOR_SPEED_MAX) {
        char reply[64];
        snprintf(reply, sizeof(reply),
                 "[MOTOR] Speed must be %d-%d. Got: %d\n",
                 MOTOR_SPEED_MIN, MOTOR_SPEED_MAX, new_speed);
        motor_reply(clients[idx].sock, reply);
        return;
    }

    motor_set_speed(new_speed);

    char log_entry[64];
    snprintf(log_entry, sizeof(log_entry), "SET SPEED → %d%%", new_speed);
    motor_log(clients[idx].name, log_entry);

    char reply[64];
    snprintf(reply, sizeof(reply), "[MOTOR] Speed set to %d%%\n", new_speed);
    motor_reply(clients[idx].sock, reply);
}