#include <iostream>      // Ввод/вывод
#include <winsock2.h>    // Сокеты Windows
#include <thread>        // Потоки
#include <windows.h>     // Кодировка консоли

#pragma comment(lib, "ws2_32.lib") // Подключение Winsock библиотеки

// Сокет клиента
SOCKET sock;

// Получение сообщений от сервера
void receiveMessages() {
    char buffer[1024];

    while (true) {
        // Приём данных от сервера
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);

        // Если соединение закрыто
        if (bytes <= 0) break;

        buffer[bytes] = '\0'; // Завершаем строку
        std::string msg(buffer);

        // Проверка выхода
        if (msg == "exit") {
            std::cout << "\n[Сервер вышел]\n";
            break;
        }

        // Вывод сообщения сервера
        std::cout << "\nСервер: " << msg << std::endl;
    }
}

// Отправка сообщений серверу
void sendMessages() {
    std::string msg;

    while (true) {
        std::getline(std::cin, msg); // Ввод текста

        // Отправка на сервер
        send(sock, msg.c_str(), msg.length(), 0);

        // Выход из чата
        if (msg == "exit") break;
    }
}

int main() {
    // Поддержка русского языка
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);

    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa); // Инициализация Winsock

    // Создание сокета
    sock = socket(AF_INET, SOCK_STREAM, 0);

    // Настройка сервера
    sockaddr_in server;
    server.sin_family = AF_INET; // IPv4
    server.sin_port = htons(8080); // Порт
    server.sin_addr.s_addr = inet_addr("192.168.0.105"); // IP сервера

    // Подключение к серверу
    connect(sock, (sockaddr*)&server, sizeof(server));

    std::cout << "Подключено к серверу!\n";

    // Поток приёма сообщений
    std::thread t1(receiveMessages);

    // Поток отправки сообщений
    std::thread t2(sendMessages);

    // Ожидание завершения
    t1.join();
    t2.join();

    // Закрытие соединения
    closesocket(sock);
    WSACleanup();
}