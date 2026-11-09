#pragma once
#include <algorithm>
#include <cmath>

namespace forge {

struct V3 { float x = 0, y = 0, z = 0; };
inline V3 operator+(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
inline V3 operator-(V3 a, V3 b) { return {a.x - b.x, a.y - b.y, a.z - b.z}; }
inline V3 operator*(V3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
inline float dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline V3 cross(V3 a, V3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }
inline float len(V3 a) { return std::sqrt(dot(a, a)); }
inline V3 norm(V3 a) { float l = len(a); return l > 1e-6f ? a * (1.f / l) : V3{0, 1, 0}; }

// Matriz 4x4 en orden de columnas (como OpenGL).
struct M4 { float m[16]; };

inline M4 ident() { M4 r{}; r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1; return r; }

inline M4 mul(const M4& a, const M4& b) {
    M4 r{};
    for (int c = 0; c < 4; c++)
        for (int row = 0; row < 4; row++) {
            float s = 0;
            for (int k = 0; k < 4; k++) s += a.m[k * 4 + row] * b.m[c * 4 + k];
            r.m[c * 4 + row] = s;
        }
    return r;
}

inline M4 perspective(float fovy, float aspect, float n, float f) {
    M4 r{};
    float t = 1.f / std::tan(fovy * 0.5f);
    r.m[0] = t / aspect; r.m[5] = t;
    r.m[10] = (f + n) / (n - f); r.m[11] = -1;
    r.m[14] = 2 * f * n / (n - f);
    return r;
}

inline M4 ortho(float l, float r_, float b, float t, float n, float f) {
    M4 r = ident();
    r.m[0] = 2 / (r_ - l); r.m[5] = 2 / (t - b); r.m[10] = -2 / (f - n);
    r.m[12] = -(r_ + l) / (r_ - l); r.m[13] = -(t + b) / (t - b); r.m[14] = -(f + n) / (f - n);
    return r;
}

inline M4 lookAt(V3 e, V3 c, V3 up) {
    V3 f = norm(c - e), s = norm(cross(f, up)), u = cross(s, f);
    M4 r = ident();
    r.m[0] = s.x; r.m[4] = s.y; r.m[8] = s.z;
    r.m[1] = u.x; r.m[5] = u.y; r.m[9] = u.z;
    r.m[2] = -f.x; r.m[6] = -f.y; r.m[10] = -f.z;
    r.m[12] = -dot(s, e); r.m[13] = -dot(u, e); r.m[14] = dot(f, e);
    return r;
}

// Posicion + giro en Y (grados) + escala.
inline M4 modelMat(V3 p, float yawDeg, V3 s) {
    float a = yawDeg * 0.0174532925f, c = std::cos(a), sn = std::sin(a);
    M4 r = ident();
    r.m[0] = c * s.x; r.m[2] = -sn * s.x;
    r.m[5] = s.y;
    r.m[8] = sn * s.z; r.m[10] = c * s.z;
    r.m[12] = p.x; r.m[13] = p.y; r.m[14] = p.z;
    return r;
}

// Rayo contra caja alineada a los ejes (centro c, mitades h). t = distancia de entrada.
inline bool rayBox(V3 o, V3 d, V3 c, V3 h, float& t) {
    float t0 = 0, t1 = 1e9f;
    float oo[3] = {o.x - c.x, o.y - c.y, o.z - c.z}, dd[3] = {d.x, d.y, d.z}, hh[3] = {h.x, h.y, h.z};
    for (int a = 0; a < 3; a++) {
        if (std::fabs(dd[a]) < 1e-8f) {
            if (std::fabs(oo[a]) > hh[a]) return false;
        } else {
            float inv = 1.f / dd[a], n = (-hh[a] - oo[a]) * inv, f = (hh[a] - oo[a]) * inv;
            if (n > f) std::swap(n, f);
            t0 = std::max(t0, n);
            t1 = std::min(t1, f);
            if (t0 > t1) return false;
        }
    }
    t = t0;
    return true;
}

}  // namespace forge
