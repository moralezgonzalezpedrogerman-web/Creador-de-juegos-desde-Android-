#include "scene.h"
#include <cstdio>
#include <cstdlib>
#include <sstream>

namespace forge {

static std::vector<std::string> split(const std::string& l) {
    std::vector<std::string> t;
    std::istringstream ss(l);
    std::string w;
    while (ss >> w) t.push_back(w);
    return t;
}

const char* kindName(Kind k) {
    switch (k) {
        case Kind::Cube: return "cube";
        case Kind::Floor: return "floor";
        case Kind::Wall: return "wall";
        case Kind::Item: return "item";
        case Kind::Model: return "model";
        case Kind::Light: return "light";
        case Kind::Player: return "player";
    }
    return "cube";
}

const char* evName(Ev e) {
    switch (e) {
        case Ev::Start: return "Start";
        case Ev::Interact: return "Interact";
        case Ev::Near: return "Near";
        case Ev::Shot: return "Shot";
    }
    return "Start";
}

static bool kindFromName(const std::string& s, Kind& k) {
    for (int i = 0; i <= (int)Kind::Player; i++)
        if (s == kindName((Kind)i)) { k = (Kind)i; return true; }
    return false;
}

static const char* actName(HudAct a) {
    switch (a) {
        case HudAct::None: return "none";
        case HudAct::Jump: return "jump";
        case HudAct::Shoot: return "shoot";
        case HudAct::Interact: return "interact";
    }
    return "none";
}

std::vector<HudBtn> defaultHud() {
    HudBtn s;
    s.label = "DISPARAR"; s.x = 0.88f; s.y = 0.68f; s.r = 0.12f; s.act = HudAct::Shoot;
    s.color[0] = 1.f; s.color[1] = 0.35f; s.color[2] = 0.3f;
    HudBtn j;
    j.label = "SALTAR"; j.x = 0.73f; j.y = 0.86f; j.r = 0.09f; j.act = HudAct::Jump;
    j.color[0] = 0.3f; j.color[1] = 0.7f; j.color[2] = 1.f;
    return {s, j};
}

// ---------------------------------------------------------------- lectura
struct Parser {
    std::vector<std::string> lines;
    size_t i = 0;
    std::string err;

