/*
    TRABALHO CG - UNIOESTE
*/

#include <SDL2/SDL.h>
#include <vector>
#include <iostream>
#include <ctime>
#include <cstdlib>
#include "rasterizer.h" // Inclui math_utils.h automaticamente

const int TARGET_FPS = 60;
const int FRAME_DELAY = 1000 / TARGET_FPS;

// Geometria
Vec4 verts_cubo[8] = {
    {-1,-1,-1}, {1,-1,-1}, {1,1,-1}, {-1,1,-1},
    {-1,-1,1}, {1,-1,1}, {1,1,1}, {-1,1,1}
};
int indices[12][3] = {
    {0,1,2}, {0,2,3}, {5,4,7}, {5,7,6}, {3,2,6}, {3,6,7},
    {4,5,1}, {4,1,0}, {4,0,3}, {4,3,7}, {1,5,6}, {1,6,2}
};

// Globais de Controle
Vec4 g_cam_pos(0,0,0);
Vec4 g_light_pos(2,3,-5);
float g_fov = 1.04f;
bool g_use_phong = true;

enum Modo { M_OBJ, M_LUZ, M_CAM, M_MAT };
Modo modo_atual = M_OBJ;
const char* nomes_modos[] = {"OBJETO", "LUZ", "CAMERA", "MATERIAL"};

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    std::srand(std::time(nullptr));
    
    SDL_Window* win = SDL_CreateWindow("CG Final - N:Novo, Del:Back, M:Modo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, LARGURA, ALTURA, 0);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, LARGURA, ALTURA);
    
    // Buffers locais
    std::vector<uint32_t> framebuffer(LARGURA * ALTURA);
    std::vector<float> zbuffer(LARGURA * ALTURA);
    
    std::vector<Cubo> cena;
    
    // Cubo Inicial
    Cubo c1; c1.posicao=Vec4(0,0,-5); c1.rotacao=Vec4(0.5,0.6,0); c1.escala=Vec4(1,1,1); c1.cor_base=0xFFFF0000;
    c1.ka=0.2; c1.kd=0.7; c1.ks=0.8; c1.shininess=50; cena.push_back(c1);
    
    int sel_idx = 0;
    bool running = true;
    
    std::cout << "[N] Criar | [BACK] Del | [C] Cor | [M] Render | [TAB] Controle" << std::endl;

    while(running) {
        Uint32 start = SDL_GetTicks();
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type==SDL_QUIT) running=false;
            if(e.type==SDL_KEYDOWN) {
                float s = 0.2f;
                
                // --- COMANDOS GERAIS ---
                if(e.key.keysym.sym == SDLK_m) { g_use_phong = !g_use_phong; std::cout << "Render: " << (g_use_phong?"PHONG":"FLAT") << std::endl; }
                if(e.key.keysym.sym == SDLK_TAB) { modo_atual = (Modo)((modo_atual+1)%4); std::cout << "Modo: " << nomes_modos[modo_atual] << std::endl; }
                
                // --- GERENCIAMENTO DE CUBOS ---
                if(e.key.keysym.sym == SDLK_n) {
                    Cubo novo = c1; novo.posicao = Vec4(0,0,-5);
                    novo.cor_base = (255<<24)|(rand()%255<<16)|(rand()%255<<8)|rand()%255;
                    cena.push_back(novo); sel_idx = cena.size()-1;
                    std::cout << "Cubo criado." << std::endl;
                }
                if(e.key.keysym.sym == SDLK_BACKSPACE && !cena.empty()) {
                    cena.erase(cena.begin()+sel_idx);
                    if(sel_idx >= cena.size()) sel_idx = cena.empty()?0:cena.size()-1;
                    std::cout << "Cubo apagado." << std::endl;
                }
                if(e.key.keysym.sym == SDLK_c && !cena.empty()) {
                    cena[sel_idx].cor_base = (255<<24)|(rand()%255<<16)|(rand()%255<<8)|rand()%255;
                    std::cout << "Nova cor." << std::endl;
                }
                if(e.key.keysym.sym == SDLK_SPACE && !cena.empty()) {
                    sel_idx = (sel_idx+1)%cena.size();
                    std::cout << "Selecionado: " << sel_idx << std::endl;
                }

                // --- CONTROLES POR MODO ---
                if(modo_atual == M_OBJ && !cena.empty()) {
                    if(e.key.keysym.sym==SDLK_w) cena[sel_idx].posicao.y += s;
                    if(e.key.keysym.sym==SDLK_s) cena[sel_idx].posicao.y -= s;
                    if(e.key.keysym.sym==SDLK_a) cena[sel_idx].posicao.x -= s;
                    if(e.key.keysym.sym==SDLK_d) cena[sel_idx].posicao.x += s;
                    if(e.key.keysym.sym==SDLK_q) cena[sel_idx].posicao.z += s;
                    if(e.key.keysym.sym==SDLK_e) cena[sel_idx].posicao.z -= s;
                    if(e.key.keysym.sym==SDLK_LEFT) cena[sel_idx].rotacao.y -= 0.1;
                    if(e.key.keysym.sym==SDLK_RIGHT) cena[sel_idx].rotacao.y += 0.1;
                    if(e.key.keysym.sym==SDLK_UP) cena[sel_idx].rotacao.x -= 0.1;
                    if(e.key.keysym.sym==SDLK_DOWN) cena[sel_idx].rotacao.x += 0.1;
                }
                else if(modo_atual == M_LUZ) {
                    if(e.key.keysym.sym==SDLK_w) g_light_pos.y += s;
                    if(e.key.keysym.sym==SDLK_s) g_light_pos.y -= s;
                    if(e.key.keysym.sym==SDLK_a) g_light_pos.x -= s;
                    if(e.key.keysym.sym==SDLK_d) g_light_pos.x += s;
                    if(e.key.keysym.sym==SDLK_q) g_light_pos.z += s;
                    if(e.key.keysym.sym==SDLK_e) g_light_pos.z -= s;
                }
                else if(modo_atual == M_CAM) {
                    if(e.key.keysym.sym==SDLK_w) g_cam_pos.y += s;
                    if(e.key.keysym.sym==SDLK_s) g_cam_pos.y -= s;
                    if(e.key.keysym.sym==SDLK_a) g_cam_pos.x -= s;
                    if(e.key.keysym.sym==SDLK_d) g_cam_pos.x += s;
                    if(e.key.keysym.sym==SDLK_q) g_cam_pos.z += s;
                    if(e.key.keysym.sym==SDLK_e) g_cam_pos.z -= s;
                    if(e.key.keysym.sym==SDLK_UP) g_fov -= 0.05;
                    if(e.key.keysym.sym==SDLK_DOWN) g_fov += 0.05;
                }
                else if(modo_atual == M_MAT && !cena.empty()) {
                    if(e.key.keysym.sym==SDLK_1) cena[sel_idx].ka += 0.05;
                    if(e.key.keysym.sym==SDLK_2) cena[sel_idx].ka -= 0.05;
                    if(e.key.keysym.sym==SDLK_3) cena[sel_idx].kd += 0.05;
                    if(e.key.keysym.sym==SDLK_4) cena[sel_idx].kd -= 0.05;
                    if(e.key.keysym.sym==SDLK_5) cena[sel_idx].ks += 0.05;
                    if(e.key.keysym.sym==SDLK_6) cena[sel_idx].ks -= 0.05;
                    if(e.key.keysym.sym==SDLK_7) cena[sel_idx].shininess += 5;
                    if(e.key.keysym.sym==SDLK_8) cena[sel_idx].shininess -= 5;
                }
            }
        }
        
        std::fill(framebuffer.begin(), framebuffer.end(), 0xFF222222);
        std::fill(zbuffer.begin(), zbuffer.end(), 1000.0f);
        
        Mat4 proj = perspective(g_fov, (float)LARGURA/ALTURA, 0.1f, 100.0f);
        Mat4 view = translate(-g_cam_pos.x, -g_cam_pos.y, -g_cam_pos.z);
        
        for(auto& cubo : cena) {
            Mat4 model = translate(cubo.posicao.x, cubo.posicao.y, cubo.posicao.z) * rotateY(cubo.rotacao.y) * rotateX(cubo.rotacao.x) * scale(cubo.escala.x);
            Mat4 mvp = proj * view * model;
            
            Vec4 world_v[8], proj_v[8];
            for(int i=0; i<8; i++) {
                world_v[i] = model * verts_cubo[i];
                proj_v[i] = mvp * verts_cubo[i];
                if(proj_v[i].w!=0) { proj_v[i].x/=proj_v[i].w; proj_v[i].y/=proj_v[i].w; proj_v[i].z/=proj_v[i].w; }
            }
            
            for(int i=0; i<12; i++) {
                Vec4 p1 = proj_v[indices[i][0]];
                Vec4 p2 = proj_v[indices[i][1]];
                Vec4 p3 = proj_v[indices[i][2]];
                
                if(p1.z<=0 || p2.z<=0 || p3.z<=0) continue;
                
                Vec4 v1_w = world_v[indices[i][0]];
                Vec4 n = (world_v[indices[i][2]] - v1_w).cross(world_v[indices[i][1]] - v1_w);
                n.normalize();
                if(n.dot(g_cam_pos - v1_w) <= 0) continue;
                
                int x1 = (p1.x+1)*0.5*LARGURA, y1 = (1-p1.y)*0.5*ALTURA;
                int x2 = (p2.x+1)*0.5*LARGURA, y2 = (1-p2.y)*0.5*ALTURA;
                int x3 = (p3.x+1)*0.5*LARGURA, y3 = (1-p3.y)*0.5*ALTURA;
                
                if(g_use_phong) {
                    fill_triangle_phong(x1, y1, p1.w, v1_w, x2, y2, p2.w, world_v[indices[i][1]], x3, y3, p3.w, world_v[indices[i][2]], n, cubo, g_light_pos, g_cam_pos, framebuffer, zbuffer);
                } else {
                    Vec4 centro = (v1_w + world_v[indices[i][1]] + world_v[indices[i][2]]) * 0.333f;
                    uint32_t c = calc_luz(centro, n, cubo, g_light_pos, g_cam_pos);
                    fill_triangle_flat(x1, y1, p1.w, x2, y2, p2.w, x3, y3, p3.w, c, framebuffer, zbuffer);
                }
            }
        }
        
        SDL_UpdateTexture(tex, NULL, framebuffer.data(), LARGURA*4);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);
        
        int t = SDL_GetTicks() - start;
        if(FRAME_DELAY > t) SDL_Delay(FRAME_DELAY - t);
    }
    
    SDL_DestroyTexture(tex); SDL_DestroyRenderer(ren); SDL_DestroyWindow(win); SDL_Quit();
    return 0;
}