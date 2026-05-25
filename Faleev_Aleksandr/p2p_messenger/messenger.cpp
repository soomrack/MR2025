#include <iostream>
#include <string>
#include <thread>
#include <mutex>
#include <chrono>
#include <array>
#include <vector>
#include <algorithm>

#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")
void receiver_loop();
namespace {

struct RoleConfig {
    bool        startAsClient;
    WORD        myColor;
    WORD        peerColor;
    std::string name;
};

struct CommandEntry {
    std::string_view trigger;
    std::string_view emoji;
};

struct EmojiEntry {
    std::string_view token;
    std::string_view emoji;
};

const std::array<EmojiEntry, 12> kEmojiReplacements{ {
    {":)",  "😊"},
    {":-)", "😊"},
    {":(",  "😢"},
    {":-(", "😢"},
    {":D",  "😄"},
    {":-D", "😄"},
    {";)",  "😉"},
    {";-)", "😉"},
    {":P",  "😛"},
    {":-P", "😛"},
    {":o",  "😮"},
    {":'(", "😭"}
} };

const std::array<EmojiEntry, 2> kEmojiReplacementsExtra{ {
    {"<3", "❤️"},
    {"(y)", "👍"}
} };

const std::array<CommandEntry, 7> kEmojiCommands{ {
    {"/smile", "😊"},
    {"/sad",   "😢"},
    {"/laugh", "😂"},
    {"/love",  "❤️"},
    {"/cool",  "😎"},
    {"/wink",  "😉"},
    {"/angry", "😠"}
} };

RoleConfig parseRole(int argc, char* argv[]);

bool         is_client = true;
SOCKET       sock       = INVALID_SOCKET;
bool         running    = true;
WORD         g_myColor  = FOREGROUND_RED | FOREGROUND_INTENSITY;
WORD         g_peerColor= FOREGROUND_GREEN | FOREGROUND_INTENSITY;
std::mutex   mtx;
std::thread  recv_thread;

} // namespace

static std::string replace_emojis(const std::string& text);
static void log_sys(const std::string& msg);
static void log_me(const std::string& msg);
static void log_peer(const std::string& msg);
static bool init_connection(bool as_server);
static void start_receiver();
static void input_loop();
static void cleanup();
static void init_system();

namespace {

void replace_all(std::string& text, std::string_view token, std::string_view emoji) {
    if (token.empty()) return;
    size_t pos = 0;
    while ((pos = text.find(token, pos)) != std::string::npos) {
        text.replace(pos, token.size(), emoji);
        pos += emoji.size();
    }
}

RoleConfig makeRoleConfig(std::string_view role) {
    if (role == "client") {
        return { true, static_cast<WORD>(FOREGROUND_GREEN | FOREGROUND_INTENSITY),
                      static_cast<WORD>(FOREGROUND_RED   | FOREGROUND_INTENSITY),
                      "КЛИЕНТ (пишу)" };
    }
    if (role == "server") {
        return { false, static_cast<WORD>(FOREGROUND_RED   | FOREGROUND_INTENSITY),
                       static_cast<WORD>(FOREGROUND_GREEN | FOREGROUND_INTENSITY),
                       "СЕРВЕР (слушаю)" };
    }
    throw std::runtime_error("Unknown role: " + std::string(role));
}

RoleConfig parseRole(int argc, char* argv[]) {
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--role" || arg == "-r") {
            if (i + 1 >= argc) {
                throw std::runtime_error("Флаг --role требует значения: client или server");
            }
            return makeRoleConfig(argv[i + 1]);
        }
        if (arg.rfind("--role=", 0) == 0) {
            return makeRoleConfig(arg.substr(7));
        }
    }
    throw std::runtime_error("[TypeError] missing 1 required flasg --role (client|server)");
}

} // namespace

static std::string replace_emojis(const std::string& text) {
    std::string result = text;
    for (const auto& entry : kEmojiReplacements) {
        replace_all(result, entry.token, entry.emoji);
    }
    for (const auto& entry : kEmojiReplacementsExtra) {
        replace_all(result, entry.token, entry.emoji);
    }
    replace_all(result, "(n)", "👎");
    return result;
}

static void log_with_color(const std::string& msg, WORD color) {
    std::lock_guard<std::mutex> lock(mtx);
    HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    GetConsoleScreenBufferInfo(hConsole, &csbi);
    SetConsoleTextAttribute(hConsole, color);
    std::cout << msg << "\n";
    SetConsoleTextAttribute(hConsole, csbi.wAttributes);
}

