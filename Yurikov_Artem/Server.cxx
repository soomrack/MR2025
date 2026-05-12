#include <iostream>
#include <ws2tcpip.h>
#include <string>
#include <map>
#include <functional>
#include <iomanip>
#include <sstream>
#include <vector>
#include <algorithm>
#include <set>
#include <thread>
#include <atomic>
#include <mutex>
#include <conio.h>

#pragma comment(lib, "Ws2_32.lib")

const unsigned short kServerPort = 8080;

class CommandProcessor {
 public:
  enum class CommandKind {
    kLocal,
    kTerminate
  };

 private:
  std::map<std::string, std::function<bool(SOCKET, const std::string&)>> commands_;
  std::map<std::string, std::string> command_descriptions_;
  std::set<std::string> terminate_commands_;
  mutable std::mutex cout_mutex_;

 public:
  CommandProcessor() {
    RegisterCommand("*Quit", "Disconnect and exit", CommandKind::kTerminate,
      [this](SOCKET socket, const std::string& command) -> bool {
        {
          std::lock_guard<std::mutex> lock(this->cout_mutex_);
          std::cout << "\nClosing connection and exiting..." << std::endl;
        }
        send(socket, command.c_str(), command.length(), 0);
        return false;
      });

    RegisterCommand("*Help", "Show available commands", CommandKind::kLocal,
      [this](SOCKET, const std::string&) -> bool {
        std::lock_guard<std::mutex> lock(this->cout_mutex_);
        std::cout << "\n=== Available commands ===" << std::endl;
        for (const auto& entry : command_descriptions_) {
          std::cout << "  " << entry.first << " - " << entry.second << std::endl;
        }
        std::cout << "==========================\n" << std::endl;
        return true;
      });

    RegisterCommand("*Clear", "Clear screen", CommandKind::kLocal,
      [this](SOCKET, const std::string&) -> bool {
        system("cls");
        {
          std::lock_guard<std::mutex> lock(this->cout_mutex_);
          std::cout << "Screen cleared. Enter *Help for commands.\n" << std::endl;
        }
        return true;
      });
  }

  void RegisterCommand(const std::string& command,
                       const std::string& description,
                       CommandKind kind,
                       std::function<bool(SOCKET, const std::string&)> handler) {
    commands_[command] = handler;
    command_descriptions_[command] = description;
    if (kind == CommandKind::kTerminate) {
      terminate_commands_.insert(command);
    }
  }

  bool HandleCommand(SOCKET socket, const std::string& message, bool& should_exit) {
    if (message.empty()) {
      return true;
    }
    auto iterator = commands_.find(message);
    if (iterator != commands_.end()) {
      bool result = iterator->second(socket, message);
      if (!result) {
        should_exit = true;
      }
      return true;
    }
    return false;
  }

  bool SendToClient(SOCKET socket, const std::string& message) {
    int send_result = send(socket, message.c_str(), message.length(), 0);
    if (send_result == SOCKET_ERROR) {
      std::cerr << "send failed: " << WSAGetLastError() << std::endl;
      return false;
    }
    return true;
  }

  bool IsTerminateCommand(const std::string& message) const {
    return terminate_commands_.find(message) != terminate_commands_.end();
  }

  void SafePrint(const std::string& message, bool new_line = true) {
    std::lock_guard<std::mutex> lock(cout_mutex_);
    if (new_line) {
      std::cout << message << std::endl;
    } else {
      std::cout << message << std::flush;
    }
  }
};

bool WinSockInit() {
  WSADATA wsa_data;
  int startup_result = WSAStartup(MAKEWORD(2, 2), &wsa_data);
  if (startup_result != 0) {
    std::cerr << "WSAStartup failed\n";
    return false;
  }
  return true;
}

SOCKET CreateSocket() {
  SOCKET server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (server_socket == INVALID_SOCKET) {
    std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
  }
  return server_socket;
}

sockaddr_in ConfigureSocket() {
  sockaddr_in socket_address{};
  socket_address.sin_family = AF_INET;
  socket_address.sin_addr.s_addr = INADDR_ANY;
  socket_address.sin_port = htons(kServerPort);
  return socket_address;
}

bool BindSocket(SOCKET server_socket, const sockaddr_in& socket_address) {
  int reuse_option = 1;
  setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR,
             reinterpret_cast<char*>(&reuse_option), sizeof(reuse_option));

  if (bind(server_socket, reinterpret_cast<const sockaddr*>(&socket_address),
           sizeof(socket_address)) == SOCKET_ERROR) {
    std::cerr << "Bind failed: " << WSAGetLastError() << "\n";
    return false;
  }
  return true;
}

bool StartListening(SOCKET server_socket) {
  if (listen(server_socket, SOMAXCONN) == SOCKET_ERROR) {
    std::cerr << "Listen failed: " << WSAGetLastError() << "\n";
    return false;
  }
  return true;
}

