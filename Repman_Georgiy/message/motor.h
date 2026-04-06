#ifndef MOTOR_H
#define MOTOR_H

#include "types.h"

// Для Windows необходимо указать правильный COM-порт (например, "COM3")
#ifdef _WIN32
    #define MOTOR_UART_PORT   "COM3"      // измените под ваш адаптер
    #define MOTOR_BAUD_RATE   115200
#else
    #define MOTOR_UART_PORT   "/dev/ttyAMA0"
    #define MOTOR_BAUD_RATE   115200
#endif

#define MOTOR_LOG_FILE       "motor.log"
#define MOTOR_SPEED_DEFAULT  50
#define MOTOR_SPEED_MIN      0
#define MOTOR_SPEED_MAX      100

int  motor_init(void);
void motor_cleanup(void);
void motor_set_speed(int speed);
int  motor_get_speed(void);
void uart_read_arduino(void);
void cmd_drive      (Client clients[], int idx, const char *args);
void cmd_drive_key  (Client clients[], int idx, const char *args);
void cmd_drive_speed(Client clients[], int idx, const char *args);

#endif