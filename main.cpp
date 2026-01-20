/**
 * TRABALHO FINAL DE COMPUTAÇÃO GRÁFICA - UNIOESTE 2025
 * Implementação: Pipeline Gráfico de Alvy Ray Smith
 */

#include <SDL2/SDL.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>

// --- CONFIGURAÇÕES GERAIS ---
const int LARGURA = 800;
const int ALTURA = 600;
const int TARGET_FPS = 60;
const int FRAME_DELAY = 1000 / TARGET_FPS;

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

// Fábrica de Matrizes
Mat4 translate(float tx, float ty, float tz) {
    Mat4 mat;
    mat.m[0][3] = tx; mat.m[1][3] = ty; mat.m[2][3] = tz;
    return mat;
}

Mat4 scale(float s) {
    Mat4 mat;
    mat.m[0][0] = s; mat.m[1][1] = s; mat.m[2][2] = s;
    return mat;
}

Mat4 rotateX(float angle) {
    Mat4 mat;
    float c = std::cos(angle), s = std::sin(angle);
    mat.m[1][1] = c; mat.m[1][2] = -s;
    mat.m[2][1] = s; mat.m[2][2] = c;
    return mat;
}

Mat4 rotateY(float angle) {
    Mat4 mat;
    float c = std::cos(angle), s = std::sin(angle);
    mat.m[0][0] = c; mat.m[0][2] = s;
    mat.m[2][0] = -s; mat.m[2][2] = c;
    return mat;
}

Mat4 rotateZ(float angle) {
    Mat4 mat;
    float c = std::cos(angle), s = std::sin(angle);
    mat.m[0][0] = c; mat.m[0][1] = -s;
    mat.m[1][0] = s; mat.m[1][1] = c;
    return mat;
}

Mat4 perspective(float fov, float aspect, float near, float far) {
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

// --- ESTRUTURAS DE DADOS DA CENA ---

struct Cubo {
    Vec4 posicao;
    Vec4 rotacao;
    Vec4 escala;
    uint32_t cor_base;

    // Material (Phong)
    float ka = 0.2f;    // Ambiente
    float kd = 0.7f;    // Difusa
    float ks = 0.8f;    // Especular
    float shininess = 50.0f; // Brilho
};

// Buffers Globais
std::vector<uint32_t> framebuffer(LARGURA * ALTURA);
std::vector<float> zbuffer(LARGURA * ALTURA);

// Geometria Padrão do Cubo
Vec4 vertices_cubo[8] = {
    {-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1}, {-1,  1, -1}, // Frente (Z-)
    {-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1,  1,  1}  // Trás (Z+)
};

int indices_triangulos[12][3] = {
    {0, 1, 2}, {0, 2, 3}, // Frente
    {5, 4, 7}, {5, 7, 6}, // Trás
    {3, 2, 6}, {3, 6, 7}, // Topo
    {4, 5, 1}, {4, 1, 0}, // Base
    {4, 0, 3}, {4, 3, 7}, // Esquerda
    {1, 5, 6}, {1, 6, 2}  // Direita
};

// --- VARIÁVEIS DE CONTROLE E ESTADO ---
enum Modo { M_OBJETO, M_LUZ, M_CAMERA, M_MATERIAL };
Modo modo_atual = M_OBJETO;
const char* nomes_modos[] = { "OBJETO (WASD+Setas)", "LUZ (Posicao)", "CAMERA (Posicao/Zoom)", "MATERIAL (Ka, Kd, Ks)" };

Vec4 g_cam_pos(0, 0, 0);       // Câmera
float g_fov = 1.04f;           // Field of View
Vec4 g_light_pos(2.0f, 3.0f, -5.0f); // Luz pontual

// --- FUNÇÕES DE RASTERIZAÇÃO E ILUMINAÇÃO ---

uint8_t clamp(float val) {
    return (val > 255) ? 255 : (val < 0 ? 0 : (uint8_t)val);
}

// Shader de Pixel (Cálculo de Iluminação Phong Simplificado)
uint32_t calculate_phong_pixel(Vec4 fragPos, Vec4 normal, Vec4 lightPos, Vec4 viewPos, uint32_t baseColor, Cubo mat) {
    // Extrai RGB
    uint8_t r = (baseColor >> 16) & 0xFF;
    uint8_t g = (baseColor >> 8) & 0xFF;
    uint8_t b = baseColor & 0xFF;

    // Normaliza vetores
    Vec4 N = normal; N.normalize();
    Vec4 L = lightPos - fragPos; L.normalize();
    Vec4 V = viewPos - fragPos; V.normalize();

    // Reflexo: R = 2(N.L)N - L
    float n_dot_l = std::max(0.0f, N.dot(L));
    Vec4 R = (N * (2.0f * n_dot_l)) - L;
    R.normalize();

    // Modelo Phong: Ambiente + Difusa + Especular
    float ambient = mat.ka;
    float diffuse = mat.kd * n_dot_l;
    float specular = 0.0f;
    
    if (n_dot_l > 0.0f) {
        float r_dot_v = std::max(0.0f, R.dot(V));
        specular = mat.ks * std::pow(r_dot_v, mat.shininess);
    }

    // A cor especular geralmente é branca, somada ao final
    float intensity = ambient + diffuse;
    
    int fr = (int)(r * intensity + specular * 255);
    int fg = (int)(g * intensity + specular * 255);
    int fb = (int)(b * intensity + specular * 255);

    return (255 << 24) | (clamp(fr) << 16) | (clamp(fg) << 8) | clamp(fb);
}

// Utilitários de Interpolação
void swap_int(int& a, int& b) { int t = a; a = b; b = t; }
void swap_float(float& a, float& b) { float t = a; a = b; b = t; }
void swap_vec4(Vec4& a, Vec4& b) { Vec4 t = a; a = b; b = t; }

float interpolate(float min, float max, float factor) { return min + (max - min) * factor; }
Vec4 interpolate_vec(Vec4 min, Vec4 max, float factor) { return min + (max - min) * factor; }

// Scanline Rasterizer com Z-Buffer e Interpolação de Perspectiva
void fill_triangle_phong(int x1, int y1, float z1, Vec4 w1, 
                         int x2, int y2, float z2, Vec4 w2, 
                         int x3, int y3, float z3, Vec4 w3, 
                         Vec4 normal_face, Cubo material) {
    
    // Ordenar vértices por Y
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
            
            // 1. Z-Buffer Check
            float z = interpolate(Az, Bz, phi);
            
            if (x >= 0 && x < LARGURA && (y1+i) >= 0 && (y1+i) < ALTURA) {
                int idx = (y1 + i) * LARGURA + x;
                if (z < zbuffer[idx]) {
                    zbuffer[idx] = z;

                    // 2. Interpolação da Posição do Pixel no Mundo
                    Vec4 pixel_pos_world = interpolate_vec(Aw, Bw, phi);

                    // 3. Aplicação do Modelo Phong
                    uint32_t cor = calculate_phong_pixel(pixel_pos_world, normal_face, g_light_pos, g_cam_pos, material.cor_base, material);
                    
                    framebuffer[idx] = cor;
                }
            }
        }
    }
}

