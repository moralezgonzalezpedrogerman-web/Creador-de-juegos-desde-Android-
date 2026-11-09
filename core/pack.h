#pragma once
#include "scene.h"
#include <string>
#include <vector>

namespace forge {

// Contenedor simple: "FRGP" + n archivos (nombre + bytes).
bool packWrite(const Files& files, std::vector<uint8_t>& out);
bool packRead(const uint8_t* data, size_t size, Files& out);

// Exportar a .exe: runtime de Windows + paquete + pie. No hace falta compilar nada.
bool exportExe(const std::vector<uint8_t>& runtime, const std::vector<uint8_t>& pack,
               std::vector<uint8_t>& out);
// Lee el paquete pegado al final de un ejecutable (el propio runtime al arrancar).
bool extractFromExe(const std::string& path, std::vector<uint8_t>& pack);

}  // namespace forge
