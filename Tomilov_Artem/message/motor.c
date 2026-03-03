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

static int  g_uart_fd     = -1;
static int  g_speed       = MOTOR_SPEED_DEFAULT;
static int  g_initialized = 0;

// Буфер для приёма данных от Arduino ("ENC:xxx,xxx\n")
static char g_rx_buf[64];
static int  g_rx_idx = 0;

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

// Настраивает UART порт для общения с Arduino.
// Параметры: 115200 бод, 8 бит данных, без чётности, 1 стоп-бит (8N1).
static int uart_open(void) {
    // Проверяем существование устройства
    if (access(MOTOR_UART_PORT, F_OK) != 0) {
        printf("[MOTOR] Device does not exist: %s\n", MOTOR_UART_PORT);
        printf("[MOTOR] Available serial devices:\n");
        system("ls -la /dev/ttyAMA* /dev/serial* 2>/dev/null || echo '  none found'");
        return -1;
    }

    int fd = open(MOTOR_UART_PORT, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        perror("[MOTOR] open UART");
        printf("[MOTOR] Hint: try  sudo chmod 666 %s\n", MOTOR_UART_PORT);
        printf("[MOTOR] Hint: check  sudo usermod -aG dialout $USER\n");
        return -1;
    }

    struct termios options;
    tcgetattr(fd, &options);

    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);

    // Режим 8N1
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |=  CS8;
    options.c_cflag |=  CLOCAL | CREAD;

    // Сырой режим — без обработки символов
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_oflag &= ~OPOST;

    tcsetattr(fd, TCSANOW, &options);
    return fd;
}

