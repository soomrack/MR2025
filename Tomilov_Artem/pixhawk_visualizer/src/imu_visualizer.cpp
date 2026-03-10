/*
 * imu_visualizer.cpp
 *
 * Визуализация инерциальной навигации Pixhawk в РЕАЛЬНОМ ВРЕМЕНИ.
 *
 * Поток визуализации:
 *   1. Читает RawIMU пакеты из SharedBuffer::queue
 *   2. Интегрирует кватернион ориентации (гироскоп)
 *   3. Поворачивает акселерометр в мировую СК, вычитает g
 *   4. Двойное интегрирование → скорость и позиция
 *   5. ZUPT: если ω < порога → обнуляем скорость (уменьшаем дрейф)
 *   6. Рендерит: 3D модель FC на текущей позиции + траектория + графики
 *
 * Зависимости: GLFW3, GLEW, GLM, OpenGL 3.3
 */

#include "imu_visualizer.h"

#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>
#include <deque>
#include <thread>
#include <chrono>
#include <string>
#include <functional>

// ═══════════════════════════════════════════════════════════════
//  1. ИНЕРЦИАЛЬНАЯ НАВИГАЦИЯ (выполняется в потоке визуализации)
// ═══════════════════════════════════════════════════════════════

static void integrate_imu(SharedBuffer& buf, const RawIMU& raw, double dt)
{
    if (dt <= 0 || dt > 0.5) return;

    // ── Калибровка
    double ax = (raw.xacc  - Calib::AX_BIAS)  * Calib::ACCEL_SCALE;
    double ay = (raw.yacc  - Calib::AY_BIAS)  * Calib::ACCEL_SCALE;
    double az = (raw.zacc  - Calib::AZ_BIAS)  * Calib::ACCEL_SCALE;
    double gx = (raw.xgyro - Calib::GX_BIAS)  * Calib::GYRO_SCALE;
    double gy = (raw.ygyro - Calib::GY_BIAS)  * Calib::GYRO_SCALE;
    double gz = (raw.zgyro - Calib::GZ_BIAS)  * Calib::GYRO_SCALE;

    std::lock_guard<std::mutex> lk(buf.nav_mtx);
    NavState& n = buf.nav;

    n.ax = ax; n.ay = ay; n.az = az;
    n.wx = gx; n.wy = gy; n.wz = gz;

    // ── Интегрирование кватерниона (ось-угол)
    //
    //   θ  = ‖ω‖ · dt
    //   dq = ( cos(θ/2),  ω/‖ω‖ · sin(θ/2) )
    //   q  = q ⊗ dq  →  нормализация
    //
    double theta = std::sqrt(gx*gx + gy*gy + gz*gz) * dt;
    if (theta > 1e-10) {
        double norm_w = std::sqrt(gx*gx + gy*gy + gz*gz);
        double s  = std::sin(theta * 0.5) / norm_w;
        double c  = std::cos(theta * 0.5);
        double dqw =  c;
        double dqx = gx * s;
        double dqy = gy * s;
        double dqz = gz * s;

        // q_new = q ⊗ dq
        double qw = n.qw*dqw - n.qx*dqx - n.qy*dqy - n.qz*dqz;
        double qx = n.qw*dqx + n.qx*dqw + n.qy*dqz - n.qz*dqy;
        double qy = n.qw*dqy - n.qx*dqz + n.qy*dqw + n.qz*dqx;
        double qz = n.qw*dqz + n.qx*dqy - n.qy*dqx + n.qz*dqw;

        double norm = std::sqrt(qw*qw + qx*qx + qy*qy + qz*qz);
        n.qw = qw/norm; n.qx = qx/norm;
        n.qy = qy/norm; n.qz = qz/norm;
    }

    // ── Матрица поворота из кватерниона
    //
    //   Формула Родрига для вращения вектора из СК тела в мировую:
    //   a_world = R · a_body
    //
    double qw=n.qw, qx=n.qx, qy=n.qy, qz=n.qz;
    double r00 = 1-2*(qy*qy+qz*qz),  r01 = 2*(qx*qy-qw*qz), r02 = 2*(qx*qz+qw*qy);
    double r10 = 2*(qx*qy+qw*qz),    r11 = 1-2*(qx*qx+qz*qz),r12 = 2*(qy*qz-qw*qx);
    double r20 = 2*(qx*qz-qw*qy),    r21 = 2*(qy*qz+qw*qx),  r22 = 1-2*(qx*qx+qy*qy);

    // Ускорение в мировой СК
    double ax_w = r00*ax + r01*ay + r02*az;
    double ay_w = r10*ax + r11*ay + r12*az;
    double az_w = r20*ax + r21*ay + r22*az;

    // Вычитаем гравитацию (g направлена вниз по Z мировой СК)
    // В горизонтальном покое: az_w ≈ -9.81 (платой Z вниз = -g)
    az_w -= (-9.81);

    // ── ZUPT — Zero Velocity Update
    //   Если угловая скорость мала → скорее всего статика → обнуляем скорость
    //   Это сильно уменьшает дрейф позиции при длительном покое
    double gyro_norm = std::sqrt(gx*gx + gy*gy + gz*gz);
    if (gyro_norm < Calib::ZUPT_GYRO_THRESH) {
        // Мягкое затухание скорости (не резкое обнуление)
        n.vx *= 0.85;
        n.vy *= 0.85;
        n.vz *= 0.85;
        // Малые ускорения в статике — шум, игнорируем
        if (std::abs(ax_w) < 0.3) ax_w = 0;
        if (std::abs(ay_w) < 0.3) ay_w = 0;
        if (std::abs(az_w) < 0.3) az_w = 0;
    }

    // ── Интегрирование скорости (1-й интеграл)
    n.vx += ax_w * dt;
    n.vy += ay_w * dt;
    n.vz += az_w * dt;

    // ── Интегрирование позиции (2-й интеграл)
    n.px += n.vx * dt;
    n.py += n.vy * dt;
    n.pz += n.vz * dt;

    // ── Углы Эйлера из кватерниона (конвенция ZYX)
    const double R2D = 180.0 / M_PI;
    n.roll  = std::atan2(2*(qw*qx + qy*qz), 1-2*(qx*qx + qy*qy)) * R2D;
    double sp = std::max(-1.0, std::min(1.0, 2*(qw*qy - qz*qx)));
    n.pitch = std::asin(sp) * R2D;
    n.yaw   = std::atan2(2*(qw*qz + qx*qy), 1-2*(qy*qy + qz*qz)) * R2D;
}

