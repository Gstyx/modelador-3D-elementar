# Trabalho Final de Computação Gráfica - Renderizador 3D

Este projeto implementa um pipeline gráfico completo (do zero) baseado na teoria de **Alvy Ray Smith**. O renderizador suporta transformações geométricas, recorte (clipping), projeção, rasterização e modelos de iluminação avançados.

**Disciplina:** Computação Gráfica - Unioeste 2026  
**Alunos:**
* Guilherme Altmeyer Soares
* Igor Correa Domingues de Almeida
* Maria Quevedo

---

## 🚀 Funcionalidades Implementadas

O projeto não utiliza OpenGL moderno (como `glBegin`/`glEnd` ou Shaders GLSL prontos). Toda a matemática e lógica de pixels foi implementada manualmente em C++:

1.  **Pipeline Gráfico Completo:** Implementação manual de matrizes de Modelo, Visão e Projeção (MVP), incluindo transformações de viewport.
2.  **Rasterização (Scanline):** Algoritmo para preenchimento de triângulos interpolando atributos vértice a vértice.
3.  **Ocultação de Superfícies (Z-Buffer):** Algoritmo para resolver a visibilidade e profundidade dos pixels.
4.  **Recorte Geométrico (Clipping):** Implementação do algoritmo **Sutherland-Hodgman** para recortar triângulos contra o plano *Near* da câmera, evitando artefatos visuais.
5.  **Iluminação e Shading:**
    * **Flat Shading:** Cor constante calculada por face.
    * **Phong Shading (Pixel Shader):** Interpolação de vetores normais e cálculo de luz (Ambiente + Difusa + Especular) pixel a pixel.
6.  **Materiais RGB:** Controle independente dos canais Vermelho, Verde e Azul para os coeficientes $K_a$, $K_d$ e $K_s$.
7.  **Interatividade:** Controle total de câmera, luz, objetos, materiais e *viewport* em tempo de execução.

---

## 🎮 Manual de Uso

A aplicação funciona através de **Modos de Edição**. Use a tecla **TAB** para alternar entre controlar o Objeto, a Luz, a Câmera, os Materiais ou a Viewport.

### Comandos Gerais (Funcionam em qualquer modo)

| Tecla | Função | Descrição |
| :--- | :--- | :--- |
| **TAB** | **Alternar Modo** | Cicla entre: Objeto $\to$ Luz $\to$ Câmera $\to$ Material $\to$ Viewport. |
| **M** | **Renderizador** | Alterna entre **Phong** (Suave) e **Flat** (Constante/Facetado). |
| **N** | **Novo Cubo** | Cria um cubo na posição inicial $(0, 0, -5)$ com cor aleatória. |
| **ESPAÇO** | **Selecionar** | Alterna a seleção para o próximo cubo da cena. |
| **C** | **Cor Aleatória** | Atribui uma cor difusa aleatória ao cubo selecionado. |
| **BACKSPACE**| **Apagar** | Remove o cubo selecionado da cena (se houver mais de um). |
| **ESC** | **Sair** | Fecha a aplicação. |

---

### 🕹️ Controles por Modo

Verifique o terminal ou a barra de título para saber em qual modo você está.

#### 1. Modo OBJETO
Controla as transformações geométricas do cubo selecionado.
* **W / S:** Translação Vertical (Eixo Y).
* **A / D:** Translação Horizontal (Eixo X).
* **Q / E:** Translação Profundidade (Eixo Z).
* **Setas ESQ / DIR:** Rotação no Eixo Y (Yaw).
* **Setas CIMA / BAIXO:** Rotação no Eixo X (Pitch).

#### 2. Modo LUZ
Move a posição da fonte de luz pontual no mundo.
* **W / S / A / D:** Move a luz nos eixos X e Y.
* **Q / E:** Aproxima ou afasta a luz (Eixo Z).
* *Dica:* Use o renderizador Phong (**M**) para ver o reflexo especular se movendo.

#### 3. Modo CÂMERA
Move o observador (olho) pelo mundo e ajusta a lente.
* **W / S / A / D:** Move a posição da câmera (Strafe).
* **Q / E:** Move a câmera para frente/trás.
* **Setas CIMA / BAIXO:** Ajusta o **FOV** (Zoom da lente/Window).

#### 4. Modo MATERIAL (Edição RGB)
Permite editar os coeficientes de iluminação ($K_a, K_d, K_s$) separando por canais de cor.

**Passo 1: Selecione o Coeficiente**
* **Tecla 1:** Seleciona **Ka** (Ambiente - Cor da sombra/luz base).
* **Tecla 2:** Seleciona **Kd** (Difuso - Cor principal do objeto).
* **Tecla 3:** Seleciona **Ks** (Especular - Cor do brilho/reflexo).

**Passo 2: Edite a Cor (RGB)**
* **A / D:** Diminui / Aumenta **Vermelho (R)**.
* **S / W:** Diminui / Aumenta **Verde (G)**.
* **Q / E:** Diminui / Aumenta **Azul (B)**.

**Outros:**
* **7 / 8:** Aumenta/Diminui o **Brilho (Shininess)** (Concentração do ponto de luz).

#### 5. Modo VIEWPORT
Ajusta a área de desenho na janela (Recorte 2D).
* **W / S:** Aumenta/Diminui a Altura da viewport.
* **A / D:** Aumenta/Diminui a Largura da viewport.
* **Setas:** Movem a posição (X, Y) da viewport na tela.

---

## 🛠️ Pré-requisitos e Instalação

Para compilar este projeto, é necessário ter um compilador C++ moderno e a biblioteca `SDL2`.

### No Linux (Ubuntu/Debian)

1.  **Instale as dependências:**
    ```bash
    sudo apt-get update
    sudo apt-get install build-essential libsdl2-dev
    ```

2.  **Compile o projeto:**
    ```bash
    g++ main.cpp -o renderizador -lSDL2
    ```

3.  **Execute:**
    ```bash
    ./renderizador
    ```

---

## 📚 Referência Teórica

O pipeline implementado segue a sequência clássica:
1.  **Espaço do Objeto** $\to$ *Matriz de Modelo* $\to$ **Espaço do Mundo**.
2.  **Espaço do Mundo** $\to$ *Matriz de Visão* $\to$ **Espaço da Câmera**.
3.  **Recorte (Clipping):** Os triângulos são recortados geometricamente no espaço da câmera.
4.  **Espaço da Câmera** $\to$ *Matriz de Projeção* $\to$ **Espaço de Recorte (Clip Space)**.
5.  **Divisão Perspectiva:** $(x/w, y/w, z/w)$ $\to$ **Coordenadas Normalizadas (NDC)**.
6.  **Transformação de Viewport:** Conversão para coordenadas de tela (pixels).
7.  **Rasterização:** Interpolação baricêntrica e Z-Buffer.