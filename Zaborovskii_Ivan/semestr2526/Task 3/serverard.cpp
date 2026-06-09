#include <iostream>
#include <string>
#include <cstring>
#include <fstream>
#include <vector>
#include <ctime>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sstream>

#define PORT 5555
#define SERIAL_PORT "/dev/ttyACM0"
#define DRY_THRESHOLD 300  // порог "сухости", подстрой под свой датчик

using namespace std;

int serial_fd;

struct SensorRecord {
    double value;
    time_t timestamp;
};

vector<SensorRecord> history;

bool init_serial() {
    serial_fd = open(SERIAL_PORT, O_RDWR | O_NOCTTY);
    if (serial_fd < 0) {
        cerr << "Failed to open serial port" << endl;
        return false;
    }

    struct termios tty;
    tcgetattr(serial_fd, &tty);
    
    cfsetospeed(&tty, B9600);
    cfsetispeed(&tty, B9600);
    
    tty.c_cflag &= ~PARENB;
    tty.c_cflag &= ~CSTOPB;
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;
    tty.c_cflag &= ~CRTSCTS;
    tty.c_cflag |= CREAD | CLOCAL;
    
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_oflag &= ~OPOST;
    
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10;
    
    tcsetattr(serial_fd, TCSANOW, &tty);
    cout << "Arduino connected" << endl;
    return true;
}

string read_water_level() {
    string line;
    char c;
    while (true) {
        int n = read(serial_fd, &c, 1);
        if (n > 0) {
            if (c == '\n') break;
            if (c != '\r') line += c;
        } else {
            break;
        }
    }
    return line;
}

double parse_value(const string& raw) {
    string value = raw;
    size_t colon = raw.find(':');
    if (colon != string::npos) {
        value = raw.substr(colon + 1);
    }
    while (!value.empty() && value.front() == ' ') value.erase(0, 1);
    while (!value.empty() && value.back() == ' ') value.pop_back();
    
    try {
        return stod(value);
    } catch (...) {
        return -1;
    }
}

string handle_get() {
    string raw = read_water_level();
    if (raw.empty()) return "No data from sensor\n";
    
    double value = parse_value(raw);
    time_t now = time(nullptr);
    
    history.push_back({value, now});
    if (history.size() > 1000) history.erase(history.begin());
    
    stringstream response;
    response << "=== CURRENT WATER LEVEL ===\n";
    response << "Raw: " << raw << "\n";
    response << "Value: " << value << "\n";
    response << "Time: " << ctime(&now);
    
    return response.str();
}

string handle_last_dry() {
    SensorRecord* found = nullptr;
    for (auto it = history.rbegin(); it != history.rend(); ++it) {
        if (it->value < DRY_THRESHOLD && it->value >= 0) {
            found = &(*it);
            break;
        }
    }
    
    if (found) {
        stringstream response;
        response << "=== LAST TIME DRY ===\n";
        response << "Value: " << found->value << "\n";
        response << "Time: " << ctime(&found->timestamp);
        return response.str();
    } else {
        return "No dry records since server started\n";
    }
}

string handle_stream(int client_socket) {
    cout << "Starting stream mode..." << endl;
    
    while (true) {
        string raw = read_water_level();
        if (raw.empty()) {
            usleep(100000);
            continue;
        }
        
        double value = parse_value(raw);
        time_t now = time(nullptr);
        
        history.push_back({value, now});
        if (history.size() > 1000) history.erase(history.begin());
        
        stringstream response;
        response << "Time: " << ctime(&now);
        response << "Value: " << value << " | Raw: " << raw << "\n";
        
        string resp_str = response.str();
        int sent = send(client_socket, resp_str.c_str(), resp_str.length(), 0);
        
        if (sent <= 0) {
            cout << "Stream disconnected" << endl;
            break;
        }
        
        cout << value << endl;  // вывод в консоль сервера
        usleep(500000);  // обновление каждые 500мс
    }
    
    return "";
}

int main() {
    if (!init_serial()) return 1;
    
    int server_fd, client_socket;
    struct sockaddr_in address;
    int opt = 1;
    socklen_t addrlen = sizeof(address);
    char buffer[1024] = {0};

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    bind(server_fd, (struct sockaddr*)&address, sizeof(address));
    listen(server_fd, 3);

    cout << "Water Sensor Server listening on port " << PORT << "..." << endl;
    cout << "Dry threshold: " << DRY_THRESHOLD << endl;

    while (true) {
        client_socket = accept(server_fd, (struct sockaddr*)&address, &addrlen);
        
        memset(buffer, 0, sizeof(buffer));
        read(client_socket, buffer, sizeof(buffer));
        
        string cmd(buffer);
        cmd.erase(cmd.find_last_not_of(" \n\r\t") + 1);
        
        cout << "Command: " << cmd << endl;
        
        string response;
        
        if (cmd == "GET") {
            response = handle_get();
            send(client_socket, response.c_str(), response.length(), 0);
            close(client_socket);
        }
        else if (cmd == "LAST_DRY") {
            response = handle_last_dry();
            send(client_socket, response.c_str(), response.length(), 0);
            close(client_socket);
        }
        else if (cmd == "STREAM") {
            handle_stream(client_socket);
            close(client_socket);
        }
        else {
            response = "Unknown command. Use: GET, LAST_DRY or STREAM\n";
            send(client_socket, response.c_str(), response.length(), 0);
            close(client_socket);
        }
    }

    close(serial_fd);
    return 0;
}