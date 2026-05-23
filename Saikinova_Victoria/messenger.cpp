// Server 
#include <iostream>
#include <vector>
#include <WinSock2.h>
#include <WS2tcpip.h>
#include <stdio.h>
#include <string>
#include <cstring>   
#include <cerrno> 
#include <string.h>
#include <conio.h>

#pragma comment(lib, "Ws2_32.lib")

#define _CRT_SECURE_NO_WARNINGS

// Key variables
bool flag = true;
bool input_mode = false;  // Флаг режима ввода с клавиатуры
int erStat;
int packet_size = 0;

// Constants
const char IP_SERV[] = "10.141.1.38";
const int PORT_NUM = 8052;
const short BUFF_SIZE = 1024;


// ==================== CHECK FUNCTIONS ====================
void check_launching_sockets() {
    if (erStat != 0) {
        std::cout << "Error WinSock version initialization # " << WSAGetLastError() << std::endl;
    }
    else {
        std::cout << "WinSock initialization is OK" << std::endl;
    }
}


void check_socket(SOCKET ServSock) {
    if (ServSock == INVALID_SOCKET) {
        std::cout << "Error initialization socket # " << WSAGetLastError() << std::endl;
        WSACleanup();
    }
    else {
        std::cout << "Server socket initialization is OK" << std::endl;
    }
}


void check_IP() {
    if (erStat <= 0) {
        std::cout << "Error in IP translation to special numeric format" << std::endl;
    }
}


void check_connect(SOCKET ServSock) {
    if (erStat != 0) {
        std::cout << "Error Socket binding to server info. Error # " << WSAGetLastError() << std::endl;
        closesocket(ServSock);
        WSACleanup();
    }
    else {
        std::cout << "Binding socket to Server info is OK" << std::endl;
    }
}


void check_listen(SOCKET ServSock) {
    if (erStat != 0) {
        std::cout << "Can't start to listen. Error # " << WSAGetLastError() << std::endl;
        closesocket(ServSock);
        WSACleanup();
    }
    else {
        std::cout << "Listening..." << std::endl;
    }
}


void check_Client_socket(SOCKET ClientConn, SOCKET ServSock, sockaddr_in clientInfo) {
    if (ClientConn == INVALID_SOCKET) {
        std::cout << "Client detected, but can't connect. Error # " << WSAGetLastError() << std::endl;
        closesocket(ClientConn);
    }
    else {
        std::cout << "Connection to a client established successfully" << std::endl;
        char clientIP[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &clientInfo.sin_addr, clientIP, INET_ADDRSTRLEN);
        std::cout << "Client connected with IP address: " << clientIP << std::endl;
    }
}


void check_send(int result, SOCKET ClientConn, SOCKET ServSock) {
    if (result == SOCKET_ERROR) {
        std::cout << "Can't send message to Client. Error # " << WSAGetLastError() << std::endl;
    }
}


// COMMAND CHECK
bool check_command(char* servBuff, const std::string& command) {
    std::string servStr(servBuff);
    while (!servStr.empty() && (servStr.back() == '\n' || servStr.back() == '\r')) {
        servStr.pop_back();
    }
    return servStr == command;
}
 

// ACTION FUNCTIONS
void bye(SOCKET& ClientConn) {
    std::cout << "\n* Closing connection by command /bye" << std::endl;
    if (ClientConn != INVALID_SOCKET) {
        shutdown(ClientConn, SD_BOTH);
        closesocket(ClientConn);
        ClientConn = INVALID_SOCKET;
    }
    flag = false;
    input_mode = false;
}
 

void you() {
    flag = false;
    input_mode = true;  // Включаем режим ввода
    std::cout << "\nYour message: ";
    std::cout.flush();
}


void play(SOCKET ClientConn) {
    const char* msg = "10\n";
    for (int i = 0; i < 6; i++) {
        send(ClientConn, msg, (int)strlen(msg), 0);
        std::cout << msg << std::endl;
    }
}


// ОБРАБОТКА СОЕДИНЕНИЙ
SOCKET accept_new_client(SOCKET ServSock) {
    sockaddr_in clientInfo;
    ZeroMemory(&clientInfo, sizeof(clientInfo));
    int clientInfo_size = sizeof(clientInfo);

    std::cout << "\n* Waiting for new client connection..." << std::endl;
    SOCKET newClient = accept(ServSock, (sockaddr*)&clientInfo, &clientInfo_size);
    check_Client_socket(newClient, ServSock, clientInfo);
    return newClient;
}


void handle_disconnect(SOCKET& ClientConn) {
    std::cout << "\n! Client disconnected." << std::endl;
    if (ClientConn != INVALID_SOCKET) {
        shutdown(ClientConn, SD_BOTH);
        closesocket(ClientConn);
        ClientConn = INVALID_SOCKET;
    }
}


