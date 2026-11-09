#include "game.h"
#include <algorithm>
#include <cmath>

namespace forge {

static bool solidKind(Kind k) {
    return k == Kind::Cube || k == Kind::Floor || k == Kind::Wall || k == Kind::Model;
}

static bool hasEvent(const Obj& o, Ev e) {
    for (auto& s : o.scripts) if (s.ev == e) return true;
    return false;
}

V3 Game::forward() const {
    return {std::sin(yaw) * std::cos(pitch), std::sin(pitch), -std::cos(yaw) * std::cos(pitch)};
}

void Game::load(Project p) {
    proj = std::move(p);
    sc = nullptr;
    vars.clear();
    runners_.clear();
    shootCd = recoil = flash = 0;
    if (!proj.scenes.empty()) enterScene(proj.scenes[0].name);
}

void Game::bindScene(int idx) {
    if (idx < 0 || idx >= (int)proj.scenes.size()) { sc = nullptr; return; }
    sc = &proj.scenes[idx];
}

void Game::enterScene(const std::string& name) {
    Scene* found = nullptr;
    for (auto& s : proj.scenes) if (s.name == name) { found = &s; break; }
    if (!found) return;
    sc = found;
    runners_.clear();
    near_.assign(sc->objs.size(), 0);
    base_.clear();
    for (auto& o : sc->objs) base_.push_back(o.scale);
    pos = sc->start;
    vel = V3();
    yaw = 0;
    pitch = 0;
    for (size_t i = 0; i < sc->objs.size(); i++) fire((int)i, Ev::Start);
}

void Game::start(int obj, int script) {
    for (auto& r : runners_) if (r.obj == obj && r.script == script) return;
    Runner r;
    r.obj = obj;
    r.script = script;
    r.stack.push_back({&sc->objs[obj].scripts[script].blocks, 0, 0});
    runners_.push_back(std::move(r));
}

void Game::fire(int obj, Ev ev) {
    auto& scripts = sc->objs[obj].scripts;
    for (size_t j = 0; j < scripts.size(); j++)
        if (scripts[j].ev == ev) start(obj, (int)j);
}

bool Game::blocked(V3 p) const {
    const float hw = 0.3f, h = 1.7f, eps = 1e-4f;
    for (auto& o : sc->objs) {
        if (!o.alive || !o.visible || !o.solid || !solidKind(o.kind)) continue;
        if (p.x + hw <= o.pos.x - o.scale.x * .5f + eps || p.x - hw >= o.pos.x + o.scale.x * .5f - eps) continue;
        if (p.y + h <= o.pos.y - o.scale.y * .5f + eps || p.y >= o.pos.y + o.scale.y * .5f - eps) continue;
        if (p.z + hw <= o.pos.z - o.scale.z * .5f + eps || p.z - hw >= o.pos.z + o.scale.z * .5f - eps) continue;
        return true;
    }
    return false;
}

void Game::tap3D() {
    V3 e = eye(), d = forward();
    float best = 4.f;
    int hit = -1;
    for (size_t i = 0; i < sc->objs.size(); i++) {
        auto& o = sc->objs[i];
        if (!o.alive || !o.visible) continue;
        if (!(hasEvent(o, Ev::Interact) || (o.solid && solidKind(o.kind)))) continue;
        float t;
        if (rayBox(e, d, o.pos, o.scale * .5f, t) && t < best) { best = t; hit = (int)i; }
    }
    if (hit >= 0) fire(hit, Ev::Interact);
}

void Game::shoot3D() {
    V3 e = eye(), d = forward();
    float best = proj.weapon.range;
    int hit = -1;
    for (size_t i = 0; i < sc->objs.size(); i++) {
        auto& o = sc->objs[i];
        if (!o.alive || !o.visible) continue;
        if (!(hasEvent(o, Ev::Shot) || (o.solid && solidKind(o.kind)))) continue;
        float t;
        if (rayBox(e, d, o.pos, o.scale * .5f, t) && t < best) { best = t; hit = (int)i; }
    }
    if (hit >= 0) fire(hit, Ev::Shot);
}

void Game::tap2D(float x, float y, float aspect) {
    float wx = (x - .5f) * kView2DHeight * aspect, wy = (.5f - y) * kView2DHeight;
    for (int i = (int)sc->objs.size() - 1; i >= 0; i--) {
        auto& o = sc->objs[i];
        if (!o.alive || !o.visible || !hasEvent(o, Ev::Interact)) continue;
        if (std::fabs(wx - o.pos.x) <= o.scale.x * .5f && std::fabs(wy - o.pos.y) <= o.scale.y * .5f) {
            fire(i, Ev::Interact);
            return;
        }
    }
}

void Game::update(float dt, const Input& in, float aspect) {
    if (!pending_.empty()) {
        std::string n = pending_;
        pending_.clear();
        enterScene(n);
    }
    if (!sc) return;
    dt = std::min(dt, 0.05f);
    time += dt;
    if (sayT > 0) sayT -= dt;

    if (sc->is3d) {
        yaw += in.lookX;
        pitch = std::max(-1.5f, std::min(1.5f, pitch - in.lookY));
        V3 f{std::sin(yaw), 0, -std::cos(yaw)}, r{std::cos(yaw), 0, std::sin(yaw)};
        V3 w = f * in.moveY + r * in.moveX;
        float l = len(w);
        if (l > 1) w = w * (1.f / l);
        vel.x = w.x * 4.5f;
        vel.z = w.z * 4.5f;
        vel.y -= 20.f * dt;
        if (in.jump && ground) vel.y = 7.f;

        V3 p = pos;
        p.x += vel.x * dt; if (blocked(p)) p.x = pos.x;
        p.z += vel.z * dt; if (blocked(p)) p.z = pos.z;
        V3 q = p;
        q.y += vel.y * dt;
        ground = false;
        if (blocked(q)) { if (vel.y < 0) ground = true; vel.y = 0; }
        else p.y = q.y;
        pos = p;
        if (pos.y < -60) { pos = sc->start; vel = V3(); }

        // Evento "cerca": se dispara al entrar en el radio.
        for (size_t i = 0; i < sc->objs.size(); i++) {
            auto& o = sc->objs[i];
            if (!o.alive || !hasEvent(o, Ev::Near)) continue;
            float dx = o.pos.x - pos.x, dz = o.pos.z - pos.z, dy = o.pos.y - (pos.y + 1.f);
            bool isNear = std::sqrt(dx * dx + dz * dz) < 1.2f + .5f * std::max(o.scale.x, o.scale.z) && std::fabs(dy) < 3.f;
            if (isNear && !near_[i]) fire((int)i, Ev::Near);
            near_[i] = isNear;
        }
        if (in.tap) tap3D();

        // Disparo
        shootCd = std::max(0.f, shootCd - dt);
        recoil = std::max(0.f, recoil - dt * 6.f);
        flash = std::max(0.f, flash - dt);
        if (in.shoot && shootCd <= 0 && proj.weapon.enabled) {
            shootCd = proj.weapon.rate;
            recoil = 1.f;
            flash = 0.07f;
            if (onSound) onSound("disparo");
            shoot3D();
        }
    } else if (in.tap) {
        tap2D(in.tapX, in.tapY, aspect);
    }

    for (size_t i = 0; i < runners_.size(); i++) step(runners_[i], dt);
    runners_.erase(std::remove_if(runners_.begin(), runners_.end(),
                                  [](const Runner& r) { return r.stack.empty(); }),
                   runners_.end());
}

void Game::step(Runner& r, float dt) {
    Obj& o = sc->objs[r.obj];
    if (r.gT > 0) {
        float s = std::min(dt, r.gT);
        o.pos = o.pos + r.gV * s;
        r.gT -= dt;
        if (r.gT > 0) return;
    }
    if (r.wait > 0) {
        r.wait -= dt;
        if (r.wait > 0) return;
    }
    int budget = 256;
    while (!r.stack.empty() && budget-- > 0) {
        Frame& f = r.stack.back();
        if (f.idx >= f.list->size()) {
            if (f.loop == -1 || f.loop > 0) {  // otra vuelta, cediendo un frame
                if (f.loop > 0) f.loop--;
                f.idx = 0;
                return;
            }
            r.stack.pop_back();
            continue;
        }
        const Block& b = (*f.list)[f.idx++];
        switch (b.op) {
            case Op::SetPos: o.pos = {b.a, b.b, b.c}; break;
            case Op::MovePos: o.pos = o.pos + V3{b.a, b.b, b.c}; break;
            case Op::Glide:
                if (b.d <= 0) o.pos = o.pos + V3{b.a, b.b, b.c};
                else { r.gV = V3{b.a, b.b, b.c} * (1.f / b.d); r.gT = b.d; return; }
                break;
            case Op::Turn: o.yaw += b.a; break;
            case Op::SetSize: o.scale = base_[r.obj] * (b.a / 100.f); break;
            case Op::ChangeSize: o.scale = o.scale + base_[r.obj] * (b.a / 100.f); break;
            case Op::SetColor: o.color[0] = b.a; o.color[1] = b.b; o.color[2] = b.c; break;
            case Op::Show: o.visible = true; break;
            case Op::Hide: o.visible = false; break;
            case Op::Destroy: o.alive = false; break;
            case Op::PlaySound: if (onSound) onSound(b.s); break;
            case Op::Music: if (onMusic) onMusic(b.s, true); break;
            case Op::StopMusic: if (onMusic) onMusic("", false); break;
            case Op::Wait: r.wait = b.a; return;
            case Op::Repeat: { int n = (int)b.a; if (n > 0) r.stack.push_back({&b.body, 0, n - 1}); break; }
            case Op::Forever: r.stack.push_back({&b.body, 0, -1}); break;
            case Op::If: {
                float v = vars[b.s];
                bool ok = b.b == 0 ? v == b.a : b.b == 1 ? v > b.a : v < b.a;
                const std::vector<Block>* l = ok ? &b.body : &b.other;
                if (!l->empty()) r.stack.push_back({l, 0, 0});
                break;
            }
            case Op::SetVar: vars[b.s] = b.a; break;
            case Op::ChangeVar: vars[b.s] += b.a; break;
            case Op::SwitchScene: pending_ = b.s; r.stack.clear(); return;
            case Op::Say: say = b.s; sayT = 3.f; break;
        }
    }
}

}  // namespace forge
