#include <iostream>
#include <thread>
#include <atomic>
#include <cstring>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <opencv2/opencv.hpp>

const char* SERVER_IP = "10.254.241.246";   // IP вашей Raspberry Pi
const int SERVER_PORT = 8080;

std::atomic<bool> running(true);
int sock = -1;

void sendCmd(const std::string& cmd) {
    if (sock >= 0) {
        send(sock, cmd.c_str(), cmd.size(), 0);
    }
}

void videoReceiver() {
    cv::namedWindow("Robot Control", cv::WINDOW_AUTOSIZE);
    while (running) {
        uint32_t frameSize = 0;
        int ret = recv(sock, &frameSize, sizeof(frameSize), MSG_WAITALL);
        if (ret <= 0) break;
        std::vector<uchar> buffer(frameSize);
        ret = recv(sock, buffer.data(), frameSize, MSG_WAITALL);
        if (ret <= 0) break;
        cv::Mat frame = cv::imdecode(buffer, cv::IMREAD_COLOR);
        if (!frame.empty()) {
            cv::putText(frame, "Arrows: move | W/S: laser servo | A/D: cam servo | Q/E: laser | SPACE: stop",
                        cv::Point(10, 20), cv::FONT_HERSHEY_SIMPLEX, 0.45, cv::Scalar(0,255,0), 1);
            cv::imshow("Robot Control", frame);
        }
        if (cv::waitKey(1) == 27) break; // ESC
    }
    cv::destroyAllWindows();
    running = false;
}

int main() {
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Socket creation failed" << std::endl;
        return 1;
    }
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr) <= 0) {
        std::cerr << "Invalid address" << std::endl;
        return 1;
    }
    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        std::cerr << "Connection to RPi failed" << std::endl;
        return 1;
    }
    std::cout << "Connected to RPi server" << std::endl;

    std::thread videoThread(videoReceiver);

    int camAngle = 90, laserAngle = 90;

    while (running) {
        char key = cv::waitKey(50);
        if (key == 27) break;   // ESC

        // Стрелки (движение)
        if (key == 2490368 || key == 182) {          // вверх
            sendCmd("M[250,250]");
        } else if (key == 2621440 || key == 183) {   // вниз
            sendCmd("M[-250,-250]");
        } else if (key == 2424832 || key == 180) {   // влево
            sendCmd("M[-250,250]");
        } else if (key == 2555904 || key == 181) {   // вправо
            sendCmd("M[250,-250]");
        }
        // WASD – шаг 5° для серв
        else if (key == 'w' || key == 'W') {
            laserAngle = std::min(180, laserAngle + 5);
            sendCmd("L[" + std::to_string(laserAngle) + "]");
        } else if (key == 's' || key == 'S') {
            laserAngle = std::max(0, laserAngle - 5);
            sendCmd("L[" + std::to_string(laserAngle) + "]");
        } else if (key == 'a' || key == 'A') {
            camAngle = std::max(0, camAngle - 5);
            sendCmd("C[" + std::to_string(camAngle) + "]");
        } else if (key == 'd' || key == 'D') {
            camAngle = std::min(180, camAngle + 5);
            sendCmd("C[" + std::to_string(camAngle) + "]");
        }
        // Лазер
        else if (key == 'q' || key == 'Q') {
            sendCmd("LASER[1]");
        } else if (key == 'e' || key == 'E') {
            sendCmd("LASER[0]");
        }
        // Аварийная остановка
        else if (key == ' ') {
            sendCmd("STOP");
        }
    }

    running = false;
    videoThread.join();
    close(sock);
    return 0;
}