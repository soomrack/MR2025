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
    // Горячие клавиши для смайликов: нажмите "T+цифра"
    // ============================================================================

    typedef struct {
        char key;
        const char* smile;
    } HotkeySmile;

    static const HotkeySmile HOTKEY_SMILES[] = {
        {'1', "-_-"},   // T1 → -_-
        {'2', "0_0"},   // T2 → 0_0
        {'3', "xD"},    // T3 → xD
        {'4', ":D"},    // T4 → :D
        {'5', ":("},    // T5 → :(
        {'6', ":)"},    // T6 → :)
        {'7', ">_<"},   // T7 → >_<
        {'8', "T_T"},   // T8 → T_T
        {'9', "^_^"},   // T9 → ^_^
        {'0', "o_O"}    // T0 → o_O
    };

    #define HOTKEY_COUNT (sizeof(HOTKEY_SMILES) / sizeof(HOTKEY_SMILES[0]))

    // ============================================================================
    // Вспомогательные функции
    // ============================================================================

    char* get_timestamp(char* buf) {
        time_t now = time(NULL);
        struct tm* t = localtime(&now);
        sprintf(buf, "[%02d:%02d:%02d]", t->tm_hour, t->tm_min, t->tm_sec);
        return buf;
    }

    const char* get_smile_by_hotkey(char key) {
        size_t i; 
        for (i = 0; i < HOTKEY_COUNT; i++) {
            if (HOTKEY_SMILES[i].key == key) {
                return HOTKEY_SMILES[i].smile;
            }
        }
        return NULL;
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
    // Вывод справки
    // ============================================================================

    void print_usage(const char* prog) {
        WORD old_color;
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
        old_color = csbi.wAttributes;
        
        set_console_color(COLOR_SYSTEM);
        printf("\n=== Peer-to-Peer Chat v2.1 ===\n\n");
        printf("Usage:\n");
        printf("  %s <listen_port> [remote_ip:port]\n\n", prog);
        printf("Examples:\n");
        printf("  %s 55555                    # Listen only\n", prog);
        printf("  %s 55556 192.168.1.50:55555 # Connect to peer\n\n", prog);
        printf("Commands:\n");
        printf("  /help     - Show this help message\n");
        printf("  /exit     - Exit the chat (same as 'exit')\n");
        printf("  exit      - Exit the chat\n\n");
        printf("Hotkeys for smileys (type T + number):\n");
        printf("  T1 → -_-    T2 → 0_0    T3 → xD     T4 → :D\n");
        printf("  T5 → :(     T6 → :)     T7 → >_<    T8 → T_T\n");
        printf("  T9 → ^_^    T0 → o_O\n\n");
        printf("Colors:\n");
        printf("  < Cyan  - Incoming messages (from peer)\n");
        printf("  > Yellow - Outgoing messages (yours)\n");
        printf("  White   - System messages\n\n");
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
            print_usage(argv[0]);
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
    // Поток: приём сообщений от пира
    // ============================================================================

    DWORD WINAPI ReceiveThread(LPVOID lpParam) {
        char buffer[BUFFER_SIZE];
        
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
            size_t len = strlen(buffer);
            while (len > 0 && (buffer[len-1] == '\n' || buffer[len-1] == '\r')) {
                buffer[--len] = '\0';
            }
            
            set_console_color(COLOR_INCOMING);
            char timestamp[16];
            printf("\r< %s %s\n", get_timestamp(timestamp), buffer);
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
    // Обработка пользовательского ввода с горячими клавишами
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
        
        // Обработка команды /help
        if (strcmp(input, "/help") == 0) {
            print_usage("chat.exe");
            return 0;
        }
        
        // Команда выхода (поддержка /exit и exit)
        if (strcmp(input, "exit") == 0 || strcmp(input, "/exit") == 0) {
            return -1;
        }
        
        // Проверка горячей клавиши: "T" + цифра
        const char* message_to_send = input;
        
        if (len == 2 && input[0] == 'T') {
            const char* smile = get_smile_by_hotkey(input[1]);
            if (smile) {
                message_to_send = smile;
            }
        }
        
        // Отправляем только чистый текст, без таймстампа
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
        
        // Этап 7: Главный цикл ввода с клавиатуры
        printf("You: ");
        fflush(stdout);
        
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
