#include <iostream>
#include <fstream>
#include <cstring>
#include <ctime>
#include <iomanip>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>

// MAVLink заголовки
#include <common/mavlink.h>

// ---------- НАСТРОЙКИ ----------
const char* PIXHAWK_PORT = "/dev/ttyACM0";
const int BAUDRATE = B115200;
const char* CSV_FILE = "imu_log.csv";
bool LOG_TO_CSV = true;
// --------------------------------

// Глобальные переменные для корректного завершения
volatile bool running = true;
std::ofstream csv_file;

// Обработчик сигнала Ctrl+C
void signal_handler(int signum) {
    std::cout << "\nStopping..." << std::endl;
    running = false;
}

// Функция для настройки последовательного порта
int setup_serial_port(const char* port, int baudrate) {
    int fd = open(port, O_RDWR | O_NOCTTY);
    
    if (fd < 0) {
        std::cerr << "Error opening port: " << port << std::endl;
        return -1;
    }
    
    struct termios tty;
    memset(&tty, 0, sizeof(tty));
    
    if (tcgetattr(fd, &tty) != 0) {
        std::cerr << "Error getting port attributes" << std::endl;
        close(fd);
        return -1;
    }
    
    // Настройка параметров порта
    cfsetospeed(&tty, baudrate);
    cfsetispeed(&tty, baudrate);
    
    tty.c_cflag |= (CLOCAL | CREAD);    // Включить приёмник
    tty.c_cflag &= ~CSIZE;
    tty.c_cflag |= CS8;                 // 8 бит данных
    tty.c_cflag &= ~PARENB;             // Без паритета
    tty.c_cflag &= ~CSTOPB;             // 1 стоп-бит
    tty.c_cflag &= ~CRTSCTS;            // Без аппаратного контроля потока
    
    tty.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    tty.c_iflag &= ~(IXON | IXOFF | IXANY);
    tty.c_oflag &= ~OPOST;
    
    tty.c_cc[VMIN] = 0;
    tty.c_cc[VTIME] = 10;  // Таймаут 1 секунда
    
    if (tcsetattr(fd, TCSANOW, &tty) != 0) {
        std::cerr << "Error setting port attributes" << std::endl;
        close(fd);
        return -1;
    }
    
    return fd;
}

// Получить текущее время в секундах с микросекундами
double get_timestamp() {
    struct timeval tv;
    gettimeofday(&tv, nullptr);
    return tv.tv_sec + tv.tv_usec / 1000000.0;
}

// Обработка полученного MAVLink сообщения
void handle_message(mavlink_message_t* msg, int& message_count) {
    if (msg->msgid == MAVLINK_MSG_ID_RAW_IMU) {
        mavlink_raw_imu_t imu;
        mavlink_msg_raw_imu_decode(msg, &imu);
        
        message_count++;
        double ts = get_timestamp();
        
        // Вывод в консоль каждое 10-е сообщение
        if (message_count % 10 == 0) {
            std::cout << "[" << message_count << "] "
                      << "ACC: X=" << std::setw(6) << imu.xacc
                      << " Y=" << std::setw(6) << imu.yacc
                      << " Z=" << std::setw(6) << imu.zacc
                      << " | GYRO: X=" << std::setw(6) << imu.xgyro
                      << " Y=" << std::setw(6) << imu.ygyro
                      << " Z=" << std::setw(6) << imu.zgyro
                      << " | MAG: X=" << std::setw(6) << imu.xmag
                      << " Y=" << std::setw(6) << imu.ymag
                      << " Z=" << std::setw(6) << imu.zmag
                      << std::endl;
        }
        
        // Запись в CSV
        if (LOG_TO_CSV && csv_file.is_open()) {
            csv_file << std::fixed << std::setprecision(6) << ts << ","
                     << imu.xacc << "," << imu.yacc << "," << imu.zacc << ","
                     << imu.xgyro << "," << imu.ygyro << "," << imu.zgyro << ","
                     << imu.xmag << "," << imu.ymag << "," << imu.zmag
                     << std::endl;
            
            // Принудительная запись каждые 50 сообщений
            if (message_count % 50 == 0) {
                csv_file.flush();
            }
        }
    }
}

int main() {
    // Установка обработчика сигналов
    signal(SIGINT, signal_handler);
    
    std::cout << "Connecting to Pixhawk..." << std::endl;
    
    // Открываем последовательный порт
    int serial_fd = setup_serial_port(PIXHAWK_PORT, BAUDRATE);
    if (serial_fd < 0) {
        return 1;
    }
    
    std::cout << "Connected to " << PIXHAWK_PORT << std::endl;
    
    // Открываем CSV файл
    if (LOG_TO_CSV) {
        csv_file.open(CSV_FILE);
        if (!csv_file.is_open()) {
            std::cerr << "Error opening CSV file" << std::endl;
            close(serial_fd);
            return 1;
        }
        
        // Записываем заголовок
        csv_file << "timestamp,AX_raw,AY_raw,AZ_raw,GX_raw,GY_raw,GZ_raw,MX_raw,MY_raw,MZ_raw" << std::endl;
    }
    
    std::cout << "Listening for RAW_IMU messages..." << std::endl;
    
    mavlink_message_t msg;
    mavlink_status_t status;
    uint8_t buffer[2048];
    int message_count = 0;
    
    // Основной цикл чтения
    while (running) {
        int n = read(serial_fd, buffer, sizeof(buffer));
        
        if (n > 0) {
            // Разбираем полученные байты через MAVLink парсер
            for (int i = 0; i < n; i++) {
                if (mavlink_parse_char(MAVLINK_COMM_0, buffer[i], &msg, &status)) {
                    // Получено полное сообщение
                    handle_message(&msg, message_count);
                }
            }
        } else if (n < 0) {
            std::cerr << "Error reading from serial port" << std::endl;
            break;
        }
    }
    
    // Закрываем ресурсы
    if (csv_file.is_open()) {
        csv_file.close();
    }
    close(serial_fd);
    
    std::cout << "Program terminated. Total messages: " << message_count << std::endl;
    std::cout << "Data saved to " << CSV_FILE << std::endl;
    
    return 0;
}