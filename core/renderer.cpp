#include "renderer.h"
#include "model.h"
#include <cmath>
#include <cstdio>

#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG
#define STBI_ONLY_JPEG
#define STBI_NO_STDIO
#include "stb_image.h"

#ifdef FORGE_GLES
#define SHV "#version 300 es\nprecision highp float;\nprecision highp int;\nprecision highp sampler2D;\nprecision highp sampler2DShadow;\n"
#else
#define SHV "#version 330 core\n"
#endif

namespace forge {

// ---------------------------------------------------------------- shaders
static const char* kMainVS = SHV R"(
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aN;
layout(location=2) in vec2 aUV;
uniform mat4 uVP, uM, uLVP;
uniform int uUV1;
uniform float uTile;
out vec3 vN; out vec3 vWP; out vec2 vUV; out vec4 vLP;
void main(){
  vec4 wp = uM * vec4(aPos, 1.0);
  vWP = wp.xyz;
  vN = normalize(mat3(uM) * aN);
  if (uUV1 == 1) vUV = aUV;
  else {
    vec3 n = abs(vN);
    vec2 uv = (n.y > 0.5) ? wp.xz : ((n.x > 0.5) ? wp.zy : wp.xy);
    vUV = uv / uTile;
  }
  vLP = uLVP * wp;
  gl_Position = uVP * wp;
}
)";

static const char* kMainFS = SHV R"(
in vec3 vN; in vec3 vWP; in vec2 vUV; in vec4 vLP;
out vec4 oColor;
uniform sampler2D uTex;
uniform sampler2DShadow uSh;
uniform vec4 uColor;
uniform vec3 uSunDir, uSunCol, uSkyTop, uSkyBot, uCam;
uniform float uAmb, uFog, uTexel;
uniform int uUnlit, uNL;
uniform vec4 uPL[4];
uniform vec3 uPC[4];
void main(){
  vec4 t = texture(uTex, vUV) * uColor;
  if (uUnlit == 1) { oColor = t; return; }
  vec3 n = normalize(vN);
  float ndl = max(dot(n, -uSunDir), 0.0);
  vec3 p = vLP.xyz / vLP.w * 0.5 + 0.5;
  float sh = 1.0;
  if (p.z <= 1.0 && p.x > 0.0 && p.x < 1.0 && p.y > 0.0 && p.y < 1.0) {
    float b = 0.0012;
    sh = 0.25 * (texture(uSh, vec3(p.xy + uTexel * vec2(-0.5, -0.5), p.z - b))
               + texture(uSh, vec3(p.xy + uTexel * vec2( 0.5, -0.5), p.z - b))
               + texture(uSh, vec3(p.xy + uTexel * vec2(-0.5,  0.5), p.z - b))
               + texture(uSh, vec3(p.xy + uTexel * vec2( 0.5,  0.5), p.z - b)));
  }
  vec3 hemi = mix(uSkyBot * 0.5, uSkyTop, n.y * 0.5 + 0.5) * uAmb * 1.5;
  vec3 light = hemi + uSunCol * ndl * sh;
  for (int i = 0; i < 4; i++) {
    if (i >= uNL) break;
    vec3 d = uPL[i].xyz - vWP;
    float dist = length(d);
    float at = clamp(1.0 - dist / uPL[i].w, 0.0, 1.0);
    light += uPC[i] * max(dot(n, d / max(dist, 0.001)), 0.0) * at * at * 1.5;
  }
  vec3 c = t.rgb * light;
  float f = clamp(exp(-uFog * length(uCam - vWP)), 0.0, 1.0);
  oColor = vec4(mix(uSkyBot, c, f), t.a);
}
)";

static const char* kDepthVS = SHV R"(
layout(location=0) in vec3 aPos;
uniform mat4 uLVP, uM;
void main(){ gl_Position = uLVP * uM * vec4(aPos, 1.0); }
)";
static const char* kDepthFS = SHV R"(
void main(){}
)";

static const char* kSkyVS = SHV R"(
uniform vec3 uFwd, uRight, uUp;
uniform float uTX, uTY;
out vec3 vRay;
void main(){
  vec2 p = vec2(float((gl_VertexID << 1) & 2), float(gl_VertexID & 2)) * 2.0 - 1.0;
  vRay = uFwd + uRight * p.x * uTX + uUp * p.y * uTY;
  gl_Position = vec4(p, 0.0, 1.0);
}
)";
static const char* kSkyFS = SHV R"(
in vec3 vRay;
out vec4 oColor;
uniform vec3 uTop, uBot, uSunDir, uSunCol;
void main(){
  vec3 d = normalize(vRay);
  float t = clamp(d.y * 1.2 + 0.2, 0.0, 1.0);
  vec3 col = mix(uBot, uTop, pow(t, 0.7));
  float s = max(dot(d, -uSunDir), 0.0);
  col += uSunCol * (pow(s, 600.0) * 2.0 + pow(s, 8.0) * 0.12);
  oColor = vec4(col, 1.0);
}
)";

