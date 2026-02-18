#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <iostream> //для ввода или вывода
#include <winsock2.h> //для работы с сокетами
#include <string> //для работы со строками
#include <thread> //для работы с потоками (прием смс, напоминания)
#include <windows.h> //функции windows (цвет и beep)
#include <chrono> // для работы со временем (таймеры)
#include <atomic> //для атомарных переменных (безопасность процесса)
#include <map> //для эмодзи

#pragma comment(lib, "ws2_32.lib") //подключаем библиотеку

using namespace std;

//Глобальные переменные
atomic<bool> running(true); //флаг работы программы (безопасен для потоков)
string otherName; //имя собеседника
HANDLE hConsole; //разрешение в виде числа для управления цветом

//Цвета для консоли (коды)
const int MY_COLOR = 10;      // Зеленый
const int OTHER_COLOR = 9;    // Синий+
const int SYSTEM_COLOR = 14;  // Желтый
const int DEFAULT_COLOR = 7;  // Белый

//Объявление функций
void setColor(int color);
void playReceiveSound();//звук полученного сообщения
void playReminderSound();//звук напоминания
void setupConsole(); //настройка перед началом работы
void showWelcomeScreen(); //показ приветственного экрана
string getName(); //запрос имени
string getMode(); //запрос режима работы
void showChatStart(const string& myName);//показ стартового экрана чата

//Структура таймера (для напоминаний)
struct ReminderTracker {
    chrono::steady_clock::time_point lastMessageTime; //фиксирует время последнего смс
    chrono::steady_clock::time_point lastReminderTime; //фиксирует время последнего напоминания
    bool waitingForMyResponse;  //флаг, указывающий, ждем ли мы ответ от пользователя

    ReminderTracker() {
        lastMessageTime = chrono::steady_clock::now();//устанавливаем текущее время
        lastReminderTime = chrono::steady_clock::now();
        waitingForMyResponse = false;//ответа изначально не ждем
    }

    void messageReceived() {
        lastMessageTime = chrono::steady_clock::now();//обновляем время последнего смс
        waitingForMyResponse = true;
    }

    void messageSent() {
        lastMessageTime = chrono::steady_clock::now();
        waitingForMyResponse = false;
    }

    bool needReminder() {
        //если не ждем ответа - напоминание не нужно
        if (!waitingForMyResponse) return false;

        auto now = chrono::steady_clock::now();
        auto secondsSinceLastMsg = chrono::duration_cast<chrono::seconds>(now - lastMessageTime).count(); //сколько секунд прошло с последнего смс
        auto secondsSinceLastReminder = chrono::duration_cast<chrono::seconds>(now - lastReminderTime).count();

        if (secondsSinceLastMsg >= 60 && secondsSinceLastReminder >= 60) {
            lastReminderTime = now;
            return true; //нужно отправить напоминание
        }
        return false;
    }
};

ReminderTracker reminder;

//Структура для эмодзи
struct EmojiManager {
    map<string, string> emojiMap;

    void init() {
        emojiMap["/smile"] = ":-)";
        emojiMap["/love"] = "<3";
        emojiMap["/cool"] = "B-)";
    }

    //заменяет все найденные команды в сообщении на эмодзи
    string replaceCommands(const string& message) {
        string result = message;

        for (map<string, string>::iterator it = emojiMap.begin(); it != emojiMap.end(); ++it) {
            string cmd = it->first;
            string emoji = it->second;

            size_t pos = 0;
            while ((pos = result.find(cmd, pos)) != string::npos) {
                result.replace(pos, cmd.length(), emoji);
                pos += emoji.length();
            }
        }
        return result;
    }

    void showHelp() {
        cout << endl;
        setColor(SYSTEM_COLOR);
        cout << "Доступные команды:" << endl;
        setColor(DEFAULT_COLOR);
        cout << "  /smile  -> " << emojiMap["/smile"] << " (смайлик)" << endl;
        cout << "  /love   -> " << emojiMap["/love"] << " (сердечко)" << endl;
        cout << "  /cool   -> " << emojiMap["/cool"] << " (крутой)" << endl;
        setColor(SYSTEM_COLOR);
        cout << "----------------------------------------" << endl;
        setColor(DEFAULT_COLOR);
    }
};

EmojiManager emoji;

//Структура чата
struct ChatSession {
    SOCKET socket;
    string myName;

    void start() {
        thread receiver(&ChatSession::receiveMessages, this);
        receiver.detach();

        thread reminderThread(&ChatSession::reminderLoop, this);
        reminderThread.detach();
    }

