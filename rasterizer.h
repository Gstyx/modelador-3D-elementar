/**
 * RASTERIZER.H
 * Núcleo do renderizador: Iluminação, Recorte e Rasterização (Scanline).
 */

#ifndef RASTERIZER_H
#define RASTERIZER_H

#include "math_utils.h"
#include <vector>
#include <algorithm>

// ==========================================
//   PIXEL SHADER (ILUMINAÇÃO DE PHONG)
// ==========================================
// Calcula a cor final de um fragmento/pixel baseado na posição, normal e material.
// Modelo utilizado: Blinn-Phong simplificado.
inline uint32_t calc_luz_rgb(Vec4 pos, Vec4 norm, Cubo cubo, Vec4 lightPos, Vec4 camPos, Vec3 lightColor, Vec3 ambientColor) {
    
    Vec4 L = lightPos - pos; L.normalize(); // Vetor Luz (Ponto -> Luz)
    Vec4 N = norm; N.normalize();           // Normal da superfície
    Vec4 V = camPos - pos; V.normalize();   // Vetor Visão (Ponto -> Câmera)
    
    // 1. Componente Difusa (Lei de Lambert): Intensidade depende do ângulo entre Luz e Normal
    float diff = std::max(0.0f, N.dot(L));
    
    // 2. Componente Especular (Reflexo): Depende do ângulo entre Reflexo e Visão
    Vec4 R = (N * (2.0f * N.dot(L))) - L; R.normalize(); // Vetor Reflexo
    float spec = (diff > 0) ? std::pow(std::max(0.0f, R.dot(V)), cubo.mat.shininess) : 0.0f;
    
    // Combinação Final: Ambiente + Difusa + Especular
    // I = Ka*Ia + Kd*Il*(N.L) + Ks*Il*(R.V)^n
    Vec3 ambient  = cubo.mat.ka * ambientColor;
    Vec3 diffuse  = cubo.mat.kd * lightColor * diff;
    Vec3 specular = cubo.mat.ks * lightColor * spec;
    
    Vec3 final = ambient + diffuse + specular;
    
    // Clamp e conversão para uint32 (ARGB) para o buffer de vídeo
    return (255<<24) | (clamp(final.x*255)<<16) | (clamp(final.y*255)<<8) | clamp(final.z*255);
}

// ==========================================
//   RECORTE (CLIPPING) - SUTHERLAND-HODGMAN
// ==========================================
// Corta triângulos que atravessam o plano Near da câmera.
// Essencial para evitar divisão por zero na projeção e artefatos visuais atrás da câmera.
void clip_triangle_sutherland_hodgman(const Vec4& v1, const Vec4& v2, const Vec4& v3, std::vector<Vec4>& out_poly) {
    const float Z_NEAR = -0.1f; // Plano de corte no Espaço da Câmera (View Space)
    
    Vec4 input_poly[3] = {v1, v2, v3};
    std::vector<Vec4> output_poly; // O resultado pode ser 0, 3 ou 4 vértices
    
    Vec4 prev_vert = input_poly[2];
    // No View Space (OpenGL style), a câmera olha para -Z. Então "dentro" é Z <= -0.1
    bool prev_inside = (prev_vert.z <= Z_NEAR); 
    
    for(int i=0; i<3; i++) {
        Vec4 curr_vert = input_poly[i];
        bool curr_inside = (curr_vert.z <= Z_NEAR);
        
        if (curr_inside) {
            if (!prev_inside) {
                // Entrando na tela: Calcula ponto de intersecção na borda
                float t = (Z_NEAR - prev_vert.z) / (curr_vert.z - prev_vert.z);
                output_poly.push_back(lerp_vertex(prev_vert, curr_vert, t));
            }
            output_poly.push_back(curr_vert); // Vértice válido
        } else if (prev_inside) {
            // Saindo da tela: Calcula ponto de intersecção e descarta o resto
            float t = (Z_NEAR - prev_vert.z) / (curr_vert.z - prev_vert.z);
            output_poly.push_back(lerp_vertex(prev_vert, curr_vert, t));
        }
        
        prev_vert = curr_vert;
        prev_inside = curr_inside;
    }
    
    // Triangulação: Se o recorte gerou um quadrado (4 pontos), dividimos em 2 triângulos
    if (output_poly.size() < 3) return; 
    
    out_poly.push_back(output_poly[0]);
    out_poly.push_back(output_poly[1]);
    out_poly.push_back(output_poly[2]);
    
    if (output_poly.size() == 4) {
        out_poly.push_back(output_poly[0]);
        out_poly.push_back(output_poly[2]);
        out_poly.push_back(output_poly[3]);
    }
}

