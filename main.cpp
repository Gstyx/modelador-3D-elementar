/**
 * TRABALHO FINAL - COMPUTACAO GRAFICA (UNIOESTE)
 * Implementacao do Pipeline Grafico Classico (Alvy Ray Smith / Blinn)
 */

#include <SDL2/SDL.h>
#include <vector>
#include <iostream>
#include <cstdio>
#include <ctime>
#include <cstdlib>
#include <algorithm>
#include "rasterizer.h" 

const int TARGET_FPS = 60;
const int FRAME_DELAY = 1000 / TARGET_FPS;

// --- GEOMETRIA ---
Vec4 verts_cubo[8] = { {-1,-1,-1}, {1,-1,-1}, {1,1,-1}, {-1,1,-1}, {-1,-1,1}, {1,-1,1}, {1,1,1}, {-1,1,1} };
int indices[12][3] = { {0,1,2}, {0,2,3}, {5,4,7}, {5,7,6}, {3,2,6}, {3,6,7}, {4,5,1}, {4,1,0}, {4,0,3}, {4,3,7}, {1,5,6}, {1,6,2} };

// --- ESTADO GLOBAL ---
Vec4 g_cam_pos(0,0,0);
Vec4 g_light_pos(2,3,-5);
Vec3 g_ambient_color(0.2f, 0.2f, 0.2f); // Cor da Luz Ambiente (Ila)
Vec3 g_light_color(1.0f, 1.0f, 1.0f);   // Cor da Luz Pontual (Il)
float g_fov = 1.04f;
bool g_use_phong = true;
int g_vp_x = 0, g_vp_y = 0, g_vp_w = SCREEN_W, g_vp_h = SCREEN_H;

// --- CONTROLE DE INTERFACE ---
enum Modo { M_OBJ, M_LUZ, M_CAM, M_MAT, M_VIEW, M_LIGHT_COLOR };
Modo modo_atual = M_OBJ;
int mat_sel_type = 2; // 2 = Kd (Cor Base)
int sel_idx = 0;
int light_sel_type = 1; // 1 = Ambiente, 2 = Pontual

void atualizar_interface(const std::vector<Cubo>& cena) {
    printf("\r                                                                                \r"); // Limpa linha
    
    switch(modo_atual) {
        case M_OBJ:
            printf("[MODO: OBJETO %d] Pos: %.2f %.2f %.2f | Rot: %.2f %.2f", 
                   sel_idx, cena[sel_idx].posicao.x, cena[sel_idx].posicao.y, cena[sel_idx].posicao.z,
                   cena[sel_idx].rotacao.x, cena[sel_idx].rotacao.y);
            break;
        case M_LUZ:
            printf("[MODO: LUZ] Pos: %.2f %.2f %.2f", g_light_pos.x, g_light_pos.y, g_light_pos.z);
            break;
        case M_CAM:
            printf("[MODO: CAMERA] Pos: %.2f %.2f %.2f | FOV: %.2f", g_cam_pos.x, g_cam_pos.y, g_cam_pos.z, g_fov);
            break;
        case M_LIGHT_COLOR:{
            Vec3* lc = (light_sel_type == 1) ? &g_ambient_color : &g_light_color;
            printf("[MODO: COR DA LUZ] %s | R:%.2f G:%.2f B:%.2f", 
                (light_sel_type == 1 ? "AMBIENTE" : "PONTUAL"), lc->x, lc->y, lc->z);
            break;  }
        case M_VIEW:
            printf("[MODO: VIEWPORT] X,Y: %d,%d | W,H: %d,%d", g_vp_x, g_vp_y, g_vp_w, g_vp_h);
            break;
        case M_MAT:
            const Cubo& c = cena[sel_idx];
            Vec3 target = (mat_sel_type==1) ? c.mat.ka : (mat_sel_type==2 ? c.mat.kd : c.mat.ks);
            char tipo[20];
            if(mat_sel_type==1) sprintf(tipo, "Ka (Ambiente)");
            else if(mat_sel_type==2) sprintf(tipo, "Kd (COR BASE)");
            else sprintf(tipo, "Ks (REFLEXO)");
            
            printf("[MODO: MATERIAL] %s | R:%.2f G:%.2f B:%.2f | Brilho: %.0f", 
                   tipo, target.x, target.y, target.z, c.mat.shininess);
            break;
            
    }
    fflush(stdout);
}

