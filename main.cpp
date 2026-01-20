/**
 * TRABALHO FINAL DE COMPUTAÇÃO GRÁFICA 
 * * MODO DE USO:
 * - TAB: Alterna Modos (OBJETO -> LUZ -> CAMERA -> MATERIAL)
 * - WASD / SETAS: Movem ou rotacionam dependendo do modo
 */

#include <SDL2/SDL.h>
#include <vector>
#include <iostream>
#include <string>
#include <algorithm> 
#include "rasterizer.h" // Inclui math_utils.h automaticamente

const int TARGET_FPS = 60;
const int FRAME_DELAY = 1000 / TARGET_FPS;

// Buffers Globais
std::vector<uint32_t> framebuffer(LARGURA * ALTURA);
std::vector<float> zbuffer(LARGURA * ALTURA);

// Variáveis de Estado Global
Vec4 g_cam_pos(0, 0, 0);       
float g_fov = 1.04f;           
Vec4 g_light_pos(2.0f, 3.0f, -5.0f); 

// Dados Geométricos do Cubo
Vec4 vertices_cubo[8] = {
    {-1, -1, -1}, { 1, -1, -1}, { 1,  1, -1}, {-1,  1, -1}, 
    {-1, -1,  1}, { 1, -1,  1}, { 1,  1,  1}, {-1,  1,  1}  
};

int indices_triangulos[12][3] = {
    {0, 1, 2}, {0, 2, 3}, {5, 4, 7}, {5, 7, 6}, 
    {3, 2, 6}, {3, 6, 7}, {4, 5, 1}, {4, 1, 0}, 
    {4, 0, 3}, {4, 3, 7}, {1, 5, 6}, {1, 6, 2}  
};

// --- CONTROLE DE MODOS ---
enum Modo { M_OBJETO, M_LUZ, M_CAMERA, M_MATERIAL };
Modo modo_atual = M_OBJETO;
const char* nomes_modos[] = { 
    "OBJETO (WASD=Mover, Setas=Girar, +/-=Escala)", 
    "LUZ (WASD/QE=Mover)", 
    "CAMERA (WASD/QE=Mover, Cima/Baixo=Zoom)", 
    "MATERIAL (1-8=Parametros)" 
};

