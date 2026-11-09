#include "scene.h"

namespace forge {

static const char* kDemo = R"FORGE(
project demo

# Menu en 2D (como Pocket Code)
scene Menu 2d
sky 0.10 0.12 0.25 0.20 0.25 0.45
obj cube Boton 0 0 0 5 1.4 0.2 0 0.95 0.55 0.15 - 0 1 0
on Interact
  scene Nivel1
end

# Nivel en 3D (como FPS Creator)
scene Nivel1 3d
sky 0.22 0.42 0.80 0.80 0.88 0.95
sun -0.45 -1.0 -0.35 1.0 0.95 0.85
env 0.35 0.010
start 0 0.05 6

obj floor Suelo 0 -0.1 0 40 0.2 40 0 0.45 0.62 0.40 - 1 1 0
obj wall Pared_fondo 0 1.5 -10 12 3 0.5 0 0.80 0.80 0.86 - 1 1 0
obj wall Pared_izq -6 1.5 -5 0.5 3 10 0 0.80 0.80 0.86 - 1 1 0
obj wall Pared_der 6 1.5 -5 0.5 3 10 0 0.80 0.80 0.86 - 1 1 0

# Puerta: al interactuar sube y deja pasar
obj wall Puerta 0 1.2 -4 3 2.4 0.4 0 0.55 0.35 0.20 - 1 1 0
on Interact
  sound puerta
  repeat 24 {
    movepos 0 0.1 0
    wait 0.02
  }
end

obj item Gema 3 1.0 -3 0.5 0.5 0.5 0 1.0 0.80 0.10 - 0 1 1
on Interact
  sound recoger
  add gemas 1
  say Gema recogida
  destroy
end

obj item Gema2 -3 1.0 -7 0.5 0.5 0.5 0 0.2 0.8 1.0 - 0 1 1
on Interact
  add gemas 1
  say Gema recogida
  destroy
end

obj light Lampara -4 3 -6 14 1 1 0 1.0 0.70 0.40 - 0 1 0

obj cube Cubo_giratorio 3 0.6 -6 1.2 1.2 1.2 0 0.9 0.25 0.25 - 1 1 0
on Start
  forever {
    turn 2
  }
end
)FORGE";

bool loadDemo(Project& out) {
    std::string text = kDemo;
    Files f;
    f["project.forge"] = std::vector<uint8_t>(text.begin(), text.end());
    std::string err;
    return loadProject(f, out, err);
}

}  // namespace forge