// ═══════════════════════════════════════════════════════════════
//  2. ШЕЙДЕРЫ
// ═══════════════════════════════════════════════════════════════

static const char* VERT3D = R"glsl(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aColor;
layout(location=2) in vec3 aNormal;
out vec3 vColor; out vec3 vNormal; out vec3 vPos;
uniform mat4 uMVP; uniform mat4 uModel;
void main(){
    vPos    = vec3(uModel * vec4(aPos,1));
    vNormal = mat3(transpose(inverse(uModel))) * aNormal;
    vColor  = aColor;
    gl_Position = uMVP * vec4(aPos,1);
})glsl";

static const char* FRAG3D = R"glsl(
#version 330 core
in vec3 vColor; in vec3 vNormal; in vec3 vPos;
out vec4 outColor;
uniform vec3 uLight; uniform float uAlpha;
void main(){
    float d = max(dot(normalize(vNormal), normalize(uLight)), 0.0);
    outColor = vec4(vColor * (0.35 + 0.65*d), uAlpha);
})glsl";

// Шейдер для 2D HUD и 3D линий (без освещения)
static const char* VERT_FLAT = R"glsl(
#version 330 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aColor;
out vec3 vColor;
uniform mat4 uMVP;
void main(){ vColor = aColor; gl_Position = uMVP * vec4(aPos,1); })glsl";

static const char* FRAG_FLAT = R"glsl(
#version 330 core
in vec3 vColor; out vec4 outColor;
uniform float uAlpha;
void main(){ outColor = vec4(vColor, uAlpha); })glsl";

// ═══════════════════════════════════════════════════════════════
//  3. ГЕОМЕТРИЯ
// ═══════════════════════════════════════════════════════════════

