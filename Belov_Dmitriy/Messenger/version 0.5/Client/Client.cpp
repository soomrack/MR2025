#include "Client.h"// подключение заголовка клиента, для определения функций и классов (Client, sendText, connectTo)

void Client::connectTo(const char* ip, uint16_t port) {//функция подключения к серверу по IP и порту
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);//(IPv4, потоковый сокет, явно говорим, что используем TCP)

    sockaddr_in addr{};// структура, описывающая адрес сервера
    addr.sin_family = AF_INET;// IPv4-адрес
    addr.sin_port = htons(port);// Host TO Network Short, порты, всегда так
    addr.sin_addr.s_addr = inet_addr(ip);// IP

    connect(sock, (sockaddr*)&addr, sizeof(addr));//Фактическое подключение к серверу
}

void Client::sendText(const std::string& text) {// Отправить текстовое сообщение серверу
    MessageHeader header{
        MessageType::Text,
        static_cast<uint32_t>(text.size())
    };

    sendAll((char*)&header, sizeof(header));// Отправка заголовка сообщения
    sendAll(text.data(), text.size());// Отправка тела сообщения
}
