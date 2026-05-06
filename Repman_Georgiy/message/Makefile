# ============================================================================
# Makefile для Chat MAX 228.0 (Linux / Windows / кросс-компиляция aarch64)
# ============================================================================

CC = gcc
CFLAGS = -Wall -Wextra -O2 -g

# Определяем операционную систему
ifeq ($(OS),Windows_NT)
    # Windows
    LIBS = -lws2_32
    MKDIR = if not exist build mkdir build
    RM = del /Q
    EXE = .exe
else
    # Linux / Unix
    LIBS =
    MKDIR = mkdir -p build
    RM = rm -f
    EXE =
endif

# Поддержка кросс-компиляции для aarch64
ifneq ($(CROSS_COMPILE),)
    CC = $(CROSS_COMPILE)gcc
endif

TARGET_SERVER = chat_server$(EXE)
TARGET_CLIENT = chat_client$(EXE)

BUILD_DIR = build

all: $(BUILD_DIR) $(BUILD_DIR)/$(TARGET_SERVER) $(BUILD_DIR)/$(TARGET_CLIENT)

$(BUILD_DIR):
	$(MKDIR)

$(BUILD_DIR)/$(TARGET_SERVER): server.c motor.c $(BUILD_DIR)
	$(CC) $(CFLAGS) server.c motor.c -o $@ $(LIBS)

$(BUILD_DIR)/$(TARGET_CLIENT): client.c $(BUILD_DIR)
	$(CC) $(CFLAGS) client.c -o $@ $(LIBS)

clean:
	$(RM) $(BUILD_DIR)/$(TARGET_SERVER) $(BUILD_DIR)/$(TARGET_CLIENT)
	# Удалить папку build в Linux (опционально)
	-rmdir build 2>/dev/null || true

.PHONY: all clean