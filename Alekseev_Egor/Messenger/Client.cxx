#include <iostream>
#include <winsock2.h>  
#include <ws2tcpip.h>   
#include <string.h>
#pragma comment(lib, "Ws2_32.lib")  

const unsigned short SERVER_PORT = 8080;

bool WinSock_Init(){
    WSADATA wsaData;
    int result = WSAStartup(MAKEWORD(2, 2), &wsaData);
    if (result != 0) {
        std::cerr << "WSAStartup failed\n";
        return false;
    }
    return true;    
}

SOCKET create_socket(){
    SOCKET client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket == INVALID_SOCKET) {
        std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
    }
    return client_socket;
}

sockaddr_in configure_socket(){
    sockaddr_in server{};
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);    
    return server;
}

std::string enter_address(){
    std::string address;
    std::cout << "Enter server's address (e.g: 127.0.0.1):" << std::endl;
    std::getline(std::cin, address);
    return address;
}

bool convert_address(sockaddr_in& server,std::string address){ 
    int result = inet_pton(AF_INET, address.c_str(), &server.sin_addr);
    if (result <= 0) {
        std::cerr << "inet_pton failed: " << result << std::endl;
        return false;
    }
    return true;
}

bool connect_to_server (SOCKET client_socket, sockaddr_in server){
    int result = connect(client_socket, (sockaddr*)&server, sizeof(server));
    if (result == SOCKET_ERROR) {
        std::cerr << "connect failed: " << WSAGetLastError() << std::endl;
        return false;
    }
    else{
        std::cout <<"Connected to server"<<std::endl;
        return true;
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

bool receive_message(SOCKET client_socket){
    char buffer[1024] = {};
    int received_bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    
    if (received_bytes > 0) {
        buffer[received_bytes] = '\0';
        std::string message(buffer);

        if (message == "*Quit") {
            std::cout << "Connection closed by server..." << std::endl;
            return false;
        } 

        std::cout << '\n' << "Received message: " << buffer << std::endl;
        std::cout << "---------------------------------------" << std::endl;
        return true;
    } 
    if (received_bytes == 0) {
        std::cout << "Server disconnected..." << std::endl;
        return false;
    }
    else {
        std::cerr << "Error receiving data" << std::endl;
        return false;
    }
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
    SOCKET server_socket = INVALID_SOCKET;
    SOCKET client_socket = INVALID_SOCKET;

    if (!WinSock_Init()) return 1;
    client_socket = create_socket();
    if (client_socket == INVALID_SOCKET) return 1;
    sockaddr_in address = configure_socket();
    if (!convert_address(address, enter_address())) return 1;
    if (!connect_to_server (client_socket, address)) return 1;

    while (true){
        if (!send_message(client_socket)) break;
        if (!receive_message(client_socket)) break;
    }

close_socket(server_socket, client_socket);
std::cout << "Stopping client..."<< std::endl;
return 0;
}