    void receiveMessages() {//бесконечно ждет смс от собеседника
        char buffer[1024];
        while (running) {
            memset(buffer, 0, sizeof(buffer));
            int bytes = recv(socket, buffer, sizeof(buffer) - 1, 0);

            //проверяет не отключился ли собеседник
            if (bytes <= 0) {
                cout << "\n";
                setColor(SYSTEM_COLOR);
                cout << otherName << " отключился!" << endl;
                setColor(DEFAULT_COLOR);
                running = false;
                break;
            }

            string message(buffer);

            reminder.messageReceived();
            playReceiveSound();

            cout << "\r";
            setColor(OTHER_COLOR);
            cout << otherName;
            setColor(DEFAULT_COLOR);
            cout << ": " << message << endl;

            setColor(MY_COLOR);
            cout << myName << ": " << flush;
            setColor(DEFAULT_COLOR);
        }
    }
    //каждые 5 секунд проверяет нужно ли напоминание
    void reminderLoop() {
        while (running) {
            this_thread::sleep_for(chrono::seconds(5));

            if (reminder.needReminder() && running) {
                cout << "\r";
                setColor(SYSTEM_COLOR);
                cout << "\aНАПОМИНАНИЕ" << endl;
                setColor(OTHER_COLOR);
                cout << otherName;
                setColor(DEFAULT_COLOR);
                cout << " написал(а) вам, а вы ещё не ответили!" << endl;

                playReminderSound();

                setColor(MY_COLOR);
                cout << myName << ": " << flush;
                setColor(DEFAULT_COLOR);
            }
        }
    }

    void sendMessage(const string& msg) {
        string processedMessage = emoji.replaceCommands(msg);
        send(socket, processedMessage.c_str(), processedMessage.length(), 0);
        reminder.messageSent();
    }

    void close() {
        closesocket(socket);
    }
};

//Структура для управления сетевым соединением
struct ConnectionManager {
    SOCKET chatSocket;
    string myName;

    bool initWinsock() {
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
            cout << "Ошибка инициализации Winsock!" << endl;
            return false;
        }
        return true;
    }

    bool startServer() {
        setColor(SYSTEM_COLOR);
        cout << "\n=== РЕЖИМ СЕРВЕРА ===" << endl;
        setColor(DEFAULT_COLOR);

        SOCKET serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            cout << "Ошибка создания сокета!" << endl;
            return false;
        }

        sockaddr_in addr;
        addr.sin_family = AF_INET;//тип адреса
        addr.sin_port = htons(12345);
        addr.sin_addr.s_addr = INADDR_ANY; //IP-адрес

        if (bind(serverSocket, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            cout << "Ошибка bind! Порт занят." << endl;
            closesocket(serverSocket);
            return false;
        }

        if (listen(serverSocket, 1) == SOCKET_ERROR) {
            cout << "Ошибка listen!" << endl;
            closesocket(serverSocket);
            return false;
        }

        showIPAddresses();

        cout << "\nОжидание подключения на порту 12345..." << endl;

        sockaddr_in clientAddr;
        int clientSize = sizeof(clientAddr);
        chatSocket = accept(serverSocket, (sockaddr*)&clientAddr, &clientSize);

        if (chatSocket == INVALID_SOCKET) {
            cout << "Ошибка подключения!" << endl;
            closesocket(serverSocket);
            return false;
        }

        setColor(SYSTEM_COLOR);
        cout << "\nПодключено от: ";
        setColor(DEFAULT_COLOR);
        cout << inet_ntoa(clientAddr.sin_addr) << endl;

        exchangeNames();

        setColor(OTHER_COLOR);
        cout << otherName;
        setColor(DEFAULT_COLOR);
        cout << " подключился(ась)!" << endl;

        closesocket(serverSocket);
        return true;
    }

    bool startClient() {
        setColor(SYSTEM_COLOR);
        cout << "\n=== РЕЖИМ КЛИЕНТА ===" << endl;
        setColor(DEFAULT_COLOR);

        string serverIP;
        cout << "Введите IP адрес сервера (друга): ";
        getline(cin, serverIP);

        if (serverIP.empty()) {
            cout << "IP не введен. Выход..." << endl;
            return false;
        }

        chatSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (chatSocket == INVALID_SOCKET) {
            cout << "Ошибка создания сокета!" << endl;
            return false;
        }

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(12345);
        addr.sin_addr.s_addr = inet_addr(serverIP.c_str());

        if (addr.sin_addr.s_addr == INADDR_NONE) {
            cout << "Некорректный IP адрес!" << endl;
            closesocket(chatSocket);
            return false;
        }

        cout << "Подключение к " << serverIP << ":12345..." << endl;

        //пытается подключиться до трех раз
        if (!tryConnect(addr)) {
            return false;
        }

        exchangeNames();

        cout << "Подключено к ";
        setColor(OTHER_COLOR);
        cout << otherName;
        setColor(DEFAULT_COLOR);
        cout << "!" << endl;

        return true;
    }

    void showIPAddresses() {
        char hostname[256];
        gethostname(hostname, sizeof(hostname));
        struct hostent* host = gethostbyname(hostname);

        setColor(SYSTEM_COLOR);
        cout << "\nВаши IP-адреса (скажите другу один из них):" << endl;
        setColor(DEFAULT_COLOR);
        if (host != NULL) {
            for (int i = 0; host->h_addr_list[i] != NULL; i++) {
                struct in_addr addr;
                memcpy(&addr, host->h_addr_list[i], sizeof(addr));
                cout << "  " << inet_ntoa(addr) << endl;
            }
        }
    }

    bool tryConnect(sockaddr_in& addr) {
        int attempts = 0;
        int connectResult = SOCKET_ERROR;

        while (attempts < 3 && connectResult == SOCKET_ERROR) {
            if (attempts > 0) {
                cout << "Попытка " << attempts + 1 << "... ждем 2 секунды" << endl;
                Sleep(2000);
            }
            connectResult = connect(chatSocket, (sockaddr*)&addr, sizeof(addr));
            attempts++;
        }

        if (connectResult == SOCKET_ERROR) {
            cout << "Не удалось подключиться!" << endl;
            cout << "Проверьте:" << endl;
            cout << "1. Сервер запущен на другом компьютере" << endl;
            cout << "2. IP адрес правильный" << endl;
            cout << "3. Брандмауэр не блокирует порт" << endl;
            cout << "4. Компьютеры в одной сети" << endl;
            return false;
        }
        return true;
    }

    void exchangeNames() {
        send(chatSocket, myName.c_str(), myName.length(), 0);

        char nameBuffer[256] = { 0 };
        recv(chatSocket, nameBuffer, 255, 0);
        otherName = nameBuffer;
    }

    void cleanup() {
        WSACleanup();
    }
};