// --- MAIN ---

int main(int argc, char* argv[]) {
    // 1. Inicialização SDL
    if(SDL_Init(SDL_INIT_VIDEO) < 0) return 1;
    SDL_Window* window = SDL_CreateWindow("Modelador 3D - Unioeste 2025 (Phong + ZBuffer)", 
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                          LARGURA, ALTURA, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, 
                                             SDL_TEXTUREACCESS_STREAMING, LARGURA, ALTURA);

    // 2. Configuração da Cena
    std::vector<Cubo> cena;
    
    // Cubo Vermelho
    Cubo c1;
    c1.posicao = Vec4(-1.2f, 0, -5);
    c1.rotacao = Vec4(0.5f, 0.6f, 0);
    c1.escala = Vec4(1, 1, 1);
    c1.cor_base = 0xFFFF0000;
    cena.push_back(c1);

    // Cubo Verde
    Cubo c2;
    c2.posicao = Vec4(0.8f, 0, -4.5f);
    c2.rotacao = Vec4(0, -0.3f, 0);
    c2.escala = Vec4(1, 1, 1);
    c2.cor_base = 0xFF00FF00;
    c2.shininess = 100.0f; // Mais polido
    cena.push_back(c2);

    int cubo_idx = 0;
    bool running = true;
    
    // Mostra instruções iniciais no terminal
    std::cout << "--- TRABALHO COMPUTACAO GRAFICA 2025 ---" << std::endl;
    std::cout << "TAB: Alterna Modos (Objeto -> Luz -> Camera -> Material)" << std::endl;
    std::cout << "ESPACO: Alterna Cubo Selecionado" << std::endl;

    while (running) {
        Uint32 frameStart = SDL_GetTicks();
        SDL_Event event;

        // 3. Gerenciamento de Input
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            
            if (event.type == SDL_KEYDOWN) {
                float step = 0.2f;
                float rot_step = 0.1f;
                float mat_step = 0.05f;

                switch (event.key.keysym.sym) {
                    case SDLK_TAB:
                        modo_atual = (Modo)((modo_atual + 1) % 4);
                        std::cout << "\n[MODO] " << nomes_modos[modo_atual] << std::endl;
                        break;

                    case SDLK_SPACE:
                        cubo_idx = (cubo_idx + 1) % cena.size();
                        std::cout << "-> Cubo Selecionado: " << cubo_idx << std::endl;
                        break;

                    default:
                        if (modo_atual == M_OBJETO) {
                            if(event.key.keysym.sym == SDLK_w) cena[cubo_idx].posicao.y += step;
                            if(event.key.keysym.sym == SDLK_s) cena[cubo_idx].posicao.y -= step;
                            if(event.key.keysym.sym == SDLK_a) cena[cubo_idx].posicao.x -= step;
                            if(event.key.keysym.sym == SDLK_d) cena[cubo_idx].posicao.x += step;
                            if(event.key.keysym.sym == SDLK_q) cena[cubo_idx].posicao.z += step;
                            if(event.key.keysym.sym == SDLK_e) cena[cubo_idx].posicao.z -= step;
                            if(event.key.keysym.sym == SDLK_UP) cena[cubo_idx].rotacao.x -= rot_step;
                            if(event.key.keysym.sym == SDLK_DOWN) cena[cubo_idx].rotacao.x += rot_step;
                            if(event.key.keysym.sym == SDLK_LEFT) cena[cubo_idx].rotacao.y -= rot_step;
                            if(event.key.keysym.sym == SDLK_RIGHT) cena[cubo_idx].rotacao.y += rot_step;
                            if(event.key.keysym.sym == SDLK_KP_PLUS) cena[cubo_idx].escala.x += 0.1f;
                            if(event.key.keysym.sym == SDLK_KP_MINUS) if(cena[cubo_idx].escala.x>0.1) cena[cubo_idx].escala.x -= 0.1f;
                        }
                        else if (modo_atual == M_LUZ) {
                            if(event.key.keysym.sym == SDLK_w) g_light_pos.y += step;
                            if(event.key.keysym.sym == SDLK_s) g_light_pos.y -= step;
                            if(event.key.keysym.sym == SDLK_a) g_light_pos.x -= step;
                            if(event.key.keysym.sym == SDLK_d) g_light_pos.x += step;
                            if(event.key.keysym.sym == SDLK_q) g_light_pos.z += step;
                            if(event.key.keysym.sym == SDLK_e) g_light_pos.z -= step;
                            std::cout << "Luz: " << g_light_pos.x << " " << g_light_pos.y << " " << g_light_pos.z << std::endl;
                        }
                        else if (modo_atual == M_CAMERA) {
                            if(event.key.keysym.sym == SDLK_w) g_cam_pos.y += step;
                            if(event.key.keysym.sym == SDLK_s) g_cam_pos.y -= step;
                            if(event.key.keysym.sym == SDLK_a) g_cam_pos.x -= step;
                            if(event.key.keysym.sym == SDLK_d) g_cam_pos.x += step;
                            if(event.key.keysym.sym == SDLK_q) g_cam_pos.z += step;
                            if(event.key.keysym.sym == SDLK_e) g_cam_pos.z -= step;
                            if(event.key.keysym.sym == SDLK_UP) g_fov = std::max(0.1f, g_fov - 0.05f); // Zoom In
                            if(event.key.keysym.sym == SDLK_DOWN) g_fov += 0.05f; // Zoom Out
                        }
                        else if (modo_atual == M_MATERIAL) {
                            if(event.key.keysym.sym == SDLK_1) cena[cubo_idx].ka = std::min(1.0f, cena[cubo_idx].ka + mat_step);
                            if(event.key.keysym.sym == SDLK_2) cena[cubo_idx].ka = std::max(0.0f, cena[cubo_idx].ka - mat_step);
                            if(event.key.keysym.sym == SDLK_3) cena[cubo_idx].kd = std::min(1.0f, cena[cubo_idx].kd + mat_step);
                            if(event.key.keysym.sym == SDLK_4) cena[cubo_idx].kd = std::max(0.0f, cena[cubo_idx].kd - mat_step);
                            if(event.key.keysym.sym == SDLK_5) cena[cubo_idx].ks = std::min(1.0f, cena[cubo_idx].ks + mat_step);
                            if(event.key.keysym.sym == SDLK_6) cena[cubo_idx].ks = std::max(0.0f, cena[cubo_idx].ks - mat_step);
                            if(event.key.keysym.sym == SDLK_7) cena[cubo_idx].shininess += 5.0f;
                            if(event.key.keysym.sym == SDLK_8) cena[cubo_idx].shininess = std::max(1.0f, cena[cubo_idx].shininess - 5.0f);
                            std::cout << "Mat: Ka=" << cena[cubo_idx].ka << " Kd=" << cena[cubo_idx].kd << " Ks=" << cena[cubo_idx].ks << std::endl;
                        }
                        break;
                }
            }
        }

        // 4. Renderização (Pipeline)
        std::fill(framebuffer.begin(), framebuffer.end(), 0xFF222222); // Fundo cinza escuro
        std::fill(zbuffer.begin(), zbuffer.end(), 1000.0f);            // Limpa Z-Buffer

        // Matrizes Globais
        // Projeção
        Mat4 proj = perspective(g_fov, (float)LARGURA/ALTURA, 0.1f, 100.0f);
        // Câmera (View Matrix)
        Mat4 view = translate(-g_cam_pos.x, -g_cam_pos.y, -g_cam_pos.z);

        for (const auto& cubo : cena) {
            // Matriz Modelo
            Mat4 model_local = translate(cubo.posicao.x, cubo.posicao.y, cubo.posicao.z) * rotateX(cubo.rotacao.x) * rotateY(cubo.rotacao.y) * rotateZ(cubo.rotacao.z) *
                               scale(cubo.escala.x);

            // MVP = Projection * View * Model
            Mat4 mvp = proj * view * model_local;

            // Pipeline de Vértices
            Vec4 world_verts[8];
            Vec4 proj_verts[8];

            for(int i=0; i<8; i++) {
                // Posição no mundo (Para iluminação)
                world_verts[i] = model_local * vertices_cubo[i];
                
                // Posição projetada (Para tela)
                Vec4 v = mvp * vertices_cubo[i];
                if (v.w != 0) { v.x /= v.w; v.y /= v.w; v.z /= v.w; }
                proj_verts[i] = v;
            }

            // Pipeline de Faces (Triângulos)
            for(int i=0; i<12; i++) {
                Vec4 p1 = proj_verts[indices_triangulos[i][0]];
                Vec4 p2 = proj_verts[indices_triangulos[i][1]];
                Vec4 p3 = proj_verts[indices_triangulos[i][2]];

                // Clipping Z Simples
                if (p1.z <= 0 || p2.z <= 0 || p3.z <= 0) continue;

                // Cálculo da Normal
                Vec4 v1_w = world_verts[indices_triangulos[i][0]];
                Vec4 v2_w = world_verts[indices_triangulos[i][1]];
                Vec4 v3_w = world_verts[indices_triangulos[i][2]];
                
                Vec4 aresta1 = v2_w - v1_w;
                Vec4 aresta2 = v3_w - v1_w;
                Vec4 normal = aresta2.cross(aresta1); // Corrigido
                normal.normalize();

                // Back-face Culling
                // Vetor Visão no espaço do mundo: CameraPos - VertexPos
                Vec4 viewDir = g_cam_pos - v1_w; 
                if (normal.dot(viewDir) <= 0) continue;

                // Viewport Transform
                int x1 = (int)((p1.x + 1.0f) * 0.5f * LARGURA);
                int y1 = (int)((1.0f - p1.y) * 0.5f * ALTURA);
                int x2 = (int)((p2.x + 1.0f) * 0.5f * LARGURA);
                int y2 = (int)((1.0f - p2.y) * 0.5f * ALTURA);
                int x3 = (int)((p3.x + 1.0f) * 0.5f * LARGURA);
                int y3 = (int)((1.0f - p3.y) * 0.5f * ALTURA);

                // Rasterização
                fill_triangle_phong(x1, y1, p1.w, v1_w,
                                    x2, y2, p2.w, v2_w,
                                    x3, y3, p3.w, v3_w,
                                    normal, cubo);
            }
        }

        // 5. Apresentação
        SDL_UpdateTexture(texture, nullptr, framebuffer.data(), LARGURA * sizeof(uint32_t));
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        // Controle de FPS
        int frameTime = SDL_GetTicks() - frameStart;
        if (FRAME_DELAY > frameTime) SDL_Delay(FRAME_DELAY - frameTime);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}