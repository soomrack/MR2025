#pragma once // защита от дублирования
#include <WinSock2.h>// главная библиотека Windows для работы с сетью
#include <stdexcept>// чтобы не проверять if (res < 0) везде

#pragma comment(lib, "Ws2_32.lib") // подключение библиотеки WinSock

class SocketBase { //базовый класс для сокетов.
protected:// всё, что ниже, доступно SocketBase и  классам-наследникам
    SOCKET sock = INVALID_SOCKET;// начальное значение до создания сокета

    void sendAll(const char* data, int size) {// отправка всех данных
        int sent = 0;
        while (sent < size) {//цикл пока отправленно меньше чем нужно
            int res = send(sock, data + sent, size - sent, 0); 
            if (res <= 0) throw std::runtime_error("send failed");// ошибка если что то сломалось и 0, если соединение закрыто
            sent += res;// учёт ушедших байтов
        }
    }

    void recvAll(char* data, int size) {// приём данных
        int received = 0;
        while (received < size) {
            int res = recv(sock, data + received, size - received, 0);
            if (res <= 0) throw std::runtime_error("recv failed");
            received += res;
        }
    }

public:
    virtual ~SocketBase() {//освобождение ресурсов
        if (sock != INVALID_SOCKET)
            closesocket(sock);
    }
};