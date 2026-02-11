#include "Server.h"// подключение заголовка сервера

void Server::start(uint16_t port) {// Запускает сервер и начинает слушать указанный порт
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);// создание серверного сокета
    if (sock == INVALID_SOCKET)// проверка создания сокета
        throw std::runtime_error("socket failed");// выдача ошибки

    sockaddr_in addr{};//Структура для описания адреса сервера, обнуляем, чтобы не было мусора
    addr.sin_family = AF_INET;//работает по IPv4
    addr.sin_port = htons(port);//указание порта
    addr.sin_addr.s_addr = INADDR_ANY;//принимаем подключения на всех сетевых интерфейсах

    bind(sock, (sockaddr*)&addr, sizeof(addr));//привязываем сокет к порту, если он уже не занят( если занят, то bind падает)
    listen(sock, SOMAXCONN);

    std::cout << "Server listening on port " << port << std::endl;// ообщение, чтобы понимать, что сервер запущен и порт правильный
}

void Server::run() {// функция принимает клиента и начинает с ним работать
    sockaddr_in clientAddr{};// Структура для хранения адреса клиента, заполнится после accept
    int size = sizeof(clientAddr);

    SOCKET client = accept(sock, (sockaddr*)&clientAddr, &size);
    if (client == INVALID_SOCKET)
        throw std::runtime_error("accept failed");// проверка подключения клиента и выдача ошибки

    std::cout << "Client connected\n";// уведомление что клиент подключен

    while (true) {// пока клиент подключён — сервер принимает данные
        MessageHeader header{};//читает строго размер заголовка
        int res = recv(client, (char*)&header, sizeof(header), 0);// recv работает с байтами, приводим к char* 
        if (res <= 0) break;//если клиент ушёл — выходим из цикла

        Message msg;//создаём объект Message
        msg.header = header;//сохраняем заголовок
        msg.data.resize(header.size);//Готовим память под данные, размер которых извесен из заголовка

        recv(client, msg.data.data(), header.size, 0);//Приём тела сообщения

        if (msg.header.type == MessageType::Text) {//Проверяем тип сообщения
            std::string text(msg.data.begin(), msg.data.end());//Превращаем байты обратно в строку
            std::cout << "Received: " << text << std::endl;//Выводим сообщение в консоль сервера
        }
    }

    closesocket(client);//закрываем соединение
}