int main(int argc, char* argv[]) {
    SDL_Init(SDL_INIT_VIDEO);
    std::srand(std::time(nullptr));
    SDL_Window* win = SDL_CreateWindow("CG Final - Pipeline Completo", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_W, SCREEN_H, 0);
    SDL_Renderer* ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture* tex = SDL_CreateTexture(ren, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, SCREEN_W, SCREEN_H);
    
    std::vector<uint32_t> fb(SCREEN_W * SCREEN_H);
    std::vector<float> zb(SCREEN_W * SCREEN_H);
    std::vector<Cubo> cena;
    
    // Configuração Inicial da Cena
    Cubo c1; c1.posicao=Vec4(-1.2,0,-5); c1.rotacao=Vec4(0.5,0.6,0); c1.escala=Vec4(1,1,1);
    c1.mat.ka = Vec3(0.1,0.0,0.0); c1.mat.kd = Vec3(0.8,0.0,0.0); c1.mat.ks = Vec3(1.0,1.0,1.0); c1.mat.shininess=50;
    cena.push_back(c1);

    Cubo c2; c2.posicao=Vec4(1.2,0,-5); c2.rotacao=Vec4(0,-0.3,0); c2.escala=Vec4(1,1,1);
    c2.mat.ka = Vec3(0.0,0.1,0.0); c2.mat.kd = Vec3(0.0,0.8,0.0); c2.mat.ks = Vec3(1.0,1.0,1.0); c2.mat.shininess=100;
    cena.push_back(c2);
    
    bool running = true;
    
    std::cout << "=== RENDERIZADOR 3D FINAL ===" << std::endl;
    std::cout << "[TAB] Alternar Modos | [Espaco] Alternar Cubo" << std::endl;
    std::cout << "[M] Renderizador (Phong/Flat) | [C] Cor Aleatoria" << std::endl;
    atualizar_interface(cena);

    while(running) {
        Uint32 start = SDL_GetTicks();
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type==SDL_QUIT) running=false;
            if(e.type==SDL_KEYDOWN) {
                float s = 0.2f; float mat_s = 0.05f;
                
                // Comandos Gerais
                if(e.key.keysym.sym == SDLK_m) { g_use_phong = !g_use_phong; std::cout << "\n[SISTEMA] Render: " << (g_use_phong?"PHONG":"FLAT") << std::endl; }
                if(e.key.keysym.sym == SDLK_TAB) modo_atual = (Modo)((modo_atual + 1) % 6);
                if(e.key.keysym.sym == SDLK_SPACE && !cena.empty()) sel_idx = (sel_idx+1)%cena.size();
                
                // [RESTORED] Cor Aleatória
                if(e.key.keysym.sym == SDLK_c && !cena.empty()) {
                    cena[sel_idx].mat.kd = Vec3((rand()%100)/100.0f, (rand()%100)/100.0f, (rand()%100)/100.0f);
                }
                
                // Novo Cubo
                if(e.key.keysym.sym == SDLK_n) {
                    Cubo novo = c1; novo.posicao = Vec4(0,0,-5);
                    novo.mat.kd = Vec3((rand()%100)/100.0f, (rand()%100)/100.0f, (rand()%100)/100.0f);
                    cena.push_back(novo); sel_idx = cena.size()-1;
                }

                // Lógica de Controle por Modo
                if(modo_atual == M_OBJ && !cena.empty()) {
                    if(e.key.keysym.sym==SDLK_w) cena[sel_idx].posicao.y += s;
                    if(e.key.keysym.sym==SDLK_s) cena[sel_idx].posicao.y -= s;
                    if(e.key.keysym.sym==SDLK_a) cena[sel_idx].posicao.x -= s;
                    if(e.key.keysym.sym==SDLK_d) cena[sel_idx].posicao.x += s;
                    if(e.key.keysym.sym==SDLK_q) cena[sel_idx].posicao.z += s;
                    if(e.key.keysym.sym==SDLK_e) cena[sel_idx].posicao.z -= s;
                    
                    if(e.key.keysym.sym==SDLK_LEFT) cena[sel_idx].rotacao.y -= 0.1;
                    if(e.key.keysym.sym==SDLK_RIGHT) cena[sel_idx].rotacao.y += 0.1;
                    if(e.key.keysym.sym==SDLK_UP)   cena[sel_idx].rotacao.x -= 0.1;
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
                    if(e.key.keysym.sym==SDLK_UP) g_fov -= 0.05f;
                    if(e.key.keysym.sym==SDLK_DOWN) g_fov += 0.05f;
                }
                else if(modo_atual == M_VIEW) {
                    if(e.key.keysym.sym==SDLK_w) g_vp_h -= 10;
                    if(e.key.keysym.sym==SDLK_s) g_vp_h += 10;
                    if(e.key.keysym.sym==SDLK_a) g_vp_w -= 10;
                    if(e.key.keysym.sym==SDLK_d) g_vp_w += 10;
                    if(e.key.keysym.sym==SDLK_UP) g_vp_y -= 10;
                    if(e.key.keysym.sym==SDLK_DOWN) g_vp_y += 10;
                    if(e.key.keysym.sym==SDLK_LEFT) g_vp_x -= 10;
                    if(e.key.keysym.sym==SDLK_RIGHT) g_vp_x += 10;
                }
                else if(modo_atual == M_MAT && !cena.empty()) {
                    // Seleciona coeficiente
                    if(e.key.keysym.sym==SDLK_1 || e.key.keysym.sym==SDLK_KP_1) mat_sel_type = 1; 
                    if(e.key.keysym.sym==SDLK_2 || e.key.keysym.sym==SDLK_KP_2) mat_sel_type = 2; 
                    if(e.key.keysym.sym==SDLK_3 || e.key.keysym.sym==SDLK_KP_3) mat_sel_type = 3; 
                    
                    Vec3* target = (mat_sel_type==1) ? &cena[sel_idx].mat.ka : 
                                   (mat_sel_type==2) ? &cena[sel_idx].mat.kd : &cena[sel_idx].mat.ks;
                    
                    // Edita RGB com WASD+QE
                    if(e.key.keysym.sym==SDLK_d) target->x = std::min(1.0f, target->x + 0.05f); // Red+
                    if(e.key.keysym.sym==SDLK_a) target->x = std::max(0.0f, target->x - 0.05f); // Red-
                    if(e.key.keysym.sym==SDLK_w) target->y = std::min(1.0f, target->y + 0.05f); // Green+
                    if(e.key.keysym.sym==SDLK_s) target->y = std::max(0.0f, target->y - 0.05f); // Green-
                    if(e.key.keysym.sym==SDLK_e) target->z = std::min(1.0f, target->z + 0.05f); // Blue+
                    if(e.key.keysym.sym==SDLK_q) target->z = std::max(0.0f, target->z - 0.05f); // Blue-
                    
                    if(e.key.keysym.sym==SDLK_7 || e.key.keysym.sym==SDLK_KP_7) cena[sel_idx].mat.shininess += 5.0f;
                    if(e.key.keysym.sym==SDLK_8 || e.key.keysym.sym==SDLK_KP_8) cena[sel_idx].mat.shininess = std::max(1.0f, cena[sel_idx].mat.shininess - 5.0f);
                }
                else if(modo_atual == M_LIGHT_COLOR) {
                    if(e.key.keysym.sym == SDLK_1) light_sel_type = 1;
                    if(e.key.keysym.sym == SDLK_2) light_sel_type = 2;
                    
                    Vec3* target = (light_sel_type == 1) ? &g_ambient_color : &g_light_color;
                    
                    if(e.key.keysym.sym == SDLK_d) target->x = std::min(1.0f, target->x + 0.05f); // R+
                    if(e.key.keysym.sym == SDLK_a) target->x = std::max(0.0f, target->x - 0.05f); // R-
                    if(e.key.keysym.sym == SDLK_w) target->y = std::min(1.0f, target->y + 0.05f); // G+
                    if(e.key.keysym.sym == SDLK_s) target->y = std::max(0.0f, target->y - 0.05f); // G-
                    if(e.key.keysym.sym == SDLK_e) target->z = std::min(1.0f, target->z + 0.05f); // B+
                    if(e.key.keysym.sym == SDLK_q) target->z = std::max(0.0f, target->z - 0.05f); // B-
                }
                                
                atualizar_interface(cena);
            }
        }
        
        std::fill(fb.begin(), fb.end(), 0xFF222222);
        std::fill(zb.begin(), zb.end(), 1000.0f);
        
        // --- PIPELINE GRÁFICO (Alvy Ray Smith) ---
        
        // 1. Matrizes de Camera e Projeção (Mundo -> View -> Clip)
        Mat4 proj = perspective(g_fov, (float)SCREEN_W/SCREEN_H, 0.1f, 100.0f);
        Mat4 view = translate(-g_cam_pos.x, -g_cam_pos.y, -g_cam_pos.z);
        
        for(auto& cubo : cena) {
            // 2. Matriz de Modelo (Objeto -> Mundo)
            Mat4 model = translate(cubo.posicao.x, cubo.posicao.y, cubo.posicao.z) * rotateY(cubo.rotacao.y) * rotateX(cubo.rotacao.x) * scale(cubo.escala.x);
            Mat4 model_view = view * model; 
            
            // 3. Transformação de Vértices para View Space
            Vec4 view_verts[8];
            for(int i=0; i<8; i++) view_verts[i] = model_view * verts_cubo[i];
            
            for(int i=0; i<12; i++) {
                Vec4 v1 = view_verts[indices[i][0]];
                Vec4 v2 = view_verts[indices[i][1]];
                Vec4 v3 = view_verts[indices[i][2]];
                
                // 4. RECORTE GEOMÉTRICO (Clipping Sutherland-Hodgman)
                std::vector<Vec4> clipped;
                clip_triangle_sutherland_hodgman(v1, v2, v3, clipped);
                
                for(size_t k=0; k < clipped.size(); k+=3) {
                    Vec4 t1 = clipped[k], t2 = clipped[k+1], t3 = clipped[k+2];
                    
                    // 5. Back-Face Culling
                    Vec4 n = (t3 - t1).cross(t2 - t1); n.normalize();
                    if(n.dot(t1 * -1.0f) <= 0) continue;
                    
                    // 6. Projeção e Divisão Perspectiva
                    Vec4 p1 = proj*t1, p2 = proj*t2, p3 = proj*t3;
                    if(p1.w!=0) { p1.x/=p1.w; p1.y/=p1.w; p1.z/=p1.w; }
                    if(p2.w!=0) { p2.x/=p2.w; p2.y/=p2.w; p2.z/=p2.w; }
                    if(p3.w!=0) { p3.x/=p3.w; p3.y/=p3.w; p3.z/=p3.w; }
                    
                    // 7. Transformação de Viewport (Normalizado -> Tela)
                    int x1 = (p1.x+1)*0.5*g_vp_w + g_vp_x; int y1 = (1-p1.y)*0.5*g_vp_h + g_vp_y;
                    int x2 = (p2.x+1)*0.5*g_vp_w + g_vp_x; int y2 = (1-p2.y)*0.5*g_vp_h + g_vp_y;
                    int x3 = (p3.x+1)*0.5*g_vp_w + g_vp_x; int y3 = (1-p3.y)*0.5*g_vp_h + g_vp_y;
                    
                    // 8. Rasterização
                    if(g_use_phong) {
                        fill_phong(x1, y1, p1.w, t1, x2, y2, p2.w, t2, x3, y3, p3.w, t3, 
                                    n, cubo, g_light_pos, Vec4(0,0,0), fb, zb, 
                                    g_vp_w, g_vp_h, g_vp_x, g_vp_y, g_light_color, g_ambient_color);
                    } else {
                        Vec4 lightPosView = view * g_light_pos;
                        Vec4 centro = (t1 + t2 + t3) * 0.333f;
                        uint32_t c = calc_luz_rgb(centro, n, cubo, lightPosView, Vec4(0,0,0), 
                                     g_light_color, g_ambient_color);   
                        fill_flat(x1,y1,p1.w, x2,y2,p2.w, x3,y3,p3.w, c, fb, zb, g_vp_w, g_vp_h, g_vp_x, g_vp_y);
                    }
                }
            }
        }
        
        SDL_UpdateTexture(tex, NULL, fb.data(), SCREEN_W*4);
        SDL_RenderCopy(ren, tex, NULL, NULL);
        SDL_RenderPresent(ren);
        int t = SDL_GetTicks() - start;
        if(FRAME_DELAY > t) SDL_Delay(FRAME_DELAY - t);
    }
    
    SDL_DestroyTexture(tex); SDL_DestroyRenderer(ren); SDL_DestroyWindow(win); SDL_Quit();
    return 0;
}