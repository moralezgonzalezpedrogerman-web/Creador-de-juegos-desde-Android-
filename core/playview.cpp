#include "playview.h"
#include "imgui.h"
#include "renderer.h"

namespace forge {

void PlayView::start(Project p) {
    hud.setup(p);
    ctl.hud = &hud;
    ctl.reset();
    if (audio) {
        audio->stopMusic();
        audio->setAssets(p.assets);  // los buffers de los vectores sobreviven al mover el proyecto
        game.onSound = [this](const std::string& n) { audio->play(n); };
        game.onMusic = [this](const std::string& n, bool on) { if (on) audio->music(n); else audio->stopMusic(); };
    } else {
        game.onSound = nullptr;
        game.onMusic = nullptr;
    }
    game.load(std::move(p));
}

void PlayView::frame(Renderer* r, int w, int h, float dt, const Input* direct) {
    ctl.w = w;
    ctl.h = h;
    ctl.is3d = game.sc && game.sc->is3d;
    Input in = direct ? *direct : ctl.consume();
    float aspect = h > 0 ? (float)w / (float)h : 1.f;
    game.update(dt, in, aspect);
    if (r) r->render(game, w, h);
    hud.draw(ImGui::GetForegroundDrawList(), game, r, w, h, showButtons && !direct);
}

}  // namespace forge
