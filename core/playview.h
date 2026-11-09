#pragma once
#include "audio.h"
#include "controls.h"
#include "game.h"
#include "hud.h"

namespace forge {

class Renderer;

// Un juego en marcha: logica + dibujo + botones + audio. Lo usan los runtimes y el modo "Jugar" del editor.
class PlayView {
public:
    Game game;
    Hud hud;
    PlayControls ctl;
    Audio* audio = nullptr;
    bool showButtons = true;

    void start(Project p);
    // Requiere un frame de ImGui activo (el HUD se dibuja con ImGui). r puede ser null en pruebas.
    void frame(Renderer* r, int w, int h, float dt, const Input* direct = nullptr);
};

}  // namespace forge
