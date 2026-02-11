// ======= Подключения =======
#include <WinSock2.h>
#include <iostream>
#include <vector>
#include <stdexcept>
#include <cstdint>

#pragma comment(lib, "Ws2_32.lib") // Линкуем WinSock автоматически

// ======================================================
//                ОПИСАНИЕ СООБЩЕНИЯ
// ======================================================

// Типы сообщений (можем расширять)
enum class MessageType : uint32_t {
    Text  = 1,
    Image = 2,
    Video = 3
};

// Заголовок сообщения
struct MessageHeader {
    MessageType type; // тип данных
    uint32_t size;    // размер тела сообщения
};

// Полное сообщение
struct Message {
    MessageHeader header;
    std::vector<char> data; // универсальный контейнер под любые данные
};

// ======================================================
//                БАЗОВЫЙ КЛАСС СОКЕТА
// ======================================================

class SocketBase {
protected:
    SOCKET sock = INVALID_SOCKET;

    // Гарантированная отправка ВСЕХ байтов
    void sendAll(SOCKET s, const char* data, int size) {
        int sent = 0;
        while (sent < size) {
            int res = send(s, data + sent, size - sent, 0);
            if (res <= 0)
                throw std::runtime_error("send failed");
            sent += res;
        }
    }

    // Гарантированный приём ВСЕХ байтов
    void recvAll(SOCKET s, char* data, int size) {
        int received = 0;
        while (received < size) {
            int res = recv(s, data + received, size - received, 0);
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

// ======================================================
//                       СЕРВЕР
// ======================================================

class Server : public SocketBase {
public:
    void start(uint16_t port) {

        // Создание сокета (IPv4 + TCP)
        sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (sock == INVALID_SOCKET)
            throw std::runtime_error("socket failed");

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY; // принимаем подключения с любого IP

        // Привязка к порту
        if (bind(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR)
            throw std::runtime_error("bind failed");

        // Начинаем слушать
        if (listen(sock, SOMAXCONN) == SOCKET_ERROR)
            throw std::runtime_error("listen failed");

        std::cout << "Server listening on port " << port << std::endl;
    }

    void run() {

        sockaddr_in clientAddr{};
        int size = sizeof(clientAddr);

        // Ожидание клиента
        SOCKET client = accept(sock, (sockaddr*)&clientAddr, &size);
        if (client == INVALID_SOCKET)
            throw std::runtime_error("accept failed");

        std::cout << "Client connected\n";

        while (true) {

            try {

                // ===== Читаем заголовок =====
                MessageHeader header{};
                recvAll(client, (char*)&header, sizeof(header));

                // ===== Читаем тело =====
                Message msg;
                msg.header = header;
                msg.data.resize(header.size);

                recvAll(client, msg.data.data(), header.size);

                // ===== Обработка типов =====

                if (msg.header.type == MessageType::Text) {

                    std::string text(msg.data.begin(), msg.data.end());
                    std::cout << "Received text: " << text << std::endl;
                }
                else if (msg.header.type == MessageType::Image) {

                    std::cout << "Received image, size: "
                              << msg.header.size << " bytes\n";

                    // Здесь можно сохранить файл
                    // std::ofstream("image.jpg", std::ios::binary)...
                }
                else if (msg.header.type == MessageType::Video) {

                    std::cout << "Received video, size: "
                              << msg.header.size << " bytes\n";
                }

            }
            catch (...) {
                std::cout << "Client disconnected\n";
                break;
            }
        }

        closesocket(client);
    }
};

// ======================================================
//                       MAIN
// ======================================================

int main() {

    WSADATA wsa;

    // Инициализация WinSock
    WSAStartup(MAKEWORD(2, 2), &wsa);

    try {

        Server server;
        server.start(54000);
        server.run();

    }
    catch (const std::exception& e) {

        std::cout << "Error: " << e.what() << std::endl;
    }

    WSACleanup();
}
