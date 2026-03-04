#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream>
#include <winsock2.h>
#include <string>
#include <thread>
#include <windows.h>
#include <chrono>
#include <atomic>
#include <map>
#include <memory>
#include <iomanip>
#include <sstream>
#include <ctime>
#include <conio.h>

#pragma comment(lib, "ws2_32.lib")

using namespace std;

// Класс для работы со временем
class TimeUtils {
public:
    static string getCurrentTime() {
        auto now = chrono::system_clock::now();
        auto time_t = chrono::system_clock::to_time_t(now);

        tm timeinfo;
        localtime_s(&timeinfo, &time_t);

        stringstream ss;
        ss << "["
            << setw(2) << setfill('0') << timeinfo.tm_hour << ":"
            << setw(2) << setfill('0') << timeinfo.tm_min << ":"
            << setw(2) << setfill('0') << timeinfo.tm_sec
            << "]";

        return ss.str();
    }
};

// Класс для управления цветом в консоли
class ConsoleColor {
private:
    HANDLE hConsole;
    static const int MY_COLOR = 10;      // зеленый
    static const int OTHER_COLOR = 9;    // синий
    static const int SYSTEM_COLOR = 14;  // желтый
    static const int DEFAULT_COLOR = 7;  // белый
    static const int TIME_COLOR = 8;     // серый для времени
    static const int INPUT_COLOR = 15;   // ярко-белый для ввода

public:
    ConsoleColor() {
        hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    }

    void setColor(int color) const {
        SetConsoleTextAttribute(hConsole, color);
    }

    void setMyColor() const { setColor(MY_COLOR); }
    void setOtherColor() const { setColor(OTHER_COLOR); }
    void setSystemColor() const { setColor(SYSTEM_COLOR); }
    void setDefaultColor() const { setColor(DEFAULT_COLOR); }
    void setTimeColor() const { setColor(TIME_COLOR); }
    void setInputColor() const { setColor(INPUT_COLOR); }
};

// Класс для сбора статистики
class ChatStatistics {
private:
    int messagesSent;
    int messagesReceived;
    chrono::steady_clock::time_point startTime;

public:
    ChatStatistics() : messagesSent(0), messagesReceived(0) {
        startTime = chrono::steady_clock::now();
    }

    void incrementSent() { messagesSent++; }
    void incrementReceived() { messagesReceived++; }

    int getMessagesSent() const { return messagesSent; }
    int getMessagesReceived() const { return messagesReceived; }
    int getTotalMessages() const { return messagesSent + messagesReceived; }

    double getSessionDuration() const {
        auto now = chrono::steady_clock::now();
        return chrono::duration_cast<chrono::seconds>(now - startTime).count();
    }

    void showStats(const ConsoleColor& console) const {
        console.setSystemColor();
        cout << "\n========== СТАТИСТИКА ЧАТА ==========" << endl;
        console.setDefaultColor();

        cout << "Отправлено сообщений: " << messagesSent << endl;
        cout << "Получено сообщений: " << messagesReceived << endl;
        cout << "Всего сообщений: " << getTotalMessages() << endl;

        int minutes = static_cast<int>(getSessionDuration()) / 60;
        int seconds = static_cast<int>(getSessionDuration()) % 60;
        cout << "Длительность сессии: " << minutes << " мин " << seconds << " сек" << endl;

        if (messagesSent + messagesReceived > 0) {
            double avgPerMinute = getTotalMessages() / (getSessionDuration() / 60.0);
            cout << "Средняя активность: " << avgPerMinute << " сообщ/мин" << endl;
        }

        console.setSystemColor();
        cout << "======================================\n" << endl;
        console.setDefaultColor();
    }
};

// Класс для управления эмодзи
class EmojiManager {
private:
    map<string, string> emojiMap;

    void initEmojiMap() {
        emojiMap["/smile"] = ":-)";
        emojiMap["/love"] = "<3";
        emojiMap["/cool"] = "B-)";
    }

public:
    EmojiManager() {
        initEmojiMap();
    }

