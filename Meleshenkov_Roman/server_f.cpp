#include <iostream>      // Для ввода и вывода (cout, cin)
#include <winsock2.h>    // Библиотека Windows для работы с сокетами
#include <thread>        // Для многопоточности (одновременная отправка и получение)
#include <windows.h>     // Для настройки кодировки консоли

#pragma comment(lib, "ws2_32.lib") // Подключение библиотеки Winsock

// Глобальный сокет клиента (чтобы использовать в потоках)
SOCKET client_socket;

// Функция для получения сообщений от клиента
void receiveMessages() {
    char buffer[1024]; // Буфер для входящих данных

    while (true) {
        // Получаем данные от клиента
        int bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);

        // Если соединение разорвано
        if (bytes <= 0) break;

        buffer[bytes] = '\0'; // Завершаем строку
        std::string msg(buffer); // Преобразуем в строку

        // Если клиент завершил чат
        if (msg == "exit") {
            std::cout << "\n[Клиент вышел]\n";
            break;
        }

        // Вывод сообщения клиента
        std::cout << "\nКлиент: " << msg << std::endl;
    }
}

// Функция для отправки сообщений клиенту
void sendMessages() {
    std::string msg;

    while (true) {
        std::getline(std::cin, msg); // Ввод сообщения с клавиатуры

        // Отправка сообщения клиенту
        send(client_socket, msg.c_str(), msg.length(), 0);

        // Завершение чата
        if (msg == "exit") break;
    }
}

int main() {
    // Установка UTF-8 для поддержки русского языка
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa); // Инициализация Winsock

    // Создание TCP-сокета
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, 0);

    // Настройка адреса сервера
    sockaddr_in server;
    server.sin_family = AF_INET;       // IPv4
    server.sin_addr.s_addr = INADDR_ANY; // Принимать любые подключения
    server.sin_port = htons(8080);     // Порт 8080

    // Привязка сокета к адресу
    bind(server_socket, (sockaddr*)&server, sizeof(server));

    // Прослушивание подключений
    listen(server_socket, 1);

    std::cout << "Ожидание клиента...\n";

    sockaddr_in client;
    int len = sizeof(client);

    // Принятие клиента
    client_socket = accept(server_socket, (sockaddr*)&client, &len);

    std::cout << "Клиент подключился!\n";

    // Поток для получения сообщений
    std::thread t1(receiveMessages);

    // Поток для отправки сообщений
    std::thread t2(sendMessages);

    // Ожидание завершения потоков
    t1.join();
    t2.join();

    // Закрытие сокетов
    closesocket(server_socket);
    WSACleanup();
}