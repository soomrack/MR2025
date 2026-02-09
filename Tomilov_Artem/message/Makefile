# Makefile для чат-мессенджера

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -pedantic
LDFLAGS = 

# Файлы
SERVER_SRC = server.c
CLIENT_SRC = client.c
SERVER_BIN = chat_server
CLIENT_BIN = chat_client

# Цели
all: $(SERVER_BIN) $(CLIENT_BIN)

$(SERVER_BIN): $(SERVER_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(CLIENT_BIN): $(CLIENT_SRC)
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

# Компиляция только сервера
server: $(SERVER_BIN)

# Компиляция только клиента
client: $(CLIENT_BIN)

# Запуск сервера
run-server: $(SERVER_BIN)
	./$(SERVER_BIN)

# Запуск клиента
run-client: $(CLIENT_BIN)
	./$(CLIENT_BIN)

# Очистка
clean:
	rm -f $(SERVER_BIN) $(CLIENT_BIN)

# Очистка и пересборка
rebuild: clean all

.PHONY: all server client run-server run-client clean rebuild