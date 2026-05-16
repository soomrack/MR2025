#include <iostream>
#include <winsock2.h>
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
 private:
  std::map<std::string, std::function<bool(SOCKET, const std::string&)>> commands_;
  std::map<std::string, std::string> command_descriptions_;
  std::set<std::string> terminate_commands_;
  mutable std::mutex cout_mutex_;

 public:
  enum class CommandKind {
    kLocal,
    kTerminate
  };

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

  bool SendToServer(SOCKET socket, const std::string& message) {
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
    std::cerr << "WSAStartup failed: " << startup_result << std::endl;
    return false;
  }
  return true;
}

SOCKET CreateSocket() {
  SOCKET client_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (client_socket == INVALID_SOCKET) {
    std::cerr << "Socket creation failed: " << WSAGetLastError() << std::endl;
  }
  return client_socket;
}

sockaddr_in ConfigureSocket() {
  sockaddr_in server_address{};
  server_address.sin_family = AF_INET;
  server_address.sin_port = htons(kServerPort);
  return server_address;
}

std::string EnterAddress() {
  std::string server_address;
  std::cout << "Enter server's address (e.g: 127.0.0.1):" << std::endl;
  std::cout << "> ";
  std::getline(std::cin, server_address);
  return server_address;
}

bool ConvertAddress(sockaddr_in& server_address, std::string address_text) {
  server_address.sin_addr.s_addr = inet_addr(address_text.c_str());
  if (server_address.sin_addr.s_addr == INADDR_NONE) {
    std::cerr << "Invalid address format: " << address_text << std::endl;
    return false;
  }
  return true;
}

bool ConnectToServer(SOCKET client_socket, sockaddr_in server_address) {
  std::cout << "Attempting to connect to "
            << inet_ntoa(server_address.sin_addr) << ":"
            << ntohs(server_address.sin_port) << "..." << std::endl;

  int connect_result = connect(client_socket,
                               reinterpret_cast<sockaddr*>(&server_address),
                               sizeof(server_address));
  if (connect_result == SOCKET_ERROR) {
    int error_code = WSAGetLastError();
    std::cerr << "Connect failed. Error code: " << error_code << std::endl;
    return false;
  } else {
    std::cout << "Successfully connected to server!" << std::endl;
    return true;
  }
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
        command_processor.SafePrint("\n[Server]: " + incoming_message);
        command_processor.SafePrint("\nServer disconnected. Press any key to exit...");
        is_running = false;
        break;
      }

      command_processor.SafePrint("\r\033[K", false);
      command_processor.SafePrint("[Server]: " + incoming_message);
      command_processor.SafePrint("[You]: ", false);
    } else if (received_bytes == 0) {
      command_processor.SafePrint("\n[System]: Server disconnected");
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
  command_processor.SafePrint("[You]: ", false);

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
              command_processor.SafePrint("[You]: ", false);
            }
            continue;
          }

          if (!command_processor.SendToServer(client_socket, outgoing_message)) {
            command_processor.SafePrint("[Error]: Failed to send message");
            is_running = false;
            break;
          }
          outgoing_message.clear();
        }

        if (is_running) {
          command_processor.SafePrint("[You]: ", false);
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

void CloseSocket(SOCKET client_socket) {
  if (client_socket != INVALID_SOCKET) {
    shutdown(client_socket, SD_BOTH);
    closesocket(client_socket);
  }
  WSACleanup();
}

int main() {
  SOCKET client_socket = INVALID_SOCKET;

  std::cout << "Starting client..." << std::endl;

  if (!WinSockInit()) {
    std::cerr << "Failed to initialize Winsock" << std::endl;
    return 1;
  }

  client_socket = CreateSocket();
  if (client_socket == INVALID_SOCKET) {
    WSACleanup();
    return 1;
  }

  sockaddr_in server_address = ConfigureSocket();
  std::string server_ip = EnterAddress();

  if (!ConvertAddress(server_address, server_ip)) {
    CloseSocket(client_socket);
    return 1;
  }

  if (!ConnectToServer(client_socket, server_address)) {
    CloseSocket(client_socket);
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

  CloseSocket(client_socket);
  std::cout << "\nClient stopped." << std::endl;

  std::cout << "Press any key to exit..." << std::endl;
  _getch();

  return 0;
}