    string replaceCommands(const string& message) const {
        string result = message;

        for (const auto& [cmd, emoji] : emojiMap) {
            size_t pos = 0;
            while ((pos = result.find(cmd, pos)) != string::npos) {
                result.replace(pos, cmd.length(), emoji);
                pos += emoji.length();
            }
        }
        return result;
    }

    void showHelp(const ConsoleColor& console) const {
        cout << endl;
        console.setSystemColor();
        cout << "========== ДОСТУПНЫЕ КОМАНДЫ ==========" << endl;
        console.setDefaultColor();

        for (const auto& [cmd, emoji] : emojiMap) {
            cout << "  " << cmd << " -> " << emoji << endl;
        }

        console.setSystemColor();
        cout << "----------------------------------------" << endl;
        console.setDefaultColor();
        cout << "  /help - показать это меню" << endl;
        cout << "  /exit - выход из чата" << endl;
        cout << "  /stats - показать статистику чата" << endl;
        console.setSystemColor();
        cout << "========================================\n" << endl;
        console.setDefaultColor();
    }
};

// Класс для отслеживания напоминаний
class ReminderTracker {
private:
    chrono::steady_clock::time_point lastMessageTime;
    chrono::steady_clock::time_point lastReminderTime;
    bool waitingForMyResponse;

public:
    ReminderTracker()
        : lastMessageTime(chrono::steady_clock::now())
        , lastReminderTime(chrono::steady_clock::now())
        , waitingForMyResponse(false) {
    }

    void messageReceived() {
        lastMessageTime = chrono::steady_clock::now();
        waitingForMyResponse = true;
    }

    void messageSent() {
        lastMessageTime = chrono::steady_clock::now();
        waitingForMyResponse = false;
    }

    bool needReminder() {
        if (!waitingForMyResponse) return false;

        auto now = chrono::steady_clock::now();
        auto secondsSinceLastMsg = chrono::duration_cast<chrono::seconds>(now - lastMessageTime).count();
        auto secondsSinceLastReminder = chrono::duration_cast<chrono::seconds>(now - lastReminderTime).count();

        if (secondsSinceLastMsg >= 60 && secondsSinceLastReminder >= 60) {
            lastReminderTime = now;
            return true;
        }
        return false;
    }

    void reset() {
        waitingForMyResponse = false;
    }
};

// Класс для управления сокетом
class SocketManager {
private:
    SOCKET socket;

public:
    SocketManager() : socket(INVALID_SOCKET) {}

    ~SocketManager() {
        close();
    }

    bool create() {
        socket = ::socket(AF_INET, SOCK_STREAM, 0);
        return socket != INVALID_SOCKET;
    }

    bool bind(int port) {
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = INADDR_ANY;

        return ::bind(socket, (sockaddr*)&addr, sizeof(addr)) != SOCKET_ERROR;
    }

    bool listen(int backlog) {
        return ::listen(socket, backlog) != SOCKET_ERROR;
    }

    SOCKET accept(sockaddr_in& clientAddr) {
        int clientSize = sizeof(clientAddr);
        return ::accept(socket, (sockaddr*)&clientAddr, &clientSize);
    }

    bool connect(const string& ip, int port) {
        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        addr.sin_addr.s_addr = inet_addr(ip.c_str());

        if (addr.sin_addr.s_addr == INADDR_NONE) {
            return false;
        }

        return ::connect(socket, (sockaddr*)&addr, sizeof(addr)) != SOCKET_ERROR;
    }

    int send(const string& data) const {
        return ::send(socket, data.c_str(), data.length(), 0);
    }

    string receive() const {
        char buffer[1024] = { 0 };
        int bytes = recv(socket, buffer, sizeof(buffer) - 1, 0);
        if (bytes <= 0) return "";
        return string(buffer);
    }

    void close() {
        if (socket != INVALID_SOCKET) {
            closesocket(socket);
            socket = INVALID_SOCKET;
        }
    }

    SOCKET getSocket() const { return socket; }
    void setSocket(SOCKET s) { socket = s; }
};

