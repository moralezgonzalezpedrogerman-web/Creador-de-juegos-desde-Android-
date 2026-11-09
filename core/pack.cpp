#include "pack.h"
#include <cstdio>
#include <cstring>

namespace forge {

static const char kFooter[8] = {'F', 'O', 'R', 'G', 'E', 'P', 'K', '1'};

static void w32(std::vector<uint8_t>& o, uint32_t v) {
    for (int i = 0; i < 4; i++) o.push_back((v >> (8 * i)) & 255);
}
static bool r32(const uint8_t* d, size_t n, size_t& p, uint32_t& v) {
    if (p + 4 > n) return false;
    v = (uint32_t)d[p] | ((uint32_t)d[p + 1] << 8) | ((uint32_t)d[p + 2] << 16) | ((uint32_t)d[p + 3] << 24);
    p += 4;
    return true;
}

bool packWrite(const Files& files, std::vector<uint8_t>& out) {
    out.clear();
    out.insert(out.end(), {'F', 'R', 'G', 'P'});
    w32(out, (uint32_t)files.size());
    for (auto& f : files) {
        w32(out, (uint32_t)f.first.size());
        out.insert(out.end(), f.first.begin(), f.first.end());
        w32(out, (uint32_t)f.second.size());
        out.insert(out.end(), f.second.begin(), f.second.end());
    }
    return true;
}

bool packRead(const uint8_t* d, size_t n, Files& out) {
    out.clear();
    if (n < 8 || memcmp(d, "FRGP", 4) != 0) return false;
    size_t p = 4;
    uint32_t count;
    if (!r32(d, n, p, count)) return false;
    for (uint32_t i = 0; i < count; i++) {
        uint32_t nl, sz;
        if (!r32(d, n, p, nl) || p + nl > n) return false;
        std::string name((const char*)d + p, nl);
        p += nl;
        if (!r32(d, n, p, sz) || p + sz > n) return false;
        out[name].assign(d + p, d + p + sz);
        p += sz;
    }
    return true;
}

bool exportExe(const std::vector<uint8_t>& runtime, const std::vector<uint8_t>& pack,
               std::vector<uint8_t>& out) {
    if (runtime.size() < 2 || runtime[0] != 'M' || runtime[1] != 'Z') return false;  // no es un .exe
    out = runtime;
    out.insert(out.end(), pack.begin(), pack.end());
    uint64_t sz = pack.size();
    for (int i = 0; i < 8; i++) out.push_back((sz >> (8 * i)) & 255);
    out.insert(out.end(), kFooter, kFooter + 8);
    return true;
}

bool extractFromExe(const std::string& path, std::vector<uint8_t>& pack) {
    FILE* f = fopen(path.c_str(), "rb");
    if (!f) return false;
    bool ok = false;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    if (n >= 16) {
        uint8_t foot[16];
        fseek(f, n - 16, SEEK_SET);
        if (fread(foot, 1, 16, f) == 16 && memcmp(foot + 8, kFooter, 8) == 0) {
            uint64_t sz = 0;
            for (int i = 0; i < 8; i++) sz |= (uint64_t)foot[i] << (8 * i);
            if (sz + 16 <= (uint64_t)n) {
                pack.resize((size_t)sz);
                fseek(f, n - 16 - (long)sz, SEEK_SET);
                ok = fread(pack.data(), 1, (size_t)sz, f) == (size_t)sz;
            }
        }
    }
    fclose(f);
    return ok;
}

}  // namespace forge
