#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define PORT 5000
#define BUF_SIZE 1024
#define MAX_CLIENTS 10
#define NAME_LEN 32

typedef struct {
    int sock;
    char name[NAME_LEN];
    int named;
} Client;


static void server_run(void);
static int  server_socket_create(void);
static void clients_init(Client clients[]);
static void server_loop(int server_fd, Client clients[]);
static void fdset_build(int server_fd, Client clients[], fd_set *set, int *max_sd);
static void accept_client(int server_fd, Client clients[]);
static void handle_clients(Client clients[], fd_set *set);
static void handle_client_message(Client clients[], int i);
static void client_disconnect(Client *c);
static void broadcast(Client clients[], const char *msg, int except);



int main(void) {
    server_run();
    return 0;
}



static void server_run(void) {
    int server_fd = server_socket_create();
    Client clients[MAX_CLIENTS];

    clients_init(clients);

    printf("Chat server started on port %d\n", PORT);
    server_loop(server_fd, clients);
}

static int server_socket_create(void) {
    int fd;
    struct sockaddr_in addr;

    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(PORT);

    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(1);
    }

    if (listen(fd, 5) < 0) {
        perror("listen"); exit(1);
    }

    return fd;
}

static void clients_init(Client clients[]) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].sock = 0;
        clients[i].named = 0;
    }
}

static void server_loop(int server_fd, Client clients[]) {
    fd_set readfds;

    while (1) {
        int max_sd;
        FD_ZERO(&readfds);

        fdset_build(server_fd, clients, &readfds, &max_sd);
        select(max_sd + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(server_fd, &readfds))
            accept_client(server_fd, clients);

        handle_clients(clients, &readfds);
    }
}

static void fdset_build(int server_fd, Client clients[], fd_set *set, int *max_sd) {
    FD_SET(server_fd, set);
    *max_sd = server_fd;

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock > 0) {
            FD_SET(clients[i].sock, set);
            if (clients[i].sock > *max_sd)
                *max_sd = clients[i].sock;
        }
    }
}

static void accept_client(int server_fd, Client clients[]) {
    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);
    int fd = accept(server_fd, (struct sockaddr*)&addr, &len);

    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock == 0) {
            clients[i].sock = fd;
            clients[i].named = 0;
            send(fd, "Welcome!", 17, 0);
            return;
        }
    }

    close(fd);
}

static void handle_clients(Client clients[], fd_set *set) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock > 0 && FD_ISSET(clients[i].sock, set))
            handle_client_message(clients, i);
    }
}

static void handle_client_message(Client clients[], int i) {
    char buffer[BUF_SIZE];
    int n = read(clients[i].sock, buffer, BUF_SIZE - 1);

    if (n <= 0) {
        client_disconnect(&clients[i]);
        return;
    }

    buffer[n] = '\0';

    if (!clients[i].named) {
        strncpy(clients[i].name, buffer, NAME_LEN - 1);
        clients[i].name[strcspn(clients[i].name, "\n")] = '\0';
        clients[i].named = 1;

        char msg[BUF_SIZE];
        snprintf(msg, BUF_SIZE, "[SERVER] %s joined the chat\n", clients[i].name);
        broadcast(clients, msg, -1);
    } else {
        char msg[BUF_SIZE];
        snprintf(msg, BUF_SIZE, "[%s] %s", clients[i].name, buffer);
        broadcast(clients, msg, i);
    }
}

static void client_disconnect(Client *client) {
    printf("Client disconnected: %d\n", client->sock);
    close(client->sock);
    client->sock = 0;
    client->named = 0;
}

static void broadcast(Client clients[], const char *msg, int except) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].sock > 0 && i != except)
            send(clients[i].sock, msg, strlen(msg), 0);
    }
}
