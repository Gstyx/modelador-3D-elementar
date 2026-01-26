# Trabalho Final de Computação Gráfica - Renderizador 3D

Este projeto implementa um pipeline gráfico completo baseado no algoritmo de Alvy Ray Smith. 

**Aluno:** Guilherme Altmeyer Soares; Igor Corread Domingues de Almeida; Maria Quevedo
**Disciplina:** Computação Gráfica - Unioeste 2026

---

##  Funcionalidades Implementadas

1.  **Pipeline Gráfico:** Implementação manual de matrizes de Modelo, Visão e Projeção (MVP).
2.  **Rasterização:** Algoritmo *Scanline* para preenchimento de triângulos.
3.  **Ocultação de Superfícies:** Algoritmo **Z-Buffer** para resolver a visibilidade de pixels.
4.  **Iluminação e Shading:**
    * **Flat Shading (Constante):** Cor calculada uma vez por face.
    * **Phong Shading (Simplificado):** Interpolação de vetores e cálculo de luz pixel a pixel (Pixel Shader).
5.  **Interatividade:** Câmera, luzes, objetos e materiais editáveis em tempo de execução.

---
##   MANUAL

M,Alternar Renderizador,Troca entre Phong (Suave/Pixel-Shader) e Flat (Constante/Facetado).

TAB,Alternar Modo,Muda o que estás a controlar: Objeto -> Luz -> Câmara -> Material.

N,Novo Cubo,"Cria um novo cubo na posição inicial (0, 0, -5) com cor aleatória."

BACKSPACE,Apagar,Remove o cubo que está selecionado atualmente.

C,Mudar Cor,Atribui uma nova cor aleatória ao cubo selecionado.

ESPAÇO,Selecionar,Alterna a seleção para o próximo cubo da cena (se houver mais de um).

ESC,Sair,Fecha a aplicação.

**Modo OBJETO**

Controla a posição e rotação do cubo selecionado.

    W / S: Move para Cima / Baixo (Eixo Y).

    A / D: Move para Esquerda / Direita (Eixo X).

    Q / E: Move para Frente / Trás (Eixo Z).

    Setas ESQ / DIR: Roda o cubo no eixo Y.

    Setas CIMA / BAIXO: Roda o cubo no eixo X.

**Modo LUZ**

Move a posição da fonte de luz pontual.

    W / S / A / D: Move a luz nos eixos X e Y.

    Q / E: Aproxima ou afasta a luz (Eixo Z).

    Nota: Mude para renderização Phong (Tecla M) para visualizar melhor o reflexo especular.

**Modo CÂMARA**

Move o observador pelo mundo e ajusta o zoom.

    W / S / A / D: Move a câmara (Strafe).

    Q / E: Zoom

**Modo MATERIAL**

Altera as propriedades físicas de reflexão da luz do cubo selecionado.

    1 / 2: Aumenta/Diminui Ka (Luz Ambiente - Claridade na sombra).

    3 / 4: Aumenta/Diminui Kd (Luz Difusa - Intensidade da cor do objeto).

    5 / 6: Aumenta/Diminui Ks (Luz Especular - Intensidade do reflexo branco).

    7 / 8: Aumenta/Diminui Brilho (Shininess - Concentração do ponto de luz).

## Pré-requisitos e Instalação

Para compilar este projeto, é necessário ter o compilador `g++` e a biblioteca de desenvolvimento da `SDL2` instalada.

### No Linux (Ubuntu/Debian)
Execute no terminal:
```bash
sudo apt-get update
sudo apt-get install build-essential libsdl2-dev

## Para compilar
g++ main.cpp -o renderizador -lSDL2
./renderizador