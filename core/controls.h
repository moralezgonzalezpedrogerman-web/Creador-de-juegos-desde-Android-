#pragma once
#include "game.h"
#include "hud.h"
#include <cstdint>

namespace forge {

struct Ptr {
    bool down = false, left = false, hud = false;
    float sx = 0, sy = 0, lx = 0, ly = 0, moved = 0;
    int64_t t0 = 0;
};

// Convierte toques en entrada de juego: joystick flotante a la izquierda, mirar a la derecha,
// toque corto = interactuar (o saltar a la izquierda), y los botones del HUD.
// El editor tambien lee p[] para sus gestos.
class PlayControls {
public:
    Hud* hud = nullptr;
    int w = 1, h = 1;
    bool is3d = false;
    Ptr p[10];

    void begin(int id, float x, float y, int64_t ms);
    void move(int id, float x, float y);
    void end(int id, int64_t ms, bool cancel = false);
    Input consume();  // arma la entrada de este frame y limpia lo de un solo uso
    void reset();
    int downCount() const;

private:
    float moveX_ = 0, moveY_ = 0, lookX_ = 0, lookY_ = 0, tapX_ = 0.5f, tapY_ = 0.5f;
    bool tap_ = false, jump_ = false;
};

}  // namespace forge
