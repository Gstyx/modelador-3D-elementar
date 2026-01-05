#include <SDL2/SDL.h>
#include <vector>
#include <cmath>
#include "math_utils.h"

const int LARGURA = 800;
const int ALTURA = 600;
const int TARGET_FPS = 60;
const int FRAME_DELAY = 1000 / TARGET_FPS; 
std::vector<uint32_t> framebuffer(LARGURA * ALTURA);

// Função para pintar pixel (com verificação de limites)
void put_pixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < LARGURA && y >= 0 && y < ALTURA) {
        framebuffer[y * LARGURA + x] = color;
    }
}

// Algoritmo de Bresenham para desenhar linhas (Rasterização básica)
void draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = std::abs(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int dy = -std::abs(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int err = dx + dy, e2;
    
    while (true) {
        put_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        e2 = 2 * err;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

// Definição do Cubo (Coordenadas locais, centrado em 0,0,0)
// 8 Vértices
Vec4 vertices_cubo[8] = {
    {-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1}, {-1,  1, -1}, // Frente
    {-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1,  1,  1}  // Trás
};

// 12 Arestas (Índices dos vértices que se conectam)
int arestas[12][2] = {
    {0,1}, {1,2}, {2,3}, {3,0}, // Frente
    {4,5}, {5,6}, {6,7}, {7,4}, // Trás
    {0,4}, {1,5}, {2,6}, {3,7}  // Conexões Frente-Trás
};

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Trabalho CG - Wireframe", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, LARGURA, ALTURA, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, LARGURA, ALTURA);

    bool running = true;
    float angulo = 0.0f;

    while (running) {
        Uint32 frameStart = SDL_GetTicks();
        SDL_Event event;

        while (SDL_PollEvent(&event)) if (event.type == SDL_QUIT) running = false;

        // Limpa tela (Preto)
        std::fill(framebuffer.begin(), framebuffer.end(), 0xFF000000);

        // --- PIPELINE GRÁFICO ---
        
        // 1. Configuração das Matrizes
        angulo += 0.02f;
        Mat4 model = translate(0, 0, -5) * rotateY(angulo) * rotateX(angulo * 0.5f); // Move p/ fundo e gira
        Mat4 proj = perspective(1.04f, (float)LARGURA/ALTURA, 0.1f, 100.0f); // 1.04 rad ~= 60 graus
        
        // Matriz Final (ModelViewProjection)
        Mat4 mvp = proj * model;

        // 2. Processar Vértices
        Vec4 vertices_proj[8];
        for(int i=0; i<8; i++) {
            // Aplica MVP
            Vec4 v = mvp * vertices_cubo[i];
            
            // Divisão Perspectiva (Perspectiva acontece aqui!)
            // Passamos de Homogêneo 4D para Cartesiano 3D Normalizado (NDC)
            if (v.w != 0) {
                v.x /= v.w; v.y /= v.w; v.z /= v.w;
            }
            vertices_proj[i] = v;
        }

        // 3. Desenhar Arestas (Viewport Transform na hora de desenhar)
        for(int i=0; i<12; i++) {
            Vec4 p1 = vertices_proj[arestas[i][0]];
            Vec4 p2 = vertices_proj[arestas[i][1]];

            // O intervalo visível em NDC é [-1, 1]
            if (p1.z < -1.0f || p1.z > 1.0f || p2.z < -1.0f || p2.z > 1.0f) continue;

            // Viewport: Mapear de [-1, 1] para [0, LARGURA] e [0, ALTURA]
            int x1_tela = (int)((p1.x + 1.0f) * 0.5f * LARGURA);
            int y1_tela = (int)((1.0f - p1.y) * 0.5f * ALTURA); // 1.0 - y inverte o eixo Y (tela cresce p/ baixo)
            
            int x2_tela = (int)((p2.x + 1.0f) * 0.5f * LARGURA);
            int y2_tela = (int)((1.0f - p2.y) * 0.5f * ALTURA);

            draw_line(x1_tela, y1_tela, x2_tela, y2_tela, 0xFF00FF00); // Verde Matrix
        }

        SDL_UpdateTexture(texture, nullptr, framebuffer.data(), LARGURA * sizeof(uint32_t));
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
        int frameTime = SDL_GetTicks() - frameStart;
        
        if (FRAME_DELAY > frameTime) {
            SDL_Delay(FRAME_DELAY - frameTime);
        }
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}