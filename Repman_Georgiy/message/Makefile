CC = gcc
CFLAGS = -Wall -Wextra -O2

# Определяем ОС
ifeq ($(OS),Windows_NT)
    CFLAGS += -D_WIN32
    LIBS = -lws2_32
    TARGETS = chat_client.exe chat_server.exe
else
    LIBS =
    TARGETS = chat_client chat_server
endif

all: build_folder build/client.exe build/server.exe

build_folder:
	if not exist build mkdir build
build/client.exe: client.c
	$(CC) $(CFLAGS) client.c -o $@ $(LIBS)

build/server.exe: server.c motor.c
	$(CC) $(CFLAGS) server.c motor.c -o $@ $(LIBS)

clean:
	del /Q build\*.exe 2>nul || true