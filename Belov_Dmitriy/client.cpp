
// WinSock библиотека Windows для работы с сетью
#include <WinSock2.h>
#include <windows.h>

// стандартные библиотеки C++
#include <iostream>
#include <string>
#include <vector>
#include <stdexcept>
#include <cstdint>

// Линкуем библиотеку WinSock
#pragma comment(lib, "Ws2_32.lib")

// =======================================================
// СООБЩЕНИЕ 
// =======================================================

// Типы сообщений, можно расширять
enum class MessageType : uint32_t {
    Text  = 1,
    Image = 2,
    Video = 3
};

// Заголовок сообщения (служебная информация)
struct MessageHeader {
    MessageType type; // тип сообщения
    uint32_t size;    // размер данных в байтах
};

// =======================================================
// БАЗОВЫЙ КЛАСС СОКЕТА 
// =======================================================

class SocketBase {
protected:
    SOCKET sock = INVALID_SOCKET; // дескриптор сокета

    // Гарантированная отправка всех байтов
    void sendAll(const char* data, int size) {
        int sent = 0;
        while (sent < size) {
            int res = send(sock, data + sent, size - sent, 0);
            if (res <= 0)
                throw std::runtime_error("send failed");
            sent += res;
        }
    }

    // Гарантированный приём всех байтов
    void recvAll(char* data, int size) {
        int received = 0;
        while (received < size) {
            int res = recv(sock, data + received, size - received, 0);
            if (res <= 0)
                throw std::runtime_error("recv failed");
            received += res;
        }
    }

public:
    virtual ~SocketBase() {
        if (sock != INVALID_SOCKET)
            closesocket(sock);
    }
};

// =======================================================
// КЛИЕНТ 
// =======================================================

class Client : public SocketBase {
public:

    // Подключение к серверу
    void connectTo(const char* ip, uint16_t port) {

        // создаём TCP сокет
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET)
            throw std::runtime_error("socket failed");

        sockaddr_in addr{};
        addr.sin_family = AF_INET;          // IPv4
        addr.sin_port = htons(port);        // перевод порта в сетевой формат
        addr.sin_addr.s_addr = inet_addr(ip); // преобразование строки IP

        // подключаемся к серверу
        if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
            throw std::runtime_error("connect failed");
    }

    // Отправка текстового сообщения
    void sendText(const std::string& text) {

        // Формируем заголовок
        MessageHeader header{
            MessageType::Text,
            static_cast<uint32_t>(text.size())
        };

        // Сначала отправляем заголовок
        sendAll((char*)&header, sizeof(header));

        // Затем само сообщение
        sendAll(text.data(), text.size());
    }
};

// =======================================================
// MAIN
// =======================================================

int main() {

    // Обязательная инициализация WinSock в Windows
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cout << "WSAStartup failed\n";
        return 1;
    }

    try {
        Client client;

        // Подключаемся к локальному серверу
        client.connectTo("127.0.0.1", 54000);

        // Отправляем сообщение
        client.sendText("Привет, сервер!");

        // Пауза, чтобы соединение не закрылось мгновенно
        Sleep(1000);
    }
    catch (const std::exception& e) {
        std::cout << "Error: " << e.what() << std::endl;
    }

    // Освобождаем WinSock
    WSACleanup();

    return 0;
}
