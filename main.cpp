#include <SDL2/SDL.h>
#include <vector>
#include <cmath>
#include <algorithm> // Para std::abs
#include "math_utils.h" // Certifique-se que este arquivo está na mesma pasta

const int LARGURA = 800;
const int ALTURA = 600;
const int TARGET_FPS = 60;
const int FRAME_DELAY = 1000 / TARGET_FPS;

// Framebuffer e Z-Buffer
std::vector<uint32_t> framebuffer(LARGURA * ALTURA);
std::vector<float> zbuffer(LARGURA * ALTURA);

// Estruturas
struct Cubo {
    Vec4 posicao;
    Vec4 rotacao;
    Vec4 escala;
    uint32_t cor;
};

// Dados Geométricos
Vec4 vertices_cubo[8] = {
    {-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1}, {-1,  1, -1}, // Frente
    {-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1,  1,  1}  // Trás
};

int indices_triangulos[12][3] = {
    {0, 1, 2}, {0, 2, 3}, // Frente
    {5, 4, 7}, {5, 7, 6}, // Trás
    {3, 2, 6}, {3, 6, 7}, // Topo
    {4, 5, 1}, {4, 1, 0}, // Base
    {4, 0, 3}, {4, 3, 7}, // Esquerda
    {1, 5, 6}, {1, 6, 2}  // Direita
};

// Função para pintar pixel com Z-Buffer
void put_pixel(int x, int y, float z, uint32_t color) {
    // CORREÇÃO: A lógica anterior (x < LARGURA) impedia o desenho. O correto é >=
    if (x < 0 || x >= LARGURA || y < 0 || y >= ALTURA) {
        return;
    }
    int index = y * LARGURA + x;
    
    // Teste de profundidade: Só pinta se o Z novo for menor que o Z atual
    if (z < zbuffer[index]) {
        framebuffer[index] = color;
        zbuffer[index] = z;
    }
}

// Algoritmo de Bresenham (Atualizado para compilar, Z fixo em 0.0f)
void draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    
    while (true) {
        put_pixel(x0, y0, 0.0f, color); 
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// Utilitários
void swap_int(int& a, int& b) { int t = a; a = b; b = t; }
void swap_float(float& a, float& b) { float t = a; a = b; b = t; }

float interpolate(float min, float max, float factor) {
    return min + (max - min) * factor;
}

// Rasterização de Triângulo (Scanline com Z-Buffer)
void fill_triangle(int x1, int y1, float z1, int x2, int y2, float z2, int x3, int y3, float z3, uint32_t color) {
    // Ordenar os vértices por Y
    if (y1 > y2) { swap_int(x1, x2); swap_int(y1, y2); swap_float(z1, z2); }
    if (y1 > y3) { swap_int(x1, x3); swap_int(y1, y3); swap_float(z1, z3); }
    if (y2 > y3) { swap_int(x2, x3); swap_int(y2, y3); swap_float(z2, z3); }

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

        // CORREÇÃO: Adicionadas chaves {}. Sem elas, o swap_float rodava sempre!
        if (Ax > Bx) { 
            swap_int(Ax, Bx); 
            swap_float(Az, Bz); 
        }

        for (int x = Ax; x <= Bx; x++) {
            float phi = (Bx == Ax) ? 1.0f : (float)(x - Ax) / (float)(Bx - Ax);
            float z = interpolate(Az, Bz, phi);
            put_pixel(x, y1 + i, z, color);
        }
    }
} 