int motor_init(void) {
    printf("[MOTOR] Opening UART port: %s\n", MOTOR_UART_PORT);
    g_uart_fd = uart_open();
    if (g_uart_fd < 0) {
        printf("[MOTOR] Failed to open UART port: %s\n", MOTOR_UART_PORT);
        printf("[MOTOR] Check list:\n");
        printf("[MOTOR]   1. sudo raspi-config -> Interface Options -> Serial Port\n");
        printf("[MOTOR]      'login shell over serial' = NO\n");
        printf("[MOTOR]      'serial port hardware'    = YES\n");
        printf("[MOTOR]   2. sudo usermod -aG dialout $USER  (then relogin)\n");
        printf("[MOTOR]   3. Check /boot/config.txt has: enable_uart=1\n");
        return -1;
    }

    g_speed       = MOTOR_SPEED_DEFAULT;
    g_initialized = 1;

    printf("[MOTOR] UART opened: %s at %d baud\n", MOTOR_UART_PORT, MOTOR_BAUD_RATE);
    printf("[MOTOR] Default speed: %d%%\n", g_speed);
    printf("[MOTOR] Log file: %s\n", MOTOR_LOG_FILE);

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
// ЧТЕНИЕ ДАННЫХ ЭНКОДЕРОВ ОТ ARDUINO
// ============================================================================

// Вызывается из главного цикла сервера.
// Читает байты из UART не блокируясь, накапливает строку,
// при получении \n парсит "ENC:left,right" и пишет в motor.log.
void uart_read_arduino(void) {
    if (g_uart_fd < 0) return;

    char byte;
    while (read(g_uart_fd, &byte, 1) > 0) {
        if (byte == '\n' || byte == '\r') {
            if (g_rx_idx > 0) {
                g_rx_buf[g_rx_idx] = '\0';

                if (strncmp(g_rx_buf, "CURR:", 5) == 0) {
                    float left = 0, right = 0;
                    if (sscanf(g_rx_buf + 5, "%f,%f", &left, &right) == 2) {
                        char entry[128];
                        snprintf(entry, sizeof(entry),
                                "Current: left=%.2fA right=%.2fA", left, right);
                        motor_log("INFO", entry);
                        //printf("[CURRENT] left=%.2fA right=%.2fA \n", left, right);

                        if (left > 3.0f) {
                            snprintf(entry, sizeof(entry),
                                    "LEFT motor overcurrent: %.2fA", left);
                            motor_log("WARNING", entry);
                            //printf("[WARNINIG] LEFT motor overcurrent: %.2fA \n", left);
                        }
                        if (right > 3.0f) {
                            snprintf(entry, sizeof(entry),
                                    "RIHGT motor overcurrent: %.2fA", right);
                            motor_log("WARNING", entry);
                            //printf("[WARNINIG] RIGHT motor overcurrent: %.2fA \n", right);
                        }
                    }
                }
                g_rx_idx = 0;
            }
        } else {
            if (g_rx_idx < (int)sizeof(g_rx_buf) - 1) {
                g_rx_buf[g_rx_idx++] = byte;
            } else {
                g_rx_idx = 0;
            }
        }
    }
}

// ============================================================================
// РЕАЛИЗАЦИЯ: UART ОТПРАВКА
// ============================================================================

static void uart_send(const char *cmd) {
    if (g_uart_fd < 0) {
        printf("[MOTOR] UART not open, cannot send: %s", cmd);
        return;
    }

    int len  = strlen(cmd);
    int sent = write(g_uart_fd, cmd, len);
    if (sent < 0) {
        perror("[MOTOR] UART write");
    } else {
        printf("[MOTOR] Sent to Arduino: %s", cmd);
    }
}

// ============================================================================
// РЕАЛИЗАЦИЯ: СКОРОСТЬ
// ============================================================================

void motor_set_speed(int speed) {
    if (speed < MOTOR_SPEED_MIN) speed = MOTOR_SPEED_MIN;
    if (speed > MOTOR_SPEED_MAX) speed = MOTOR_SPEED_MAX;
    g_speed = speed;

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

static void motor_reply(int sock, const char *msg) {
    send(sock, msg, strlen(msg), 0);
}

// ============================================================================
// РЕАЛИЗАЦИЯ: ОБРАБОТЧИКИ КОМАНД ЧАТА
// ============================================================================

// \drive — показывает справку по управлению.
// Клиент при получении этой команды входит в WASD-режим (raw terminal).
void cmd_drive(Client clients[], int idx, const char *args) {
    (void)args;

    if (!g_initialized) {
        motor_reply(clients[idx].sock,
                    "[MOTOR] Error: motor module not initialized.\n");
        return;
    }

    const char *msg =
        "[MOTOR] === DRIVE MODE ===\n"
        "  w         — вперёд\n"
        "  s         — назад\n"
        "  a         — поворот влево\n"
        "  d         — поворот вправо\n"
        "  пробел    — стоп\n"
        "  q         — выйти из режима управления\n"
        "  \\drive_speed N  — установить скорость (0-100)\n"
        "[MOTOR] DRIVE_MODE_START\n";   // маркер для клиента

    motor_reply(clients[idx].sock, msg);

    motor_log(clients[idx].name, "ENTERED DRIVE MODE");
}

// \drive_key <char> — принимает одиночный символ от клиента в WASD-режиме.
// Клиент отправляет эту команду при каждом нажатии клавиши.
void cmd_drive_key(Client clients[], int idx, const char *args) {
    if (!g_initialized) {
        motor_reply(clients[idx].sock,
                    "[MOTOR] Error: motor module not initialized.\n");
        return;
    }

    if (!args || strlen(args) == 0) {
        motor_reply(clients[idx].sock,
                    "[MOTOR] Usage: \\drive_key <w|a|s|d|space|q>\n");
        return;
    }

    char key = args[0];
    const char *uart_cmd = NULL;
    const char *reply    = NULL;

    switch (key) {
        case 'w':
            uart_cmd = "FORWARD\n";
            reply    = "[MOTOR] FORWARD\n";
            break;
        case 's':
            uart_cmd = "BACK\n";
            reply    = "[MOTOR] BACK\n";
            break;
        case 'a':
            uart_cmd = "LEFT\n";
            reply    = "[MOTOR] LEFT\n";
            break;
        case 'd':
            uart_cmd = "RIGHT\n";
            reply    = "[MOTOR] RIGHT\n";
            break;
        case ' ':
            uart_cmd = "STOP\n";
            reply    = "[MOTOR] STOP\n";
            break;
        case 'q':
            // Клиент выходит из режима — останавливаем моторы
            uart_send("STOP\n");
            motor_log(clients[idx].name, "EXIT DRIVE MODE — motors stopped");
            motor_reply(clients[idx].sock,
                        "[MOTOR] STOP\n"
                        "[MOTOR] DRIVE_MODE_END\n");
            return;
        default:
            motor_reply(clients[idx].sock,
                        "[MOTOR] Unknown key. Use: w a s d  space  q\n");
            return;
    }

    // Перед каждой командой движения шлём актуальную скорость
    char speed_cmd[32];
    snprintf(speed_cmd, sizeof(speed_cmd), "SPEED:%d\n", g_speed);
    uart_send(speed_cmd);
    uart_send(uart_cmd);

    // Лог
    char log_entry[64];
    snprintf(log_entry, sizeof(log_entry),
             "KEY '%c' -> %.*s speed=%d%%",
             key, (int)(strlen(uart_cmd) - 1), uart_cmd, g_speed);
    motor_log(clients[idx].name, log_entry);

    motor_reply(clients[idx].sock, reply);
}

// \drive_speed N — установить скорость от 0 до 100.
// Без аргументов — показать текущую скорость.
void cmd_drive_speed(Client clients[], int idx, const char *args) {
    if (!g_initialized) {
        motor_reply(clients[idx].sock,
                    "[MOTOR] Error: motor module not initialized.\n");
        return;
    }

    if (args == NULL || strlen(args) == 0) {
        char reply[64];
        snprintf(reply, sizeof(reply),
                 "[MOTOR] Current speed: %d%% (use: \\drive_speed 0-100)\n",
                 g_speed);
        motor_reply(clients[idx].sock, reply);
        return;
    }

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
    snprintf(log_entry, sizeof(log_entry), "SET SPEED -> %d%%", new_speed);
    motor_log(clients[idx].name, log_entry);

    char reply[64];
    snprintf(reply, sizeof(reply), "[MOTOR] Speed set to %d%%\n", new_speed);
    motor_reply(clients[idx].sock, reply);
}