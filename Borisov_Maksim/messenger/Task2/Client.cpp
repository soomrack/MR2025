// ============================================================
// TCP Клиент мониторинга — Windows / Visual Studio 2022
// ============================================================

#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <locale>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

// ============================================================
// ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ
// ============================================================
std::atomic<bool> running(true);  // Флаг — продолжать ли работу

// Преобразует UTF-8 строку в строку в кодировке Windows (CP1251 по умолчанию)
std::string utf8_to_windows(const std::string& utf8_str, UINT codePage = 1251) {
    if (utf8_str.empty()) return "";

    // 1. UTF-8 -> UTF-16 (WideChar)
    int wideSize = MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, nullptr, 0);
    if (wideSize == 0) return utf8_str; // ошибка, возвращаем как есть
    std::wstring wideStr(wideSize, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8_str.c_str(), -1, &wideStr[0], wideSize);

    // 2. UTF-16 -> Windows-1251 (или указанная codePage)
    int mbSize = WideCharToMultiByte(codePage, 0, wideStr.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (mbSize == 0) return utf8_str;
    std::string result(mbSize, '\0');
    WideCharToMultiByte(codePage, 0, wideStr.c_str(), -1, &result[0], mbSize, nullptr, nullptr);

    // Убираем завершающий нуль
    if (!result.empty() && result.back() == '\0') result.pop_back();
    return result;
}

// ============================================================
// ПОТОК ПРИЁМА ДАННЫХ
// Работает параллельно с вводом команд.
// Получает всё что сервер присылает и выводит на экран.
// ============================================================
void receiveThread(SOCKET sock) {
    char buffer[4096];
    while (running) {
        memset(buffer, 0, sizeof(buffer));
        int bytes = recv(sock, buffer, sizeof(buffer) - 1, 0);

        if (bytes <= 0) {
            if (running) {
                std::cout << "\n[!] Соединение с сервером разорвано." << std::endl;
            }
            running = false;
            break;
        }

        // Выводим полученные данные
        // std::string(buffer, bytes) — создаём строку ровно из bytes байт
        std::string utf8_data(buffer, bytes);
        std::string win_data = utf8_to_windows(utf8_data, 1251); // или 866
        std::cout << win_data;

        // Добавляем "приглашение" к вводу после вывода данных
        // flush сбрасывает буфер вывода без переноса строки
        std::cout.flush();
    }
}

int main() {
    // ————————————————————————————————
    // Инициализация Winsock
    // ————————————————————————————————
    setlocale(LC_ALL, "Russian");
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        std::cerr << "Ошибка WSAStartup" << std::endl;
        return 1;
    }

    // ————————————————————————————————
    // Запрашиваем IP сервера у пользователя
    // ————————————————————————————————
    std::string serverIP;
    std::cout << "=== Клиент мониторинга Raspberry Pi ===" << std::endl;
    std::cout << "Введите IP-адрес Raspberry Pi (или Enter для 127.0.0.1): ";
    std::getline(std::cin, serverIP);
    if (serverIP.empty()) serverIP = "127.0.0.1";

    // ————————————————————————————————
    // Создание сокета и подключение
    // ————————————————————————————————
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (sock == INVALID_SOCKET) {
        std::cerr << "Ошибка создания сокета" << std::endl;
        WSACleanup();
        return 1;
    }

    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(54000);
    inet_pton(AF_INET, serverIP.c_str(), &serverAddr.sin_addr);

    std::cout << "Подключение к " << serverIP << ":54000..." << std::endl;

    if (connect(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Не удалось подключиться. Ошибка: " << WSAGetLastError() << std::endl;
        std::cerr << "Проверьте IP и что сервер запущен." << std::endl;
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    std::cout << "Подключено! Ожидайте приветствие от сервера..." << std::endl << std::endl;

    // ————————————————————————————————
    // Запускаем поток приёма данных
    // ————————————————————————————————
    std::thread recvThread(receiveThread, sock);

    // ————————————————————————————————
    // Основной цикл — ввод команд
    // ————————————————————————————————
    std::string command;
    while (running) {
        std::cout << "> ";  // Приглашение к вводу
        std::getline(std::cin, command);

        if (!running) break;  // Сервер мог отключиться пока мы читали

        if (command.empty()) continue;  // Пропускаем пустой ввод

        // Отправляем команду серверу
        int bytesSent = send(sock, command.c_str(), (int)command.size(), 0);
        if (bytesSent == SOCKET_ERROR) {
            std::cerr << "Ошибка отправки. Соединение потеряно." << std::endl;
            running = false;
            break;
        }

        // Если пользователь написал exit — завершаем
        if (command == "exit") {
            running = false;
            break;
        }

        // Небольшая пауза чтобы поток приёма успел вывести ответ
        // прежде чем мы снова выведем приглашение ">"
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    // ————————————————————————————————
    // Очистка
    // ————————————————————————————————
    running = false;
    closesocket(sock);
    WSACleanup();

    // Ждём завершения потока приёма
    if (recvThread.joinable()) recvThread.join();

    std::cout << "Отключено." << std::endl;
    return 0;
}