struct V3 { float x,y,z, r,g,b, nx,ny,nz; };
struct VF { float x,y,z, r,g,b; };  // flat (линии, HUD, траектория)

static void box_face(std::vector<V3>& v,
    float x0,float y0,float z0, float x1,float y1,float z1,
    float x2,float y2,float z2, float x3,float y3,float z3,
    float r,float g,float b, float nx,float ny,float nz)
{
    V3 a={x0,y0,z0,r,g,b,nx,ny,nz}, b_={x1,y1,z1,r,g,b,nx,ny,nz};
    V3 c={x2,y2,z2,r,g,b,nx,ny,nz}, d={x3,y3,z3,r,g,b,nx,ny,nz};
    v.push_back(a); v.push_back(b_); v.push_back(c);
    v.push_back(a); v.push_back(c);  v.push_back(d);
}

static void push_box(std::vector<V3>& v,
    float cx,float cy,float cz, float sx,float sy,float sz,
    float r,float g,float b)
{
    float hx=sx/2,hy=sy/2,hz=sz/2;
    float x0=cx-hx,x1=cx+hx,y0=cy-hy,y1=cy+hy,z0=cz-hz,z1=cz+hz;
    box_face(v,x1,y0,z0,x1,y1,z0,x1,y1,z1,x1,y0,z1,r*.9f,g*.9f,b*.9f,1,0,0);
    box_face(v,x0,y0,z1,x0,y1,z1,x0,y1,z0,x0,y0,z0,r*.7f,g*.7f,b*.7f,-1,0,0);
    box_face(v,x0,y1,z0,x1,y1,z0,x1,y1,z1,x0,y1,z1,r,g,b,0,1,0);
    box_face(v,x0,y0,z1,x1,y0,z1,x1,y0,z0,x0,y0,z0,r*.6f,g*.6f,b*.6f,0,-1,0);
    box_face(v,x0,y0,z1,x1,y0,z1,x1,y1,z1,x0,y1,z1,r,g,b,0,0,1);
    box_face(v,x0,y1,z0,x1,y1,z0,x1,y0,z0,x0,y0,z0,r*.5f,g*.5f,b*.5f,0,0,-1);
}

static void push_arrow(std::vector<V3>& v,
    float ox,float oy,float oz,
    float dx,float dy,float dz,
    float len, float r,float g,float b)
{
    float t=0.03f, ht=0.10f;
    float ex=ox+dx*len, ey=oy+dy*len, ez=oz+dz*len;
    if (dx!=0) { push_box(v,(ox+ex)/2,oy,oz,len-ht,t,t,r,g,b); push_box(v,ex,ey,ez,ht,t*2.5f,t*2.5f,r,g,b); }
    if (dy!=0) { push_box(v,ox,(oy+ey)/2,oz,t,len-ht,t,r,g,b); push_box(v,ex,ey,ez,t*2.5f,ht,t*2.5f,r,g,b); }
    if (dz!=0) { push_box(v,ox,oy,(oz+ez)/2,t,t,len-ht,r,g,b); push_box(v,ex,ey,ez,t*2.5f,t*2.5f,ht,r,g,b); }
}

// Минималистичная модель FC
static std::vector<V3> build_fc()
{
    std::vector<V3> v;
    // Корпус — тёмно-серый прямоугольник
    push_box(v, 0,0,0,   0.8f,0.8f,0.14f, 0.13f,0.14f,0.19f);
    // Полоска «нос» (направление Y+)
    push_box(v, 0,0.37f,0.08f, 0.7f,0.06f,0.04f, 0.95f,0.95f,0.95f);
    // Рёбра для объёма
    push_box(v, 0.37f,0,0.08f, 0.04f,0.8f,0.04f, 0.22f,0.22f,0.28f);
    push_box(v,-0.37f,0,0.08f, 0.04f,0.8f,0.04f, 0.22f,0.22f,0.28f);
    // Оси X=красный, Y=зелёный, Z=синий
    push_arrow(v,0,0,0.1f, 1,0,0, 0.7f, 0.95f,0.10f,0.10f);
    push_arrow(v,0,0,0.1f, 0,1,0, 0.7f, 0.10f,0.92f,0.10f);
    push_arrow(v,0,0,0.07f,0,0,1, 0.7f, 0.15f,0.42f,0.98f);
    return v;
}

