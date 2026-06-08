#include <iostream> //библиотечки
#include <vector>
#include <thread> 
#include <mutex>  
#include <algorithm>  
#include <cstring> 
#include <string>

#ifdef _WIN32 //для винды
    #include <winsock2.h> 
    #include <ws2tcpip.h>   
    #pragma comment(lib, "ws2_32.lib")
    using socket_t = SOCKET;
#else
    #include <sys/socket.h>
    #include <netinet/in.h>
    #include <arpa/inet.h>
    #include <unistd.h>
    using socket_t = int;
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
#endif

std::vector<socket_t> clients; //создание вектора из сокетов 
std::mutex clients_mutex; //останавливает многопоточность

void broadcast(const std::string& message, socket_t sender) { //функция вещания
    std::lock_guard<std::mutex> lock(clients_mutex); //блокировка потоков
    for (auto client : clients) {     //проход по клиентам
        if (client != sender) {       // не отправить самому отправителю
            send(client, message.c_str(), message.size(), 0); //рассылка
        }
    }
}

void handle_client(socket_t client_sock) { //для каждого клиента отдельный поток
    char buffer[1024];                     // создание буфера
    while (true) {
        memset(buffer, 0, sizeof(buffer));  // очищаем буфер
        int bytes = recv(client_sock, buffer, sizeof(buffer) - 1, 0);    //(возвращает количество байт)функция копирует данные в буфер если есть новые если нет блокируется 
        if (bytes <= 0) {
            std::cout << "Client disconnected.\n"; //выводим если клиент отключился
            break;
        }
        std::string msg(buffer);   //перевод в строку
        std::cout << "Received: " << msg; //выводим в сервер принятое сообщение
        broadcast(msg, client_sock); //вызываем вещание
    }

    {
        std::lock_guard<std::mutex> lock(clients_mutex);  //блокируем многопоточность
        clients.erase(std::remove(clients.begin(), clients.end(), client_sock), clients.end()); //удаление клиента из вектора
    }
#ifdef _WIN32
    closesocket(client_sock); //закрытие клиенского сокета
#else
    close(client_sock);
#endif
}

int main() {
#ifdef _WIN32
    WSADATA wsa;                          //Инициализация Winsock
    WSAStartup(MAKEWORD(2, 2), &wsa);
#endif

    socket_t server_sock = socket(AF_INET, SOCK_STREAM, 0); //создание серверного сокета
    if (server_sock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed\n";        //вывести если сокет дропнется cerr-стандартный вывод ошибок
        return 1;
    }

    sockaddr_in addr{};               //структура с локальным адрессом сокета 
    addr.sin_family = AF_INET;              //IPv4
    addr.sin_port = htons(12345);           //порт 12345
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_sock, (sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) { //привязываем сокету значения
        std::cerr << "Bind failed\n";    // выводим если сокет не привязался
        return 1;
    }

    if (listen(server_sock, SOMAXCONN) == SOCKET_ERROR) { //переводим сокет в режим прослушивания
        std::cerr << "Listen failed\n"; // выводим если сокет не переведён в режим прослушивания
        return 1;
    }

    std::cout << "Server started on port 12345\n"; // выводим если сервер запустился

    while (true) {
        sockaddr_in client_addr{}; //структура, в которую accept запишет адрес подключившегося клиента (IP и порт)
        socklen_t client_len = sizeof(client_addr);
        socket_t client_sock = accept(server_sock, (sockaddr*)&client_addr, &client_len); //создание клиенского сокета на сервере входит в режим ожидания
        if (client_sock == INVALID_SOCKET) {
            std::cerr << "Accept failed\n"; //выводим если клиентский сокет не робит
            continue;
        }

        {
            std::lock_guard<std::mutex> lock(clients_mutex);  //блокировка потоков
            clients.push_back(client_sock);                   // добавление клиента в список          
        }

        std::thread(handle_client, client_sock).detach(); //структура запускает handle_client
    }

#ifdef _WIN32
    closesocket(server_sock);
    WSACleanup();
#else
    close(server_sock);
#endif
    return 0;
}