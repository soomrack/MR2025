#include <iostream>
#include <string>
#include <sstream>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

class ChatClient {
private:
    SOCKET sock;

public:
    ChatClient();

    bool init_network();  // инициализация сети
    void connect_to_server(const std::string& ip, int port);  // подключение к серверу
    void send_raw(const std::string& data);  // отправка
    void receive();  //  прием
    void disconnect();  // отключение
    void cleanup();  // закрытие сокета
    bool is_connected() const;
};


class CommandHandler {
private:
    ChatClient& client;

public:
    CommandHandler(ChatClient& c);

    void process(const std::string& input);

private:  // команды пользователя
    void help();
    void connect_command(const std::string& input);
    void message_command(const std::string& input);
    void disconnect_command();
};



// ChatClient

ChatClient::ChatClient() : sock(INVALID_SOCKET) {}  // Конструктор класса, инициализация до входа в конструктор

bool ChatClient::init_network() {
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {  // инициализация WinSock
        std::cout << "Ошибка инициализации WinSock\n";
        return false;
    }
    return true;
}

void ChatClient::connect_to_server(const std::string& ip, int port) {
    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);  // создание сокета

    if (sock == INVALID_SOCKET) {
        std::cout << "Ошибка создания сокета\n";
        return;
    }

    sockaddr_in addr{};  // структура адреса 
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    inet_pton(AF_INET, ip.c_str(), &addr.sin_addr);

    if (connect(sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {  // функция подключения
        std::cout << "Не удалось подключиться к серверу\n";
        closesocket(sock);
        sock = INVALID_SOCKET;
        return;
    }
}

void ChatClient::send_raw(const std::string& data) {
    if (!is_connected()) {
        std::cout << "Нет подключения к серверу\n";
        return;
    }
    send(sock, data.c_str(), data.size(), 0);  // отправка данных в поток TCP
    receive();
}

void ChatClient::receive() {
    char buffer[1024]{};
    int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);

    if (bytes > 0) {
        buffer[bytes] = '\0';
        std::cout << "Ответ сервера: " << buffer << std::endl;
    }
}

void ChatClient::disconnect() {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
}

void ChatClient::cleanup() {
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
    WSACleanup();
}

bool ChatClient::is_connected() const {  // const после метода не дает методу изменить объект
    return sock != INVALID_SOCKET;
}



// CommandHandler

CommandHandler::CommandHandler(ChatClient& c) : client(c) {}

void CommandHandler::process(const std::string& input) {
    if (input.rfind("/", 0) != 0) {
        if (!client.is_connected()) {
            std::cout << "Сначала подключитесь к серверу\n";
            return;
        }
        client.send_raw(input);
        return;
    }

    if (input == "/help") {
        help();
    }
    else if (input.rfind("/connect", 0) == 0) connect_command(input);
    else if (input.rfind("/message", 0) == 0) message_command(input);
    else if (input == "/disconnect") disconnect_command();
    else std::cout << "Неизвестная команда. Введите /help\n";
}

void CommandHandler::help() {
    std::cout << "Команды:\n";
    std::cout << "  /help\n";
    std::cout << "  /connect <ip> <port>\n";
    std::cout << "  /message <text>\n";
    std::cout << "  /disconnect\n";
}

void CommandHandler::connect_command(const std::string& input) {
    std::stringstream ss(input);  // строка, работающая как поток ввода
    std::string cmd, ip;
    int port = 0;

    ss >> cmd >> ip >> port; // разделение потока ss по пробелам на команду, ip и порт

    if (ip.empty() || port == 0) {
        std::cout << "Использование: /connect <ip> <port>\n";
        return;
    }

    client.connect_to_server(ip, port);
}

void CommandHandler::message_command(const std::string& input) {
    if (!client.is_connected()) {
        std::cout << "Сначала подключитесь к серверу\n";
        return;
    }

    std::string text = input.substr(9);
    if (text.empty()) {
        std::cout << "Сообщение не может быть пустым\n";
        return;
    }
    client.send_raw("/message " + text);
}

void CommandHandler::disconnect_command() {
    client.disconnect();
}



// Функции в main()

ChatClient client;
CommandHandler* handler = nullptr;

void init() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    if (!client.init_network()) {
        exit(EXIT_FAILURE);
    }
    handler = new CommandHandler(client);
}

void run() {
    std::string input;
    while (true) {
        if (!std::getline(std::cin, input))
            break;

        if (input.empty())
            continue;

        handler->process(input);
    }
}

void shutdown() {
    client.cleanup();
    delete handler;
}



int main() {
    init();
    run();
    shutdown();
}
