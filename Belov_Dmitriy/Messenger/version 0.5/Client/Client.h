#pragma once// // защита от дублирования
#include "SocketBase.h"// Client наследуется от SocketBase
#include "Message.h"// описывает тип сообщения, заголовок, структуру данных клиента
#include <string>// автоматически хранит размер строки

class Client : public SocketBase {// Объявление класса-наследника Клиента
public:
    void connectTo(const char* ip, uint16_t port);// Подключение к серверу
    void sendText(const std::string& text);// Отправка текстового сообщения
    // если нужно будет доработать добавление видео или картинок, то это нужно делать именно здесь
};