// ---------------------------------------------------------------- helpers
static GLuint compile(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    GLint ok = 0;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, sizeof log, nullptr, log);
        fprintf(stderr, "shader: %s\n", log);
    }
    return s;
}

static bool build(GLuint& prog, const char* vs, const char* fs) {
    GLuint v = compile(GL_VERTEX_SHADER, vs), f = compile(GL_FRAGMENT_SHADER, fs);
    prog = glCreateProgram();
    glAttachShader(prog, v);
    glAttachShader(prog, f);
    glLinkProgram(prog);
    glDeleteShader(v);
    glDeleteShader(f);
    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(prog, sizeof log, nullptr, log);
        fprintf(stderr, "link: %s\n", log);
    }
    return ok != 0;
}

GLint Renderer::Prog::u(const char* n) {
    auto it = loc.find(n);
    if (it != loc.end()) return it->second;
    GLint l = glGetUniformLocation(id, n);
    loc[n] = l;
    return l;
}

// Atributos de vertice: posicion, normal, uv (32 bytes) sobre el VBO enlazado.
void Renderer::setupAttribs() {
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 32, (const void*)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 32, (const void*)12);
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 32, (const void*)24);
}

// ---------------------------------------------------------------- init
bool Renderer::init(int shadowSize) {
    shadowSize_ = shadowSize;
    if (!build(main_.id, kMainVS, kMainFS) || !build(sky_.id, kSkyVS, kSkyFS) ||
        !build(depth_.id, kDepthVS, kDepthFS))
        return false;

    // Cubo unitario: 24 vertices + 36 indices de 16 bits.
    static const float F[6][9] = {  // n, u, v
        {1, 0, 0, 0, 0, -1, 0, 1, 0}, {-1, 0, 0, 0, 0, 1, 0, 1, 0}, {0, 1, 0, 1, 0, 0, 0, 0, -1},
        {0, -1, 0, 1, 0, 0, 0, 0, 1}, {0, 0, 1, 1, 0, 0, 0, 1, 0},  {0, 0, -1, -1, 0, 0, 0, 1, 0}};
    static const int S[4][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
    float vtx[24 * 8];
    unsigned short idx[36];
    for (int f = 0; f < 6; f++) {
        const float* a = F[f];
        for (int k = 0; k < 4; k++) {
            float s = (float)S[k][0], t = (float)S[k][1];
            float* v = vtx + (f * 4 + k) * 8;
            for (int c = 0; c < 3; c++) v[c] = a[c] * 0.5f + (s * a[3 + c] + t * a[6 + c]) * 0.5f;
            v[3] = a[0]; v[4] = a[1]; v[5] = a[2];
            v[6] = (s + 1) * 0.5f; v[7] = (t + 1) * 0.5f;
        }
        unsigned short b = (unsigned short)(f * 4), *i = idx + f * 6;
        i[0] = b; i[1] = b + 1; i[2] = b + 2; i[3] = b; i[4] = b + 2; i[5] = b + 3;
    }
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);
    glGenBuffers(1, &vbo_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_);
    glBufferData(GL_ARRAY_BUFFER, sizeof vtx, vtx, GL_STATIC_DRAW);
    glGenBuffers(1, &ibo_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof idx, idx, GL_STATIC_DRAW);
    setupAttribs();
    glGenVertexArrays(1, &vaoEmpty_);

    // Textura blanca 1x1.
    unsigned char px[4] = {255, 255, 255, 255};
    glGenTextures(1, &white_);
    glBindTexture(GL_TEXTURE_2D, white_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Mapa de sombras de 16 bits (2 MB a 1024x1024).
    glGenTextures(1, &shadowTex_);
    glBindTexture(GL_TEXTURE_2D, shadowTex_);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, shadowSize_, shadowSize_, 0, GL_DEPTH_COMPONENT,
                 GL_UNSIGNED_SHORT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    glGenFramebuffers(1, &fbo_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex_, 0);
    GLenum none = GL_NONE;
    glDrawBuffers(1, &none);
    glReadBuffer(GL_NONE);
    bool ok = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return ok;
}

void Renderer::shutdown() {
    // Al perder el contexto GL (Android) los objetos ya no existen: solo olvidamos los ids.
    main_ = Prog(); sky_ = Prog(); depth_ = Prog();
    vao_ = vbo_ = ibo_ = vaoEmpty_ = fbo_ = shadowTex_ = white_ = 0;
    tex_.clear();
    models_.clear();
}

Renderer::TexInfo& Renderer::texture(const Game& g, const std::string& name) {
    static TexInfo none;
    if (name.empty()) { none.id = white_; return none; }
    auto it = tex_.find(name);
    if (it != tex_.end()) return it->second;
    TexInfo ti;
    ti.id = white_;
    auto a = g.proj.assets.find(name);
    if (a != g.proj.assets.end()) {
        int w, h, c;
        stbi_set_flip_vertically_on_load(1);
        unsigned char* d = stbi_load_from_memory(a->second.data(), (int)a->second.size(), &w, &h, &c, 4);
        if (d) {
            glGenTextures(1, &ti.id);
            glBindTexture(GL_TEXTURE_2D, ti.id);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, d);
            glGenerateMipmap(GL_TEXTURE_2D);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            ti.w = w; ti.h = h;
            stbi_image_free(d);  // la copia en RAM se libera: solo queda la de la GPU
        }
    }
    return tex_[name] = ti;
}