static void log_sys(const std::string& msg) {
    log_with_color(msg, FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
}

static void log_me(const std::string& msg) {
    log_with_color(msg, g_myColor);
}

static void log_peer(const std::string& msg) {
    log_with_color(msg, g_peerColor);
}

static bool init_connection(bool as_server) {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) return false;

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char*>(&opt), sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);

    if (as_server) {
        addr.sin_addr.s_addr = INADDR_ANY;
        if (bind(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) return false;
        if (listen(sock, 1) < 0) return false;
        log_sys("Ожидание подключения...");
        int conn = accept(sock, nullptr, nullptr);
        closesocket(sock);
        sock = conn;
        if (sock == INVALID_SOCKET) return false;
        log_sys("Подключено.");
    } else {
        inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);
        log_sys("Подключение к серверу...");
        if (connect(sock, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0) return false;
        log_sys("Подключено.");
    }
    return true;
}

void switch_role(bool become_client) {
    running = false;
    if (sock != INVALID_SOCKET) shutdown(sock, SD_BOTH);

    if (recv_thread.joinable()) {
        if (std::this_thread::get_id() == recv_thread.get_id()) {
            recv_thread.detach();
        } else {
            recv_thread.join();
        }
    }
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }

    is_client = become_client;
    log_sys("Роль: " + std::string(is_client ? "КЛИЕНТ (пишу)" : "СЕРВЕР (слушаю)"));

    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    if (init_connection(!is_client)) {
        running = true;
        recv_thread = std::thread(receiver_loop);
    } else {
        log_sys("Ошибка сети. Выход.");
        running = false;
    }
}

void receiver_loop() {
    char buf[1024];
    while (running) {
        int bytes = recv(sock, buf, static_cast<int>(sizeof(buf)) - 1, 0);
        if (bytes > 0) {
            buf[bytes] = '\0';
            std::string msg = buf;
            log_peer(msg);
        } else {
            if (running) log_sys("[СИСТЕМА] Связь потеряна.");
            break;
        }
    }
}

static void start_receiver() {
    recv_thread = std::thread(receiver_loop);
}

static void input_loop() {
    std::string input;
    while (running) {
        std::getline(std::cin, input);
        if (input.empty()) continue;

        if (input == "/exit") { running = false; continue; }
        if (input == "/help") {
            log_sys("Смайлики: :) :( :D ;) :P :o :'( <3 (y) (n)");
            continue;
        }

        // emoji-команды и отправка — без проверки is_client
        auto it = std::find_if(kEmojiCommands.begin(), kEmojiCommands.end(),
            [&](const CommandEntry& e){ return input == e.trigger; });
        if (it != kEmojiCommands.end()) {
            const std::string emoji(it->emoji);
            send(sock, emoji.c_str(), static_cast<int>(emoji.size()), 0);
            log_me(emoji);
            continue;
        }

        std::string msg = replace_emojis(input);
        send(sock, msg.c_str(), static_cast<int>(msg.size()), 0);
        log_me(msg);
    }
}

static void cleanup() {
    running = false;
    if (sock != INVALID_SOCKET) shutdown(sock, SD_BOTH);
    if (recv_thread.joinable()) {
        if (std::this_thread::get_id() != recv_thread.get_id()) {
            recv_thread.join();
        }
    }
    if (sock != INVALID_SOCKET) {
        closesocket(sock);
        sock = INVALID_SOCKET;
    }
    WSACleanup();
    log_sys("Работа завершена.");
}

static void init_system() {
    SetConsoleCP(65001);
    SetConsoleOutputCP(65001);
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        std::cerr << "Ошибка WinSock\n";
        std::exit(1);
    }
}

int main(int argc, char* argv[]) {
    try {
        RoleConfig cfg = parseRole(argc, argv);
        is_client = cfg.startAsClient;
        g_myColor = cfg.myColor;
        g_peerColor = cfg.peerColor;
    } catch (const std::exception& ex) {
        std::cerr << ex.what() << "\n"
                  << "usage: messenger --role [client|server]\n";
        return 1;
    }

    init_system();
    if (!init_connection(!is_client)) return 1;
    start_receiver();
    input_loop();
    cleanup();
    return 0;
}
