#ifndef MOTOR_H
#define MOTOR_H

#include "types.h"

// ============================================================================
// НАСТРОЙКИ
// ============================================================================

#define MOTOR_UART_PORT    "/dev/ttyAMA0"
#define MOTOR_BAUD_RATE    115200         
#define MOTOR_LOG_FILE     "motor.log"
#define MOTOR_SPEED_MIN    0
#define MOTOR_SPEED_MAX    100
#define MOTOR_SPEED_DEFAULT 50

// ============================================================================
// ПРОТОТИПЫ: ИНИЦИАЛИЗАЦИЯ И УПРАВЛЕНИЕ
// ============================================================================

// Открывает UART порт, инициализирует мотор.
int  motor_init(void);

// Закрывает UART порт.
void motor_cleanup(void);

// Устанавливает скорость 0-100. Отправляет команду на STM32.
void motor_set_speed(int speed);

// Возвращает текущую установленную скорость.
int  motor_get_speed(void);

// ============================================================================
// ПРОТОТИПЫ: ОБРАБОТЧИКИ КОМАНД ЧАТА
// Такик же как у остальныу команды сервера
// ============================================================================

void cmd_drive_forward(Client clients[], int idx, const char *args);
void cmd_drive_back   (Client clients[], int idx, const char *args);
void cmd_drive_stop   (Client clients[], int idx, const char *args);
void cmd_drive_speed  (Client clients[], int idx, const char *args);

#endif // MOTOR_H