// Класс для управления чат-сессией
class ChatSession {
private:
    SocketManager socketManager;
    string myName;
    string otherName;
    atomic<bool>& running;
    ReminderTracker& reminder;
    EmojiManager& emoji;
    ConsoleColor& console;
    ChatStatistics& stats;

    void clearLine() {
        cout << '\r' << string(80, ' ') << '\r' << flush;
    }

    void showInput(const string& input) {
        console.setInputColor();
        cout << input << flush;
        console.setDefaultColor();
    }

public:
    ChatSession(SOCKET socket, const string& myName, const string& otherName,
        atomic<bool>& run, ReminderTracker& rem, EmojiManager& em,
        ConsoleColor& cons, ChatStatistics& stat)
        : myName(myName), otherName(otherName), running(run), reminder(rem),
        emoji(em), console(cons), stats(stat) {
        socketManager.setSocket(socket);
    }

    void start() {
        thread receiver(&ChatSession::receiveMessages, this);
        receiver.detach();

        thread reminderThread(&ChatSession::reminderLoop, this);
        reminderThread.detach();
    }

    void receiveMessages() {
        while (running) {
            string message = socketManager.receive();

            if (message.empty()) {
                clearLine();
                console.setSystemColor();
                cout << otherName << " отключился!" << endl;
                console.setDefaultColor();
                running = false;
                break;
            }

            stats.incrementReceived();
            reminder.messageReceived();

            // Очищаем строку с вводом
            clearLine();

            // Выводим полученное сообщение
            console.setTimeColor();
            cout << TimeUtils::getCurrentTime() << " ";
            console.setOtherColor();
            cout << otherName;
            console.setDefaultColor();
            cout << ": " << message << endl;
        }
    }

    void reminderLoop() {
        while (running) {
            this_thread::sleep_for(chrono::seconds(5));

            if (reminder.needReminder() && running) {
                clearLine();

                console.setSystemColor();
                cout << "НАПОМИНАНИЕ: ";
                console.setTimeColor();
                cout << TimeUtils::getCurrentTime() << " ";
                console.setOtherColor();
                cout << otherName;
                console.setDefaultColor();
                cout << " ждет ответа!" << endl;
            }
        }
    }

    string readInput() {
        string input;
        char ch;

        while (running) {
            if (_kbhit()) {
                ch = _getch();

                if (ch == '\r') { // Enter
                    // Очищаем строку с вводом перед отправкой
                    clearLine();
                    return input;
                }
                else if (ch == '\b') { // Backspace
                    if (!input.empty()) {
                        input.pop_back();
                        // Перерисовываем строку ввода
                        cout << '\r' << string(80, ' ') << '\r';
                        showInput(input);
                    }
                }
                else {
                    input += ch;
                    cout << ch << flush; // Показываем символ
                }
            }
            this_thread::sleep_for(chrono::milliseconds(10));
        }
        return "";
    }

    void sendMessage(const string& msg) {
        string processedMessage = emoji.replaceCommands(msg);

        if (msg == "/help") {
            emoji.showHelp(console);
            return;
        }

        if (msg == "/stats") {
            stats.showStats(console);
            return;
        }

        if (msg != "/exit") {
            socketManager.send(processedMessage);
            stats.incrementSent();
            reminder.messageSent();

            // Выводим отправленное сообщение
            console.setTimeColor();
            cout << TimeUtils::getCurrentTime() << " ";
            console.setMyColor();
            cout << myName;
            console.setDefaultColor();
            cout << ": " << processedMessage << endl;
        }
    }

    string getName() const { return myName; }
    void close() {
        socketManager.close();
    }
};

// Класс для управления подключением
class ConnectionManager {
private:
    static const int PORT = 8080;
    static const int MAX_ATTEMPTS = 3;

    SocketManager chatSocket;
    string myName;
    string otherName;
    ConsoleColor& console;

public:
    ConnectionManager(ConsoleColor& cons) : console(cons) {}

