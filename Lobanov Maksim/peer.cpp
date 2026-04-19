#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include <signal.h>
#include <time.h>

#define BUFFER_SIZE 1024
#define DEFAULT_PORT 55555

volatile sig_atomic_t running = 1;
SOCKET peerSocket = INVALID_SOCKET;  // Активное соединение с пиром
CRITICAL_SECTION lock;               // Для потокобезопасного доступа к peerSocket

// ============================================================================
// Цвета консоли (Windows API)
// ============================================================================

#define COLOR_DEFAULT   FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
#define COLOR_INCOMING  FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY  // Cyan
#define COLOR_OUTGOING  FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY   // Yellow
#define COLOR_SYSTEM    FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE        // White

// ============================================================================
// Структура для распарсенных данных сенсоров
// ============================================================================

typedef struct {
    int engineTemp;
    int coolantTemp;
    int ambientTemp;
    int oilPressure;
    int coolantLevel;
    int fuelLevel;
    int rpm;
    int load;
    int runtime;
    float voltage;
    int engineRunning;
    int warning;  // флаг от сервера
    int valid;    // флаг успешного парсинга
} ParsedSensorData;

// ============================================================================
// Пороговые значения для предупреждений
// ============================================================================

#define TEMP_ENGINE_CRITICAL       80
#define TEMP_ENGINE_WARNING        70   
#define TEMP_COOLANT_CRITICAL     100
#define TEMP_COOLANT_WARNING       90
#define OIL_PRESSURE_MIN           150
#define OIL_PRESSURE_MAX           600
#define COOLANT_LEVEL_MIN           25
#define COOLANT_LEVEL_WARNING       35
#define FUEL_LEVEL_MIN              15
#define FUEL_LEVEL_WARNING          25
#define VOLTAGE_MIN                12.0f
#define VOLTAGE_MAX                15.0f
#define RPM_MAX                    3000
#define LOAD_WARNING                90

// ============================================================================
// Цвета для предупреждений
// ============================================================================

#define COLOR_WARNING     FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY  // Yellow
#define COLOR_CRITICAL    FOREGROUND_RED | FOREGROUND_INTENSITY                      // Bright Red
#define COLOR_INFO        FOREGROUND_GREEN | FOREGROUND_BLUE                         // Cyan

void set_console_color(WORD color) {
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
}

void reset_console_color(void) {
    set_console_color(COLOR_DEFAULT);
}

void signal_handler(int sig) {
    running = 0;
    printf("\n[Shutting down...]\n");
    if (peerSocket != INVALID_SOCKET) {
        shutdown(peerSocket, SD_BOTH); // Прерываем recv/send
    }
}

// ============================================================================
// Вспомогательные функции
// ============================================================================

char* get_timestamp(char* buf) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    sprintf(buf, "[%02d:%02d:%02d]", t->tm_hour, t->tm_min, t->tm_sec);
    return buf;
}


// ============================================================================
// Инициализация и завершение
// ============================================================================

void initialize_program(void) {
    signal(SIGINT, signal_handler);
    InitializeCriticalSection(&lock);
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("[!] WSAStartup failed\n");
        exit(1);
    }
}

void cleanup_resources(void) {
    EnterCriticalSection(&lock);
    if (peerSocket != INVALID_SOCKET) {
        closesocket(peerSocket);
        peerSocket = INVALID_SOCKET;
    }
    LeaveCriticalSection(&lock);
    
    Sleep(500);
    DeleteCriticalSection(&lock);
    WSACleanup();
    reset_console_color();  
}


// ============================================================================
// Парсинг адреса
// ============================================================================

int parse_address(const char* addr, char* ip_out, int* port_out) {
    const char* sep = strchr(addr, ':');
    if (!sep) {
        printf("[!] Invalid address format. Use IP:PORT\n");
        return -1;
    }
    
    size_t ip_len = (size_t)(sep - addr);
    if (ip_len >= 64) return -1;
    
    strncpy(ip_out, addr, ip_len);
    ip_out[ip_len] = '\0';
    
    *port_out = atoi(sep + 1);
    if (*port_out <= 0 || *port_out > 65535) {
        printf("[!] Invalid port: %d\n", *port_out);
        return -1;
    }
    
    return 0;
}

// ============================================================================
// Проверка аргументов
// ============================================================================

