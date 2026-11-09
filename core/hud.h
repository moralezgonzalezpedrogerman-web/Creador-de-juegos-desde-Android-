#pragma once
#include "game.h"
#include <vector>

struct ImDrawList;

namespace forge {

class Renderer;

// Botones tactiles (saltar, disparar...), arma en primera persona, joystick, texto y variables.
class Hud {
public:
    std::vector<HudBtn> btns;  // los que estan activos (por defecto o los del proyecto)
    struct Joy { bool active = false; float ox = 0, oy = 0, x = 0, y = 0; } joy;

    void setup(const Project& p);
    // Un dedo toca la pantalla: true si lo captura un boton (en escenas 3D).
    bool down(int id, float px, float py, int w, int h, bool is3d);
    void up(int id);
    void collect(Input& in);  // traduce botones presionados a entrada de juego
    void draw(ImDrawList* dl, const Game& g, Renderer* r, int w, int h, bool showButtons);

private:
    struct St { int ptr = -1; bool tapPending = false; };
    std::vector<St> st_;
};

}  // namespace forge
