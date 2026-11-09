#include "hud.h"
#include "renderer.h"
#include "imgui.h"
#include <cmath>
#include <cstdio>

namespace forge {

void Hud::setup(const Project& p) {
    btns = p.hudDefault ? defaultHud() : p.hud;
    st_.assign(btns.size(), St());
    joy = Joy();
}

bool Hud::down(int id, float px, float py, int w, int h, bool is3d) {
    if (!is3d) return false;
    for (size_t i = 0; i < btns.size(); i++) {
        const HudBtn& b = btns[i];
        if (b.act == HudAct::None) continue;
        float dx = px - b.x * w, dy = py - b.y * h, r = b.r * h;
        if (dx * dx + dy * dy <= r * r) {
            st_[i].ptr = id;
            if (b.act == HudAct::Interact) st_[i].tapPending = true;
            return true;
        }
    }
    return false;
}

void Hud::up(int id) {
    for (auto& s : st_) if (s.ptr == id) s.ptr = -1;
}

void Hud::collect(Input& in) {
    for (size_t i = 0; i < btns.size() && i < st_.size(); i++) {
        if (st_[i].ptr >= 0) {
            if (btns[i].act == HudAct::Jump) in.jump = true;
            if (btns[i].act == HudAct::Shoot) in.shoot = true;
        }
        if (st_[i].tapPending) { in.tap = true; st_[i].tapPending = false; }
    }
}

static ImU32 col(float r, float g, float b, float a) {
    return IM_COL32((int)(r * 255), (int)(g * 255), (int)(b * 255), (int)(a * 255));
}

void Hud::draw(ImDrawList* dl, const Game& g, Renderer* rend, int w, int h, bool showButtons) {
    if (!g.sc || !dl) return;
    bool is3d = g.sc->is3d;
    ImFont* font = ImGui::GetFont();
    float fs = ImGui::GetFontSize();

    // Arma en primera persona.
    if (is3d && g.proj.weapon.enabled) {
        const Weapon& wp = g.proj.weapon;
        float sh = wp.scale * h, cx = w * 0.62f + wp.ox * w, bottom = h + wp.oy * h + g.recoil * 0.04f * h;
        int tw = 1, th = 1;
        unsigned tid = rend ? rend->textureId(g, wp.tex, &tw, &th) : 0;
        float top;
        if (tid) {
            float sw = sh * (float)tw / (float)th;
            dl->AddImage((ImTextureID)(intptr_t)tid, ImVec2(cx - sw * 0.5f, bottom - sh), ImVec2(cx + sw * 0.5f, bottom),
                         ImVec2(0, 1), ImVec2(1, 0));
            top = bottom - sh;
        } else {  // arma basica dibujada
            float u = sh * 0.5f;
            dl->AddRectFilled(ImVec2(cx - 0.20f * u, bottom - 0.9f * u), ImVec2(cx + 0.20f * u, bottom), col(.18f, .18f, .2f, 1), 4.f);
            dl->AddRectFilled(ImVec2(cx - 0.34f * u, bottom - 1.35f * u), ImVec2(cx + 0.24f * u, bottom - 0.85f * u), col(.3f, .3f, .34f, 1), 4.f);
            dl->AddRectFilled(ImVec2(cx - 0.10f * u, bottom - 1.75f * u), ImVec2(cx + 0.04f * u, bottom - 1.30f * u), col(.22f, .22f, .26f, 1), 3.f);
            top = bottom - 1.75f * u;
        }
        if (g.flash > 0) dl->AddCircleFilled(ImVec2(cx, top), sh * 0.12f, col(1.f, .85f, .4f, .9f));
    }

    // Botones y joystick.
    if (showButtons && is3d) {
        for (size_t i = 0; i < btns.size(); i++) {
            const HudBtn& b = btns[i];
            bool pressed = i < st_.size() && st_[i].ptr >= 0;
            ImVec2 c(b.x * w, b.y * h);
            float r = b.r * h;
            dl->AddCircleFilled(c, r, col(b.color[0], b.color[1], b.color[2], pressed ? 0.65f : 0.30f), 40);
            dl->AddCircle(c, r, col(b.color[0], b.color[1], b.color[2], 0.9f), 40, 3.f);
            ImVec2 ts = font->CalcTextSizeA(fs, 1e9f, 0.f, b.label.c_str());
            dl->AddText(font, fs, ImVec2(c.x - ts.x * 0.5f, c.y - ts.y * 0.5f), IM_COL32(255, 255, 255, 230), b.label.c_str());
        }
        if (joy.active) {
            float R = w * 0.08f;
            dl->AddCircle(ImVec2(joy.ox, joy.oy), R, IM_COL32(255, 255, 255, 110), 40, 3.f);
            float dx = joy.x - joy.ox, dy = joy.y - joy.oy, l = std::sqrt(dx * dx + dy * dy);
            if (l > R) { dx *= R / l; dy *= R / l; }
            dl->AddCircleFilled(ImVec2(joy.ox + dx, joy.oy + dy), R * 0.4f, IM_COL32(255, 255, 255, 140), 24);
        }
    }

    // Variables (contadores) arriba a la izquierda.
    if (g.proj.showVars && !g.vars.empty()) {
        float y = fs * 0.5f;
        for (auto& v : g.vars) {
            char buf[96];
            snprintf(buf, sizeof buf, "%s: %g", v.first.c_str(), (double)v.second);
            ImVec2 ts = font->CalcTextSizeA(fs, 1e9f, 0.f, buf);
            dl->AddRectFilled(ImVec2(fs * 0.4f, y - 2), ImVec2(fs * 0.4f + ts.x + 12, y + ts.y + 2), IM_COL32(0, 0, 0, 110), 4.f);
            dl->AddText(font, fs, ImVec2(fs * 0.4f + 6, y), IM_COL32(255, 255, 255, 240), buf);
            y += ts.y + 8;
        }
    }

    // Texto del bloque "decir".
    if (g.sayT > 0 && !g.say.empty()) {
        ImVec2 ts = font->CalcTextSizeA(fs * 1.2f, 1e9f, 0.f, g.say.c_str());
        float x = (w - ts.x) * 0.5f, y = h * 0.14f;
        dl->AddRectFilled(ImVec2(x - 14, y - 8), ImVec2(x + ts.x + 14, y + ts.y + 8), IM_COL32(0, 0, 0, 150), 8.f);
        dl->AddText(font, fs * 1.2f, ImVec2(x, y), IM_COL32(255, 255, 255, 255), g.say.c_str());
    }
}

}  // namespace forge
