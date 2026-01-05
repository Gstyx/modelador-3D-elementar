#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <cmath>
#include <algorithm>
#include <iostream>

// --- CLASSE VETOR (x, y, z, w) ---
struct Vec4 {
    float x, y, z, w;

    // Construtor: Padrão w=1 para pontos, w=0 para vetores de direção
    Vec4(float _x = 0, float _y = 0, float _z = 0, float _w = 1) 
        : x(_x), y(_y), z(_z), w(_w) {}

    // Operação: Soma de vetores
    Vec4 operator+(const Vec4& v) const {
        return Vec4(x + v.x, y + v.y, z + v.z, w + v.w);
    }

    // Operação: Subtração de vetores
    Vec4 operator-(const Vec4& v) const {
        return Vec4(x - v.x, y - v.y, z - v.z, w - v.w);
    }

    // Operação: Multiplicação por escalar
    Vec4 operator*(float s) const {
        return Vec4(x * s, y * s, z * s, w * s);
    }

    // Produto Escalar 
    float dot(const Vec4& v) const {
        return x * v.x + y * v.y + z * v.z; // Ignoramos W no produto escalar 3D
    }

    // Produto Vetorial 
    Vec4 cross(const Vec4& v) const {
        return Vec4(
            y * v.z - z * v.y,
            z * v.x - x * v.z,
            x * v.y - y * v.x,
            0.0f // W é 0 para vetores resultantes
        );
    }

    // Normalizar vetor 
    void normalize() {
        float len = std::sqrt(x*x + y*y + z*z);
        if (len > 0) {
            x /= len; y /= len; z /= len;
        }
    }
};

// CLASSE MATRIZ 4x4 
struct Mat4 {
    float m[4][4]; // m[linha][coluna]

    // Construtor: Cria uma Matriz Identidade por padrão
    Mat4() {
        for(int i=0; i<4; i++)
            for(int j=0; j<4; j++)
                m[i][j] = (i == j) ? 1.0f : 0.0f;
    }

    // Operação: Multiplicação Matriz x Matriz (Composição de transformações)
    Mat4 operator*(const Mat4& other) const {
        Mat4 res;
        for(int i=0; i<4; i++) {
            for(int j=0; j<4; j++) {
                res.m[i][j] = 0;
                for(int k=0; k<4; k++) {
                    res.m[i][j] += m[i][k] * other.m[k][j];
                }
            }
        }
        return res;
    }

    // Operação: Multiplicação Matriz x Vetor (Transformar vértice)
    // Fórmula: v' = M * v
    Vec4 operator*(const Vec4& v) const {
        return Vec4(
            m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3]*v.w,
            m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3]*v.w,
            m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3]*v.w,
            m[3][0]*v.x + m[3][1]*v.y + m[3][2]*v.z + m[3][3]*v.w
        );
    }
};

// --- FÁBRICA DE MATRIZES DE TRANSFORMAÇÃO ---
// Requisito 10: Translação, Rotação e Escala 

Mat4 translate(float tx, float ty, float tz) {
    Mat4 mat; // Identidade
    mat.m[0][3] = tx;
    mat.m[1][3] = ty;
    mat.m[2][3] = tz;
    return mat;
}

Mat4 scale(float s) { // Escala uniforme (Requisito 10: igual nos 3 eixos)
    Mat4 mat;
    mat.m[0][0] = s;
    mat.m[1][1] = s;
    mat.m[2][2] = s;
    return mat;
}

Mat4 rotateX(float angle_rad) {
    Mat4 mat;
    float c = std::cos(angle_rad);
    float s = std::sin(angle_rad);
    mat.m[1][1] = c; mat.m[1][2] = -s;
    mat.m[2][1] = s; mat.m[2][2] = c;
    return mat;
}

Mat4 rotateY(float angle_rad) {
    Mat4 mat;
    float c = std::cos(angle_rad);
    float s = std::sin(angle_rad);
    mat.m[0][0] = c; mat.m[0][2] = s;
    mat.m[2][0] = -s; mat.m[2][2] = c;
    return mat;
}

Mat4 rotateZ(float angle_rad) {
    Mat4 mat;
    float c = std::cos(angle_rad);
    float s = std::sin(angle_rad);
    mat.m[0][0] = c; mat.m[0][1] = -s;
    mat.m[1][0] = s; mat.m[1][1] = c;
    return mat;
}

Mat4 perspective(float fov_rad, float aspect, float near, float far) {
    Mat4 mat; // Inicia identidade
    float tan_half_fov = std::tan(fov_rad / 2.0f);
    
    // Zera tudo primeiro (importante pois a identidade tem 1s que atrapalham aqui)
    for(int i=0; i<4; i++) for(int j=0; j<4; j++) mat.m[i][j] = 0.0f;

    mat.m[0][0] = 1.0f / (aspect * tan_half_fov);
    mat.m[1][1] = 1.0f / tan_half_fov;
    mat.m[2][2] = -(far + near) / (far - near);
    mat.m[2][3] = -(2.0f * far * near) / (far - near);
    mat.m[3][2] = -1.0f; // Isso coloca o -Z no W, essencial para a divisão perspectiva
    
    return mat;
}

#endif