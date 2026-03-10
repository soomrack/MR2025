#pragma once
#include <mutex>
#include <deque>
#include <atomic>
#include <cmath>
#include <array>

namespace Calib {
    constexpr double AX_BIAS     =  31.0;
    constexpr double AY_BIAS     =  24.0;
    constexpr double AZ_BIAS     =   0.0;
    constexpr double ACCEL_SCALE =  9.81 / 3050.0;
    constexpr double GX_BIAS     = -7.0;
    constexpr double GY_BIAS     =  1.5;
    constexpr double GZ_BIAS     =  3.0;
    constexpr double GYRO_SCALE  =  0.001;
    constexpr double ZUPT_GYRO_THRESH = 0.015;
}

struct RawIMU {
    double timestamp;
    int16_t xacc,yacc,zacc;
    int16_t xgyro,ygyro,zgyro;
    int16_t xmag,ymag,zmag;
};

struct NavState {
    double qw=1,qx=0,qy=0,qz=0;
    double wx=0,wy=0,wz=0;
    double vx=0,vy=0,vz=0;
    double px=0,py=0,pz=0;
    double roll=0,pitch=0,yaw=0;
    double ax=0,ay=0,az=0;
};

struct SharedBuffer {
    std::mutex           mtx;
    std::deque<RawIMU>   queue;
    static const int     MAX_QUEUE = 500;
    std::atomic<bool>    running{true};
    std::mutex           nav_mtx;
    NavState             nav;
    std::mutex           trail_mtx;
    std::deque<std::array<float,3>> trail;
    static const int     MAX_TRAIL = 2000;

    void push(const RawIMU& r){
        std::lock_guard<std::mutex> lk(mtx);
        if((int)queue.size()<MAX_QUEUE) queue.push_back(r);
    }
    bool pop(RawIMU& out){
        std::lock_guard<std::mutex> lk(mtx);
        if(queue.empty()) return false;
        out=queue.front(); queue.pop_front(); return true;
    }
    void add_trail_point(float x,float y,float z){
        std::lock_guard<std::mutex> lk(trail_mtx);
        trail.push_back({x,y,z});
        if((int)trail.size()>MAX_TRAIL) trail.pop_front();
    }
};