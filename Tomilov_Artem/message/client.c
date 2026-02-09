#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 5000
#define BUF_SIZE 1024

static void client_run(void);
static int connect_to_server(void);
static void chat_loop(int sockfd);

int main(void) {
    client_run();
    return 0;
}

static void client_run(void) {
    int sockfd = connect_to_server();
    printf("Connected to chat. \n");
    printf("Enter your name: \n");
    chat_loop(sockfd);
}

static int connect_to_server(void) {
    int sockfd;
    struct sockaddr_in server_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        exit(1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    // укажи IP сервера в локальной сети (например "192.168.1.10")
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("inet_pton"); 
        exit(1);
    }

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect"); 
        exit(1);
    }
    return sockfd; 
}


static void chat_loop(int sockfd) {

    fd_set readfds;
    char buffer[BUF_SIZE];

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        int max_sd = sockfd > STDIN_FILENO ? sockfd : STDIN_FILENO;
        select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if (FD_ISSET(sockfd, &readfds)) {
            int n = read(sockfd, buffer, BUF_SIZE);
            if (n <= 0) {
                printf("Disconnected from server\n");
                close(sockfd);
                exit(0);
            }
            buffer[n] = '\0';
            printf("%s", buffer);
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            fgets(buffer, BUF_SIZE, stdin);
            send(sockfd, buffer, strlen(buffer), 0);
        }
    }

}
