#ifndef FORGE_GLES
#include "gl.h"

#define X(r, n, a) PFN_##n n = nullptr;
GL_FUNCS(X)
#undef X

bool forgeLoadGL(void* (*getProc)(const char*)) {
    bool ok = true;
#define X(r, n, a) n = (PFN_##n)getProc(#n); if (!n) ok = false;
    GL_FUNCS(X)
#undef X
    return ok;
}
#endif
