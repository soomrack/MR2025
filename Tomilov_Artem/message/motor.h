#ifndef MOTOR_H
#define MOTOR_H

#include "types.h"

// ============================================================================
// НАСТРОЙКИ UART И СКОРОСТИ
// ============================================================================

#define MOTOR_UART_PORT      "/dev/ttyAMA0"   // UART порт на Raspberry Pi
#define MOTOR_BAUD_RATE      115200
#define MOTOR_LOG_FILE       "motor.log"

#define MOTOR_SPEED_DEFAULT  50
#define MOTOR_SPEED_MIN      0
#define MOTOR_SPEED_MAX      100

// ============================================================================
// ИНИЦИАЛИЗАЦИЯ И ОЧИСТКА
// ============================================================================

int  motor_init(void);
void motor_cleanup(void);

// ============================================================================
// УПРАВЛЕНИЕ СКОРОСТЬЮ
// ============================================================================

void motor_set_speed(int speed);
int  motor_get_speed(void);

// ============================================================================
// ЧТЕНИЕ ЭНКОДЕРОВ (вызывать из главного цикла)
// ============================================================================

void uart_read_encoders(void);

// ============================================================================
// ОБРАБОТЧИКИ КОМАНД ЧАТА
// ============================================================================

// \drive              — показывает справку и входит в WASD-режим
// \drive_key <char>   — отправляет одну клавишу (w/a/s/d/space) на Arduino
// \drive_speed <N>    — устанавливает скорость 0-100
void cmd_drive      (Client clients[], int idx, const char *args);
void cmd_drive_key  (Client clients[], int idx, const char *args);
void cmd_drive_speed(Client clients[], int idx, const char *args);

#endif // MOTOR_H