// Вспомогательные функции
void setColor(int color) {
    SetConsoleTextAttribute(hConsole, color);
}

void playReceiveSound() {
    Beep(1000, 200); 
}

void playReminderSound() {
    Beep(800, 300);
    Sleep(100);
    Beep(800, 300);
}

void setupConsole() {
    hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleOutputCP(1251);
    SetConsoleCP(1251);
    setlocale(LC_ALL, "Russian");
}

void showWelcomeScreen() {
    setColor(SYSTEM_COLOR);
    cout << "=====================================" << endl;
    cout << "              ЧАТ" << endl;
    cout << "=====================================\n" << endl;
    setColor(DEFAULT_COLOR);
}

string getName() {
    string name;
    cout << "Введите ваше имя: ";
    getline(cin, name);
    if (name.empty()) return "Пользователь";
    return name;
}

string getMode() {
    string mode;
    cout << "\nВыберите режим:" << endl;
    cout << "1 - Я СЕРВЕР (подключаю друга)" << endl;
    cout << "2 - Я КЛИЕНТ (подключаюсь к другу)" << endl;
    cout << "Ваш выбор (1 или 2): ";
    getline(cin, mode);
    return mode;
}

void showChatStart(const string& myName) {
    setColor(SYSTEM_COLOR);
    cout << "\n=====================================" << endl;
    cout << "           ЧАТ НАЧАТ" << endl;
    cout << "=====================================" << endl;
    setColor(DEFAULT_COLOR);

    cout << "Вы: ";
    setColor(MY_COLOR);
    cout << myName << endl;
    setColor(DEFAULT_COLOR);

    cout << "Собеседник: ";
    setColor(OTHER_COLOR);
    cout << otherName << endl;
    setColor(DEFAULT_COLOR);

    cout << "Для выхода введите 'exit'" << endl;
}

// Инициализация приложения
bool initializeApplication(ConnectionManager & connection, const string & myName) {
    // настройка консоли уже выполнена в main перед вызовом

    // инициализация смайликов
    emoji.init();

    // инициализация Winsock
    connection.myName = myName;
    if (!connection.initWinsock()) {
        return false;
    }

    return true;
}

// Установка соединения в зависимости от режима
bool establishConnection(const string& mode, ConnectionManager& connection) {
    bool connected = false;

    if (mode == "1") {
        connected = connection.startServer();
    }
    else if (mode == "2") {
        connected = connection.startClient();
    }
    else {
        cout << "Неверный выбор!" << endl;
        return false;
    }

    return connected;
}

// Запуск чат-сессии и основной цикл общения
void runChatSession(const string& myName, ConnectionManager& connection) {
    // запуск чата
    showChatStart(myName);
    emoji.showHelp();

    ChatSession chat;
    chat.socket = connection.chatSocket;
    chat.myName = myName;
    chat.start();

    // основной цикл отправки сообщений
    string message;
    while (running) {
        setColor(MY_COLOR);
        cout << myName << ": " << flush;
        setColor(DEFAULT_COLOR);

        getline(cin, message);

        if (message == "exit") {
            send(connection.chatSocket, message.c_str(), message.length(), 0);
            break;
        }

        chat.sendMessage(message);
    }

    // завершение работы чата
    running = false;
    Sleep(1000);
    chat.close();
}

// Завершение работы приложения
void shutdownApplication(ConnectionManager& connection) {
    connection.cleanup();

    setColor(SYSTEM_COLOR);
    cout << "\nНажмите Enter для выхода...";
    setColor(DEFAULT_COLOR);
    cin.get();
}

int main() {
    // Инициализация
    setupConsole();
    showWelcomeScreen();

    // Получаем имя и режим
    string myName = getName();
    string mode = getMode();

    // Менеджер соединений
    ConnectionManager connection;
    if (!initializeApplication(connection, myName)) {
        return 1;
    }

    //Установка соединения
    if (!establishConnection(mode, connection)) {
        connection.cleanup();
        return 1;
    }

    runChatSession(myName, connection);
    shutdownApplication(connection);

    return 0;
}