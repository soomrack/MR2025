#include <iostream>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>
#include <limits>
#include <stdio.h>

int server();
int client();
void sensors_DATA(int sock);
void arduino_DATA(int sock);

int main()
{
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

        char buffer[4096] = {};
	char message[4096]= {};

	do
	{
        	memset(buffer, 0, sizeof(buffer));	
		ssize_t bytes = read(client_fd, buffer, sizeof(buffer) -1);

        	if (bytes > 0)
		{       
			buffer[bytes] ='\0';
			std::cout << "\nClient: " << buffer << std::endl;
			std::cout << "\nServer: ";
			std::cin.getline(message, sizeof(message));
			send(client_fd, message, strlen(message), 0);

			if(std::strcmp(buffer, "sensors") == 0)
			{
				std::cout << "Client wonna the Data" << std::endl;
				sensors_DATA(client_fd);
				continue;
			}
			if(std::strcmp(buffer, "arduino") == 0)
			{
				arduino_DATA(client_fd);
				continue;
			}
			}

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

        // string server_ip = getenv("SERVER_IP"); задумка, чтобы не вписывать ip но не знаю как реализовать

        inet_pton(AF_INET, "127.0.0.1", &server.sin_addr);

        if (connect(sock, (sockaddr*)&server, sizeof(server)) < 0)
        {
                perror("connect");
                close(sock);
                return 1;
        }
	
        char message[4096] = {};
	char buffer[4096] = {};

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
			std::cout << "\nServer: " << buffer << std::endl;

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


void sensors_DATA(int sock)
{
	char bash_buffer[128] = {};
	char temperature_buffer[4096] = {};
	temperature_buffer[4096] = '\0';

	FILE* pipe = popen("sensors", "r");
	if (!pipe){

		const char* err = "Error: Device dont have lm-sensor!";

		send(sock, err, strlen(err), 0);
		return;
	     	}

	while (fgets(bash_buffer, sizeof(bash_buffer), pipe) != NULL)
	{
		//if (strlen(temperature_buffer) + strlen(bash_buffer) < sizeof(temperature_buffer) -1)
	//	{
		strncat(temperature_buffer, bash_buffer, sizeof(temperature_buffer) - strlen(temperature_buffer) -1);
	//	}	
	}	
		pclose(pipe);

		send(sock, temperature_buffer, strlen(temperature_buffer), 0);
			
	return;
}

void arduino_DATA(int sock)
{
	char bash_buffer[5] = {};
	char arduino_buffer[16] = {};
	arduino_buffer[16] = '\0';

	FILE* pipe = popen("stty -F /dev/ttyUSB0 9600 raw && head -n 1 /dev/ttyUSB0", "r");
	if(!pipe)
	{
	
		const char* err = "Error: Arduino";

		send(sock, err, strlen(err), 0);
		return;
	}

	while (fgets(bash_buffer, sizeof(bash_buffer), pipe) != NULL) 
		{
		strncat(arduino_buffer, bash_buffer, sizeof(arduino_buffer) - strlen(arduino_buffer) -1);
		}
	pclose(pipe);
	send(sock, arduino_buffer, strlen(arduino_buffer), 0);

	return;
}	

