#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _WIN32
    #include <windows.h>
    #include <io.h>        // для _access
#else
    #include <unistd.h>
    #include <fcntl.h>
    #include <termios.h>
    #include <sys/socket.h>
#endif

#include "motor.h"

// ============================================================================
// ВНУТРЕННИЕ ПЕРЕМЕННЫЕ
// ============================================================================

static int  g_uart_handle = -1;   // на Linux – файловый дескриптор, на Windows – HANDLE
static int  g_speed       = MOTOR_SPEED_DEFAULT;
static int  g_initialized = 0;
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
// РЕАЛИЗАЦИЯ UART (ОТКРЫТИЕ / ЗАКРЫТИЕ)
// ============================================================================

#ifdef _WIN32
static int uart_open(void) {
    HANDLE h = CreateFileA(
        MOTOR_UART_PORT,
        GENERIC_READ | GENERIC_WRITE,
        0, NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );
    if (h == INVALID_HANDLE_VALUE) {
        printf("[MOTOR] Cannot open COM port %s. Error %lu\n", MOTOR_UART_PORT, GetLastError());
        return -1;
    }

    DCB dcb = {0};
    dcb.DCBlength = sizeof(DCB);
    if (!GetCommState(h, &dcb)) {
        printf("[MOTOR] GetCommState failed\n");
        CloseHandle(h);
        return -1;
    }

    dcb.BaudRate = CBR_115200;
    dcb.ByteSize = 8;
    dcb.Parity   = NOPARITY;
    dcb.StopBits = ONESTOPBIT;
    dcb.fDtrControl = DTR_CONTROL_DISABLE;
    dcb.fRtsControl = RTS_CONTROL_DISABLE;

    if (!SetCommState(h, &dcb)) {
        printf("[MOTOR] SetCommState failed\n");
        CloseHandle(h);
        return -1;
    }

    // Устанавливаем таймауты
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout         = 50;
    timeouts.ReadTotalTimeoutConstant    = 50;
    timeouts.ReadTotalTimeoutMultiplier  = 10;
    timeouts.WriteTotalTimeoutConstant   = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;
    SetCommTimeouts(h, &timeouts);

    return (int)h;
}
#else
static int uart_open(void) {
    if (access(MOTOR_UART_PORT, F_OK) != 0) {
        printf("[MOTOR] Device does not exist: %s\n", MOTOR_UART_PORT);
        system("ls -la /dev/ttyAMA* /dev/serial* 2>/dev/null || echo '  none found'");
        return -1;
    }

    int fd = open(MOTOR_UART_PORT, O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd < 0) {
        perror("[MOTOR] open UART");
        return -1;
    }

    struct termios options;
    tcgetattr(fd, &options);
    cfsetispeed(&options, B115200);
    cfsetospeed(&options, B115200);
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;
    options.c_cflag |= CLOCAL | CREAD;
    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_iflag &= ~(IXON | IXOFF | IXANY);
    options.c_oflag &= ~OPOST;
    tcsetattr(fd, TCSANOW, &options);
    return fd;
}
#endif

// ============================================================================
// ОТПРАВКА КОМАНДЫ ЧЕРЕЗ UART
// ============================================================================

static void uart_send(const char *cmd) {
    if (g_uart_handle < 0) return;

#ifdef _WIN32
    HANDLE h = (HANDLE)g_uart_handle;
    DWORD written;
    WriteFile(h, cmd, (DWORD)strlen(cmd), &written, NULL);
#else
    write(g_uart_handle, cmd, strlen(cmd));
#endif
    printf("[MOTOR] Sent: %s", cmd);
}

// ============================================================================
// ИНИЦИАЛИЗАЦИЯ И ОЧИСТКА
// ============================================================================

int motor_init(void) {
    printf("[MOTOR] Opening UART port: %s\n", MOTOR_UART_PORT);
    g_uart_handle = uart_open();
    if (g_uart_handle < 0) {
        printf("[MOTOR] Failed to open UART port\n");
        return -1;
    }
    g_speed = MOTOR_SPEED_DEFAULT;
    g_initialized = 1;
    printf("[MOTOR] UART opened at %d baud\n", MOTOR_BAUD_RATE);
    uart_send("STOP\n");
    motor_log("SERVER", "INIT — motors stopped");
    return 0;
}

void motor_cleanup(void) {
    if (g_uart_handle >= 0) {
        uart_send("STOP\n");
        motor_log("SERVER", "SHUTDOWN — motors stopped");
#ifdef _WIN32
        CloseHandle((HANDLE)g_uart_handle);
#else
        close(g_uart_handle);
#endif
        g_uart_handle = -1;
    }
    g_initialized = 0;
}

// ============================================================================
// ЧТЕНИЕ ДАННЫХ ОТ ARDUINO (НЕБЛОКИРУЮЩЕЕ)
// ============================================================================

