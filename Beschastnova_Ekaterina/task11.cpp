#define _CRT_SECURE_NO_WARNINGS
#pragma comment(lib, "ws2_32.lib")

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <time.h>

#define COLOR_DEFAULT   7
#define COLOR_SYSTEM    10  // Зеленый
#define COLOR_THEM      11  // Голубой
#define COLOR_YOU       14  // Желтый
#define COLOR_ERROR     12  // Красный

void setColor(WORD color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

#define BUFFER_SIZE 1024

SOCKET peerSocket = INVALID_SOCKET;
CRITICAL_SECTION lock;
volatile int running = 1;
int global_listen_port = 0;

char my_name[32];
char other_name[32] = "Собеседник";
int name_exchanged = 0;

// для шифрование
char secret_key[64] = "";
int encryption_enabled = 0;
int has_key_locally = 0;

void crypt(char* data, int length) {
    if (!encryption_enabled || strlen(secret_key) == 0) return;
    int key_len = (int)strlen(secret_key);
    for (int i = 0; i < length; i++) {
        data[i] = data[i] ^ secret_key[i % key_len];
    }
}

// Очистка строки ввода
void clear_user_input_line() {
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    COORD coord = { 0, csbi.dwCursorPosition.Y - 1 };
    if (coord.Y < 0) coord.Y = 0;
    DWORD written;
    FillConsoleOutputCharacter(hConsole, ' ', csbi.dwSize.X, coord, &written);
    SetConsoleCursorPosition(hConsole, coord);
}

// Поток приема сообщений
DWORD WINAPI ReceiveThread(LPVOID lpParam) {
    char buffer[BUFFER_SIZE];
    while (running) {
        if (peerSocket != INVALID_SOCKET) {
            memset(buffer, 0, BUFFER_SIZE);
            int bytes = recv(peerSocket, buffer, BUFFER_SIZE - 1, 0);

            if (bytes <= 0) { //ошибка при чтении
                if (running) {
                    printf("\r"); 
                    setColor(COLOR_ERROR);
                    printf("[Система] Собеседник отключился.\n");
                    setColor(COLOR_DEFAULT);
                }
                running = 0;
                break;
            }
            buffer[bytes] = '\0';
            // Обработка системной команды включения шифрования
            if (strcmp(buffer, "SYS_SECRET_START") == 0) {
                encryption_enabled = 1;
                printf("\r                                      \r"); // Очистка "You: "
                setColor(COLOR_SYSTEM);
                printf("[Система] Включен секретный режим. Введите /password [пароль]\n");
                setColor(COLOR_YOU);
                printf("You: "); fflush(stdout);
                setColor(COLOR_DEFAULT);
                continue;
            }

            // Обмен именами
            if (!name_exchanged) {
                strncpy(other_name, buffer, 31);
                name_exchanged = 1;
                printf("\r                                     \r");
                setColor(COLOR_SYSTEM);
                printf("[Система] На связи: %s\n", other_name);
                setColor(COLOR_YOU);
                printf("You: "); fflush(stdout);
                setColor(COLOR_DEFAULT);
                continue;
            }

            // Дешифровка
            if (encryption_enabled && has_key_locally) {
                crypt(buffer, bytes);
            }

            // вывод
            char ts[16]; time_t now = time(NULL); struct tm* t = localtime(&now);
            strftime(ts, sizeof(ts), "%H:%M:%S", t);
            printf("\r                                    \r"); 
            setColor(encryption_enabled ? 15 : COLOR_THEM);
            printf("[%s] %s: %s\n", ts, other_name, buffer);

            setColor(COLOR_YOU);
            printf("You: "); fflush(stdout);
            setColor(COLOR_DEFAULT);
        }
        Sleep(50);
    }
    return 0;
}

// Входящие
DWORD WINAPI ServerThread(LPVOID lpParam) {
    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)global_listen_port);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(serverSock, (struct sockaddr*)&addr, sizeof(addr));
    listen(serverSock, 1);

    SOCKET client = accept(serverSock, NULL, NULL);
    if (client != INVALID_SOCKET) {
        EnterCriticalSection(&lock);
        if (peerSocket == INVALID_SOCKET) {
            peerSocket = client;
            send(peerSocket, my_name, (int)strlen(my_name), 0);
        }
        LeaveCriticalSection(&lock);
    }
    closesocket(serverSock);
    return 0;
}

int main() {
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    InitializeCriticalSection(&lock);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    printf("--- ЧАТ ---\nВаше имя: ");
    fgets(my_name, 32, stdin);
    my_name[strcspn(my_name, "\n")] = 0;

    printf("Выберите порт (1/2): ");
    int choice; scanf("%d", &choice); getchar();
    global_listen_port = (choice == 1) ? 5000 : 5001;
    int target_port = (choice == 1) ? 5001 : 5000;

    printf("Шифровка сообщений:\n /set password [] \n /password \n Ждем подключение собеседника");

    // Запуск сервера
    CreateThread(NULL, 0, ServerThread, NULL, 0, NULL);
    Sleep(500);

    // Попытка подключиться
    SOCKET clientSock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in target_addr;
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons((unsigned short)target_port);
    inet_pton(AF_INET, "127.0.0.1", &target_addr.sin_addr);

    if (connect(clientSock, (struct sockaddr*)&target_addr, sizeof(target_addr)) != SOCKET_ERROR) {
        EnterCriticalSection(&lock);
        if (peerSocket == INVALID_SOCKET) {
            peerSocket = clientSock;
            send(peerSocket, my_name, (int)strlen(my_name), 0);
        }
        else {
            closesocket(clientSock);
        }
        LeaveCriticalSection(&lock);
    }

    CreateThread(NULL, 0, ReceiveThread, NULL, 0, NULL);

    char input[BUFFER_SIZE];
    while (running) {
        setColor(COLOR_YOU);
        printf("You: ");
        if (!fgets(input, BUFFER_SIZE, stdin)) break;
        input[strcspn(input, "\n")] = 0;

        if (strlen(input) == 0) continue;
        if (strcmp(input, "exit") == 0) { running = 0; break; }

        // Команда /set password
        if (strncmp(input, "/set password ", 14) == 0) {
            strcpy(secret_key, input + 14);
            encryption_enabled = 1;
            has_key_locally = 1;
            send(peerSocket, "SYS_SECRET_START", 16, 0);
            clear_user_input_line();
            setColor(COLOR_SYSTEM);
            printf("[!] Секретный чат включен!\n");
            setColor(COLOR_YOU);
            continue;
        }

        // Команда /password
        if (strncmp(input, "/password ", 10) == 0) {
            strcpy(secret_key, input + 10);
            has_key_locally = 1;
            clear_user_input_line();
            printf("[!] Пароль принят!\n");
            continue;
        }

        // Отправка обычного сообщения
        if (peerSocket != INVALID_SOCKET) {
            clear_user_input_line();

            // Вывод себе
            time_t now = time(NULL); struct tm* t = localtime(&now);
            setColor(encryption_enabled ? 14 : COLOR_YOU);
            printf("[%02d:%02d:%02d] %s: %s\n", t->tm_hour, t->tm_min, t->tm_sec, my_name, input);
            setColor(COLOR_DEFAULT);

            // Шифруем для сети
            int len = (int)strlen(input);
            if (encryption_enabled && has_key_locally) {
                crypt(input, len);
            }
            send(peerSocket, input, len, 0);
        }
        else {
            printf("Ждем подключения...\n");
        }
    }

    WSACleanup();
    return 0;
}