int main(int argc, char* argv[]) {
    if(SDL_Init(SDL_INIT_VIDEO) < 0) return 1;
    SDL_Window* window = SDL_CreateWindow("Modelador 3D - Modular", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, LARGURA, ALTURA, SDL_WINDOW_SHOWN);
    SDL_Renderer* renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, LARGURA, ALTURA);

    std::vector<Cubo> cena;
    
    // Cubo 1 (Vermelho)
    Cubo c1; 
    c1.posicao = Vec4(-1.2f, 0, -5); 
    c1.rotacao = Vec4(0.5f, 0.6f, 0); 
    c1.escala = Vec4(1,1,1); 
    c1.cor_base = 0xFFFF0000; 
    cena.push_back(c1);

    // Cubo 2 (Verde)
    Cubo c2; 
    c2.posicao = Vec4(0.8f, 0, -4.5f); 
    c2.rotacao = Vec4(0, -0.3f, 0); 
    c2.escala = Vec4(1,1,1); 
    c2.cor_base = 0xFF00FF00; 
    c2.shininess = 100.0f; 
    cena.push_back(c2);

    int cubo_idx = 0;
    bool running = true;
    
    std::cout << "--- SISTEMA INICIADO ---" << std::endl;
    std::cout << "MODO ATUAL: " << nomes_modos[modo_atual] << std::endl;

    while (running) {
        Uint32 frameStart = SDL_GetTicks();
        SDL_Event event;

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
                            if(event.key.keysym.sym == SDLK_KP_PLUS || event.key.keysym.sym == SDLK_EQUALS) cena[cubo_idx].escala.x += 0.1f;
                            if(event.key.keysym.sym == SDLK_KP_MINUS || event.key.keysym.sym == SDLK_MINUS) if(cena[cubo_idx].escala.x>0.1) cena[cubo_idx].escala.x -= 0.1f;
                        }
                        else if (modo_atual == M_LUZ) {
                            if(event.key.keysym.sym == SDLK_w) g_light_pos.y += step;
                            if(event.key.keysym.sym == SDLK_s) g_light_pos.y -= step;
                            if(event.key.keysym.sym == SDLK_a) g_light_pos.x -= step;
                            if(event.key.keysym.sym == SDLK_d) g_light_pos.x += step;
                            if(event.key.keysym.sym == SDLK_q) g_light_pos.z += step;
                            if(event.key.keysym.sym == SDLK_e) g_light_pos.z -= step;
                            std::cout << "Luz: " << g_light_pos.x << " " << g_light_pos.y << " " << g_light_pos.z << "\r" << std::flush;
                        }
                        else if (modo_atual == M_CAMERA) {
                            if(event.key.keysym.sym == SDLK_w) g_cam_pos.y += step;
                            if(event.key.keysym.sym == SDLK_s) g_cam_pos.y -= step;
                            if(event.key.keysym.sym == SDLK_a) g_cam_pos.x -= step;
                            if(event.key.keysym.sym == SDLK_d) g_cam_pos.x += step;
                            if(event.key.keysym.sym == SDLK_q) g_cam_pos.z += step;
                            if(event.key.keysym.sym == SDLK_e) g_cam_pos.z -= step;
                            if(event.key.keysym.sym == SDLK_UP) g_fov = std::max(0.1f, g_fov - 0.05f);
                            if(event.key.keysym.sym == SDLK_DOWN) g_fov += 0.05f;
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
                            std::cout << "Mat: Ka=" << cena[cubo_idx].ka << " Kd=" << cena[cubo_idx].kd << " Ks=" << cena[cubo_idx].ks << "\r" << std::flush;
                        }
                        break;
                }
            }
        }

        // --- RENDERIZAÇÃO ---
        std::fill(framebuffer.begin(), framebuffer.end(), 0xFF222222);
        std::fill(zbuffer.begin(), zbuffer.end(), 1000.0f);

        Mat4 proj = perspective(g_fov, (float)LARGURA/ALTURA, 0.1f, 100.0f);
        Mat4 view = translate(-g_cam_pos.x, -g_cam_pos.y, -g_cam_pos.z);

        for (const auto& cubo : cena) {
            Mat4 model_local = translate(cubo.posicao.x, cubo.posicao.y, cubo.posicao.z) * rotateX(cubo.rotacao.x) * rotateY(cubo.rotacao.y) * rotateZ(cubo.rotacao.z) * scale(cubo.escala.x);
            Mat4 mvp = proj * view * model_local;

            Vec4 world_verts[8];
            Vec4 proj_verts[8];

            for(int i=0; i<8; i++) {
                world_verts[i] = model_local * vertices_cubo[i];
                Vec4 v = mvp * vertices_cubo[i];
                if (v.w != 0) { v.x /= v.w; v.y /= v.w; v.z /= v.w; }
                proj_verts[i] = v;
            }

            for(int i=0; i<12; i++) {
                Vec4 p1 = proj_verts[indices_triangulos[i][0]];
                Vec4 p2 = proj_verts[indices_triangulos[i][1]];
                Vec4 p3 = proj_verts[indices_triangulos[i][2]];

                if (p1.z <= 0 || p2.z <= 0 || p3.z <= 0) continue;

                Vec4 v1_w = world_verts[indices_triangulos[i][0]];
                Vec4 aresta1 = world_verts[indices_triangulos[i][1]] - v1_w;
                Vec4 aresta2 = world_verts[indices_triangulos[i][2]] - v1_w;
                
                Vec4 normal = aresta2.cross(aresta1); 
                normal.normalize();

                if (normal.dot(g_cam_pos - v1_w) <= 0) continue;

                int x1 = (int)((p1.x + 1.0f) * 0.5f * LARGURA); int y1 = (int)((1.0f - p1.y) * 0.5f * ALTURA);
                int x2 = (int)((p2.x + 1.0f) * 0.5f * LARGURA); int y2 = (int)((1.0f - p2.y) * 0.5f * ALTURA);
                int x3 = (int)((p3.x + 1.0f) * 0.5f * LARGURA); int y3 = (int)((1.0f - p3.y) * 0.5f * ALTURA);

                fill_triangle_phong(x1, y1, p1.w, v1_w, x2, y2, p2.w, world_verts[indices_triangulos[i][1]], x3, y3, p3.w, world_verts[indices_triangulos[i][2]], 
                                    normal, cubo, g_light_pos, g_cam_pos, framebuffer, zbuffer);
            }
        }

        SDL_UpdateTexture(texture, nullptr, framebuffer.data(), LARGURA * sizeof(uint32_t));
        SDL_RenderCopy(renderer, texture, nullptr, nullptr);
        SDL_RenderPresent(renderer);

        int frameTime = SDL_GetTicks() - frameStart;
        if (FRAME_DELAY > frameTime) SDL_Delay(FRAME_DELAY - frameTime);
    }

    SDL_DestroyTexture(texture); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit();
    return 0;
}