void validate_arguments(int argc, char* argv[], int* listen_port, const char** remote_addr) {
    if (argc < 2) {
        printf("данные для подключения неверны");
        exit(1);
    }
    
    *listen_port = atoi(argv[1]);
    if (*listen_port <= 0 || *listen_port > 65535) {
        printf("[!] Invalid port: %d\n", *listen_port);
        exit(1);
    }
    
    *remote_addr = (argc >= 3) ? argv[2] : NULL;
}

// ============================================================================
// Создание слушающего сокета
// ============================================================================

SOCKET create_listening_socket(int port) {
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        printf("[Server] socket() failed\n");
        return INVALID_SOCKET;
    }
    
    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, (int)sizeof(opt));
    
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("[Server] bind() failed on port %d\n", port);
        closesocket(sock);
        return INVALID_SOCKET;
    }
    
    if (listen(sock, SOMAXCONN) == SOCKET_ERROR) {
        printf("[Server] listen() failed\n");
        closesocket(sock);
        return INVALID_SOCKET;
    }
    
    return sock;
}

// ============================================================================
// Парсинг строки данных формата key=value (Windows-совместимая версия)
// ============================================================================

int parse_sensor_data(const char* buffer, ParsedSensorData* data) {
    if (!buffer || !data) return -1;
    
    memset(data, 0, sizeof(ParsedSensorData));
    data->valid = 0;
    
    // Работаем напрямую с буфером, копируя только текущую строку
    char line_buf[256];
    const char* ptr = buffer;
    int fields_parsed = 0;
    
    while (*ptr && fields_parsed < 12) {
        // Копируем строку до \n или \r
        int i = 0;
        while (*ptr && *ptr != '\n' && *ptr != '\r' && i < 255) {
            line_buf[i++] = *ptr++;
        }
        line_buf[i] = '\0';
        
        // Пропускаем разделители строк
        while (*ptr == '\n' || *ptr == '\r') ptr++;
        
        // Пропускаем пустые строки
        if (line_buf[0] == '\0') continue;
        
        // Парсим key=value
        char key[64] = {0}, value[64] = {0};
        if (sscanf(line_buf, "%63[^=]=%63s", key, value) == 2) {
            if (strcmp(key, "engineTemp") == 0) { data->engineTemp = atoi(value); fields_parsed++; }
            else if (strcmp(key, "coolantTemp") == 0) { data->coolantTemp = atoi(value); fields_parsed++; }
            else if (strcmp(key, "ambientTemp") == 0) { data->ambientTemp = atoi(value); fields_parsed++; }
            else if (strcmp(key, "oilPressure") == 0) { data->oilPressure = atoi(value); fields_parsed++; }
            else if (strcmp(key, "coolantLevel") == 0) { data->coolantLevel = atoi(value); fields_parsed++; }
            else if (strcmp(key, "fuelLevel") == 0) { data->fuelLevel = atoi(value); fields_parsed++; }
            else if (strcmp(key, "rpm") == 0) { data->rpm = atoi(value); fields_parsed++; }
            else if (strcmp(key, "load") == 0) { data->load = atoi(value); fields_parsed++; }
            else if (strcmp(key, "runtime") == 0) { data->runtime = atoi(value); fields_parsed++; }
            else if (strcmp(key, "voltage") == 0) { data->voltage = (float)atof(value); fields_parsed++; }
            else if (strcmp(key, "engineRunning") == 0) { data->engineRunning = atoi(value); fields_parsed++; }
            else if (strcmp(key, "warning") == 0) { data->warning = atoi(value); fields_parsed++; }
        }
    }
    
    data->valid = (fields_parsed >= 8);
    return data->valid ? 0 : -1;
}

// ============================================================================
// Проверка данных и генерация предупреждений
// Возвращает уровень: 0 = ОК, 1 = Warning, 2 = Critical
// ============================================================================

