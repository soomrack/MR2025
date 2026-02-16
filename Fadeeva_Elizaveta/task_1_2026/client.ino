#include <WinSock2.h>
#include <windows.h>
#include <iostream>
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include <sstream>
#include <stdexcept>
#include <cstdint>

#pragma comment(lib, "Ws2_32.lib")

// ============================================================
// ПРОТОКОЛ СООБЩЕНИЙ 
// ============================================================

// Тип сообщения
// Пока один тип (Text), но enum class оставлен
// чтобы легко добавить новые типы (например File, Image и т.д.)
enum class MessageType : uint32_t {
    Text = 1
};


// Заголовок сообщения
struct MessageHeader {
    MessageType type;
    uint32_t size;// размер тела сообщения в байта
};


// Глобальный сокет клиента, оставлен глобальным чтобы receiveLoop мог работать в отдельном потоке
SOCKET clientSocket = INVALID_SOCKET;
bool connected = false;// Флаг подключения к серверу
bool running = true;// Флаг работы всей программы
std::mutex coutMutex;// для безопасного вывода в консоль, потому что receiveLoop работает в отдельном потоке


// ============================================================
// Утилиты
// ============================================================


void setupConsole() {// поддержка UTF8 консолью
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
}


void initWinSock() {// Инициализация WinSock вынесена в функцию
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);
}

void cleanupWinSock() {//завершение работы WinSock, экономия места в main путём увеличения строк в программе, бред
    WSACleanup();
}


// ============================================================
// НИЗКОУРОВНЕВАЯ ОТПРАВКА 
// ============================================================


void sendAll(const char* data, int size) {// Отправка всех байт
    int sent = 0;
    while (sent < size) {
        int res = send(clientSocket, data + sent, size - sent, 0);
        if (res <= 0) throw std::runtime_error("send failed");
        sent += res;
    }
}

void recvAll(char* data, int size) {// Получение всех байтов
    int received = 0;
    while (received < size) {
        int res = recv(clientSocket, data + received, size - received, 0);
        if (res <= 0) throw std::runtime_error("recv failed");
        received += res;
    }
}

// ============================================================
//  EMOJI
// ============================================================

std::string replaceEmoji(std::string text) {// Логика эмодзи
    std::vector<std::pair<std::string, std::string>> emojis = {
        {":fire:", u8"🔥"},
        {":smile:", u8"😄"},
        {":sad:", u8"😢"},
        {":heart:", u8"❤️"},
        {":ok:", u8"👌"}
    };

    for (auto& e : emojis) {
        size_t pos;
        while ((pos = text.find(e.first)) != std::string::npos)
            text.replace(pos, e.first.length(), e.second);
    }
    return text;
}


// ============================================================
// ПРИЁМ СООБЩЕНИЙ 
// ============================================================


void receiveLoop() {// Поток приёма сообщений
    try {
        while (connected) {

            MessageHeader header{};
            recvAll((char*)&header, sizeof(header));

            std::vector<char> data(header.size);
            recvAll(data.data(), header.size);

            std::string text(data.begin(), data.end());

            std::lock_guard<std::mutex> lock(coutMutex);
            std::cout << "\n" << text << "\n> ";
        }
    }
    catch (...) {
        std::lock_guard<std::mutex> lock(coutMutex);
        std::cout << "\nDisconnected.\n";
        connected = false;
        closesocket(clientSocket);
    }
}

// ============================================================
// ВЫСОКОУРОВНЕВАЯ ЛОГИКА 
// ============================================================

bool connectToServer(const std::string& ip, int port) {//  подключение к серверу через сокет

    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());

    if (connect(clientSocket, (sockaddr*)&addr,
        sizeof(addr)) == 0) {

        connected = true;

        // При успешном подключении запускаем поток приёма
        std::thread(receiveLoop).detach();
        std::cout << "Connected to "
                  << ip << ":" << port << "\n";
        return true;
    }

    std::cout << "Connection failed.\n";
    return false;
}

void disconnectFromServer() {// Отдельная функция отключения
    if (connected) {
        connected = false;
        closesocket(clientSocket);
        std::cout << "Disconnected.\n";
    }
}

void sendMessage(const std::string& input) {// Отправка сообщения

    std::string text = replaceEmoji(input);

    MessageHeader header{
        MessageType::Text,
        (uint32_t)text.size()
    };

    sendAll((char*)&header, sizeof(header));
    sendAll(text.data(), text.size());
}

// ============================================================
// Служебные сообщения в консоль при запуске клиента/UI
// ============================================================

void printHelp() {
    std::cout << "\n/connect <ip> <port>\n";
    std::cout << "/quit\n";
    std::cout << "/exit\n";
    std::cout << "/help\n\n";
}

// ============================================================
// ОСНОВНОЙ ЦИКЛ 
// ============================================================

void runClientEventLoop() {

    std::string input;

    while (running) {

        std::cout << "> ";
        std::getline(std::cin, input);

        if (input.rfind("/connect", 0) == 0) {

            if (connected) {
                std::cout << "Already connected.\n";
                continue;
            }

            std::stringstream ss(input);
            std::string cmd, ip;
            int port;
            ss >> cmd >> ip >> port;

            connectToServer(ip, port);
        }
        else if (input == "/quit") {
            disconnectFromServer();
        }
        else if (input == "/exit") {
            running = false;
            disconnectFromServer();
        }
        else if (input == "/help") {
            printHelp();
        }
        else {
            if (!connected) {
                std::cout << "Not connected.\n";
                continue;
            }
            sendMessage(input);
        }
    }
}

// ============================================================
// MAIN 
// ============================================================

int main() {

    setupConsole();// консоль UTF-8
    initWinSock();// запуск WinSock

    std::cout << "Messenger client started.\n";
    printHelp();

    runClientEventLoop();// запуск основной логики

    cleanupWinSock();// завершение
    return 0;
}
