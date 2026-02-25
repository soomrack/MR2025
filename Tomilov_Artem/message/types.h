#ifndef TYPES_H
#define TYPES_H

#define NAME_LEN    32
#define MAX_CLIENTS 10

// Структура клиента чата
typedef struct {
    int  sock;
    char name[NAME_LEN];
    int  named;
    int  color_index;
} Client;

// Структура команды сервера
typedef struct {
    const char *name;
    const char *description;
    void (*handler)(Client[], int, const char*);
} ServerCommand;

#endif // TYPES_H