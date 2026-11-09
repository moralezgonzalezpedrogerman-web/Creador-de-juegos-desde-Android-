#include "model.h"
#include <algorithm>
#include <cstring>


namespace forge {

static void w32(std::vector<uint8_t>& o, uint32_t v) {
    for (int i = 0; i < 4; i++) o.push_back((v >> (8 * i)) & 255);
}

bool modelWrite(const ModelData& m, std::vector<uint8_t>& out) {
    if (m.v.empty() || m.v.size() % 8 != 0 || m.idx.empty() || m.idx.size() % 3 != 0) return false;
    uint32_t nv = (uint32_t)(m.v.size() / 8), ni = (uint32_t)m.idx.size();
    bool wide = nv > 65535;
    out.clear();
    out.insert(out.end(), {'F', 'M', 'D', '1'});
    w32(out, nv);
    w32(out, ni);
    out.push_back(wide ? 1 : 0);
    out.push_back((uint8_t)std::min<size_t>(m.tex.size(), 255));
    out.insert(out.end(), m.tex.begin(), m.tex.begin() + std::min<size_t>(m.tex.size(), 255));
    for (int i = 0; i < 3; i++) {
        uint32_t u;
        memcpy(&u, &m.color[i], 4);
        w32(out, u);
    }
    size_t at = out.size();
    out.resize(at + m.v.size() * 4);
    memcpy(out.data() + at, m.v.data(), m.v.size() * 4);
    for (uint32_t i : m.idx) {
        out.push_back(i & 255);
        out.push_back((i >> 8) & 255);
        if (wide) { out.push_back((i >> 16) & 255); out.push_back((i >> 24) & 255); }
    }
    return true;
}

bool modelRead(const uint8_t* d, size_t n, ModelData& m) {
    if (n < 18 || memcmp(d, "FMD1", 4) != 0) return false;
    auto r32 = [&](size_t p) { return (uint32_t)d[p] | ((uint32_t)d[p + 1] << 8) | ((uint32_t)d[p + 2] << 16) | ((uint32_t)d[p + 3] << 24); };
    uint32_t nv = r32(4), ni = r32(8);
    bool wide = d[12] != 0;
    size_t tl = d[13], p = 14;
    if (p + tl + 12 > n || nv == 0 || ni == 0 || ni % 3 != 0) return false;
    m.tex.assign((const char*)d + p, tl);
    p += tl;
    for (int i = 0; i < 3; i++) {
        uint32_t u = r32(p);
        memcpy(&m.color[i], &u, 4);
        p += 4;
    }
    size_t vb = (size_t)nv * 32, ib = (size_t)ni * (wide ? 4 : 2);
    if (p + vb + ib > n) return false;
    m.v.resize((size_t)nv * 8);
    memcpy(m.v.data(), d + p, vb);
    p += vb;
    m.idx.resize(ni);
    for (uint32_t i = 0; i < ni; i++) {
        m.idx[i] = wide ? r32(p) : (uint32_t)(d[p] | (d[p + 1] << 8));
        p += wide ? 4 : 2;
        if (m.idx[i] >= nv) return false;
    }
    return true;
}

}  // namespace forge
