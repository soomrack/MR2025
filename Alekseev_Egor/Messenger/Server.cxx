#include <winsock2.h>
#include <ws2tcpip.h>
#include <iostream>
#pragma comment(lib, "ws2_32.lib")


const unsigned short SERVER_PORT = 8080;


bool WinSock_Init(){
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed\n";
        return false;
    }
    else return true;    
}

SOCKET create_socket(){
    SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
    }
    return server_socket;
}

sockaddr_in configure_socket(){
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY; //Принимаются соединения на всех сетевых интерфейсах
    address.sin_port = htons(SERVER_PORT);    
    return address;
}

bool bind_socket(SOCKET server_socket, const sockaddr_in&address){
    if (bind(server_socket, (sockaddr*)&address, sizeof(address)) == SOCKET_ERROR){
            std::cerr << "Bind failed: " << WSAGetLastError() << "\n";
            return false;
    }
    return true;  
}

bool start_listening(SOCKET server_socket){
    if (listen(server_socket, 1) == SOCKET_ERROR) {
        std::cerr << "Listen failed: " << WSAGetLastError() << "\n";
        return false;
    }
    return true;     
}

SOCKET accept_connection(SOCKET server_socket){
    SOCKET client_socket = accept(server_socket, nullptr, nullptr);
    if (client_socket == INVALID_SOCKET) {
        std::cerr << "Accept failed: " << WSAGetLastError() << "\n";
    }
    else{
        std::cout << "Connected successfully" << std::endl;
    }
    return client_socket;
}

bool receive_message(SOCKET client_socket){
    char buffer[1024] = {};
    int received_bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (received_bytes > 0) {
        buffer[received_bytes] = '\0';
        std::string message(buffer);

        if (message == "*Quit") {
            std::cout << "Connection closed by client..." << std::endl;
            return false;
        } 

        std::cout << '\n' << "Received message: " << buffer << std::endl;
        std::cout << "---------------------------------------" << std::endl;
        return true;
    } 
    if (received_bytes == 0) {
        std::cout << "Client disconnected..." << std::endl;
        return false;
    }
    else {
        std::cerr << "Error receiving data" << std::endl;
        return false;
    }
}

bool send_message(SOCKET client_socket){
    std::string message;
    std::cout <<"Enter message" << std::endl;
    std::getline(std::cin, message);

    if (message == "*Quit"){        
        std::cout << "Closing connection..." << std::endl;
        int result = send(client_socket, message.data(), message.length(), 0);
        if (result == SOCKET_ERROR) {
            std::cerr << "send failed: " << WSAGetLastError() << std::endl;
            return false;
            }
        return false;
        }
   
    int result = send(client_socket, message.data(), message.length(), 0);
    if (result == SOCKET_ERROR) {
        std::cerr << "send failed: " << WSAGetLastError() << std::endl;
        return false;
        }
    return (message !="*Quit");
}

void close_socket(SOCKET server_socket, SOCKET client_socket){
    if (client_socket != INVALID_SOCKET){
        closesocket(client_socket);
    }
    if (server_socket != INVALID_SOCKET){
        closesocket(server_socket);
    }
    WSACleanup();    
}

int main() {
    std::cout << "Starting server" << std::endl;

    SOCKET server_socket = INVALID_SOCKET;
    SOCKET client_socket = INVALID_SOCKET;
    
    if (!WinSock_Init()) return 1;
    server_socket = create_socket();
    if (server_socket == INVALID_SOCKET) return 1;
    sockaddr_in address = configure_socket();
    if (!bind_socket(server_socket, address)){
        close_socket(server_socket, client_socket);
        return 1;
    }
    if (!start_listening(server_socket)){
        close_socket(server_socket, client_socket);
        return 1;
    }
    client_socket = accept_connection(server_socket);
    if (client_socket == INVALID_SOCKET){
        close_socket(server_socket, client_socket);
        return 1;
    }
    while (true){
        if (!receive_message(client_socket)){
            break;
        }
        if (!send_message(client_socket)){
            break;
        }
    }
    close_socket(server_socket, client_socket);
    std::cout << "Stopping server..."<< std::endl;
    return 0;
}