unsigned Renderer::textureId(const Game& g, const std::string& name, int* w, int* h) {
    if (name.empty() || !g.proj.assets.count(name)) return 0;
    TexInfo& t = texture(g, name);
    if (t.id == white_) return 0;
    if (w) *w = t.w;
    if (h) *h = t.h;
    return t.id;
}

Renderer::ModelGL* Renderer::model(const Game& g, const std::string& name) {
    auto it = models_.find(name);
    if (it != models_.end()) return it->second.ok ? &it->second : nullptr;
    ModelGL& m = models_[name];
    auto a = g.proj.assets.find(name);
    ModelData md;
    if (a == g.proj.assets.end() || !modelRead(a->second.data(), a->second.size(), md)) return nullptr;
    glGenVertexArrays(1, &m.vao);
    glBindVertexArray(m.vao);
    glGenBuffers(1, &m.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
    glBufferData(GL_ARRAY_BUFFER, (GLsizeiptr)(md.v.size() * 4), md.v.data(), GL_STATIC_DRAW);
    glGenBuffers(1, &m.ibo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ibo);
    if (md.v.size() / 8 > 65535) {
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(md.idx.size() * 4), md.idx.data(), GL_STATIC_DRAW);
        m.itype = GL_UNSIGNED_INT;
    } else {
        std::vector<unsigned short> s(md.idx.begin(), md.idx.end());
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, (GLsizeiptr)(s.size() * 2), s.data(), GL_STATIC_DRAW);
        m.itype = GL_UNSIGNED_SHORT;
    }
    setupAttribs();
    m.count = (int)md.idx.size();
    m.tex = md.tex;
    for (int i = 0; i < 3; i++) m.color[i] = md.color[i];
    m.ok = true;
    glBindVertexArray(vao_);
    return &m;
}

void Renderer::box(Prog& p, V3 pos, V3 scale, float r, float g, float b, float a) {
    M4 m = modelMat(pos, 0, scale);
    glUniformMatrix4fv(p.u("uM"), 1, GL_FALSE, m.m);
    glUniform4f(p.u("uColor"), r, g, b, a);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, white_);
    glBindVertexArray(vao_);
    glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, nullptr);
}

void Renderer::drawObjs(Game& g, Prog& p, bool shadow) {
    for (auto& o : g.sc->objs) {
        if (!o.alive || !o.visible) continue;
        if (o.kind == Kind::Light || o.kind == Kind::Player) continue;
        float yaw = o.yaw + (o.spin ? g.time * 90.f : 0.f);
        M4 m = modelMat(o.pos, yaw, o.scale);
        glUniformMatrix4fv(p.u("uM"), 1, GL_FALSE, m.m);
        ModelGL* md = (o.kind == Kind::Model && !o.model.empty()) ? model(g, o.model) : nullptr;
        glBindVertexArray(md ? md->vao : vao_);
        if (!shadow) {
            glUniform4f(p.u("uColor"), o.color[0] * (md ? md->color[0] : 1.f), o.color[1] * (md ? md->color[1] : 1.f),
                        o.color[2] * (md ? md->color[2] : 1.f), 1.f);
            glActiveTexture(GL_TEXTURE0);
            const std::string& tn = !o.tex.empty() ? o.tex : (md ? md->tex : o.tex);
            glBindTexture(GL_TEXTURE_2D, texture(g, tn).id);
        }
        if (md) glDrawElements(GL_TRIANGLES, md->count, md->itype, nullptr);
        else glDrawElements(GL_TRIANGLES, 36, GL_UNSIGNED_SHORT, nullptr);
    }
    glBindVertexArray(vao_);
}