void uart_read_arduino(void) {
    if (g_uart_handle < 0) return;

#ifdef _WIN32
    HANDLE h = (HANDLE)g_uart_handle;
    char byte;
    DWORD bytesRead;
    while (ReadFile(h, &byte, 1, &bytesRead, NULL) && bytesRead == 1) {
        // обработка символов такая же, как в Linux
#else
    char byte;
    while (read(g_uart_handle, &byte, 1) > 0) {
#endif
        if (byte == '\n' || byte == '\r') {
            if (g_rx_idx > 0) {
                g_rx_buf[g_rx_idx] = '\0';
                if (strncmp(g_rx_buf, "CURR:", 5) == 0) {
                    float left = 0, right = 0;
                    if (sscanf(g_rx_buf + 5, "%f,%f", &left, &right) == 2) {
                        char entry[128];
                        snprintf(entry, sizeof(entry), "Current: left=%.2fA right=%.2fA", left, right);
                        motor_log("INFO", entry);
                        if (left > 3.0f) {
                            snprintf(entry, sizeof(entry), "LEFT motor overcurrent: %.2fA", left);
                            motor_log("WARNING", entry);
                        }
                        if (right > 3.0f) {
                            snprintf(entry, sizeof(entry), "RIGHT motor overcurrent: %.2fA", right);
                            motor_log("WARNING", entry);
                        }
                    }
                }
                g_rx_idx = 0;
            }
        } else {
            if (g_rx_idx < (int)sizeof(g_rx_buf) - 1)
                g_rx_buf[g_rx_idx++] = byte;
            else
                g_rx_idx = 0;
        }
    }
}

// ============================================================================
// УПРАВЛЕНИЕ СКОРОСТЬЮ
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
// ЛОГИРОВАНИЕ
// ============================================================================

static void motor_log(const char *user, const char *event) {
    FILE *f = fopen(MOTOR_LOG_FILE, "a");
    if (!f) return;
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[32];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
    fprintf(f, "[%s] %s: %s\n", time_str, user, event);
    fclose(f);
}

static void motor_reply(int sock, const char *msg) {
#ifdef _WIN32
    send(sock, msg, (int)strlen(msg), 0);
#else
    send(sock, msg, strlen(msg), 0);
#endif
}

// ============================================================================
// КОМАНДЫ
// ============================================================================

void cmd_drive(Client clients[], int idx, const char *args) {
    (void)args;
    if (!g_initialized) {
        motor_reply(clients[idx].sock, "[MOTOR] Error: motor module not initialized.\n");
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
        "[MOTOR] DRIVE_MODE_START\n";
    motor_reply(clients[idx].sock, msg);
    motor_log(clients[idx].name, "ENTERED DRIVE MODE");
}

void cmd_drive_key(Client clients[], int idx, const char *args) {
    if (!g_initialized) {
        motor_reply(clients[idx].sock, "[MOTOR] Error: motor module not initialized.\n");
        return;
    }
    if (!args || strlen(args) == 0) {
        motor_reply(clients[idx].sock, "[MOTOR] Usage: \\drive_key <w|a|s|d|space|q>\n");
        return;
    }
    char key = args[0];
    const char *uart_cmd = NULL;
    const char *reply    = NULL;
    switch (key) {
        case 'w': uart_cmd = "FORWARD\n"; reply = "[MOTOR] FORWARD\n"; break;
        case 's': uart_cmd = "BACK\n";    reply = "[MOTOR] BACK\n";    break;
        case 'a': uart_cmd = "LEFT\n";    reply = "[MOTOR] LEFT\n";    break;
        case 'd': uart_cmd = "RIGHT\n";   reply = "[MOTOR] RIGHT\n";   break;
        case ' ': uart_cmd = "STOP\n";    reply = "[MOTOR] STOP\n";    break;
        case 'q':
            uart_send("STOP\n");
            motor_log(clients[idx].name, "EXIT DRIVE MODE — motors stopped");
            motor_reply(clients[idx].sock, "[MOTOR] STOP\n[MOTOR] DRIVE_MODE_END\n");
            return;
        default:
            motor_reply(clients[idx].sock, "[MOTOR] Unknown key. Use: w a s d space q\n");
            return;
    }
    char speed_cmd[32];
    snprintf(speed_cmd, sizeof(speed_cmd), "SPEED:%d\n", g_speed);
    uart_send(speed_cmd);
    uart_send(uart_cmd);
    char log_entry[64];
    snprintf(log_entry, sizeof(log_entry), "KEY '%c' -> %.*s speed=%d%%", key, (int)(strlen(uart_cmd)-1), uart_cmd, g_speed);
    motor_log(clients[idx].name, log_entry);
    motor_reply(clients[idx].sock, reply);
}

void cmd_drive_speed(Client clients[], int idx, const char *args) {
    if (!g_initialized) {
        motor_reply(clients[idx].sock, "[MOTOR] Error: motor module not initialized.\n");
        return;
    }
    if (args == NULL || strlen(args) == 0) {
        char reply[64];
        snprintf(reply, sizeof(reply), "[MOTOR] Current speed: %d%% (use: \\drive_speed 0-100)\n", g_speed);
        motor_reply(clients[idx].sock, reply);
        return;
    }
    int new_speed = atoi(args);
    if (new_speed < MOTOR_SPEED_MIN || new_speed > MOTOR_SPEED_MAX) {
        char reply[64];
        snprintf(reply, sizeof(reply), "[MOTOR] Speed must be %d-%d. Got: %d\n", MOTOR_SPEED_MIN, MOTOR_SPEED_MAX, new_speed);
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