    bool initWinsock() {
        WSADATA wsaData;
        return WSAStartup(MAKEWORD(2, 2), &wsaData) == 0;
    }

    bool startServer() {
        console.setSystemColor();
        cout << "\n--- Режим сервера ---" << endl;
        console.setDefaultColor();

        SocketManager serverSocket;
        if (!serverSocket.create()) {
            cout << "Ошибка создания сокета!" << endl;
            return false;
        }

        if (!serverSocket.bind(PORT)) {
            cout << "Ошибка bind! Порт занят." << endl;
            return false;
        }

        if (!serverSocket.listen(1)) {
            cout << "Ошибка listen!" << endl;
            return false;
        }

        showIPAddresses();

        cout << "\nОжидание подключения на порту " << PORT << "..." << endl;

        sockaddr_in clientAddr;
        chatSocket.setSocket(serverSocket.accept(clientAddr));

        if (chatSocket.getSocket() == INVALID_SOCKET) {
            cout << "Ошибка подключения!" << endl;
            return false;
        }

        console.setSystemColor();
        cout << "\nПодключение от: ";
        console.setDefaultColor();
        cout << inet_ntoa(clientAddr.sin_addr) << endl;

        exchangeNames();

        console.setOtherColor();
        cout << otherName;
        console.setDefaultColor();
        cout << " подключился(ась)!" << endl;

        return true;
    }

    bool startClient() {
        console.setSystemColor();
        cout << "\n--- Режим клиента ---" << endl;
        console.setDefaultColor();

        string serverIP;
        cout << "Введите IP-адрес сервера (друга): ";
        getline(cin, serverIP);

        if (serverIP.empty()) {
            cout << "IP не введен. Выход..." << endl;
            return false;
        }

        if (!chatSocket.create()) {
            cout << "Ошибка создания сокета!" << endl;
            return false;
        }

        cout << "Подключение к " << serverIP << ":" << PORT << "..." << endl;

        if (!tryConnect(serverIP)) {
            return false;
        }

        exchangeNames();

        cout << "Подключение к ";
        console.setOtherColor();
        cout << otherName;
        console.setDefaultColor();
        cout << "!" << endl;

        return true;
    }

    bool exchangeNames() {
        chatSocket.send(myName);
        otherName = chatSocket.receive();
        return !otherName.empty();
    }

    string getOtherName() const { return otherName; }
    SOCKET getSocket() const { return chatSocket.getSocket(); }

    void cleanup() {
        WSACleanup();
    }

    void setMyName(const string& name) { myName = name; }

private:
    void showIPAddresses() {
        char hostname[256];
        gethostname(hostname, sizeof(hostname));
        struct hostent* host = gethostbyname(hostname);

        console.setSystemColor();
        cout << "\nВаши IP-адреса (скажите другу один из них):" << endl;
        console.setDefaultColor();

        if (host != NULL) {
            for (int i = 0; host->h_addr_list[i] != NULL; i++) {
                struct in_addr addr;
                memcpy(&addr, host->h_addr_list[i], sizeof(addr));
                cout << "  " << inet_ntoa(addr) << endl;
            }
        }
    }

    bool tryConnect(const string& ip) {
        int attempts = 0;
        bool connected = false;

        while (attempts < MAX_ATTEMPTS && !connected) {
            if (attempts > 0) {
                cout << "Попытка " << attempts + 1 << "... ждем 2 секунды" << endl;
                Sleep(2000);
            }
            connected = chatSocket.connect(ip, PORT);
            attempts++;
        }

        if (!connected) {
            cout << "Не удалось подключиться!" << endl;
            cout << "Проверьте:" << endl;
            cout << "1. Запущен ли сервер на другом компьютере" << endl;
            cout << "2. IP адрес правильный" << endl;
            cout << "3. Брандмауэр не блокирует порт" << endl;
            cout << "4. Подключение к одной сети" << endl;
            return false;
        }
        return true;
    }
};

// Класс для управления пользовательским интерфейсом
class UserInterface {
private:
    ConsoleColor console;

public:
    void setup() {
        SetConsoleOutputCP(1251);
        SetConsoleCP(1251);
        setlocale(LC_ALL, "Russian");
    }

