/**
 * RASTERIZER.H
 * Implementação de Algoritmos de Recorte (Clipping) e Rasterização (Scanline).
 */

#ifndef RASTERIZER_H
#define RASTERIZER_H

#include "math_utils.h"
#include <vector>
#include <algorithm>


// Calcula a cor final de um pixel combinando luz e material RGB
inline uint32_t calc_luz_rgb(Vec4 pos, Vec4 norm, Cubo cubo, Vec4 lightPos, Vec4 camPos) {
    Vec3 lightColor(1.0f, 1.0f, 1.0f); // Luz branca
    
    Vec4 L = lightPos - pos; L.normalize(); // Vetor Luz
    Vec4 N = norm; N.normalize();           // Normal da superfície
    Vec4 V = camPos - pos; V.normalize();   // Vetor Visão (Olho)
    
    // --- COMPONENTE DIFUSA (Lambert) ---
    float diff = std::max(0.0f, N.dot(L));
    
    // --- COMPONENTE ESPECULAR (Phong) ---
    Vec4 R = (N * (2.0f * N.dot(L))) - L; R.normalize(); // Vetor Reflexo
    float spec = (diff > 0) ? std::pow(std::max(0.0f, R.dot(V)), cubo.mat.shininess) : 0.0f;
    
    // Combinação: I = Ka + Kd(N.L) + Ks(R.V)^n
    Vec3 ambient  = cubo.mat.ka * lightColor;
    Vec3 diffuse  = cubo.mat.kd * lightColor * diff;
    Vec3 specular = cubo.mat.ks * lightColor * spec;
    
    Vec3 final = ambient + diffuse + specular;
    
    // Converte para formato hexadecimal de cor (ARGB)
    return (255<<24) | (clamp(final.x*255)<<16) | (clamp(final.y*255)<<8) | clamp(final.z*255);
}


void clip_triangle_sutherland_hodgman(const Vec4& v1, const Vec4& v2, const Vec4& v3, std::vector<Vec4>& out_poly) {
    const float Z_NEAR = -0.1f; // Limite do plano Near no View Space
    
    Vec4 input_poly[3] = {v1, v2, v3};
    std::vector<Vec4> output_poly; // Polígono recortado (pode ter 3 ou 4 vértices)
    
    Vec4 prev_vert = input_poly[2];
    bool prev_inside = (prev_vert.z <= Z_NEAR); // Inside se Z for mais negativo que -0.1
    
    for(int i=0; i<3; i++) {
        Vec4 curr_vert = input_poly[i];
        bool curr_inside = (curr_vert.z <= Z_NEAR);
        
        if (curr_inside) {
            if (!prev_inside) {
                // Entrando na região visível: Intersecção
                float t = (Z_NEAR - prev_vert.z) / (curr_vert.z - prev_vert.z);
                output_poly.push_back(lerp_vertex(prev_vert, curr_vert, t));
            }
            output_poly.push_back(curr_vert); // Adiciona vértice atual
        } else if (prev_inside) {
            // Saindo da região visível: Intersecção
            float t = (Z_NEAR - prev_vert.z) / (curr_vert.z - prev_vert.z);
            output_poly.push_back(lerp_vertex(prev_vert, curr_vert, t));
        }
        
        prev_vert = curr_vert;
        prev_inside = curr_inside;
    }
    
    // Triangulação do resultado (Converte Polígono -> Triângulos)
    if (output_poly.size() < 3) return; // Totalmente fora
    
    // Triângulo 1
    out_poly.push_back(output_poly[0]);
    out_poly.push_back(output_poly[1]);
    out_poly.push_back(output_poly[2]);
    
    // Se for um quadrilátero, adiciona o segundo triângulo
    if (output_poly.size() == 4) {
        out_poly.push_back(output_poly[0]);
        out_poly.push_back(output_poly[2]);
        out_poly.push_back(output_poly[3]);
    }
}


