#include "controls.h"
#include <algorithm>
#include <cmath>

namespace forge {

void PlayControls::reset() {
    for (auto& q : p) q = Ptr();
    moveX_ = moveY_ = lookX_ = lookY_ = 0;
    tap_ = jump_ = false;
    if (hud) hud->joy = Hud::Joy();
}

int PlayControls::downCount() const {
    int n = 0;
    for (auto& q : p) if (q.down) n++;
    return n;
}

void PlayControls::begin(int id, float x, float y, int64_t ms) {
    if (id < 0 || id >= 10) return;
    Ptr& q = p[id];
    q = Ptr();
    q.down = true;
    q.sx = q.lx = x;
    q.sy = q.ly = y;
    q.t0 = ms;
    q.left = x < w * 0.5f;
    if (hud && hud->down(id, x, y, w, h, is3d)) q.hud = true;
}

void PlayControls::move(int id, float x, float y) {
    if (id < 0 || id >= 10 || !p[id].down) return;
    Ptr& q = p[id];
    float dx = x - q.lx, dy = y - q.ly;
    q.moved += std::fabs(dx) + std::fabs(dy);
    q.lx = x;
    q.ly = y;
    if (q.hud || !is3d) return;
    if (q.left) {  // mitad izquierda: joystick flotante
        float R = w * 0.08f;
        moveX_ = std::max(-1.f, std::min(1.f, (x - q.sx) / R));
        moveY_ = std::max(-1.f, std::min(1.f, -(y - q.sy) / R));
        if (hud) { hud->joy.active = true; hud->joy.ox = q.sx; hud->joy.oy = q.sy; hud->joy.x = x; hud->joy.y = y; }
    } else {       // mitad derecha: mirar
        lookX_ += dx * 0.005f;
        lookY_ += dy * 0.005f;
    }
}

void PlayControls::end(int id, int64_t ms, bool cancel) {
    if (id < 0 || id >= 10 || !p[id].down) return;
    Ptr& q = p[id];
    if (hud) hud->up(id);
    if (q.left && !q.hud) {
        moveX_ = moveY_ = 0;
        if (hud) hud->joy.active = false;
    }
    bool shortTap = !cancel && !q.hud && q.moved < 25 && ms - q.t0 < 300;
    if (shortTap) {
        if (q.left && is3d) jump_ = true;  // toque corto a la izquierda = saltar
        else {                             // toque corto = interactuar / pulsar
            tap_ = true;
            tapX_ = q.lx / (float)w;
            tapY_ = q.ly / (float)h;
        }
    }
    q.down = false;
}

Input PlayControls::consume() {
    Input in;
    in.moveX = moveX_; in.moveY = moveY_;
    in.lookX = lookX_; in.lookY = lookY_;
    in.jump = jump_; in.tap = tap_;
    in.tapX = tapX_; in.tapY = tapY_;
    lookX_ = lookY_ = 0;
    jump_ = tap_ = false;
    if (hud) hud->collect(in);
    return in;
}

}  // namespace forge