int check_warnings(const ParsedSensorData* data, char* warning_msg, size_t msg_size) {
    if (!data || !data->valid) {
        if (warning_msg && msg_size > 0) {
            snprintf(warning_msg, msg_size, "[PARSE_ERROR] Invalid data format");
        }
        return 2;  // Critical - не можем доверять данным
    }
    
    int level = 0;  // 0=OK, 1=Warning, 2=Critical
    warning_msg[0] = '\0';
    
    // === Критические предупреждения ===
    
    if (data->engineTemp > TEMP_ENGINE_CRITICAL) {
        snprintf(warning_msg, msg_size, "CRITICAL: Engine temp %d°C > %d°C!", 
                 data->engineTemp, TEMP_ENGINE_CRITICAL);
        return 2;
    }
    
    if (data->oilPressure < OIL_PRESSURE_MIN) {
        snprintf(warning_msg, msg_size, "CRITICAL: Oil pressure %d PSI < %d PSI!", 
                 data->oilPressure, OIL_PRESSURE_MIN);
        return 2;
    }
    
    if (data->coolantLevel < COOLANT_LEVEL_MIN) {
        snprintf(warning_msg, msg_size, "CRITICAL: Coolant level %d%% < %d%%!", 
                 data->coolantLevel, COOLANT_LEVEL_MIN);
        return 2;
    }
    
    if (data->voltage < VOLTAGE_MIN) {
        snprintf(warning_msg, msg_size, "CRITICAL: Voltage %.1fV < %.1fV!", 
                 data->voltage, VOLTAGE_MIN);
        return 2;
    }
    
    // === Предупреждения уровня Warning ===
    
    if (data->engineTemp > TEMP_ENGINE_WARNING) {
        snprintf(warning_msg, msg_size, "WARNING: Engine temp %d°C approaching limit", 
                 data->engineTemp);
        level = 1;
    }
    else if (data->coolantTemp > TEMP_COOLANT_WARNING) {
        snprintf(warning_msg, msg_size, "WARNING: Coolant temp %d°C high", 
                 data->coolantTemp);
        level = 1;
    }
    else if (data->oilPressure > OIL_PRESSURE_MAX) {
        snprintf(warning_msg, msg_size, "WARNING: Oil pressure %d PSI > %d PSI", 
                 data->oilPressure, OIL_PRESSURE_MAX);
        level = 1;
    }
    else if (data->fuelLevel < FUEL_LEVEL_WARNING) {
        snprintf(warning_msg, msg_size, "WARNING: Fuel level %d%% low", 
                 data->fuelLevel);
        level = 1;
    }
    else if (data->rpm > RPM_MAX) {
        snprintf(warning_msg, msg_size, "WARNING: RPM %d > %d", 
                 data->rpm, RPM_MAX);
        level = 1;
    }
    else if (data->load > LOAD_WARNING) {
        snprintf(warning_msg, msg_size, "WARNING: Engine load %d%% high", 
                 data->load);
        level = 1;
    }
    
    // Если сервер прислал флаг warning, но мы не нашли проблем — инфо-сообщение
    if (level == 0 && data->warning) {
        snprintf(warning_msg, msg_size, "Server flag: warning=1 (check parameters)");
        level = 1;
    }
    
    return level;
}

// ============================================================================
// Поток: приём сообщений от пира 
// ============================================================================

DWORD WINAPI ReceiveThread(LPVOID lpParam) {
    char buffer[BUFFER_SIZE];
    char warning_msg[256];
    
    while (running) {
        int bytes = recv(peerSocket, buffer, BUFFER_SIZE - 1, 0);
        if (bytes <= 0) {
            if (running) {
                printf("\n[Peer disconnected]\n");
                running = 0;
            }
            break;
        }
        
        buffer[bytes] = '\0';
        
        // === ПАРСИНГ И ПРОВЕРКА ПРЕДУПРЕЖДЕНИЙ ===
        ParsedSensorData data;
        if (parse_sensor_data(buffer, &data) == 0) {
            int alert_level = check_warnings(&data, warning_msg, sizeof(warning_msg));
            
            if (alert_level > 0) {
                // Вывод предупреждения с цветом
                WORD alert_color = (alert_level == 2) ? COLOR_CRITICAL : COLOR_WARNING;
                set_console_color(alert_color);
                char ts[16];
                printf("\r  %s %s\n", get_timestamp(ts), warning_msg);
                
                if (alert_level == 2) {
                    printf("   [Diagnostics] Engine:%d°C Coolant:%d°C Oil:%d PSI Fuel:%d%% Volt:%.1fV\n",
                           data.engineTemp, data.coolantTemp, data.oilPressure, 
                           data.fuelLevel, data.voltage);
                }
                reset_console_color();
                printf("You: ");
                fflush(stdout);
            }
        }
        
        // Обычный вывод данных (если нет предупреждений)
        set_console_color(COLOR_INCOMING);
        char ts[16];
        printf("\r< %s %s\n", get_timestamp(ts), buffer);
        reset_console_color();
        printf("You: ");
        fflush(stdout);
    }
    
    return 0;
}

