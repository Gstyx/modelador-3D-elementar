#ifndef RASTERIZER_H
#define RASTERIZER_H

#include "math_utils.h" 
#include <vector>
#include <cmath>
#include <algorithm>

// [PIPELINE] Shading (Iluminação): Calcula a cor final baseada na Luz e Material
inline uint32_t calc_luz(Vec4 pos, Vec4 norm, Cubo mat, Vec4 lightPos, Vec4 camPos) {
    Vec4 L = lightPos - pos; L.normalize();
    Vec4 N = norm; N.normalize();
    Vec4 V = camPos - pos; V.normalize();
    
    float diff = std::max(0.0f, N.dot(L));
    Vec4 R = (N * (2.0f * N.dot(L))) - L; R.normalize();
    float spec = (diff > 0) ? std::pow(std::max(0.0f, R.dot(V)), mat.shininess) : 0.0f;
    
    float intensity = mat.ka + (mat.kd * diff);
    
    uint8_t r = (mat.cor_base >> 16) & 0xFF;
    uint8_t g = (mat.cor_base >> 8) & 0xFF;
    uint8_t b = mat.cor_base & 0xFF;
    
    int fr = r * intensity + mat.ks * spec * 255;
    int fg = g * intensity + mat.ks * spec * 255;
    int fb = b * intensity + mat.ks * spec * 255;
    
    return (255<<24) | (clamp(fr)<<16) | (clamp(fg)<<8) | clamp(fb);
}

// [PIPELINE] Rasterização (Conversão de Primitivas Geométricas para Fragmentos/Pixels)
inline void fill_triangle_flat(int x1, int y1, float z1, 
                               int x2, int y2, float z2, 
                               int x3, int y3, float z3, 
                               uint32_t flat_color,
                               std::vector<uint32_t>& framebuffer,
                               std::vector<float>& zbuffer) {
    
    if (y1 > y2) { swap_int(x1,x2); swap_int(y1,y2); swap_float(z1,z2); }
    if (y1 > y3) { swap_int(x1,x3); swap_int(y1,y3); swap_float(z1,z3); }
    if (y2 > y3) { swap_int(x2,x3); swap_int(y2,y3); swap_float(z2,z3); }
    
    int h = y3 - y1;
    if (h == 0) return;

    for (int i = 0; i < h; i++) {
        int y = y1 + i;
        bool lower = i > (y2 - y1) || (y2 == y1);
        int seg_h = lower ? (y3 - y2) : (y2 - y1);
        float alpha = (float)i / h;
        float beta = (float)(i - (lower ? (y2 - y1) : 0)) / seg_h;
        
        int ax = interp(x1, x3, alpha);
        int bx = lower ? interp(x2, x3, beta) : interp(x1, x2, beta);
        float az = interp(z1, z3, alpha);
        float bz = lower ? interp(z2, z3, beta) : interp(z1, z2, beta);
        
        if (ax > bx) { swap_int(ax, bx); swap_float(az, bz); }
        
        for (int x = ax; x <= bx; x++) {
            if (x < 0 || x >= LARGURA || y < 0 || y >= ALTURA) continue;
            
            float phi = (bx == ax) ? 1.0f : (float)(x - ax) / (bx - ax);
            float z = interp(az, bz, phi);
            
            int idx = y * LARGURA + x;
            
            // [PIPELINE] Ocultação de Superfícies (Z-Buffer)
            if (z < zbuffer[idx]) {
                zbuffer[idx] = z;
                framebuffer[idx] = flat_color;
            }
        }
    }
}

// [PIPELINE] Rasterização com Interpolação de Phong (Shading por Pixel)
inline void fill_triangle_phong(int x1, int y1, float z1, Vec4 w1, 
                                int x2, int y2, float z2, Vec4 w2, 
                                int x3, int y3, float z3, Vec4 w3, 
                                Vec4 normal, Cubo mat, 
                                Vec4 lightPos, Vec4 camPos,
                                std::vector<uint32_t>& framebuffer,
                                std::vector<float>& zbuffer) {
    
    if (y1 > y2) { swap_int(x1,x2); swap_int(y1,y2); swap_float(z1,z2); swap_vec4(w1,w2); }
    if (y1 > y3) { swap_int(x1,x3); swap_int(y1,y3); swap_float(z1,z3); swap_vec4(w1,w3); }
    if (y2 > y3) { swap_int(x2,x3); swap_int(y2,y3); swap_float(z2,z3); swap_vec4(w2,w3); }
    
    int h = y3 - y1;
    if (h == 0) return;

    for (int i = 0; i < h; i++) {
        int y = y1 + i;
        bool lower = i > (y2 - y1) || (y2 == y1);
        int seg_h = lower ? (y3 - y2) : (y2 - y1);
        float alpha = (float)i / h;
        float beta = (float)(i - (lower ? (y2 - y1) : 0)) / seg_h;
        
        int ax = interp(x1, x3, alpha);
        int bx = lower ? interp(x2, x3, beta) : interp(x1, x2, beta);
        float az = interp(z1, z3, alpha);
        float bz = lower ? interp(z2, z3, beta) : interp(z1, z2, beta);
        Vec4 aw = interp_vec(w1, w3, alpha);
        Vec4 bw = lower ? interp_vec(w2, w3, beta) : interp_vec(w1, w2, beta);
        
        if (ax > bx) { swap_int(ax, bx); swap_float(az, bz); swap_vec4(aw, bw); }
        
        for (int x = ax; x <= bx; x++) {
            if (x < 0 || x >= LARGURA || y < 0 || y >= ALTURA) continue;
            
            float phi = (bx == ax) ? 1.0f : (float)(x - ax) / (bx - ax);
            float z = interp(az, bz, phi);
            
            int idx = y * LARGURA + x;
            
            // [PIPELINE] Z-Buffer
            if (z < zbuffer[idx]) {
                zbuffer[idx] = z;
                // Interpolamos a posição REAL do pixel no mundo para calcular a luz correta
                Vec4 p_pixel = interp_vec(aw, bw, phi);
                framebuffer[idx] = calc_luz(p_pixel, normal, mat, lightPos, camPos);
            }
        }
    }
}

#endif