    void showWelcomeScreen() {
        console.setSystemColor();
        cout << "=====================================" << endl;
        cout << "              ЧАТ" << endl;
        cout << "=====================================\n" << endl;
        console.setDefaultColor();
    }

    string getName() {
        string name;
        cout << "Введите ваше имя: ";
        getline(cin, name);
        return name.empty() ? "Пользователь" : name;
    }

    string getMode() {
        string mode;
        cout << "\nВыберите режим:" << endl;
        cout << "1 - Как сервер (ожидание друга)" << endl;
        cout << "2 - Как клиент (подключение к другу)" << endl;
        cout << "Ваш выбор (1 или 2): ";
        getline(cin, mode);
        return mode;
    }

    void showChatStart(const string& myName, const string& otherName) {
        console.setSystemColor();
        cout << "\n=====================================" << endl;
        cout << "           НАЧАЛО ЧАТА" << endl;
        cout << "=====================================" << endl;
        console.setDefaultColor();

        cout << "Вы: ";
        console.setMyColor();
        cout << myName << endl;
        console.setDefaultColor();

        cout << "Собеседник: ";
        console.setOtherColor();
        cout << otherName << endl;
        console.setDefaultColor();

        console.setTimeColor();
        cout << "\nВремя сообщений будет отображаться в формате [ЧЧ:ММ:СС]" << endl;
        console.setDefaultColor();

        cout << "Для выхода введите '/exit'\n" << endl;
        cout << "Теперь просто печатайте - текст будет виден, но не продублируется\n" << endl;
    }

    ConsoleColor& getConsole() { return console; }
};

// Вспомогательные функции для main
bool initializeApplication(ConnectionManager& connection, const string& myName) {
    connection.setMyName(myName);

    if (!connection.initWinsock()) {
        cout << "Ошибка инициализации Winsock!" << endl;
        return false;
    }

    return true;
}

bool establishConnection(const string& mode, ConnectionManager& connection) {
    bool connected = false;

    if (mode == "1") {
        connected = connection.startServer();
    }
    else if (mode == "2") {
        connected = connection.startClient();
    }
    else {
        cout << "Неверный режим!" << endl;
        return false;
    }

    return connected;
}

void runChatSession(const string& myName, ConnectionManager& connection, ConsoleColor& console, EmojiManager& emoji, ReminderTracker& reminder, atomic<bool>& running, ChatStatistics& stats) {
    string otherName = connection.getOtherName();

    UserInterface ui;
    ui.showChatStart(myName, otherName);
    emoji.showHelp(console);

    ChatSession chat(connection.getSocket(), myName, otherName, running, reminder, emoji, console, stats);
    chat.start();

    while (running) {
        string message = chat.readInput();

        if (!running) break;

        if (message == "/exit") {
            chat.sendMessage(message);
            break;
        }

        chat.sendMessage(message);
    }

    running = false;
    Sleep(1000);
    chat.close();

    cout << endl;
    stats.showStats(console);
}

void shutdownApplication(ConnectionManager& connection) {
    connection.cleanup();

    cout << "\nНажмите Enter для выхода...";
    cin.get();
}

// Точка входа
int main() {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    // инициализация
    UserInterface ui;
    ui.setup();
    ui.showWelcomeScreen();

    // получение данных от пользователей
    string myName = ui.getName();
    string mode = ui.getMode();

    // инициализация Winsock
    ConnectionManager connection(ui.getConsole());
    if (!initializeApplication(connection, myName)) {
        return 1;
    }

    // установка соединения
    if (!establishConnection(mode, connection)) {
        connection.cleanup();
        return 1;
    }

    // запуск чата
    EmojiManager emoji;
    ReminderTracker reminder;
    atomic<bool> running(true);
    ChatStatistics stats;

    runChatSession(myName, connection, ui.getConsole(), emoji, reminder, running, stats);

    // завершение работы
    shutdownApplication(connection);

    return 0;
}