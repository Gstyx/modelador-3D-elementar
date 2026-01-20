#ifndef RASTERIZER_H
#define RASTERIZER_H

#include "math_utils.h" // <--- IMPORTANTE: Importa Vec4, Cubo, Mat4
#include <vector>
#include <cmath>
#include <algorithm>

// Shader de Pixel (Cálculo de Iluminação Phong Simplificado)
inline uint32_t calculate_phong_pixel(Vec4 fragPos, Vec4 normal, Vec4 lightPos, Vec4 viewPos, uint32_t baseColor, Cubo mat) {
    uint8_t r = (baseColor >> 16) & 0xFF;
    uint8_t g = (baseColor >> 8) & 0xFF;
    uint8_t b = baseColor & 0xFF;

    Vec4 N = normal; N.normalize();
    Vec4 L = lightPos - fragPos; L.normalize();
    Vec4 V = viewPos - fragPos; V.normalize();

    // Reflexo: R = 2(N.L)N - L
    float n_dot_l = std::max(0.0f, N.dot(L));
    Vec4 R = (N * (2.0f * n_dot_l)) - L;
    R.normalize();

    float ambient = mat.ka;
    float diffuse = mat.kd * n_dot_l;
    float specular = 0.0f;
    
    if (n_dot_l > 0.0f) {
        float r_dot_v = std::max(0.0f, R.dot(V));
        specular = mat.ks * std::pow(r_dot_v, mat.shininess);
    }

    float intensity = ambient + diffuse;
    
    int fr = (int)(r * intensity + specular * 255);
    int fg = (int)(g * intensity + specular * 255);
    int fb = (int)(b * intensity + specular * 255);

    return (255 << 24) | (clamp(fr) << 16) | (clamp(fg) << 8) | clamp(fb);
}

// Rasterizador Scanline (Recebe buffers por referência para funcionar separado)
inline void fill_triangle_phong(int x1, int y1, float z1, Vec4 w1, 
                                int x2, int y2, float z2, Vec4 w2, 
                                int x3, int y3, float z3, Vec4 w3, 
                                Vec4 normal_face, Cubo material,
                                Vec4 lightPos, Vec4 camPos,
                                std::vector<uint32_t>& framebuffer, 
                                std::vector<float>& zbuffer) {
    
    if (y1 > y2) { swap_int(x1, x2); swap_int(y1, y2); swap_float(z1, z2); swap_vec4(w1, w2); }
    if (y1 > y3) { swap_int(x1, x3); swap_int(y1, y3); swap_float(z1, z3); swap_vec4(w1, w3); }
    if (y2 > y3) { swap_int(x2, x3); swap_int(y2, y3); swap_float(z2, z3); swap_vec4(w2, w3); }

    int total_height = y3 - y1;
    if (total_height == 0) return;

    for (int i = 0; i < total_height; i++) {
        bool second_half = i > (y2 - y1) || (y2 == y1);
        int segment_height = second_half ? (y3 - y2) : (y2 - y1);
        float alpha = (float)i / total_height;
        float beta  = (float)(i - (second_half ? (y2 - y1) : 0)) / segment_height; 
        
        int Ax = x1 + (x3 - x1) * alpha;
        int Bx = second_half ? (x2 + (x3 - x2) * beta) : (x1 + (x2 - x1) * beta);

        float Az = interpolate(z1, z3, alpha);
        float Bz = second_half ? interpolate(z2, z3, beta) : interpolate(z1, z2, beta);

        Vec4 Aw = interpolate_vec(w1, w3, alpha);
        Vec4 Bw = second_half ? interpolate_vec(w2, w3, beta) : interpolate_vec(w1, w2, beta);

        if (Ax > Bx) { swap_int(Ax, Bx); swap_float(Az, Bz); swap_vec4(Aw, Bw); }

        for (int x = Ax; x <= Bx; x++) {
            float phi = (Bx == Ax) ? 1.0f : (float)(x - Ax) / (float)(Bx - Ax);
            
            float z = interpolate(Az, Bz, phi);
            
            // Note o uso de LARGURA e ALTURA definidos em math_utils.h
            if (x >= 0 && x < LARGURA && (y1+i) >= 0 && (y1+i) < ALTURA) {
                int idx = (y1 + i) * LARGURA + x;
                if (z < zbuffer[idx]) {
                    zbuffer[idx] = z;

                    Vec4 pixel_pos_world = interpolate_vec(Aw, Bw, phi);

                    uint32_t cor = calculate_phong_pixel(pixel_pos_world, normal_face, lightPos, camPos, material.cor_base, material);
                    
                    framebuffer[idx] = cor;
                }
            }
        }
    }
}

#endif