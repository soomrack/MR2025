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
    Text = 1,
    Connect = 2,
    Disconnect = 3,
    LogRequest = 4,
    LogResponse = 5
};


// Заголовок сообщения
struct MessageHeader {
    uint32_t type;
    uint32_t size;
};

// Можно добавить структуру для сообщения с метаданными
struct TextMessage {
    std::string username;
    std::string timestamp;
    std::string content;
};

// Глобальный сокет клиента, оставлен глобальным чтобы receiveLoop мог работать в отдельном потоке
SOCKET clientSocket = INVALID_SOCKET;
bool connected = false;// Флаг подключения к серверу
bool running = true;// Флаг работы всей программы
std::mutex coutMutex;// для безопасного вывода в консоль, потому что receiveLoop работает в отдельном потоке
std::string username;  // <-- НОВОЕ: имя пользователя


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
// НОВАЯ ФУНКЦИЯ: ввод имени пользователя
// ============================================================
std::string getUsername() {
    std::string name;
    while (true) {
        std::cout << "Enter your name (3-20 characters): ";
        std::getline(std::cin, name);
        
        // Убираем пробелы в начале и конце
        size_t first = name.find_first_not_of(" \t");
        size_t last = name.find_last_not_of(" \t");
        if (first == std::string::npos || last == std::string::npos) {
            std::cout << "Name cannot be empty!\n";
            continue;
        }
        name = name.substr(first, last - first + 1);
        
        // Проверка длины
        if (name.length() < 3) {
            std::cout << "Name too short! Minimum 3 characters.\n";
            continue;
        }
        if (name.length() > 20) {
            std::cout << "Name too long! Maximum 20 characters.\n";
            continue;
        }
        
        // Проверка на допустимые символы (можно расширить)
        bool valid = true;
        for (char c : name) {
            if (!isalnum(c) && c != '_' && c != '-' && c != ' ') {
                valid = false;
                break;
            }
        }
        if (!valid) {
            std::cout << "Name can only contain letters, numbers, spaces, underscores and hyphens!\n";
            continue;
        }
        
        return name;
    }
}

// ============================================================
// НОВАЯ ФУНКЦИЯ: получение текущего времени
// ============================================================
std::string getCurrentTime() {
    time_t now = time(0);
    struct tm timeinfo;
    localtime_s(&timeinfo, &now);  // Windows-safe version
    
    char buffer[80];
    strftime(buffer, sizeof(buffer), "%H:%M:%S", &timeinfo);
    return std::string(buffer);
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


void receiveLoop() { // Поток приёма сообщений
    try {
        while (connected) {

            MessageHeader header{};
            recvAll((char*)&header, sizeof(header));

            std::vector<char> data(header.size);
            recvAll(data.data(), header.size);


            if (header.type == static_cast<uint32_t>(MessageType::LogResponse)) {

                std::string text(data.begin(), data.end());

                std::lock_guard<std::mutex> lock(coutMutex);
                std::cout << "\n=== LOG RESPONSE ===\n"
                          << text
                          << "====================\n> ";
            }
            else {  

                std::string text(data.begin(), data.end());

                std::lock_guard<std::mutex> lock(coutMutex);
                std::cout << "\n" << text << "\n> ";
            }

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

bool connectToServer(const std::string& ip, int port) {
    // Сначала запрашиваем имя
    username = getUsername();
    if (username.empty()) return false;
    
    clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (clientSocket == INVALID_SOCKET) {
        std::cout << "Socket creation failed.\n";
        return false;
    }
    
    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip.c_str());
    
    if (connect(clientSocket, (sockaddr*)&addr, sizeof(addr)) == 0) {
        connected = true;
        
        // Отправляем имя серверу как отдельное сообщение
        try {
            // Создаём сообщение с именем
            std::string connectMsg = username;
            MessageHeader header{ static_cast<uint32_t>(MessageType::Connect),(uint32_t)connectMsg.size() };
            sendAll((char*)&header, sizeof(header));
            sendAll(connectMsg.data(), connectMsg.size());
            
            // Запускаем поток приёма сообщений
            std::thread(receiveLoop).detach();
            
            std::cout << "Connected to " << ip << ":" << port << " as '" << username << "'\n";
            return true;
        }
        catch (...) {
            std::cout << "Failed to send username.\n";
            closesocket(clientSocket);
            connected = false;
            return false;
        }
    }
    
    std::cout << "Connection failed.\n";
    return false;
}

void disconnectFromServer() {
    if (connected) {
        try {
            // Отправляем сообщение об отключении
            std::string disconnectMsg = username;
            MessageHeader header{ static_cast<uint32_t>(MessageType::Disconnect),(uint32_t)disconnectMsg.size()};
            sendAll((char*)&header, sizeof(header));
            sendAll(disconnectMsg.data(), disconnectMsg.size());
        }
        catch (...) {
            // Игнорируем ошибки при отключении
        }
        
        connected = false;
        closesocket(clientSocket);
        std::cout << "Disconnected from server.\n";
    }
}

void sendMessage(const std::string& input) {
    // Формируем сообщение с именем и временем
    std::string timeStr = getCurrentTime();
    std::string processedContent = replaceEmoji(input);
    
    // Формат: [Имя Время] Сообщение
    std::string fullMessage = "[" + username + " " + timeStr + "] " + processedContent;
    
    MessageHeader header{
    static_cast<uint32_t>(MessageType::Text),(uint32_t)fullMessage.size()
};
    
    sendAll((char*)&header, sizeof(header));
    sendAll(fullMessage.data(), fullMessage.size());
}


bool spam(const std::string& input) {  // Добавляем параметр input
    if (!connected) {
        std::cout << "Not connected.\n";
        return false;  // Возвращаем false вместо continue
    }
    
    std::stringstream ss(input);
    std::string cmd;
    int count;
    std::string message;
    
    ss >> cmd >> count;
    std::getline(ss, message);
    
    if (!message.empty() && message[0] == ' ')
        message = message.substr(1);
    
    for (int i = 0; i < count; i++) {
        try {
            if (i > 0) std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
            std::string spamMsg = message;
            if (count > 1) {
                spamMsg = "[" + std::to_string(i+1) + "/" + 
                         std::to_string(count) + "] " + message;
            }
            
            sendMessage(spamMsg);
        }
        catch (...) {
            std::cout << "Spam interrupted.\n";
            break;
        }
    }
    return true;
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

        else if (input.rfind("/spam", 0) == 0) {
            spam(input);
        }

        else if (input == "/help") {
            printHelp();
        }

        else if (input.rfind("/logs", 0) == 0) {

            if (!connected) {
                std::cout << "Not connected.\n";
                continue;
            }

            std::string request;

            if (input == "/logs all")
                request = "ALL";

            else if (input == "/logs warnings")
                request = "WARNINGS";

            else if (input.rfind("/logs last ", 0) == 0)
                request = "LAST " + input.substr(11);

            else {
                std::cout << "Usage:\n"
                          << "/logs all\n"
                          << "/logs warnings\n"
                          << "/logs last <minutes>\n";
                continue;
            }

            MessageHeader header{static_cast<uint32_t>(MessageType::LogRequest),(uint32_t)request.size()};

            sendAll((char*)&header, sizeof(header));
            sendAll(request.data(), request.size());
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