// Мировые оси — крупные, на сцене
static std::vector<VF> build_world_axes()
{
    std::vector<VF> v;
    float L=2.0f;
    v.push_back({0,0,0, 0.8f,0.15f,0.15f}); v.push_back({L,0,0, 0.8f,0.15f,0.15f}); // X красный
    v.push_back({0,0,0, 0.15f,0.8f,0.15f}); v.push_back({0,L,0, 0.15f,0.8f,0.15f}); // Y зелёный
    v.push_back({0,0,0, 0.15f,0.35f,0.9f}); v.push_back({0,0,L, 0.15f,0.35f,0.9f}); // Z синий
    return v;
}

// Сетка в плоскости XY
static std::vector<VF> build_grid()
{
    std::vector<VF> v;
    float gc=0.18f, step=0.5f;
    int N=10;
    for (int i=-N;i<=N;i++){
        float f=i*step;
        v.push_back({f,   (float)(-N*step),0,gc,gc,gc});
        v.push_back({f,   (float)( N*step),0,gc,gc,gc});
        v.push_back({(float)(-N*step),f,   0,gc,gc,gc});
        v.push_back({(float)( N*step),f,   0,gc,gc,gc});
    }
    return v;
}

// ═══════════════════════════════════════════════════════════════
//  4. 2D ГРАФИКИ (нижняя панель HUD)
// ═══════════════════════════════════════════════════════════════

static void add_line2(std::vector<VF>& v,
    float x0,float y0, float x1,float y1,
    float r,float g,float b, float /*a*/=1)
{
    v.push_back({x0,y0,0,r,g,b});
    v.push_back({x1,y1,0,r,g,b});
}

// Рамка панели
static void add_rect(std::vector<VF>& v,
    float x,float y,float w,float h,
    float r,float g,float b)
{
    add_line2(v,x,y,   x+w,y,   r,g,b);
    add_line2(v,x+w,y, x+w,y+h, r,g,b);
    add_line2(v,x+w,y+h,x,y+h,  r,g,b);
    add_line2(v,x,y+h, x,y,     r,g,b);
}

// Горизонтальная нулевая линия внутри панели
static void add_zero_line(std::vector<VF>& v,
    float x,float y,float w,float h,
    float vmin,float vmax,
    float r,float g,float b)
{
    float range = vmax - vmin;
    if (range < 1e-9f) return;
    float yz = y + (-vmin/range)*h;
    if (yz > y && yz < y+h)
        add_line2(v,x,yz,x+w,yz,r,g,b);
}

// ─── Скользящее окно значений
struct RingBuf {
    std::deque<float> d;
    int maxN;
    explicit RingBuf(int n=300): maxN(n){}
    void push(float v){ d.push_back(v); if((int)d.size()>maxN) d.pop_front(); }
    float vmin() const { float m=1e9f; for(auto x:d) m=std::min(m,x); return m; }
    float vmax() const { float m=-1e9f;for(auto x:d) m=std::max(m,x); return m; }
};

static void draw_ring(std::vector<VF>& v,
    const RingBuf& rb,
    float x,float y,float w,float h,
    float vmin,float vmax,
    float r,float g,float b)
{
    int n=(int)rb.d.size();
    if(n<2) return;
    float range=vmax-vmin; if(range<1e-9f)range=1;
    for(int i=0;i<n-1;i++){
        float x0=x+(float)i/(n-1)*w;
        float x1=x+(float)(i+1)/(n-1)*w;
        float y0_=y+std::max(0.f,std::min(1.f,(rb.d[i]  -vmin)/range))*h;
        float y1_=y+std::max(0.f,std::min(1.f,(rb.d[i+1]-vmin)/range))*h;
        add_line2(v,x0,y0_,x1,y1_,r,g,b);
    }
}

// ═══════════════════════════════════════════════════════════════
//  5. OpenGL УТИЛИТЫ
// ═══════════════════════════════════════════════════════════════

