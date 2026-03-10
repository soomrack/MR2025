/*
 * pixhawk_imu_reader.cpp
 *
 * Поток 1 (main): считывает MAVLink RAW_IMU, пишет в SharedBuffer + CSV
 * Поток 2 (viz):  читает SharedBuffer, интегрирует, рендерит в реальном времени
 */

#include <iostream>
#include <fstream>
#include <cstring>
#include <iomanip>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>
#include <thread>

#include <common/mavlink.h>
#include "imu_visualizer.h"

// ─── Настройки ───────────────────────────────────────────────
const char* PIXHAWK_PORT = "/dev/ttyACM0";
const int   BAUDRATE     = B115200;
const char* CSV_FILE     = "imu_log.csv";
bool        LOG_TO_CSV   = true;

// ─── Разделяемый буфер ───────────────────────────────────────
SharedBuffer g_buf;

// ─── Обработчик Ctrl+C ───────────────────────────────────────
void signal_handler(int){ g_buf.running = false; }

// ─── Серийный порт ───────────────────────────────────────────
int setup_serial(const char* port, int baud)
{
    int fd = open(port, O_RDWR | O_NOCTTY);
    if(fd<0){ std::cerr<<"[Reader] Не удалось открыть порт: "<<port<<"\n"; return -1; }
    struct termios tty; memset(&tty,0,sizeof(tty));
    tcgetattr(fd,&tty);
    cfsetospeed(&tty,baud); cfsetispeed(&tty,baud);
    tty.c_cflag |= (CLOCAL|CREAD); tty.c_cflag &= ~CSIZE; tty.c_cflag |= CS8;
    tty.c_cflag &= ~(PARENB|CSTOPB|CRTSCTS);
    tty.c_lflag &= ~(ICANON|ECHO|ECHOE|ISIG);
    tty.c_iflag &= ~(IXON|IXOFF|IXANY);
    tty.c_oflag &= ~OPOST;
    tty.c_cc[VMIN]=0; tty.c_cc[VTIME]=10;
    tcsetattr(fd,TCSANOW,&tty);
    return fd;
}

double get_ts(){
    struct timeval tv; gettimeofday(&tv,nullptr);
    return tv.tv_sec + tv.tv_usec/1e6;
}

// ─── Главный поток: считывание + передача визуализатору ──────
int main()
{
    signal(SIGINT, signal_handler);

    std::cout
        <<"╔══════════════════════════════════════════════╗\n"
        <<"║   Pixhawk IMU — Реальное время               ║\n"
        <<"║   Ctrl+C — остановить и закрыть окно         ║\n"
        <<"╚══════════════════════════════════════════════╝\n";

    // ── Запускаем визуализатор в отдельном потоке
    std::thread viz_thread([](){ run_visualizer(g_buf); });

    int serial_fd = setup_serial(PIXHAWK_PORT, BAUDRATE);
    if(serial_fd < 0){
        g_buf.running = false;
        viz_thread.join();
        return 1;
    }
    std::cout<<"[Reader] Соединение: "<<PIXHAWK_PORT<<"\n";

    std::ofstream csv;
    if(LOG_TO_CSV){
        csv.open(CSV_FILE);
        csv<<"timestamp,AX_raw,AY_raw,AZ_raw,GX_raw,GY_raw,GZ_raw,MX_raw,MY_raw,MZ_raw\n";
        std::cout<<"[Reader] Запись CSV: "<<CSV_FILE<<"\n";
    }

    std::cout<<"[Reader] Ожидание данных IMU...\n\n";

    mavlink_message_t msg;
    mavlink_status_t  status;
    uint8_t buf[2048];
    int count=0;

    while(g_buf.running)
    {
        int n = read(serial_fd, buf, sizeof(buf));
        if(n > 0){
            for(int i=0;i<n;i++){
                if(!mavlink_parse_char(MAVLINK_COMM_0, buf[i], &msg, &status)) continue;
                if(msg.msgid != MAVLINK_MSG_ID_RAW_IMU) continue;

                mavlink_raw_imu_t imu;
                mavlink_msg_raw_imu_decode(&msg,&imu);
                count++;

                double ts = get_ts();

                // ── Отправить в визуализатор
                RawIMU raw;
                raw.timestamp = ts;
                raw.xacc=imu.xacc; raw.yacc=imu.yacc; raw.zacc=imu.zacc;
                raw.xgyro=imu.xgyro; raw.ygyro=imu.ygyro; raw.zgyro=imu.zgyro;
                raw.xmag=imu.xmag;  raw.ymag=imu.ymag;  raw.zmag=imu.zmag;
                g_buf.push(raw);

                // ── Консоль каждые 50 пакетов
                if(count%50==0){
                    NavState nav;
                    { std::lock_guard<std::mutex> lk(g_buf.nav_mtx); nav=g_buf.nav; }
                    std::cout<<std::fixed<<std::setprecision(2)
                        <<"[" <<std::setw(5)<<count<<"]  "
                        <<"Pos("<<std::setw(6)<<nav.px
                        <<","   <<std::setw(6)<<nav.py
                        <<","   <<std::setw(6)<<nav.pz<<") m  "
                        <<"R="  <<std::setw(7)<<nav.roll
                        <<" P=" <<std::setw(7)<<nav.pitch
                        <<" Y=" <<std::setw(7)<<nav.yaw<<"°\n";
                }

                // ── CSV
                if(LOG_TO_CSV && csv.is_open()){
                    csv<<std::fixed<<std::setprecision(6)<<ts<<","
                       <<imu.xacc<<","<<imu.yacc<<","<<imu.zacc<<","
                       <<imu.xgyro<<","<<imu.ygyro<<","<<imu.zgyro<<","
                       <<imu.xmag<<","<<imu.ymag<<","<<imu.zmag<<"\n";
                    if(count%50==0) csv.flush();
                }
            }
        } else if(n<0){
            std::cerr<<"[Reader] Ошибка чтения\n";
            break;
        }
    }

    g_buf.running = false;
    if(csv.is_open()){ csv.flush(); csv.close(); }
    close(serial_fd);

    std::cout<<"\n[Reader] Завершено. Пакетов: "<<count<<"\n";
    viz_thread.join();
    return 0;
}