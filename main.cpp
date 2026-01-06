#include <SDL2/SDL.h>
#include <vector>
#include <cmath>
#include "math_utils.h"

const int LARGURA = 800;
const int ALTURA = 600;
const int TARGET_FPS = 60;
const int FRAME_DELAY = 1000 / TARGET_FPS;
std::vector<uint32_t> framebuffer(LARGURA * ALTURA);

//Cubo
struct Cubo {
    Vec4 posicao;
    Vec4 rotacao;
    Vec4 escala;
    uint32_t cor;
};

//Dados do cubo (padrao)
Vec4 vertices_cubo[8] = {
    {-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1}, {-1,  1, -1}, // Frente
    {-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1,  1,  1}  // Trás
};

int indices_triangulos[12][3] = {
    // Frente
    {0, 1, 2}, {0, 2, 3},
    // Trás
    {5, 4, 7}, {5, 7, 6},
    // Topo
    {3, 2, 6}, {3, 6, 7},
    // Base
    {4, 5, 1}, {4, 1, 0},
    // Esquerda
    {4, 0, 3}, {4, 3, 7},
    // Direita
    {1, 5, 6}, {1, 6, 2}
};

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

void swap_int(int& a, int& b) { int t = a; a = b; b = t; }
void swap_float(float& a, float& b) { float t = a; a = b; b = t; }



void fill_triangle(int x1, int y1, int x2, int y2, int x3, int y3, uint32_t color) {
    //Ordenar os vértices por Y
    if (y1 > y2) { swap_int(x1, x2); swap_int(y1, y2); }
    if (y1 > y3) { swap_int(x1, x3); swap_int(y1, y3); }
    if (y2 > y3) { swap_int(x2, x3); swap_int(y2, y3); }

    // Altura total do triângulo
    int total_height = y3 - y1;
    if (total_height == 0) return;

    // 2. Dividir em duas metades (parte superior e inferior)
    // Loop para varrer de y1 até y3
    for (int i = 0; i < total_height; i++) {
        bool second_half = i > (y2 - y1) || (y2 == y1);
        int segment_height = second_half ? (y3 - y2) : (y2 - y1);
        
        float alpha = (float)i / total_height;
        float beta  = (float)(i - (second_half ? (y2 - y1) : 0)) / segment_height; 
        
        // Calcular coordenadas X das arestas
        // A: vai de 1 a 3 (lado longo)
        // B: vai de 1 a 2 (primeira metade) ou de 2 a 3 (segunda metade)
        int Ax = x1 + (x3 - x1) * alpha;
        int Bx = second_half ? (x2 + (x3 - x2) * beta) : (x1 + (x2 - x1) * beta);

        // Garantir que Ax < Bx para o loop horizontal
        if (Ax > Bx) swap_int(Ax, Bx);

        // 3. Preencher a linha horizontal (Scanline)
        int y = y1 + i;
        for (int x = Ax; x <= Bx; x++) {
            put_pixel(x, y, color);
        }
    }
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("Trabalho CG - Wireframe", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, LARGURA, ALTURA, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, LARGURA, ALTURA);

    bool running = true;
    
    std::vector<Cubo> cena;

    Cubo c1;
    c1.posicao = Vec4(-1.5f, 0, -6);
    c1.rotacao = Vec4(0.5f, 0.5f, 0);
    c1.escala = Vec4(1, 1, 1);
    c1.cor = 0xFFFF0000; // Vermelho
    cena.push_back(c1);

    Cubo c2;
    c2.posicao = Vec4(0.5f, 0, -5); // X=0.5 (mais a direita), Z=-5 (mais perto)
    c2.rotacao = Vec4(0, 0.2f, 0);
    c2.escala = Vec4(1, 1, 1);
    c2.cor = 0xFF00FF00; // Verde
    cena.push_back(c2);

    while (running) {
        Uint32 frameStart = SDL_GetTicks();
        SDL_Event event;

        while (SDL_PollEvent(&event)) if (event.type == SDL_QUIT) running = false;

        // Limpa tela (Preto)
        std::fill(framebuffer.begin(), framebuffer.end(), 0xFF000000);

        // Matriz de Projeção (Câmera fixa por enquanto)
        Mat4 proj = perspective(1.04f, (float)LARGURA/ALTURA, 0.1f, 100.0f);

        // Renderizar cada objeto da cena
        for (const auto& cubo : cena) {
            // 1. Criar Matriz de Modelo (Transformações do objeto)
            // Ordem: Escala -> Rotação Z -> Rot Y -> Rot X -> Translação
            Mat4 model = translate(cubo.posicao.x, cubo.posicao.y, cubo.posicao.z) * rotateX(cubo.rotacao.x) * rotateY(cubo.rotacao.y) * rotateZ(cubo.rotacao.z) *
                         scale(cubo.escala.x); // Assumindo escala uniforme
            
            Mat4 mvp = proj * model;

            // 2. Projetar Vértices
            Vec4 proj_verts[8];
            for(int i=0; i<8; i++) {
                Vec4 v = mvp * vertices_cubo[i];
                if (v.w != 0) { v.x /= v.w; v.y /= v.w; v.z /= v.w; }
                proj_verts[i] = v;
            }

            // 3. Desenhar Triângulos (Faces Sólidas)
            for(int i=0; i<12; i++) {
                Vec4 p1 = proj_verts[indices_triangulos[i][0]];
                Vec4 p2 = proj_verts[indices_triangulos[i][1]];
                Vec4 p3 = proj_verts[indices_triangulos[i][2]];

                // Culling Simples (Ignora se algum ponto estiver atrás da câmera)
                // O correto seria "clipar", mas para simplificar, se Z < 0 ignoramos
                if (p1.z <= 0 || p2.z <= 0 || p3.z <= 0) continue; 

                // Conversão para Coordenadas de Tela
                int x1 = (int)((p1.x + 1.0f) * 0.5f * LARGURA);
                int y1 = (int)((1.0f - p1.y) * 0.5f * ALTURA);
                
                int x2 = (int)((p2.x + 1.0f) * 0.5f * LARGURA);
                int y2 = (int)((1.0f - p2.y) * 0.5f * ALTURA);
                
                int x3 = (int)((p3.x + 1.0f) * 0.5f * LARGURA);
                int y3 = (int)((1.0f - p3.y) * 0.5f * ALTURA);

                // Desenha o triângulo preenchido com a cor do cubo
                fill_triangle(x1, y1, x2, y2, x3, y3, cubo.cor);
                
                // Opcional: Desenhar contorno preto para ver as divisões (wireframe sobreposto)
                // draw_line(x1, y1, x2, y2, 0xFF000000);
                // draw_line(x2, y2, x3, y3, 0xFF000000);
                // draw_line(x3, y3, x1, y1, 0xFF000000);
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