static GLuint make_shader(GLenum t, const char* s){
    GLuint sh=glCreateShader(t);
    glShaderSource(sh,1,&s,nullptr);
    glCompileShader(sh);
    GLint ok; glGetShaderiv(sh,GL_COMPILE_STATUS,&ok);
    if(!ok){ char b[512]; glGetShaderInfoLog(sh,512,nullptr,b);
             std::cerr<<"[Shader] "<<b<<"\n"; }
    return sh;
}
static GLuint make_prog(const char* v,const char* f){
    GLuint p=glCreateProgram();
    GLuint vs=make_shader(GL_VERTEX_SHADER,v);
    GLuint fs=make_shader(GL_FRAGMENT_SHADER,f);
    glAttachShader(p,vs); glAttachShader(p,fs);
    glLinkProgram(p);
    glDeleteShader(vs); glDeleteShader(fs);
    return p;
}

struct VAO3D {
    GLuint vao=0,vbo=0;
    int count=0;
    void init(){glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);}
    void upload(const std::vector<V3>& v, GLenum usage=GL_STATIC_DRAW){
        count=(int)v.size();
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,v.size()*sizeof(V3),v.data(),usage);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(V3),(void*)0);            glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(V3),(void*)(3*4));        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2,3,GL_FLOAT,GL_FALSE,sizeof(V3),(void*)(6*4));        glEnableVertexAttribArray(2);
    }
    void draw(GLenum mode=GL_TRIANGLES){ glBindVertexArray(vao); glDrawArrays(mode,0,count); }
    void destroy(){ glDeleteVertexArrays(1,&vao);glDeleteBuffers(1,&vbo); }
};

struct VAOF {
    GLuint vao=0,vbo=0;
    int count=0;
    void init(){glGenVertexArrays(1,&vao);glGenBuffers(1,&vbo);}
    void upload(const std::vector<VF>& v, GLenum usage=GL_DYNAMIC_DRAW){
        count=(int)v.size();
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER,vbo);
        glBufferData(GL_ARRAY_BUFFER,v.size()*sizeof(VF),v.data(),usage);
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(VF),(void*)0);     glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(VF),(void*)(3*4)); glEnableVertexAttribArray(1);
    }
    void draw(GLenum mode=GL_LINES){ glBindVertexArray(vao); glDrawArrays(mode,0,count); }
    void destroy(){ glDeleteVertexArrays(1,&vao);glDeleteBuffers(1,&vbo); }
};

// ═══════════════════════════════════════════════════════════════
//  6. ГЛАВНЫЙ ЦИКЛ
// ═══════════════════════════════════════════════════════════════

static int g_W=1400, g_H=860;

