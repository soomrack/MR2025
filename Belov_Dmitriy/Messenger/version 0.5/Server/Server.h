#pragma once // подключаем файл один раз
#include "SocketBase.h"// базовая логика сокета
#include "Message.h"// формат данных, который передаётся по сети может быть и видео и изображение
#include <iostream>// для вывода информации в консоль

class Server : public SocketBase {// наследный класс сервера, слушает порт, принимает клиентов, принимает сообщения
public:
    void start(uint16_t port);// Запускает сервер, uint16_t - порт
    void run();// работа сервера
};