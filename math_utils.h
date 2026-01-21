#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <cmath>
#include <algorithm>
#include <cstdint>

// Dimensões da tela (usadas em todo o projeto)
const int LARGURA = 800;
const int ALTURA = 600;

// --- ESTRUTURAS MATEMÁTICAS ---

struct Vec4 {
    float x, y, z, w;
    Vec4(float _x=0, float _y=0, float _z=0, float _w=1) : x(_x), y(_y), z(_z), w(_w) {}
    
    Vec4 operator+(const Vec4& v) const { return Vec4(x+v.x, y+v.y, z+v.z, w+v.w); }
    Vec4 operator-(const Vec4& v) const { return Vec4(x-v.x, y-v.y, z-v.z, w-v.w); }
    Vec4 operator*(float s) const { return Vec4(x*s, y*s, z*s, w*s); }
    
    float dot(const Vec4& v) const { return x*v.x + y*v.y + z*v.z; }
    
    Vec4 cross(const Vec4& v) const {
        return Vec4(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x, 0.0f);
    }
    
    void normalize() {
        float len = std::sqrt(x*x + y*y + z*z);
        if(len > 0) { x/=len; y/=len; z/=len; }
    }
};

struct Mat4 {
    float m[4][4];
    Mat4() { for(int i=0;i<4;i++) for(int j=0;j<4;j++) m[i][j] = (i==j)?1.0f:0.0f; }
    
    Mat4 operator*(const Mat4& o) const {
        Mat4 res;
        for(int i=0;i<4;i++) {
            for(int j=0;j<4;j++) {
                res.m[i][j] = 0;
                for(int k=0;k<4;k++) res.m[i][j] += m[i][k] * o.m[k][j];
            }
        }
        return res;
    }
    
    Vec4 operator*(const Vec4& v) const {
        return Vec4(
            m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3]*v.w,
            m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3]*v.w,
            m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3]*v.w,
            m[3][0]*v.x + m[3][1]*v.y + m[3][2]*v.z + m[3][3]*v.w
        );
    }
};

// --- ESTRUTURA DO CUBO ---
struct Cubo {
    Vec4 posicao, rotacao, escala;
    uint32_t cor_base;
    float ka, kd, ks, shininess; 
};

// --- FUNÇÕES MATEMÁTICAS ---

inline Mat4 translate(float x, float y, float z) { Mat4 m; m.m[0][3]=x; m.m[1][3]=y; m.m[2][3]=z; return m; }
inline Mat4 scale(float s) { Mat4 m; m.m[0][0]=s; m.m[1][1]=s; m.m[2][2]=s; return m; }
inline Mat4 rotateY(float a) { Mat4 m; float c=std::cos(a), s=std::sin(a); m.m[0][0]=c; m.m[0][2]=s; m.m[2][0]=-s; m.m[2][2]=c; return m; }
inline Mat4 rotateX(float a) { Mat4 m; float c=std::cos(a), s=std::sin(a); m.m[1][1]=c; m.m[1][2]=-s; m.m[2][1]=s; m.m[2][2]=c; return m; }
inline Mat4 rotateZ(float a) { Mat4 m; float c=std::cos(a), s=std::sin(a); m.m[0][0]=c; m.m[0][1]=-s; m.m[1][0]=s; m.m[1][1]=c; return m; }

inline Mat4 perspective(float fov, float aspect, float n, float f) {
    Mat4 m; float t = std::tan(fov/2);
    m.m[0][0]=0; for(int i=0;i<4;i++)for(int j=0;j<4;j++) m.m[i][j]=0;
    m.m[0][0] = 1.0f/(aspect*t);
    m.m[1][1] = 1.0f/t;
    m.m[2][2] = -(f+n)/(f-n);
    m.m[2][3] = -(2*f*n)/(f-n);
    m.m[3][2] = -1.0f;
    return m;
}

// Utilitários
inline uint8_t clamp(float v) { return v>255?255:(v<0?0:(uint8_t)v); }
inline void swap_int(int& a, int& b) { int t=a; a=b; b=t; }
inline void swap_float(float& a, float& b) { float t=a; a=b; b=t; }
inline void swap_vec4(Vec4& a, Vec4& b) { Vec4 t=a; a=b; b=t; }
inline float interp(float v1, float v2, float t) { return v1 + (v2-v1)*t; }
inline Vec4 interp_vec(Vec4 v1, Vec4 v2, float t) { return v1 + (v2-v1)*t; }

#endif