void run_visualizer(SharedBuffer& buf)
{
    // ── GLFW
    glfwSetErrorCallback([](int c,const char* d){
        std::cerr<<"[GLFW "<<c<<"] "<<d<<"\n";});

    if(!glfwInit()){ std::cerr<<"[Viz] GLFW init failed\n"; return; }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR,3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR,3);
    glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_TRUE);
    glfwWindowHint(GLFW_SAMPLES,4);

    GLFWwindow* win = glfwCreateWindow(g_W,g_H,
        "Pixhawk — Инерциальная навигация (реальное время)",nullptr,nullptr);
    if(!win){
        glfwWindowHint(GLFW_OPENGL_PROFILE,GLFW_OPENGL_COMPAT_PROFILE);
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT,GL_FALSE);
        win = glfwCreateWindow(g_W,g_H,"Pixhawk IMU",nullptr,nullptr);
    }
    if(!win){ std::cerr<<"[Viz] Window failed\n"; glfwTerminate(); return; }

    glfwMakeContextCurrent(win);
    glfwSetFramebufferSizeCallback(win,[](GLFWwindow*,int w,int h){
        g_W=w; g_H=h; glViewport(0,0,w,h);});
    glfwSwapInterval(1);

    // ── GLEW
    glewExperimental=GL_TRUE;
    GLenum err=glewInit();
    if(err!=GLEW_OK){
        std::cerr<<"[Viz] GLEW: "<<glewGetErrorString(err)<<"\n";
        glfwDestroyWindow(win); glfwTerminate(); return;
    }
    glGetError();

    std::cout<<"[Viz] "<<glGetString(GL_VERSION)
             <<" | "<<glGetString(GL_RENDERER)<<"\n";

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_LINE_SMOOTH);
    glLineWidth(1.8f);

    // ── Шейдеры
    GLuint prog3d   = make_prog(VERT3D,   FRAG3D);
    GLuint progFlat = make_prog(VERT_FLAT,FRAG_FLAT);

    // ── Геометрия
    VAO3D fc_vao;  fc_vao.init();  fc_vao.upload(build_fc(), GL_STATIC_DRAW);
    VAOF  axes_vao; axes_vao.init(); axes_vao.upload(build_world_axes(),GL_STATIC_DRAW);
    VAOF  grid_vao; grid_vao.init(); grid_vao.upload(build_grid(),GL_STATIC_DRAW);
    VAOF  trail_vao;trail_vao.init();
    VAOF  hud_vao;  hud_vao.init();

    // ── Орбитальная камера
    float cam_yaw=30.f, cam_pitch=30.f, cam_dist=4.f;
    double mx0=0,my0=0; bool lmb=false;
    float scroll=0;
    glfwSetWindowUserPointer(win,&scroll);
    glfwSetScrollCallback(win,[](GLFWwindow* w,double,double dy){
        *((float*)glfwGetWindowUserPointer(w)) += (float)dy;});

    // ── Скользящие буферы для графиков
    RingBuf rb_ax(300),rb_ay(300),rb_az(300);
    RingBuf rb_gx(300),rb_gy(300),rb_gz(300);
    RingBuf rb_roll(300),rb_pitch(300),rb_yaw(300);
    RingBuf rb_vx(300),rb_vy(300),rb_vz(300);

    // Управление
    std::cout<<"[Viz] Управление: ЛКМ — вращение, Колесо — зум, R — сброс позиции\n";

    double t_prev = -1;  // метка времени предыдущего пакета

    // ─── Главный цикл рендера ───────────────────────────────────
    while(!glfwWindowShouldClose(win) && buf.running)
    {
        glfwPollEvents();

        // ── Вычитываем ВСЕ накопившиеся пакеты из очереди
        RawIMU raw;
        while(buf.pop(raw)){
            if(t_prev > 0)
                integrate_imu(buf, raw, raw.timestamp - t_prev);
            t_prev = raw.timestamp;

            // Добавить точку траектории
            NavState snap;
            { std::lock_guard<std::mutex> lk(buf.nav_mtx); snap = buf.nav; }
            buf.add_trail_point((float)snap.px,(float)snap.py,(float)snap.pz);

            // Обновить скользящие буферы графиков
            rb_ax.push((float)snap.ax); rb_ay.push((float)snap.ay); rb_az.push((float)snap.az);
            rb_gx.push((float)snap.wx); rb_gy.push((float)snap.wy); rb_gz.push((float)snap.wz);
            rb_roll.push((float)snap.roll);
            rb_pitch.push((float)snap.pitch);
            rb_yaw.push((float)snap.yaw);
            rb_vx.push((float)snap.vx);
            rb_vy.push((float)snap.vy);
            rb_vz.push((float)snap.vz);
        }

        // ── Ввод
        double mx,my; glfwGetCursorPos(win,&mx,&my);
        if(glfwGetMouseButton(win,GLFW_MOUSE_BUTTON_LEFT)==GLFW_PRESS){
            if(lmb){cam_yaw+=(float)(mx-mx0)*.4f; cam_pitch+=(float)(my-my0)*.4f;}
            lmb=true;
        } else lmb=false;
        mx0=mx; my0=my;
        cam_pitch=std::max(-89.f,std::min(89.f,cam_pitch));
        cam_dist -= scroll*0.3f;
        cam_dist = std::max(0.5f,std::min(30.f,cam_dist));
        scroll=0;

        // Сброс позиции
        if(glfwGetKey(win,GLFW_KEY_R)==GLFW_PRESS){
            std::lock_guard<std::mutex> lk(buf.nav_mtx);
            buf.nav.px=buf.nav.py=buf.nav.pz=0;
            buf.nav.vx=buf.nav.vy=buf.nav.vz=0;
            { std::lock_guard<std::mutex> lt(buf.trail_mtx); buf.trail.clear(); }
        }

        // ── Снимок состояния
        NavState nav;
        { std::lock_guard<std::mutex> lk(buf.nav_mtx); nav = buf.nav; }

        // ── Матрицы камеры
        float W=(float)g_W, H=(float)g_H;
        // 3D сцена занимает верхние 72% экрана
        float scene_h = H * 0.72f;
        float aspect3d = W / scene_h;

        glm::mat4 proj = glm::perspective(glm::radians(45.f), aspect3d, 0.01f, 200.f);

        float cy=glm::radians(cam_yaw), cp=glm::radians(cam_pitch);
        // Камера следует за контроллером
        glm::vec3 target((float)nav.px,(float)nav.py,(float)nav.pz);
        glm::vec3 cam_offset(
            cam_dist*std::cos(cp)*std::sin(cy),
            cam_dist*std::sin(cp),
            cam_dist*std::cos(cp)*std::cos(cy));
        glm::mat4 view = glm::lookAt(target+cam_offset, target, glm::vec3(0,1,0));
        glm::mat4 vp   = proj * view;

        // ── Рендер
        glViewport(0, (GLint)(H-scene_h), (GLsizei)W, (GLsizei)scene_h);
        glClearColor(0.04f,0.05f,0.09f,1.f);
        glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

        // Сетка
        glUseProgram(progFlat);
        glUniformMatrix4fv(glGetUniformLocation(progFlat,"uMVP"),1,GL_FALSE,
            glm::value_ptr(vp*glm::mat4(1)));
        glUniform1f(glGetUniformLocation(progFlat,"uAlpha"),0.35f);
        grid_vao.draw(GL_LINES);

        // Мировые оси
        glUniform1f(glGetUniformLocation(progFlat,"uAlpha"),0.7f);
        axes_vao.draw(GL_LINES);

        // Траектория
        {
            std::lock_guard<std::mutex> lt(buf.trail_mtx);
            if(buf.trail.size()>1){
                std::vector<VF> tv;
                tv.reserve(buf.trail.size()*2);
                float fade_step = 1.0f / buf.trail.size();
                for(int i=0;i<(int)buf.trail.size()-1;i++){
                    float alpha = (float)i * fade_step; // ← старые точки темнее
                    float cr = 0.1f+0.7f*alpha;
                    float cg = 0.5f+0.4f*alpha;
                    float cb = 0.9f;
                    auto& a=buf.trail[i];   auto& b=buf.trail[i+1];
                    tv.push_back({a[0],a[1],a[2],cr,cg,cb});
                    tv.push_back({b[0],b[1],b[2],cr,cg,cb});
                }
                trail_vao.upload(tv,GL_DYNAMIC_DRAW);
                glUniform1f(glGetUniformLocation(progFlat,"uAlpha"),1.0f);
                trail_vao.draw(GL_LINES);
            }
        }

        // FC модель — переместить в текущую позицию
        glm::mat4 fc_pos = glm::translate(glm::mat4(1),
            glm::vec3((float)nav.px,(float)nav.py,(float)nav.pz));
        // Применить ориентацию
        glm::quat q((float)nav.qw,(float)nav.qx,(float)nav.qy,(float)nav.qz);
        glm::mat4 fc_model = fc_pos * glm::mat4_cast(q);
        glm::mat4 fc_mvp   = vp * fc_model;

        glUseProgram(prog3d);
        glUniformMatrix4fv(glGetUniformLocation(prog3d,"uMVP"),  1,GL_FALSE,glm::value_ptr(fc_mvp));
        glUniformMatrix4fv(glGetUniformLocation(prog3d,"uModel"),1,GL_FALSE,glm::value_ptr(fc_model));
        glm::vec3 light=glm::normalize(glm::vec3(1,2,1.5f));
        glUniform3fv(glGetUniformLocation(prog3d,"uLight"),1,glm::value_ptr(light));
        glUniform1f(glGetUniformLocation(prog3d,"uAlpha"),1.0f);
        fc_vao.draw();

        // ══ 2D HUD — нижняя панель ══════════════════════════════
        glViewport(0, 0, (GLsizei)W, (GLsizei)(H*0.30f));
        glDisable(GL_DEPTH_TEST);

        float ph = H*0.30f;
        glm::mat4 ortho = glm::ortho(0.f,W, 0.f,ph, -1.f,1.f);

        glUseProgram(progFlat);
        glUniform1f(glGetUniformLocation(progFlat,"uAlpha"),1.0f);

        std::vector<VF> hud;
        float pad=8, cols=4, cw=(W-pad*(cols+1))/cols;
        float gh=ph-pad*2;

        // Панель 1: Акселерометр
        float gx1=pad, gy1=pad;
        add_rect(hud,gx1,gy1,cw,gh, 0.18f,0.18f,0.25f);
        add_zero_line(hud,gx1,gy1,cw,gh,-15,15,0.3f,0.3f,0.35f);
        draw_ring(hud,rb_ax,gx1,gy1,cw,gh,-15,15, 0.95f,0.2f,0.2f);
        draw_ring(hud,rb_ay,gx1,gy1,cw,gh,-15,15, 0.2f,0.9f,0.2f);
        draw_ring(hud,rb_az,gx1,gy1,cw,gh,-15,15, 0.2f,0.4f,0.95f);

        // Панель 2: Гироскоп
        float gx2=pad*2+cw;
        add_rect(hud,gx2,gy1,cw,gh, 0.18f,0.18f,0.25f);
        add_zero_line(hud,gx2,gy1,cw,gh,-0.15f,0.15f,0.3f,0.3f,0.35f);
        draw_ring(hud,rb_gx,gx2,gy1,cw,gh,-0.15f,0.15f, 0.95f,0.2f,0.2f);
        draw_ring(hud,rb_gy,gx2,gy1,cw,gh,-0.15f,0.15f, 0.2f,0.9f,0.2f);
        draw_ring(hud,rb_gz,gx2,gy1,cw,gh,-0.15f,0.15f, 0.2f,0.4f,0.95f);

        // Панель 3: Углы Эйлера
        float gx3=pad*3+cw*2;
        add_rect(hud,gx3,gy1,cw,gh, 0.18f,0.18f,0.25f);
        add_zero_line(hud,gx3,gy1,cw,gh,-180,180,0.3f,0.3f,0.35f);
        draw_ring(hud,rb_roll, gx3,gy1,cw,gh,-180,180, 0.95f,0.5f,0.1f);
        draw_ring(hud,rb_pitch,gx3,gy1,cw,gh,-180,180, 0.9f,0.9f,0.1f);
        draw_ring(hud,rb_yaw,  gx3,gy1,cw,gh,-180,180, 0.4f,0.8f,0.9f);

        // Панель 4: Скорость
        float gx4=pad*4+cw*3;
        add_rect(hud,gx4,gy1,cw,gh, 0.18f,0.18f,0.25f);
        add_zero_line(hud,gx4,gy1,cw,gh,-2,2,0.3f,0.3f,0.35f);
        draw_ring(hud,rb_vx,gx4,gy1,cw,gh,-2,2, 0.95f,0.2f,0.2f);
        draw_ring(hud,rb_vy,gx4,gy1,cw,gh,-2,2, 0.2f,0.9f,0.2f);
        draw_ring(hud,rb_vz,gx4,gy1,cw,gh,-2,2, 0.2f,0.4f,0.95f);

        // Разделитель между 3D и HUD
        float sep_y=ph-1;
        add_line2(hud,0,sep_y,W,sep_y, 0.25f,0.25f,0.4f);

        hud_vao.upload(hud,GL_DYNAMIC_DRAW);
        glUniformMatrix4fv(glGetUniformLocation(progFlat,"uMVP"),1,GL_FALSE,
            glm::value_ptr(ortho));
        hud_vao.draw(GL_LINES);

        glEnable(GL_DEPTH_TEST);

        glfwSwapBuffers(win);
    }

    buf.running = false; // сигнал считывателю тоже завершиться

    fc_vao.destroy(); axes_vao.destroy(); grid_vao.destroy();
    trail_vao.destroy(); hud_vao.destroy();
    glDeleteProgram(prog3d); glDeleteProgram(progFlat);
    glfwDestroyWindow(win);
    glfwTerminate();
}