// ============================================================================
// Поток: сервер (принимает входящие подключения)
// ============================================================================

DWORD WINAPI ServerThread(LPVOID lpParam) {
    int listenPort = *(int*)lpParam;
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("[Server] WSAStartup failed: %d\n", WSAGetLastError());
        return 1;
    }
    
    SOCKET serverSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (serverSock == INVALID_SOCKET) {
        printf("[Server] socket() failed: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }
    
    int opt = 1;
    setsockopt(serverSock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, (int)sizeof(opt));
    
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)listenPort);
    addr.sin_addr.s_addr = INADDR_ANY;
    
    if (bind(serverSock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("[Server] bind() failed on port %d: %d\n", listenPort, WSAGetLastError());
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }
    
    if (listen(serverSock, SOMAXCONN) == SOCKET_ERROR) {
        printf("[Server] listen() failed: %d\n", WSAGetLastError());
        closesocket(serverSock);
        WSACleanup();
        return 1;
    }
    
    printf("[Server] Listening on port %d...\n", listenPort);
    
    while (running) {
        sockaddr_in clientAddr;
        memset(&clientAddr, 0, sizeof(clientAddr));
        int addrSize = sizeof(clientAddr);
        SOCKET client = accept(serverSock, (struct sockaddr*)&clientAddr, &addrSize);
        
        if (client == INVALID_SOCKET) {
            if (WSAGetLastError() == WSAEINTR) break;
            continue;
        }
        
        EnterCriticalSection(&lock);
        if (peerSocket == INVALID_SOCKET) {
            peerSocket = client;
            char ip[INET_ADDRSTRLEN];
            strcpy(ip, inet_ntoa(clientAddr.sin_addr));
            printf("\n[+] Incoming connection from %s:%d\n", ip, ntohs(clientAddr.sin_port));
            printf("You: ");
            fflush(stdout);
        } else {
            printf("\n[!] Another peer tried to connect (rejected)\n");
            printf("You: ");
            fflush(stdout);
            closesocket(client);
        }
        LeaveCriticalSection(&lock);
    }
    
    closesocket(serverSock);
    WSACleanup();
    return 0;
}

// ============================================================================
// Подключение к удалённому пиру
// ============================================================================

void connect_to_peer(const char* address) {
    char ip[64] = {0};
    int port;
    const char* sep = strchr(address, ':');
    
    if (!sep) {
        printf("[!] Invalid address format. Use IP:PORT (e.g., 192.168.1.50:55555)\n");
        return; 
    }
    
    size_t ip_len = (size_t)(sep - address);
    if (ip_len >= sizeof(ip) - 1) ip_len = sizeof(ip) - 1;
    
    strncpy(ip, address, ip_len);
    ip[ip_len] = '\0';
    port = atoi(sep + 1);
    
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("[Client] WSAStartup failed: %d\n", WSAGetLastError());
        return;
    }
    
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        printf("[Client] socket() failed: %d\n", WSAGetLastError());
        WSACleanup();
        return;
    }
    
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((u_short)port);
    
    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        printf("[Client] Invalid IP address: %s\n", ip);
        closesocket(sock);
        WSACleanup();
        return;
    }
    
    printf("[Client] Connecting to %s:%d...\n", ip, port);
    
    if (connect(sock, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("[!] Could not connect to peer. Waiting for incoming connection...\n\n");
        closesocket(sock);
        WSACleanup();
        return;
    }
    
    EnterCriticalSection(&lock);
    if (peerSocket == INVALID_SOCKET) {
        peerSocket = sock;
        printf("[+] Connected to peer %s:%d\n", ip, port);
        printf("You: ");
        fflush(stdout);
    } else {
        closesocket(sock);
    }
    LeaveCriticalSection(&lock);
}

// ============================================================================
// СЛОВАРЬ КОМАНД (Command Dictionary)
// ============================================================================

// Тип функции-обработчика команды: возвращает -1 для выхода, 0 для продолжения
typedef int (*CommandHandler)(const char* args);

// Структура команды в таблице
typedef struct {
    const char* name;           // Имя команды (например, "/help")
    CommandHandler handler;     // Указатель на функцию-обработчик
    const char* description;    // Краткое описание (для будущего расширения)
} Command;

// ============================================================================
// Обработчики команд
// ============================================================================

// Обработчик: /exit
int cmd_exit(const char* args) {
    (void)args;
    return -1;  // Сигнал к выходу
}