SOCKET AcceptConnection(SOCKET server_socket) {
  sockaddr_in client_address;
  int address_length = sizeof(client_address);
  SOCKET client_socket = accept(server_socket,
                                reinterpret_cast<sockaddr*>(&client_address),
                                &address_length);

  if (client_socket == INVALID_SOCKET) {
    std::cerr << "Accept failed: " << WSAGetLastError() << "\n";
  } else {
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_address.sin_addr, client_ip, INET_ADDRSTRLEN);
    std::cout << "Client connected from " << client_ip << ":"
              << ntohs(client_address.sin_port) << std::endl;
  }
  return client_socket;
}

void ReceiveMessages(SOCKET client_socket,
                     std::atomic<bool>& is_running,
                     CommandProcessor& command_processor) {
  char receive_buffer[4096];

  while (is_running) {
    memset(receive_buffer, 0, sizeof(receive_buffer));
    int received_bytes = recv(client_socket, receive_buffer,
                              sizeof(receive_buffer) - 1, 0);

    if (received_bytes > 0) {
      receive_buffer[received_bytes] = '\0';
      std::string incoming_message(receive_buffer);

      if (command_processor.IsTerminateCommand(incoming_message)) {
        command_processor.SafePrint("\n[Client]: " + incoming_message);
        command_processor.SafePrint("\nClient disconnected. Press any key to exit...");
        is_running = false;
        break;
      }

      command_processor.SafePrint("\r\033[K", false);
      command_processor.SafePrint("[Client]: " + incoming_message);
      command_processor.SafePrint("[Server]: ", false);
    } else if (received_bytes == 0) {
      command_processor.SafePrint("\n[System]: Client disconnected");
      command_processor.SafePrint("\nPress any key to exit...");
      is_running = false;
      break;
    } else {
      int error_code = WSAGetLastError();
      if (error_code != WSAECONNRESET && is_running) {
        std::cerr << "Error receiving data: " << error_code << std::endl;
      }
      is_running = false;
      break;
    }
  }
}

void SendMessages(SOCKET client_socket,
                  std::atomic<bool>& is_running,
                  CommandProcessor& command_processor) {
  std::string outgoing_message;
  bool should_exit = false;

  command_processor.SafePrint("\n=== Chat started ===");
  command_processor.SafePrint("Enter *Help for available commands");
  command_processor.SafePrint("[Server]: ", false);

  while (is_running && !should_exit) {
    if (_kbhit()) {
      char input_char = _getch();

      if (input_char == '\r') {
        std::cout << std::endl;

        if (!outgoing_message.empty()) {
          if (command_processor.HandleCommand(client_socket,
                                              outgoing_message,
                                              should_exit)) {
            if (should_exit) {
              is_running = false;
              break;
            }
            outgoing_message.clear();
            if (is_running) {
              command_processor.SafePrint("[Server]: ", false);
            }
            continue;
          }

          if (!command_processor.SendToClient(client_socket, outgoing_message)) {
            command_processor.SafePrint("[Error]: Failed to send message");
            is_running = false;
            break;
          }
          outgoing_message.clear();
        }

        if (is_running) {
          command_processor.SafePrint("[Server]: ", false);
        }
      } else if (input_char == '\b') {
        if (!outgoing_message.empty()) {
          outgoing_message.pop_back();
          std::cout << "\b \b";
        }
      } else {
        outgoing_message += input_char;
        std::cout << input_char;
      }
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
  }
}

void CloseSockets(SOCKET server_socket, SOCKET client_socket) {
  if (client_socket != INVALID_SOCKET) {
    shutdown(client_socket, SD_BOTH);
    closesocket(client_socket);
  }
  if (server_socket != INVALID_SOCKET) {
    closesocket(server_socket);
  }
  WSACleanup();
}

int main() {
  std::cout << "Starting server...\n" << std::endl;
  SOCKET server_socket = INVALID_SOCKET;
  SOCKET client_socket = INVALID_SOCKET;

  if (!WinSockInit()) {
    std::cerr << "Failed to initialize Winsock" << std::endl;
    return 1;
  }

  server_socket = CreateSocket();
  if (server_socket == INVALID_SOCKET) {
    WSACleanup();
    return 1;
  }

  sockaddr_in socket_address = ConfigureSocket();
  if (!BindSocket(server_socket, socket_address)) {
    CloseSockets(server_socket, INVALID_SOCKET);
    return 1;
  }

  if (!StartListening(server_socket)) {
    CloseSockets(server_socket, INVALID_SOCKET);
    return 1;
  }

  std::cout << "Server listening on port " << kServerPort
            << ". Waiting for client...\n" << std::endl;

  client_socket = AcceptConnection(server_socket);
  if (client_socket == INVALID_SOCKET) {
    CloseSockets(server_socket, INVALID_SOCKET);
    return 1;
  }

  CommandProcessor command_processor;
  std::atomic<bool> is_running{true};

  std::thread receiver_thread(ReceiveMessages,
                              client_socket,
                              std::ref(is_running),
                              std::ref(command_processor));

  SendMessages(client_socket, is_running, command_processor);

  if (receiver_thread.joinable()) {
    receiver_thread.join();
  }

  CloseSockets(server_socket, client_socket);
  std::cout << "\nChat server stopped." << std::endl;

  std::cout << "Press any key to exit..." << std::endl;
  _getch();

  return 0;
}