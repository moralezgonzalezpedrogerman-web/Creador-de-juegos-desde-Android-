#pragma once
#include "game.h"
#include "gl.h"
#include <string>
#include <unordered_map>

namespace forge {

// Vista del editor: camara orbital (3D) o vista plana (2D) y objeto seleccionado.
struct EditorView {
    V3 eye, fwd, target;
    int sel = -1;
    float cx = 0, cy = 0, h2d = kView2DHeight;
};

// Renderizador ligero: un cubo compartido para todos los objetos, modelos .fmdl,
// sombra direccional con un mapa de profundidad de 16 bits y cielo procedural.
class Renderer {
public:
    bool init(int shadowSize = 1024);
    void shutdown();  // libera todo lo de GL (tambien al perder el contexto en Android)
    void render(Game& g, int w, int h, const EditorView* ev = nullptr);
    void clear(int w, int h, float r, float g, float b);
    // Id de textura GL de un asset (0 si no existe) y sus dimensiones.
    unsigned textureId(const Game& g, const std::string& name, int* w = nullptr, int* h = nullptr);

private:
    struct Prog {
        GLuint id = 0;
        std::unordered_map<std::string, GLint> loc;
        GLint u(const char* n);
    };
    struct TexInfo { GLuint id = 0; int w = 1, h = 1; };
    struct ModelGL {
        GLuint vao = 0, vbo = 0, ibo = 0;
        int count = 0;
        GLenum itype = GL_UNSIGNED_SHORT;
        std::string tex;
        float color[3] = {1, 1, 1};
        bool ok = false;
    };
    Prog main_, sky_, depth_;
    GLuint vao_ = 0, vbo_ = 0, ibo_ = 0, vaoEmpty_ = 0, fbo_ = 0, shadowTex_ = 0, white_ = 0;
    int shadowSize_ = 1024;
    std::unordered_map<std::string, TexInfo> tex_;
    std::unordered_map<std::string, ModelGL> models_;

    TexInfo& texture(const Game& g, const std::string& name);
    ModelGL* model(const Game& g, const std::string& name);
    void setupAttribs();
    void drawObjs(Game& g, Prog& p, bool shadow);
    void box(Prog& p, V3 pos, V3 scale, float r, float g, float b, float a);
    void render3D(Game& g, int w, int h, const EditorView* ev);
    void render2D(Game& g, int w, int h, const EditorView* ev);
};

}  // namespace forge