// Função para intensidade da luz
uint32_t apply_light(uint32_t color, float intensity) {
    if (intensity < 0.2f) intensity = 0.2f;
    if (intensity > 1.0f) intensity = 1.0f;

    uint8_t a = (color >> 24) & 0xFF;
    uint8_t r = (color >> 16) & 0xFF;
    uint8_t g = (color >> 8) & 0xFF;
    uint8_t b = color & 0xFF;

    r = static_cast<uint8_t>(r * intensity);
    g = static_cast<uint8_t>(g * intensity);
    b = static_cast<uint8_t>(b * intensity);

    return (a << 24) | (r << 16) | (g << 8) | b;
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Trabalho CG - ZBuffer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, LARGURA, ALTURA, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, LARGURA, ALTURA);

    bool running = true;
    
    std::vector<Cubo> cena;

    // Cubo Vermelho
    Cubo c1;
    c1.posicao = Vec4(-1.5f, 0, -6);
    c1.rotacao = Vec4(0.5f, 0.5f, 0);
    c1.escala = Vec4(1, 1, 1);
    c1.cor = 0xFFFF0000; 
    cena.push_back(c1);

    // Cubo Verde
    Cubo c2;
    c2.posicao = Vec4(0.5f, 0, -5); 
    c2.rotacao = Vec4(0, 0.2f, 0);
    c2.escala = Vec4(1, 1, 1);
    c2.cor = 0xFF00FF00; 
    cena.push_back(c2);

    while (running) {
        Uint32 frameStart = SDL_GetTicks();
        SDL_Event event;

        while (SDL_PollEvent(&event)) if (event.type == SDL_QUIT) running = false;

        // Limpa Framebuffer (Preto) e Z-Buffer (Infinito)
        std::fill(framebuffer.begin(), framebuffer.end(), 0xFF333333);
        std::fill(zbuffer.begin(), zbuffer.end(), 1000.0f);

        Mat4 proj = perspective(1.04f, (float)LARGURA/ALTURA, 0.1f, 100.0f);

        for (const auto& cubo : cena) {
            // 1. Matriz de Modelo (World Matrix)
            Mat4 model = translate(cubo.posicao.x, cubo.posicao.y, cubo.posicao.z) * rotateX(cubo.rotacao.x) * rotateY(cubo.rotacao.y) * rotateZ(cubo.rotacao.z) *
                         scale(cubo.escala.x);
            
            // Matriz MVP (Model View Projection) para desenhar na tela
            Mat4 mvp = proj * model;

            // Definir uma Luz (Vindo da direita e de cima)
            Vec4 luz_dir(0.5f, 0.5f, -1.0f); 
            luz_dir.normalize();

            // Transformar vértices para:
            // a) Espaço de Tela (proj_verts) - Para saber ONDE desenhar
            // b) Espaço de Mundo (world_verts) - Para saber COMO iluminar
            Vec4 proj_verts[8];
            Vec4 world_verts[8];

            for(int i=0; i<8; i++) {
                // a) Tela
                Vec4 v = mvp * vertices_cubo[i];
                if (v.w != 0) { v.x /= v.w; v.y /= v.w; v.z /= v.w; } // Divisão Perspectiva
                proj_verts[i] = v;

                // b) Mundo (apenas rotação/translação, sem projeção)
                world_verts[i] = model * vertices_cubo[i];
            }

            // Desenhar Triângulos
            for(int i=0; i<12; i++) {
                // Índices dos vértices deste triângulo
                int idx1 = indices_triangulos[i][0];
                int idx2 = indices_triangulos[i][1];
                int idx3 = indices_triangulos[i][2];

                Vec4 p1 = proj_verts[idx1];
                Vec4 p2 = proj_verts[idx2];
                Vec4 p3 = proj_verts[idx3];

                // Clipping simples de Z (se está atrás da câmera)
                if (p1.z <= 0 || p2.z <= 0 || p3.z <= 0) continue;

                // --- CÁLCULO DE ILUMINAÇÃO (FLAT SHADING) ---
                // Usamos os vértices no MUNDO para calcular a normal real da face
                Vec4 v1_w = world_verts[idx1];
                Vec4 v2_w = world_verts[idx2];
                Vec4 v3_w = world_verts[idx3];

                // 1. Vetores das arestas do triângulo
                Vec4 aresta1 = v2_w - v1_w;
                Vec4 aresta2 = v3_w - v1_w;

                // 2. Produto Vetorial para achar a Normal (perpendicular à face)
                Vec4 normal = aresta1.cross(aresta2);
                normal.normalize();

                // 3. Back-face Culling (Opcional mas recomendado nas notas do prof)
                // Se a normal aponta para longe da câmera, não desenha.
                // Vetor visão (simplificado como oposto da posição Z, pois câmera está em 0,0,0 olhando p/ -Z)
                Vec4 view_dir = v1_w * -1.0f; 
                // if (normal.dot(view_dir) <= 0) continue; // Descomente para ativar culling

                // 4. Intensidade da Luz (Produto Escalar: Normal . Luz)
                // Quanto mais alinhados, mais forte a luz.
                float intensity = normal.dot(luz_dir);
                
                // Calcula cor final da face
                uint32_t cor_face = apply_light(cubo.cor, intensity);
                // ----------------------------------------------

                // Conversão para Coordenadas de Tela (Viewport)
                int x1 = (int)((p1.x + 1.0f) * 0.5f * LARGURA);
                int y1 = (int)((1.0f - p1.y) * 0.5f * ALTURA);
                int x2 = (int)((p2.x + 1.0f) * 0.5f * LARGURA);
                int y2 = (int)((1.0f - p2.y) * 0.5f * ALTURA);
                int x3 = (int)((p3.x + 1.0f) * 0.5f * LARGURA);
                int y3 = (int)((1.0f - p3.y) * 0.5f * ALTURA);

                // Desenha usando a cor calculada pela luz
                fill_triangle(x1, y1, p1.w, x2, y2, p2.w, x3, y3, p3.w, cor_face);
            }
        }

        SDL_UpdateTexture(texture, nullptr, framebuffer.data(), LARGURA * sizeof(uint32_t));
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        int frameTime = SDL_GetTicks() - frameStart;
        if (FRAME_DELAY > frameTime) SDL_Delay(FRAME_DELAY - frameTime);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}