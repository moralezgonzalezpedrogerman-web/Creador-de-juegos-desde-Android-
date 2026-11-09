#pragma once
#include "scene.h"
#include <functional>
#include <string>
#include <vector>

namespace forge {

constexpr float kView2DHeight = 10.f;  // alto visible (unidades) en escenas 2D

struct Input {
    float moveX = 0, moveY = 0;      // -1..1 (adelante = +Y)
    float lookX = 0, lookY = 0;      // radianes a girar este frame
    bool jump = false;
    bool shoot = false;              // disparar (se mantiene)
    bool tap = false;                // toque/clic: interactuar
    float tapX = 0.5f, tapY = 0.5f;  // 0..1 en pantalla (escenas 2D)
};

struct Frame { const std::vector<Block>* list; size_t idx; int loop; };
struct Runner {
    int obj; int script;
    std::vector<Frame> stack;
    float wait = 0;
    float gT = 0; V3 gV;  // deslizamiento en curso
};

class Game {
public:
    Project proj;
    Scene* sc = nullptr;

    V3 pos, vel;
    float yaw = 0, pitch = 0;
    bool ground = false;
    float time = 0;

    // Arma
    float shootCd = 0, recoil = 0, flash = 0;

    std::map<std::string, float> vars;
    std::string say;     // texto del bloque "decir"
    float sayT = 0;

    std::function<void(const std::string&)> onSound;
    std::function<void(const std::string&, bool)> onMusic;  // (nombre, encender)

    void load(Project p);
    void enterScene(const std::string& name);
    void bindScene(int idx);  // solo apunta a la escena (editor), sin ejecutar scripts
    void update(float dt, const Input& in, float aspect);

    V3 eye() const { return {pos.x, pos.y + 1.6f, pos.z}; }
    V3 forward() const;

private:
    std::vector<Runner> runners_;
    std::vector<char> near_;
    std::vector<V3> base_;
    std::string pending_;

    bool blocked(V3 p) const;
    void fire(int obj, Ev ev);
    void start(int obj, int script);
    void step(Runner& r, float dt);
    void tap3D();
    void shoot3D();
    void tap2D(float x, float y, float aspect);
};

}  // namespace forge