// ============================================================================
// Таблица команд (легко расширять: добавьте новую строку!)
// ============================================================================

static const Command COMMAND_TABLE[] = {
    { "/exit",  cmd_exit,  "Exit the chat" },
    // Пример добавления новой команды:
    // { "/color", cmd_color, "Change console color" },
};

#define COMMAND_COUNT (sizeof(COMMAND_TABLE) / sizeof(COMMAND_TABLE[0]))

// ============================================================================
// Поиск и выполнение команды из таблицы
// ============================================================================

int execute_command(const char* input) {
    size_t i; 
    
    // Проверяем, начинается ли ввод с '/' (признак команды)
    if (input[0] != '/') {
        return 1;  // Не команда — обрабатывать как обычное сообщение
    }
    
    // Ищем команду в таблице
    for (i = 0; i < COMMAND_COUNT; i++) {
        if (strcmp(input, COMMAND_TABLE[i].name) == 0) {
            return COMMAND_TABLE[i].handler("");  // args пока не используем
        }
    }
    
    // Команда не найдена — выводим подсказку
    printf("[!] Unknown command: %s (type /help for list)\n", input);
    printf("You: ");
    fflush(stdout);
    return 0;  // Продолжаем работу
}

// ============================================================================
// Обработка пользовательского ввода 
// ============================================================================

int process_user_input(void) {
    char input[BUFFER_SIZE];
    
    if (fgets(input, BUFFER_SIZE, stdin) == NULL) {
        return -1;
    }
    
    // Удаляем \r и \n
    size_t len = strlen(input);
    while (len > 0 && (input[len-1] == '\n' || input[len-1] == '\r')) {
        input[--len] = '\0';
    }
    
    // Пустое сообщение
    if (len == 0) {
        return 0;
    }
    
    // Обработка команд через словарь
    int cmd_result = execute_command(input);
    if (cmd_result == -1) {
        return -1;  // Выход
    }
    if (cmd_result == 0) {
        return 0;   // Команда обработана (или неизвестна), ждём следующий ввод
    }
    
    const char* message_to_send = input;

    EnterCriticalSection(&lock);
    if (peerSocket != INVALID_SOCKET) {
        send(peerSocket, message_to_send, (int)strlen(message_to_send), 0);
    }
    LeaveCriticalSection(&lock);
    
    set_console_color(COLOR_OUTGOING);
    char timestamp[16];
    printf("\r\033[K> %s %s\n", get_timestamp(timestamp), message_to_send);
    reset_console_color();
    printf("You: ");
    fflush(stdout);
    
    return 0;
}

// ============================================================================
// Helper-функции для запуска потоков (ошибки обрабатываются внутри)
// ============================================================================

void start_server_thread(int listen_port) {
    HANDLE hServer = CreateThread(NULL, 0, ServerThread, &listen_port, 0, NULL);
    if (!hServer) {
        printf("Failed to start server thread\n");
        exit(1);
    }
    CloseHandle(hServer);
}

void start_receive_thread(void) {
    HANDLE hRecv = CreateThread(NULL, 0, ReceiveThread, NULL, 0, NULL);
    if (!hRecv) {
        printf("Failed to start receive thread\n");
        exit(1);
    }
    CloseHandle(hRecv);
}

// ============================================================================
// Главная функция (полностью модульная)
// ============================================================================

int main(int argc, char* argv[]) {
    int listen_port;
    const char* remote_addr;
    
    // Этап 1: Инициализация
    initialize_program();
    
    // Этап 2: Проверка аргументов
    validate_arguments(argc, argv, &listen_port, &remote_addr);
    
    // Этап 3: Запуск серверного потока
    start_server_thread(listen_port);
    
    // Этап 4: Подключение к удалённому пиру (если указан)
    if (remote_addr) {
        connect_to_peer(remote_addr);
    } else {
        printf("Waiting for peer to connect...\n\n");
    }
    
    // Этап 5: Ожидание установления соединения
    while (running && peerSocket == INVALID_SOCKET) {
        Sleep(100);
    }
    
    if (!running) {
        cleanup_resources();
        return 0;
    }
    
    // Этап 6: Запуск потока приёма сообщений
    start_receive_thread();
    
    while (running) {
        if (process_user_input() == -1) {
            running = 0;
            break;
        }
    }
    
    // Этап 8: Очистка
    cleanup_resources();
    printf("\nChat closed.\n");
    
    return 0;
}