#include "math_utils.h"
#include <SDL2/SDL.h>
#include <iostream>
#include <vector>

// Configurações da Tela
const int LARGURA = 800;
const int ALTURA = 600;

// O seu "Canvas" na memória (Framebuffer)
// Usamos uint32_t para armazenar cores no formato ARGB (Alpha, Red, Green, Blue)
std::vector<uint32_t> framebuffer(LARGURA * ALTURA);

// Função auxiliar para pintar um pixel (x, y) com uma cor (r, g, b)
void put_pixel(int x, int y, uint8_t r, uint8_t g, uint8_t b) {
    if (x >= 0 && x < LARGURA && y >= 0 && y < ALTURA) {
        // Converte R, G, B para um único inteiro de 32 bits
        uint32_t color = (255 << 24) | (r << 16) | (g << 8) | b;
        framebuffer[y * LARGURA + x] = color;
    }
}

// Função para limpar a tela (pintar tudo de preto ou cinza)
void clear_screen() {
    std::fill(framebuffer.begin(), framebuffer.end(), 0xFF333333); // Cinza escuro
}

int main(int argc, char* argv[]) {
    // 1. Inicializa a SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        std::cerr << "Erro ao iniciar SDL: " << SDL_GetError() << std::endl;
        return 1;
    }

    // 2. Cria a janela e o renderizador
    SDL_Window* window = SDL_CreateWindow("Trabalho CG - 2025", 
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 
                                          LARGURA, ALTURA, SDL_WINDOW_SHOWN);
    
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    
    // 3. Cria a textura (o link entre seu array e a placa de vídeo)
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, 
                                             SDL_TEXTUREACCESS_STREAMING, LARGURA, ALTURA);

    bool running = true;
    SDL_Event event;

    // Loop Principal
    while (running) {
        // Processa eventos (teclado, fechar janela)
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) running = false;
            // Aqui você vai adicionar os controles de câmera depois (Requisito 6 e 10)
        }

        
        clear_screen();

        /* Teste linha vermelha 
        for(int i = 100; i < 400; i++) {
            put_pixel(i, i, 255, 0, 0); // Linha diagonal vermelha
        }*/

       // 1. Definir um ponto no espaço 3D (ex: um vértice de um cubo)
        Vec4 ponto_local(1.0f, 1.0f, 1.0f); // Ponto na posição 1,1,1

        // 2. Transformações (Modelo) - Vamos girar e afastar o ponto
        static float angulo = 0.0f;
        angulo += 0.05f; // Gira um pouco a cada frame

        Mat4 matriz_rotacao = rotateY(angulo);       // Gira no eixo Y
        Mat4 matriz_translacao = translate(0, 0, 5); // Empurra para o fundo (Z=5)
        Mat4 matriz_escala = scale(100.0f);          // Aumenta para vermos na tela

        // Ordem: Escala -> Rotação -> Translação
        Mat4 model = matriz_translacao * matriz_rotacao * matriz_escala;

        // 3. Aplicar a transformação
        Vec4 ponto_transformado = model * ponto_local;

        // 4. Projeção Perspectiva Simplificada (apenas para teste rápido)
        // x_tela = x / z + centro_tela
        // y_tela = y / z + centro_tela
        if (ponto_transformado.z != 0) {
            int x_tela = (int)(ponto_transformado.x / ponto_transformado.z * 500) + LARGURA / 2;
            int y_tela = (int)(ponto_transformado.y / ponto_transformado.z * 500) + ALTURA / 2;

    // Desenha um quadrado branco onde o ponto está
    for(int w=0; w<5; w++)
        for(int h=0; h<5; h++)
            put_pixel(x_tela+w, y_tela+h, 255, 255, 255);
}
        


        // Atualiza a textura com os pixels do seu framebuffer e joga na tela
        SDL_UpdateTexture(texture, nullptr, framebuffer.data(), LARGURA * sizeof(uint32_t));
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);
    }

    // Limpeza
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}