// ==========================================
//   RASTERIZAÇÃO (SCANLINE)
// ==========================================

// Rasterizador PHONG: Interpola normais e posições para calcular a luz em CADA pixel.
inline void fill_phong(int x1, int y1, float z1, Vec4 w1, 
                       int x2, int y2, float z2, Vec4 w2, 
                       int x3, int y3, float z3, Vec4 w3, 
                       Vec4 n, Cubo cubo, Vec4 lightPos, Vec4 camPos, 
                       std::vector<uint32_t>& fb, std::vector<float>& zb, 
                       int vpw, int vph, int vpx, int vpy,
                       Vec3 lightColor, Vec3 ambientColor) {
    
    // 1. Ordenação dos vértices por Y (Bubble sort simples) para varredura vertical
    if (y1>y2) { swap_int(x1,x2); swap_int(y1,y2); swap_float(z1,z2); swap_vec4(w1,w2); }
    if (y1>y3) { swap_int(x1,x3); swap_int(y1,y3); swap_float(z1,z3); swap_vec4(w1,w3); }
    if (y2>y3) { swap_int(x2,x3); swap_int(y2,y3); swap_float(z2,z3); swap_vec4(w2,w3); }
    
    int h = y3 - y1; if (h == 0) return;

    // 2. Loop de Scanline (Linha a linha)
    for (int i = 0; i < h; i++) {
        int y = y1 + i;
        
        // Verifica se estamos na metade superior ou inferior do triângulo
        bool lower = i > (y2 - y1) || (y2 == y1);
        int seg_h = lower ? (y3 - y2) : (y2 - y1);
        
        // Pesos de interpolação vertical
        float alpha = (float)i / h;
        float beta = (float)(i - (lower ? (y2 - y1) : 0)) / seg_h;
        
        // Interpolação das bordas (X, Z e Vetores World Space)
        int ax = interp(x1, x3, alpha);
        int bx = lower ? interp(x2, x3, beta) : interp(x1, x2, beta);
        float az = interp(z1, z3, alpha);
        float bz = lower ? interp(z2, z3, beta) : interp(z1, z2, beta);
        Vec4 aw = interp_vec(w1, w3, alpha);
        Vec4 bw = lower ? interp_vec(w2, w3, beta) : interp_vec(w1, w2, beta);
        
        // Garante scan da esquerda para direita
        if (ax > bx) { swap_int(ax, bx); swap_float(az, bz); swap_vec4(aw, bw); }
        
        // 3. Loop Horizontal (Pixel a Pixel)
        for (int x = ax; x <= bx; x++) {
            // Scissor Test: Respeita a área do Viewport e da Janela
            if (x < vpx || x >= vpx+vpw || y < vpy || y >= vpy+vph) continue;
            if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) continue;
            
            float phi = (bx == ax) ? 1.0f : (float)(x - ax) / (bx - ax);
            float z = interp(az, bz, phi);
            
            int idx = y * SCREEN_W + x;
            
            // Teste de Profundidade (Z-Buffer)
            if (z < zb[idx]) {
                zb[idx] = z;
                // Interpolação da posição real no mundo para cálculo da luz
                Vec4 p = interp_vec(aw, bw, phi);
                fb[idx] = calc_luz_rgb(p, n, cubo, lightPos, camPos, lightColor, ambientColor);
            }
        }
    }
}

// Rasterizador FLAT: Cor constante calculada uma vez por triângulo (mais rápido).
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
            if (z < zb[idx]) { zb[idx] = z; fb[idx] = c; }
        }
    }
}

#endif