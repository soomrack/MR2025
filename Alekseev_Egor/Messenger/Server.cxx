#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#pragma comment(lib, "ws2_32.lib")

int result;
sockaddr_in address{}; //Структура для IPv4 адреса
SOCKET server_fd;
SOCKET client_fd;


bool WinSock_Init(){
    WSADATA wsaData;
    result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed\n";
        return false;
    }
    else return true;    
}

bool Make_Socket(){
    server_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_fd == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << "\n";
        WSACleanup();
        return false;
    }
    else return true;
}

void Socket_Config(){
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; //Принимаются соединения на всех сетевых интерфейсах
    address.sin_port = htons(8080);
}

bool Binding(){
    if (bind(server_fd, (sockaddr*)&address, sizeof(address)) == SOCKET_ERROR) {
        std::cerr << "Bind failed: " << WSAGetLastError() << "\n";
        closesocket(server_fd);
        WSACleanup();
        return false;
    }
    else return true;        
}

bool listening(){
    if (listen(server_fd, 1) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << "\n";
        closesocket(server_fd);
        WSACleanup();
        return false;
    }
    else return true;    
}

bool accept_connection(){
    client_fd = accept(server_fd, nullptr, nullptr);
    if (client_fd == INVALID_SOCKET) {
        std::cerr << "Accept failed: " << WSAGetLastError() << "\n";
        closesocket(server_fd);
        WSACleanup();
        return false;
    }
    else{
        std::cout << "Connected successfully" << std::endl;
        return true;
    }
}

bool receive_message() {
    char buffer[1024] = {};
    int received_bytes = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
    
    if (received_bytes > 0) {
        buffer[received_bytes] = '\0';
        if (std::string(buffer) == "*Quit") {
            std::cout << "Connection closed by client..." << std::endl;
            return false;
        } else {
            std::cout << '\n' << "Received message: " << buffer << std::endl;
            std::cout << "---------------------------------------" << std::endl;
            return true;
        }
    } 
    else if (received_bytes == 0) {
        std::cout << "Connection closed by client" << std::endl;
        return false;
    }
    else {
        std::cerr << "Error receiving data" << std::endl;
        return false;
    }
}

bool send_message(){
    std::string message;
    std::cout <<"Enter message" << std::endl;
    std::getline(std::cin, message);
    if (message == "*Quit"){        //Принудительная остановка
        std::cout << "Closing connection..." << std::endl;
        result = send(client_fd, message.data(), message.length(), 0);
        if (result == SOCKET_ERROR) {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
            return false;
            }
        return false;
        }
    else{
        result = send(client_fd, message.data(), message.length(), 0);
        if (result == SOCKET_ERROR) {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
            closesocket(client_fd);
            WSACleanup();
            return false;
            }
        else return true;
        }
}

void close_socket(){
    closesocket(client_fd);
    closesocket(server_fd);
    WSACleanup();  // Освобождение ресурсов WinSock
}


int main() {
    std::cout << "Starting server" << std::endl;
    if (!WinSock_Init()) return 1;
    if (!Make_Socket()) return 1;
    Socket_Config();
    if (!Binding()) return 1;
    if (!listening()) return 1;
    if (!accept_connection()) return 1;

while (true){
    if (!receive_message()) return 1;
    if (!send_message()) return 1;
}
close_socket();
return 0;
}