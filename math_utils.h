#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <cmath>
#include <algorithm>
#include <iostream>
#include <cstdint> // Para uint32_t

// --- CONSTANTES GERAIS ---

const int LARGURA = 800;
const int ALTURA = 600;

// --- ESTRUTURAS MATEMÁTICAS ---

struct Vec4 {
    float x, y, z, w;

    Vec4(float _x = 0, float _y = 0, float _z = 0, float _w = 1) 
        : x(_x), y(_y), z(_z), w(_w) {}

    Vec4 operator+(const Vec4& v) const { return Vec4(x + v.x, y + v.y, z + v.z, w + v.w); }
    Vec4 operator-(const Vec4& v) const { return Vec4(x - v.x, y - v.y, z - v.z, w - v.w); }
    Vec4 operator*(float s) const { return Vec4(x * s, y * s, z * s, w * s); }

    float dot(const Vec4& v) const { return x * v.x + y * v.y + z * v.z; }

    Vec4 cross(const Vec4& v) const {
        return Vec4(y * v.z - z * v.y, z * v.x - x * v.z, x * v.y - y * v.x, 0.0f);
    }

    void normalize() {
        float len = std::sqrt(x*x + y*y + z*z);
        if (len > 0) { x /= len; y /= len; z /= len; }
    }
};

struct Mat4 {
    float m[4][4];
    Mat4() {
        for(int i=0; i<4; i++) for(int j=0; j<4; j++) m[i][j] = (i==j) ? 1.0f : 0.0f;
    }

    Mat4 operator*(const Mat4& o) const {
        Mat4 res;
        for(int i=0; i<4; i++) {
            for(int j=0; j<4; j++) {
                res.m[i][j] = 0;
                for(int k=0; k<4; k++) res.m[i][j] += m[i][k] * o.m[k][j];
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

// --- ESTRUTURA DO CUBO (MATERIAL) ---
struct Cubo {
    Vec4 posicao;
    Vec4 rotacao; 
    Vec4 escala;
    uint32_t cor_base;

    // Material Phong
    float ka = 0.2f;    
    float kd = 0.7f;    
    float ks = 0.8f;    
    float shininess = 50.0f; 
};

// --- FUNÇÕES AUXILIARES E FÁBRICAS ---

inline Mat4 translate(float tx, float ty, float tz) {
    Mat4 mat;
    mat.m[0][3] = tx; mat.m[1][3] = ty; mat.m[2][3] = tz;
    return mat;
}

inline Mat4 scale(float s) {
    Mat4 mat;
    mat.m[0][0] = s; mat.m[1][1] = s; mat.m[2][2] = s;
    return mat;
}

inline Mat4 rotateX(float angle) {
    Mat4 mat;
    float c = std::cos(angle), s = std::sin(angle);
    mat.m[1][1] = c; mat.m[1][2] = -s;
    mat.m[2][1] = s; mat.m[2][2] = c;
    return mat;
}

inline Mat4 rotateY(float angle) {
    Mat4 mat;
    float c = std::cos(angle), s = std::sin(angle);
    mat.m[0][0] = c; mat.m[0][2] = s;
    mat.m[2][0] = -s; mat.m[2][2] = c;
    return mat;
}

inline Mat4 rotateZ(float angle) {
    Mat4 mat;
    float c = std::cos(angle), s = std::sin(angle);
    mat.m[0][0] = c; mat.m[0][1] = -s;
    mat.m[1][0] = s; mat.m[1][1] = c;
    return mat;
}

inline Mat4 perspective(float fov, float aspect, float near, float far) {
    Mat4 mat;
    float tan_half = std::tan(fov / 2.0f);
    for(int i=0; i<4; i++) for(int j=0; j<4; j++) mat.m[i][j] = 0;
    mat.m[0][0] = 1.0f / (aspect * tan_half);
    mat.m[1][1] = 1.0f / tan_half;
    mat.m[2][2] = -(far + near) / (far - near);
    mat.m[2][3] = -(2.0f * far * near) / (far - near);
    mat.m[3][2] = -1.0f;
    return mat;
}

// Helpers de Interpolação
inline void swap_int(int& a, int& b) { int t = a; a = b; b = t; }
inline void swap_float(float& a, float& b) { float t = a; a = b; b = t; }
inline void swap_vec4(Vec4& a, Vec4& b) { Vec4 t = a; a = b; b = t; }

inline float interpolate(float min, float max, float factor) { return min + (max - min) * factor; }
inline Vec4 interpolate_vec(Vec4 min, Vec4 max, float factor) { return min + (max - min) * factor; }

inline uint8_t clamp(float val) {
    return (val > 255) ? 255 : (val < 0 ? 0 : (uint8_t)val);
}

#endif