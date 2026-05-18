#include <iostream>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <limits>

int server();
int client();

int main()
{
	char buffer[1024] = {};

	int choise;

	std::cout << "Выбери режим\n" 
		<< "1. Server\n"
		<< "2. Client\n"
		<< "3. Exit\n";

	std::cin >> choise;
	std::cin.get();

	switch(choise)
        {
		case 1:
                	server();
                	break;

		case 2:
                	client();
                	break;

		case 3:
                	return 0;

                	default:

			std::cout << "Error\n";

                	break;

        }
        return 0;
}

int server()
{
	
        int server_fd = socket(AF_INET, SOCK_STREAM, 0);
	
        if (server_fd < 0)
        {
                perror("socket");
                return 1;
        }

        sockaddr_in address{};
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(8080);

        if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0)
        {
                perror("Bind");
                close(server_fd);
                return 1;
        }

        if (listen(server_fd, 1) < 0)
        {
                perror("Listen");
                close(server_fd);        
                return 1;
        }

	std::cout << "Server is listening on port 8080...\n";

        int client_fd = accept(server_fd, nullptr, nullptr); 
	
	if (client_fd < 0)
        {
                perror("accept");
                close(server_fd);
                return 1;
        }

        char buffer[1024] = {};
	char message[1024];

	do
	{
        	memset(buffer, 0, sizeof(buffer) -1);

		ssize_t bytes = read(client_fd, buffer, sizeof(buffer) -1);

        	if (bytes > 0) std::cout << "Client: " << buffer << std::endl;

		std::cout << "\n Server: ";
		std::cin.getline(message, sizeof(message));

		send(client_fd, message, strlen(message), 0);

	} while (std::strcmp(buffer, "by") != 0);

	close(client_fd);
	close(server_fd);
        
	return 0;        

}

int client()
{
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0)
        {
                perror("socket");
                return 1;
        }

        sockaddr_in server{};
        server.sin_family = AF_INET;
        server.sin_port= htons(8080);

        // string server_ip = getenv("SERVER_IP");

        inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

        if (connect(sock, (sockaddr*)&server, sizeof(server)) < 0)
        {
                perror("connect");
                close(sock);
                return 1;
        }
	
        char message[1024];
	char buffer[1024];

	do
	{
		std::cout << "\nClient: ";
		std::cin.getline(message, sizeof(message));

        	send(sock, message, strlen(message), 0);

		if (std::strcmp(message, "by") == 0) break;

		memset(buffer, 0, sizeof(buffer) -1);
		ssize_t bytes = read(sock, buffer, sizeof(buffer) -1);

		if (bytes > 0) 
		{
			buffer[bytes] = '\0';
			std::cout << "Server: " << buffer << std::endl;

			if(std::strcmp(buffer, "by") == 0) break;

		} 
		else if (bytes == 0)
		{
			std::cout << "Error: Server Dissconected!\n";
			break;
		}      
	} while(true);

	close(sock);	
        return 0;
}