// Rasterizador PHONG 
inline void fill_phong(int x1, int y1, float z1, Vec4 w1, 
                       int x2, int y2, float z2, Vec4 w2, 
                       int x3, int y3, float z3, Vec4 w3, 
                       Vec4 n, Cubo mat, Vec4 lp, Vec4 cp,
                       std::vector<uint32_t>& fb, std::vector<float>& zb,
                       int vpw, int vph, int vpx, int vpy) {
    
    // Ordenação dos vértices por Y (Y1 <= Y2 <= Y3)
    if (y1>y2) { swap_int(x1,x2); swap_int(y1,y2); swap_float(z1,z2); swap_vec4(w1,w2); }
    if (y1>y3) { swap_int(x1,x3); swap_int(y1,y3); swap_float(z1,z3); swap_vec4(w1,w3); }
    if (y2>y3) { swap_int(x2,x3); swap_int(y2,y3); swap_float(z2,z3); swap_vec4(w2,w3); }
    
    int h = y3 - y1; if (h == 0) return;

    for (int i = 0; i < h; i++) {
        int y = y1 + i;
        bool lower = i > (y2 - y1) || (y2 == y1);
        int seg_h = lower ? (y3 - y2) : (y2 - y1);
        float alpha = (float)i / h;
        float beta = (float)(i - (lower ? (y2 - y1) : 0)) / seg_h;
        
        // Interpolação das arestas (Scanline)
        int ax = interp(x1, x3, alpha);
        int bx = lower ? interp(x2, x3, beta) : interp(x1, x2, beta);
        float az = interp(z1, z3, alpha);
        float bz = lower ? interp(z2, z3, beta) : interp(z1, z2, beta);
        Vec4 aw = interp_vec(w1, w3, alpha);
        Vec4 bw = lower ? interp_vec(w2, w3, beta) : interp_vec(w1, w2, beta);
        
        if (ax > bx) { swap_int(ax, bx); swap_float(az, bz); swap_vec4(aw, bw); }
        
        for (int x = ax; x <= bx; x++) {
            // Recorte 2D do Viewport)
            if (x < vpx || x >= vpx+vpw || y < vpy || y >= vpy+vph) continue;
            if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) continue;
            
            float phi = (bx == ax) ? 1.0f : (float)(x - ax) / (bx - ax);
            float z = interp(az, bz, phi);
            
            int idx = y * SCREEN_W + x;
            
            // Z-Buffer 
            if (z < zb[idx]) {
                zb[idx] = z;
                // Interpolação Phong
                Vec4 p = interp_vec(aw, bw, phi);
                fb[idx] = calc_luz_rgb(p, n, mat, lp, cp);
            }
        }
    }
}

// Rasterizador FLAT (Cor constante para o triângulo inteiro)
inline void fill_flat(int x1, int y1, float z1, int x2, int y2, float z2, int x3, int y3, float z3, 
                      uint32_t c, std::vector<uint32_t>& fb, std::vector<float>& zb, int vpw, int vph, int vpx, int vpy) {
    if (y1>y2) { swap_int(x1,x2); swap_int(y1,y2); swap_float(z1,z2); }
    if (y1>y3) { swap_int(x1,x3); swap_int(y1,y3); swap_float(z1,z3); }
    if (y2>y3) { swap_int(x2,x3); swap_int(y2,y3); swap_float(z2,z3); }
    
    int h = y3 - y1; if (h == 0) return;

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
            if (x < vpx || x >= vpx+vpw || y < vpy || y >= vpy+vph) continue;
            if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) continue;
            
            float phi = (bx == ax) ? 1.0f : (float)(x - ax) / (bx - ax);
            float z = interp(az, bz, phi);
            
            int idx = y * SCREEN_W + x;
            // Z-Buffer Test
            if (z < zb[idx]) { zb[idx] = z; fb[idx] = c; }
        }
    }
}

#endif