#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace forge {

// Modelo 3D compacto (.fmdl): una malla estatica ya normalizada para caber en un cubo
// unitario centrado. El runtime solo lee este formato; los importadores viven en el editor.
struct ModelData {
    std::vector<float> v;        // 8 floats por vertice: posicion, normal, uv
    std::vector<uint32_t> idx;
    std::string tex;             // asset de textura ("" = ninguna)
    float color[3] = {1, 1, 1};
};

bool modelWrite(const ModelData& m, std::vector<uint8_t>& out);
bool modelRead(const uint8_t* d, size_t n, ModelData& m);

}  // namespace forge