// ==================== MAIN ====================
int main() {
    // Инициализация IP
    in_addr ip_to_num;
    erStat = inet_pton(AF_INET, IP_SERV, &ip_to_num);
    check_IP();

    // WinSock инициализация
    WSADATA wsData;
    erStat = WSAStartup(MAKEWORD(2, 2), &wsData);
    check_launching_sockets();

    // Серверный сокет
    SOCKET ServSock = socket(AF_INET, SOCK_STREAM, 0);
    check_socket(ServSock);

    // Привязка
    sockaddr_in servInfo; // Создаётся структура c данными о клиенте: его IP-адресом и портом
    ZeroMemory(&servInfo, sizeof(servInfo)); //Обнуляет память структуры. Избежать чтения "мусора"
    servInfo.sin_family = AF_INET;
    servInfo.sin_addr = ip_to_num;
    servInfo.sin_port = htons(PORT_NUM);

    erStat = bind(ServSock, (sockaddr*)&servInfo, sizeof(servInfo));
    check_connect(ServSock);

    // Listen
    erStat = listen(ServSock, SOMAXCONN);
    check_listen(ServSock);

    // Первый клиент
    SOCKET ClientConn = accept_new_client(ServSock);

    char servBuff[BUFF_SIZE] = {0};
    char clientBuff[BUFF_SIZE] = {0};

    std::cout << "\n==============================================" << std::endl;
    std::cout << "Server started. Commands: /bye | /you | /play" << std::endl;
    std::cout << "==============================================\n" << std::endl;

    // Главный цикл
    while (true) {
        // === Если клиент отключился — ждём нового ===
        if (ClientConn == INVALID_SOCKET) {
            ClientConn = accept_new_client(ServSock);
            if (ClientConn == INVALID_SOCKET) {
                Sleep(1000);
                continue;
            }
            flag = true;
            input_mode = false;
        }

        // === Если мы в режиме ввода с клавиатуры ===
        if (input_mode) {
            //проверка нажатия клавиши _kbhit()
            if (_kbhit()) { 
                // Читаем ввод с клавиатуры
                if (std::cin.getline(clientBuff, BUFF_SIZE)) {
                    if (ClientConn != INVALID_SOCKET && strlen(clientBuff) > 0) {
                        // Добавляем \n
                        size_t len = strlen(clientBuff);
                        if (len < BUFF_SIZE - 2) {
                            strcat_s(clientBuff, BUFF_SIZE, "\n");
                        }
                        int sendResult = send(ClientConn, clientBuff, (int)strlen(clientBuff), 0);
                        check_send(sendResult, ClientConn, ServSock);
                    }
                    ZeroMemory(clientBuff, BUFF_SIZE);
                }
                // Возвращаемся в режим приема
                flag = true;
                input_mode = false;
            }
            // Небольшая пауза чтобы не грузить CPU
            Sleep(10);
            continue;  // Пропускаем остальную часть цикла
        }

        // === Используем select() для проверки сокета ===
        fd_set readSet; //Создаётся набор дескрипторов (структура-маска), который сообщает select(), за какими сокетами нужно следить.
        FD_ZERO(&readSet); // Очищает набор, обнуляя все биты.
        FD_SET(ClientConn, &readSet); //для отслеживания событий чтения

        //Благодаря таймауту сервер не "зависает" в ожидании клиента. Если за 100 мс данных нет, select() просто вернёт управление, и цикл продолжит работу (проверит ввод с клавиатуры, обработает флаги и т.д.).
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000;  // 100 мс

        // Системный вызов, который проверяет, готовы ли указанные сокеты к операции
        int activity = select(0, &readSet, nullptr, nullptr, &timeout);

        //наш ClientConn готов к чтению
        if (activity > 0) {
            //Защита от ложных срабатываний
            if (FD_ISSET(ClientConn, &readSet)) {
                packet_size = recv(ClientConn, servBuff, BUFF_SIZE - 1, 0);
                //Мы читаем на 1 байт меньше размера буфера, чтобы оставить место для нуль-терминатора (\0), который нужен для работы со строками в стиле C.
                
                if (packet_size <= 0) {
                    handle_disconnect(ClientConn);
                    continue;
                }

                servBuff[packet_size] = '\0';
                std::cout << "\nClient: " << servBuff;
                //После recv данные в буфере не гарантированно заканчиваются \0. Мы добавляем его вручную, чтобы функции работы со строками (strlen, strcmp и др.) работали корректно и не вышли за пределы буфера.

                if (check_command(servBuff, "/bye")) {
                    bye(ClientConn);
                    continue; // Завершаем обработку, переходим к новому циклу
                }
                if (check_command(servBuff, "/you")) {
                    you();
                }
                if (check_command(servBuff, "/play")) {
                    play(ClientConn);
                }
            }
        }
    }

    // Cleanup
    if (ClientConn != INVALID_SOCKET) {
        shutdown(ClientConn, SD_BOTH);
        closesocket(ClientConn);
    }
    closesocket(ServSock);
    WSACleanup();
    return 0;
}
