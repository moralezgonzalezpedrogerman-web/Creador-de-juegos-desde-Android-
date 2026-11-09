#pragma once
#include "vec.h"
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace forge {

enum class Kind { Cube, Floor, Wall, Item, Model, Light, Player };
enum class Ev { Start, Interact, Near, Shot };

// Bloques estilo Pocket Code, adaptados.
enum class Op {
    SetPos, MovePos, Glide, Turn, SetSize, ChangeSize, SetColor, Show, Hide, Destroy,
    PlaySound, Music, StopMusic, Wait, Repeat, Forever, If, SetVar, ChangeVar,
    SwitchScene, Say
};

struct Block {
    Op op = Op::Wait;
    float a = 0, b = 0, c = 0, d = 0;
    std::string s;
    std::vector<Block> body, other;  // cuerpo (repetir/si/por siempre) y "si no"
};

struct Script {
    Ev ev = Ev::Start;
    std::vector<Block> blocks;
};

struct Obj {
    std::string name = "objeto";
    Kind kind = Kind::Cube;
    V3 pos, scale{1, 1, 1};
    float yaw = 0;
    float color[3] = {1, 1, 1};
    std::string tex;    // asset png/jpg ("" = sin textura)
    std::string model;  // asset .fmdl (modelo 3D importado)
    bool solid = true, visible = true, alive = true, spin = false;
    std::vector<Script> scripts;
};

struct Scene {
    std::string name = "main";
    bool is3d = true;
    V3 sunDir{-0.4f, -1.f, -0.3f};
    float sun[3] = {1.f, 0.95f, 0.85f};
    float skyTop[3] = {0.25f, 0.45f, 0.8f};
    float skyBot[3] = {0.75f, 0.85f, 0.95f};
    float ambient = 0.35f;
    float fog = 0.012f;
    V3 start{0, 0.05f, 0};
    std::vector<Obj> objs;
};

// Botones tactiles en pantalla (escenas 3D).
enum class HudAct { None, Jump, Shoot, Interact };
struct HudBtn {
    std::string label = "OK";
    float x = 0.5f, y = 0.5f, r = 0.08f;  // centro (0..1) y radio (fraccion del alto)
    HudAct act = HudAct::None;
    float color[3] = {1, 1, 1};
};

// Arma en primera persona.
struct Weapon {
    bool enabled = true;
    std::string tex;      // sprite (asset png/jpg); vacio = arma basica dibujada
    float scale = 0.5f;   // alto del sprite (fraccion del alto de pantalla)
    float ox = 0, oy = 0; // desplazamiento (fraccion de pantalla)
    float rate = 0.25f;   // segundos entre disparos
    float range = 60.f;
};

using Files = std::map<std::string, std::vector<uint8_t>>;

struct Project {
    std::string name = "juego";
    std::vector<Scene> scenes;
    Files assets;  // texturas, sonidos, modelos
    bool hudDefault = true;       // true = botones por defecto (saltar y disparar)
    std::vector<HudBtn> hud;      // botones propios (si hudDefault == false; vacio = sin botones)
    Weapon weapon;
    bool showVars = true;
};

std::vector<HudBtn> defaultHud();

bool loadProject(const Files& files, Project& out, std::string& err);
std::string writeProject(const Project& p);  // genera project.forge
bool loadDemo(Project& out);

const char* kindName(Kind k);
const char* evName(Ev e);

}  // namespace forge