void Renderer::render(Game& g, int w, int h, const EditorView* ev) {
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, w, h);
    if (!g.sc) {
        glClearColor(0, 0, 0, 1);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        return;
    }
    if (g.sc->is3d) render3D(g, w, h, ev);
    else render2D(g, w, h, ev);
}

void Renderer::clear(int w, int h, float r, float g, float b) {
    glDisable(GL_BLEND);
    glDisable(GL_SCISSOR_TEST);
    glViewport(0, 0, w, h);
    glClearColor(r, g, b, 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::render3D(Game& g, int w, int h, const EditorView* ev) {
    const Scene& S = *g.sc;
    V3 sd = norm(S.sunDir);
    V3 e = ev ? ev->eye : g.eye(), f = ev ? ev->fwd : g.forward();

    // 1) Mapa de sombras alrededor del jugador (o del punto que mira el editor).
    V3 c = ev ? ev->target : g.pos;
    V3 up = std::fabs(sd.y) < 0.99f ? V3{0, 1, 0} : V3{0, 0, 1};
    M4 lvp = mul(ortho(-24, 24, -24, 24, 1, 90), lookAt(c - sd * 40.f, c, up));
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_);
    glViewport(0, 0, shadowSize_, shadowSize_);
    glClear(GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(2.f, 4.f);
    glUseProgram(depth_.id);
    glUniformMatrix4fv(depth_.u("uLVP"), 1, GL_FALSE, lvp.m);
    drawObjs(g, depth_, true);
    glDisable(GL_POLYGON_OFFSET_FILL);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);

    // 2) Escena principal.
    glViewport(0, 0, w, h);
    glClearColor(S.skyBot[0], S.skyBot[1], S.skyBot[2], 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float aspect = h > 0 ? (float)w / (float)h : 1.f, fov = 1.2217f;
    V3 r = norm(cross(f, V3{0, 1, 0})), u = cross(r, f);
    M4 vp = mul(perspective(fov, aspect, 0.1f, 200.f), lookAt(e, e + f, V3{0, 1, 0}));

    glDisable(GL_DEPTH_TEST);
    glUseProgram(sky_.id);
    glUniform3f(sky_.u("uFwd"), f.x, f.y, f.z);
    glUniform3f(sky_.u("uRight"), r.x, r.y, r.z);
    glUniform3f(sky_.u("uUp"), u.x, u.y, u.z);
    float ty = std::tan(fov * 0.5f);
    glUniform1f(sky_.u("uTY"), ty);
    glUniform1f(sky_.u("uTX"), ty * aspect);
    glUniform3f(sky_.u("uTop"), S.skyTop[0], S.skyTop[1], S.skyTop[2]);
    glUniform3f(sky_.u("uBot"), S.skyBot[0], S.skyBot[1], S.skyBot[2]);
    glUniform3f(sky_.u("uSunDir"), sd.x, sd.y, sd.z);
    glUniform3f(sky_.u("uSunCol"), S.sun[0], S.sun[1], S.sun[2]);
    glBindVertexArray(vaoEmpty_);
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glUseProgram(main_.id);
    glUniformMatrix4fv(main_.u("uVP"), 1, GL_FALSE, vp.m);
    glUniformMatrix4fv(main_.u("uLVP"), 1, GL_FALSE, lvp.m);
    glUniform1i(main_.u("uTex"), 0);
    glUniform1i(main_.u("uSh"), 1);
    glUniform1i(main_.u("uUV1"), 0);
    glUniform1i(main_.u("uUnlit"), 0);
    glUniform1f(main_.u("uTile"), 1.f);
    glUniform1f(main_.u("uTexel"), 1.f / (float)shadowSize_);
    glUniform3f(main_.u("uSunDir"), sd.x, sd.y, sd.z);
    glUniform3f(main_.u("uSunCol"), S.sun[0], S.sun[1], S.sun[2]);
    glUniform3f(main_.u("uSkyTop"), S.skyTop[0], S.skyTop[1], S.skyTop[2]);
    glUniform3f(main_.u("uSkyBot"), S.skyBot[0], S.skyBot[1], S.skyBot[2]);
    glUniform3f(main_.u("uCam"), e.x, e.y, e.z);
    glUniform1f(main_.u("uAmb"), S.ambient);
    glUniform1f(main_.u("uFog"), S.fog);

    float pl[16] = {0}, pc[12] = {0};
    int nl = 0;
    for (auto& o : S.objs) {
        if (o.kind != Kind::Light || !o.alive || !o.visible || nl >= 4) continue;
        pl[nl * 4] = o.pos.x; pl[nl * 4 + 1] = o.pos.y; pl[nl * 4 + 2] = o.pos.z; pl[nl * 4 + 3] = o.scale.x;
        pc[nl * 3] = o.color[0]; pc[nl * 3 + 1] = o.color[1]; pc[nl * 3 + 2] = o.color[2];
        nl++;
    }
    glUniform4fv(main_.u("uPL"), 4, pl);
    glUniform3fv(main_.u("uPC"), 4, pc);
    glUniform1i(main_.u("uNL"), nl);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowTex_);
    drawObjs(g, main_, false);
    glActiveTexture(GL_TEXTURE0);

    if (ev) {
        // Marcadores del editor: luces, inicio del jugador y objeto seleccionado.
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glUniform1i(main_.u("uUnlit"), 1);
        glUniform1i(main_.u("uUV1"), 1);
        for (auto& o : S.objs)
            if (o.kind == Kind::Light) box(main_, o.pos, V3{0.35f, 0.35f, 0.35f}, o.color[0], o.color[1], o.color[2], 1.f);
        box(main_, V3{S.start.x, S.start.y + 0.85f, S.start.z}, V3{0.6f, 1.7f, 0.6f}, 0.2f, 1.f, 0.3f, 0.5f);
        if (ev->sel >= 0 && ev->sel < (int)S.objs.size()) {
            const Obj& o = S.objs[ev->sel];
            V3 sc = o.kind == Kind::Light ? V3{0.5f, 0.5f, 0.5f} : o.scale * 1.04f + V3{0.02f, 0.02f, 0.02f};
            box(main_, o.pos, sc, 1.f, 0.9f, 0.2f, 0.35f);
        }
        glDisable(GL_BLEND);
        glUniform1i(main_.u("uUnlit"), 0);
        glUniform1i(main_.u("uUV1"), 0);
    } else {
        // Mira central sin shaders extra: dos recortes con glClear.
        glEnable(GL_SCISSOR_TEST);
        glClearColor(1, 1, 1, 1);
        glScissor(w / 2 - 1, h / 2 - 8, 2, 16);
        glClear(GL_COLOR_BUFFER_BIT);
        glScissor(w / 2 - 8, h / 2 - 1, 16, 2);
        glClear(GL_COLOR_BUFFER_BIT);
        glDisable(GL_SCISSOR_TEST);
    }
}

void Renderer::render2D(Game& g, int w, int h, const EditorView* ev) {
    const Scene& S = *g.sc;
    glClearColor(S.skyBot[0], S.skyBot[1], S.skyBot[2], 1.f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    float aspect = h > 0 ? (float)w / (float)h : 1.f;
    float vh = ev ? ev->h2d : kView2DHeight, cx = ev ? ev->cx : 0.f, cy = ev ? ev->cy : 0.f;
    float hw = vh * 0.5f * aspect, hh = vh * 0.5f;
    M4 vp = ortho(cx - hw, cx + hw, cy - hh, cy + hh, -20, 20);

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glUseProgram(main_.id);
    glUniformMatrix4fv(main_.u("uVP"), 1, GL_FALSE, vp.m);
    glUniform1i(main_.u("uTex"), 0);
    glUniform1i(main_.u("uSh"), 1);
    glUniform1i(main_.u("uUV1"), 1);
    glUniform1i(main_.u("uUnlit"), 1);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowTex_);
    drawObjs(g, main_, false);  // el orden de la lista = orden de capas
    if (ev && ev->sel >= 0 && ev->sel < (int)S.objs.size()) {
        const Obj& o = S.objs[ev->sel];
        box(main_, o.pos, o.scale * 1.04f + V3{0.05f, 0.05f, 0.05f}, 1.f, 0.9f, 0.2f, 0.35f);
    }
    glActiveTexture(GL_TEXTURE0);
    glDisable(GL_BLEND);
}

}  // namespace forge
