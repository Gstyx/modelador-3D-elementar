/**
 * MATH_UTILS.H
 * Biblioteca de álgebra linear básica para o pipeline gráfico.
 * Define vetores, matrizes e as transformações geométricas fundamentais.
 */

#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <cmath>
#include <algorithm>
#include <cstdint>
#include <iostream>

const int SCREEN_W = 800;
const int SCREEN_H = 600;

// --- VETORES ---

// Vec3: Usado principalmente para cálculos de COR (RGB) e vetores de direção simples.
struct Vec3 {
    float x, y, z;
    Vec3(float _x=0, float _y=0, float _z=0) : x(_x), y(_y), z(_z) {}
    
    Vec3 operator+(const Vec3& v) const { return Vec3(x+v.x, y+v.y, z+v.z); }
    Vec3 operator*(float s)       const { return Vec3(x*s, y*s, z*s); }
    // Produto componente-a-componente (usado para aplicar cor do material sobre a luz)
    Vec3 operator*(const Vec3& v) const { return Vec3(x*v.x, y*v.y, z*v.z); } 
};

// Vec4: Coordenadas Homogêneas (X, Y, Z, W).
// Essencial para o pipeline: O componente 'W' permite translações em matrizes 4x4
// e é fundamental para a divisão perspectiva (transformar 3D em 2D).
struct Vec4 {
    float x, y, z, w;
    Vec4(float _x=0, float _y=0, float _z=0, float _w=1) : x(_x), y(_y), z(_z), w(_w) {}
    
    Vec4 operator+(const Vec4& v) const { return Vec4(x+v.x, y+v.y, z+v.z, w+v.w); }
    Vec4 operator-(const Vec4& v) const { return Vec4(x-v.x, y-v.y, z-v.z, w-v.w); }
    Vec4 operator*(float s)       const { return Vec4(x*s, y*s, z*s, w*s); }
    
    // Produto Escalar: Usado para calcular ângulos (luz vs normal)
    float dot(const Vec4& v) const { return x*v.x + y*v.y + z*v.z; }
    
    // Produto Vetorial: Usado para encontrar a Normal da superfície e Backface Culling
    Vec4 cross(const Vec4& v) const {
        return Vec4(y*v.z - z*v.y, z*v.x - x*v.z, x*v.y - y*v.x, 0.0f);
    }
    
    void normalize() {
        float len = std::sqrt(x*x + y*y + z*z);
        if(len > 0) { x/=len; y/=len; z/=len; }
    }
};

// --- MATRIZES ---

// Matriz 4x4 para Transformações Afins (Rotação, Escala, Translação, Projeção)
struct Mat4 {
    float m[4][4];
    
    // Inicia como Identidade (diagonal = 1)
    Mat4() { 
        for(int i=0; i<4; i++) for(int j=0; j<4; j++) m[i][j] = (i==j) ? 1.0f : 0.0f; 
    }
    
    // Multiplicação de Matrizes (Concatenação de transformações)
    Mat4 operator*(const Mat4& o) const {
        Mat4 res;
        for(int i=0; i<4; i++) for(int j=0; j<4; j++) {
            res.m[i][j] = 0;
            for(int k=0; k<4; k++) res.m[i][j] += m[i][k] * o.m[k][j];
        }
        return res;
    }
    
    // Aplicação da Matriz em um Vértice
    Vec4 operator*(const Vec4& v) const {
        return Vec4(
            m[0][0]*v.x + m[0][1]*v.y + m[0][2]*v.z + m[0][3]*v.w,
            m[1][0]*v.x + m[1][1]*v.y + m[1][2]*v.z + m[1][3]*v.w,
            m[2][0]*v.x + m[2][1]*v.y + m[2][2]*v.z + m[2][3]*v.w,
            m[3][0]*v.x + m[3][1]*v.y + m[3][2]*v.z + m[3][3]*v.w
        );
    }
};

// --- ESTRUTURAS DE CENA ---

struct Material {
    Vec3 ka;        // Coeficiente Ambiente (Cor da sombra)
    Vec3 kd;        // Coeficiente Difuso (Cor real do objeto)
    Vec3 ks;        // Coeficiente Especular (Cor do brilho/reflexo)
    float shininess; // Expoente de brilho (Polimento)
};

struct Cubo {
    Vec4 posicao, rotacao, escala;
    Material mat;
};

// --- FÁBRICA DE MATRIZES ---

inline Mat4 translate(float x, float y, float z) { 
    Mat4 m; 
    m.m[0][3]=x; m.m[1][3]=y; m.m[2][3]=z; 
    return m; 
}

inline Mat4 scale(float s) { 
    Mat4 m; 
    m.m[0][0]=s; m.m[1][1]=s; m.m[2][2]=s; 
    return m; 
}

inline Mat4 rotateX(float a) { 
    Mat4 m; float c=std::cos(a), s=std::sin(a); 
    m.m[1][1]=c; m.m[1][2]=-s; m.m[2][1]=s; m.m[2][2]=c; 
    return m; 
}

inline Mat4 rotateY(float a) { 
    Mat4 m; float c=std::cos(a), s=std::sin(a); 
    m.m[0][0]=c; m.m[0][2]=s; m.m[2][0]=-s; m.m[2][2]=c; 
    return m; 
}

// Projeção Perspectiva: Define o Frustum de visão.
// Transforma o espaço de visão em Clip Space.
inline Mat4 perspective(float fov, float aspect, float n, float f) {
    Mat4 m; float t = std::tan(fov/2);
    // Zera tudo pois projeção altera a diagonal principal drasticamente
    for(int i=0;i<4;i++)for(int j=0;j<4;j++) m.m[i][j]=0;
    
    m.m[0][0] = 1.0f/(aspect*t);
    m.m[1][1] = 1.0f/t;
    m.m[2][2] = -(f+n)/(f-n);
    m.m[2][3] = -(2*f*n)/(f-n);
    m.m[3][2] = -1.0f; // Joga -Z em W para fazer a divisão perspectiva depois
    return m;
}

// --- UTILITÁRIOS ---

inline uint8_t clamp(float v) { return v>255?255:(v<0?0:(uint8_t)v); }
inline void swap_int(int& a, int& b) { int t=a; a=b; b=t; }
inline void swap_float(float& a, float& b) { float t=a; a=b; b=t; }
inline void swap_vec4(Vec4& a, Vec4& b) { Vec4 t=a; a=b; b=t; }

// Interpolação Linear (Lerp) de valores simples
inline float interp(float v1, float v2, float t) { 
    return v1 + (v2-v1)*t; 
}

// Interpolação Linear de Vetores (Usado no Scanline)
inline Vec4 interp_vec(Vec4 v1, Vec4 v2, float t) { 
    return v1 + (v2-v1)*t; 
}

// Interpolação Linear para o Clipping (Gera novos vértices na borda)
inline Vec4 lerp_vertex(const Vec4& a, const Vec4& b, float t) {
    return a + (b - a) * t;
}

#endif