    bool blocks(std::vector<Block>& out) {
        while (i < lines.size()) {
            auto t = split(lines[i]);
            if (t.empty() || t[0][0] == '#') { i++; continue; }
            if (t[0] == "}" || t[0] == "end") return true;

            Block b;
            const std::string& n = t[0];
            auto F = [&](size_t k) { return k < t.size() ? (float)atof(t[k].c_str()) : 0.f; };
            auto S = [&](size_t k) { return k < t.size() ? t[k] : std::string(); };
            bool open = t.back() == "{";

            if (n == "setpos") { b.op = Op::SetPos; b.a = F(1); b.b = F(2); b.c = F(3); }
            else if (n == "movepos") { b.op = Op::MovePos; b.a = F(1); b.b = F(2); b.c = F(3); }
            else if (n == "glide") { b.op = Op::Glide; b.a = F(1); b.b = F(2); b.c = F(3); b.d = F(4); }
            else if (n == "turn") { b.op = Op::Turn; b.a = F(1); }
            else if (n == "size") { b.op = Op::SetSize; b.a = F(1); }
            else if (n == "chsize") { b.op = Op::ChangeSize; b.a = F(1); }
            else if (n == "color") { b.op = Op::SetColor; b.a = F(1); b.b = F(2); b.c = F(3); }
            else if (n == "show") b.op = Op::Show;
            else if (n == "hide") b.op = Op::Hide;
            else if (n == "destroy") b.op = Op::Destroy;
            else if (n == "sound") { b.op = Op::PlaySound; b.s = S(1); }
            else if (n == "music") { b.op = Op::Music; b.s = S(1); }
            else if (n == "stopmusic") b.op = Op::StopMusic;
            else if (n == "wait") { b.op = Op::Wait; b.a = F(1); }
            else if (n == "repeat") { b.op = Op::Repeat; b.a = F(1); }
            else if (n == "forever") b.op = Op::Forever;
            else if (n == "if") {
                b.op = Op::If;
                b.s = S(1);
                std::string c = t.size() > 2 ? t[2] : "==";
                b.b = c == ">" ? 1.f : c == "<" ? 2.f : 0.f;
                b.a = F(3);
            }
            else if (n == "set") { b.op = Op::SetVar; b.s = S(1); b.a = F(2); }
            else if (n == "add") { b.op = Op::ChangeVar; b.s = S(1); b.a = F(2); }
            else if (n == "scene") { b.op = Op::SwitchScene; b.s = S(1); }
            else if (n == "say") {
                b.op = Op::Say;
                for (size_t k = 1; k < t.size(); k++) {
                    if (open && k == t.size() - 1) break;
                    if (k > 1) b.s += ' ';
                    b.s += t[k];
                }
            }
            else { err = "bloque desconocido: " + n; return false; }

            i++;
            if (open) {
                if (!blocks(b.body)) return false;
                if (i >= lines.size()) { err = "falta una llave de cierre"; return false; }
                auto c = split(lines[i]);
                if (c.empty() || c[0] != "}") { err = "falta una llave de cierre"; return false; }
                bool els = c.size() >= 3 && c[1] == "else" && c.back() == "{";
                i++;
                if (els) {
                    if (!blocks(b.other)) return false;
                    if (i >= lines.size()) { err = "falta una llave de cierre"; return false; }
                    i++;
                }
            }
            out.push_back(std::move(b));
        }
        return true;
    }
};

static std::string unders(std::string s) {
    for (auto& c : s) if (c == '_') c = ' ';
    return s;
}

bool loadProject(const Files& files, Project& p, std::string& err) {
    auto it = files.find("project.forge");
    if (it == files.end()) { err = "falta project.forge"; return false; }

    Parser ps;
    {
        std::string text(it->second.begin(), it->second.end());
        std::istringstream ss(text);
        std::string l;
        while (std::getline(ss, l)) {
            if (!l.empty() && l.back() == '\r') l.pop_back();
            ps.lines.push_back(l);
        }
    }
    p = Project();
    for (auto& f : files)
        if (f.first != "project.forge") p.assets[f.first] = f.second;

    while (ps.i < ps.lines.size()) {
        auto t = split(ps.lines[ps.i]);
        ps.i++;
        if (t.empty() || t[0][0] == '#') continue;
        auto F = [&](size_t k) { return k < t.size() ? (float)atof(t[k].c_str()) : 0.f; };

        if (t[0] == "project") { if (t.size() > 1) p.name = t[1]; continue; }
        if (t[0] == "hud") {
            p.hudDefault = !(t.size() > 1 && (t[1] == "custom" || t[1] == "none"));
            continue;
        }
        if (t[0] == "button") {
            HudBtn b;
            b.label = t.size() > 1 ? unders(t[1]) : "OK";
            b.x = F(2); b.y = F(3); b.r = F(4);
            std::string a = t.size() > 5 ? t[5] : "none";
            b.act = a == "jump" ? HudAct::Jump : a == "shoot" ? HudAct::Shoot : a == "interact" ? HudAct::Interact : HudAct::None;
            for (int k = 0; k < 3; k++) b.color[k] = t.size() > (size_t)(6 + k) ? F(6 + k) : 1.f;
            p.hud.push_back(b);
            continue;
        }
        if (t[0] == "weapon") {
            Weapon& w = p.weapon;
            w.tex = (t.size() > 1 && t[1] != "-") ? t[1] : "";
            w.scale = F(2); w.ox = F(3); w.oy = F(4); w.rate = F(5); w.range = F(6);
            w.enabled = t.size() > 7 ? F(7) != 0 : true;
            if (w.scale <= 0) w.scale = 0.5f;
            if (w.rate <= 0) w.rate = 0.25f;
            if (w.range <= 0) w.range = 60.f;
            continue;
        }
        if (t[0] == "showvars") { p.showVars = F(1) != 0; continue; }

        if (t[0] == "scene") {
            Scene s;
            s.name = t.size() > 1 ? t[1] : "escena";
            s.is3d = !(t.size() > 2 && t[2] == "2d");
            p.scenes.push_back(s);
            continue;
        }
        if (p.scenes.empty()) { err = "linea fuera de una escena: " + t[0]; return false; }
        Scene& s = p.scenes.back();

        if (t[0] == "sky") {
            for (int k = 0; k < 3; k++) { s.skyTop[k] = F(1 + k); s.skyBot[k] = F(4 + k); }
        } else if (t[0] == "sun") {
            s.sunDir = {F(1), F(2), F(3)};
            for (int k = 0; k < 3; k++) s.sun[k] = F(4 + k);
        } else if (t[0] == "env") {
            s.ambient = F(1); s.fog = F(2);
        } else if (t[0] == "start") {
            s.start = {F(1), F(2), F(3)};
        } else if (t[0] == "obj") {
            Obj o;
            if (t.size() < 3 || !kindFromName(t[1], o.kind)) { err = "objeto invalido"; return false; }
            o.name = t[2];
            o.pos = {F(3), F(4), F(5)};
            o.scale = {t.size() > 6 ? F(6) : 1.f, t.size() > 7 ? F(7) : 1.f, t.size() > 8 ? F(8) : 1.f};
            o.yaw = F(9);
            for (int k = 0; k < 3; k++) o.color[k] = t.size() > (size_t)(10 + k) ? F(10 + k) : 1.f;
            if (t.size() > 13 && t[13] != "-") o.tex = t[13];
            if (t.size() > 14) o.solid = F(14) != 0;
            if (t.size() > 15) o.visible = F(15) != 0;
            if (t.size() > 16) o.spin = F(16) != 0;
            if (t.size() > 17 && t[17] != "-") o.model = t[17];
            s.objs.push_back(o);
        } else if (t[0] == "on") {
            if (s.objs.empty()) { err = "'on' sin objeto"; return false; }
            Script sc;
            std::string e = t.size() > 1 ? t[1] : "";
            if (e == "Start") sc.ev = Ev::Start;
            else if (e == "Interact") sc.ev = Ev::Interact;
            else if (e == "Near") sc.ev = Ev::Near;
            else if (e == "Shot") sc.ev = Ev::Shot;
            else { err = "evento desconocido: " + e; return false; }
            if (!ps.blocks(sc.blocks)) { err = ps.err; return false; }
            ps.i++;  // consume 'end'
            s.objs.back().scripts.push_back(std::move(sc));
        } else {
            err = "instruccion desconocida: " + t[0];
            return false;
        }
    }
    if (p.scenes.empty()) { err = "el proyecto no tiene escenas"; return false; }
    return true;
}

// ---------------------------------------------------------------- escritura
static std::string fmt(float v) {
    char b[48];
    snprintf(b, sizeof b, "%g", (double)v);
    return b;
}

static std::string noSpace(std::string s) {
    if (s.empty()) return "-";
    for (auto& c : s) if (c == ' ' || c == '\t') c = '_';
    return s;
}

static void writeBlocks(std::ostringstream& o, const std::vector<Block>& bs, int ind) {
    std::string pad(ind * 2, ' ');
    for (auto& b : bs) {
        o << pad;
        switch (b.op) {
            case Op::SetPos: o << "setpos " << fmt(b.a) << ' ' << fmt(b.b) << ' ' << fmt(b.c); break;
            case Op::MovePos: o << "movepos " << fmt(b.a) << ' ' << fmt(b.b) << ' ' << fmt(b.c); break;
            case Op::Glide: o << "glide " << fmt(b.a) << ' ' << fmt(b.b) << ' ' << fmt(b.c) << ' ' << fmt(b.d); break;
            case Op::Turn: o << "turn " << fmt(b.a); break;
            case Op::SetSize: o << "size " << fmt(b.a); break;
            case Op::ChangeSize: o << "chsize " << fmt(b.a); break;
            case Op::SetColor: o << "color " << fmt(b.a) << ' ' << fmt(b.b) << ' ' << fmt(b.c); break;
            case Op::Show: o << "show"; break;
            case Op::Hide: o << "hide"; break;
            case Op::Destroy: o << "destroy"; break;
            case Op::PlaySound: o << "sound " << noSpace(b.s); break;
            case Op::Music: o << "music " << noSpace(b.s); break;
            case Op::StopMusic: o << "stopmusic"; break;
            case Op::Wait: o << "wait " << fmt(b.a); break;
            case Op::Repeat: o << "repeat " << fmt(b.a) << " {"; break;
            case Op::Forever: o << "forever {"; break;
            case Op::If:
                o << "if " << noSpace(b.s) << ' ' << (b.b == 1 ? ">" : b.b == 2 ? "<" : "==") << ' ' << fmt(b.a) << " {";
                break;
            case Op::SetVar: o << "set " << noSpace(b.s) << ' ' << fmt(b.a); break;
            case Op::ChangeVar: o << "add " << noSpace(b.s) << ' ' << fmt(b.a); break;
            case Op::SwitchScene: o << "scene " << noSpace(b.s); break;
            case Op::Say: o << "say " << (b.s.empty() ? "..." : b.s); break;
        }
        o << '\n';
        if (b.op == Op::Repeat || b.op == Op::Forever || b.op == Op::If) {
            writeBlocks(o, b.body, ind + 1);
            if (b.op == Op::If && !b.other.empty()) {
                o << pad << "} else {\n";
                writeBlocks(o, b.other, ind + 1);
            }
            o << pad << "}\n";
        }
    }
}

std::string writeProject(const Project& p) {
    std::ostringstream o;
    o << "project " << noSpace(p.name) << '\n';
    if (p.hudDefault) {
        o << "hud default\n";
    } else {
        o << "hud custom\n";
        for (auto& b : p.hud)
            o << "button " << noSpace(b.label) << ' ' << fmt(b.x) << ' ' << fmt(b.y) << ' ' << fmt(b.r) << ' '
              << actName(b.act) << ' ' << fmt(b.color[0]) << ' ' << fmt(b.color[1]) << ' ' << fmt(b.color[2]) << '\n';
    }
    o << "weapon " << noSpace(p.weapon.tex) << ' ' << fmt(p.weapon.scale) << ' ' << fmt(p.weapon.ox) << ' '
      << fmt(p.weapon.oy) << ' ' << fmt(p.weapon.rate) << ' ' << fmt(p.weapon.range) << ' '
      << (p.weapon.enabled ? 1 : 0) << '\n';
    o << "showvars " << (p.showVars ? 1 : 0) << '\n';

    for (auto& s : p.scenes) {
        o << "\nscene " << noSpace(s.name) << ' ' << (s.is3d ? "3d" : "2d") << '\n';
        o << "sky " << fmt(s.skyTop[0]) << ' ' << fmt(s.skyTop[1]) << ' ' << fmt(s.skyTop[2]) << ' '
          << fmt(s.skyBot[0]) << ' ' << fmt(s.skyBot[1]) << ' ' << fmt(s.skyBot[2]) << '\n';
        o << "sun " << fmt(s.sunDir.x) << ' ' << fmt(s.sunDir.y) << ' ' << fmt(s.sunDir.z) << ' '
          << fmt(s.sun[0]) << ' ' << fmt(s.sun[1]) << ' ' << fmt(s.sun[2]) << '\n';
        o << "env " << fmt(s.ambient) << ' ' << fmt(s.fog) << '\n';
        o << "start " << fmt(s.start.x) << ' ' << fmt(s.start.y) << ' ' << fmt(s.start.z) << '\n';
        for (auto& ob : s.objs) {
            o << "obj " << kindName(ob.kind) << ' ' << noSpace(ob.name) << ' '
              << fmt(ob.pos.x) << ' ' << fmt(ob.pos.y) << ' ' << fmt(ob.pos.z) << ' '
              << fmt(ob.scale.x) << ' ' << fmt(ob.scale.y) << ' ' << fmt(ob.scale.z) << ' ' << fmt(ob.yaw) << ' '
              << fmt(ob.color[0]) << ' ' << fmt(ob.color[1]) << ' ' << fmt(ob.color[2]) << ' '
              << noSpace(ob.tex) << ' ' << (ob.solid ? 1 : 0) << ' ' << (ob.visible ? 1 : 0) << ' '
              << (ob.spin ? 1 : 0) << ' ' << noSpace(ob.model) << '\n';
            for (auto& sc : ob.scripts) {
                o << "on " << evName(sc.ev) << '\n';
                writeBlocks(o, sc.blocks, 1);
                o << "end\n";
            }
        }
    }
    return o.str();
}

}  // namespace forge
