#pragma once
#include "scene.h"
#include <string>

namespace forge {

// Audio con miniaudio: mp3, wav y flac. Los sonidos se leen desde los assets en memoria.
class Audio {
public:
    ~Audio() { shutdown(); }
    bool init(bool noDevice = false);  // noDevice: solo para pruebas
    void shutdown();
    void setAssets(const Files& f);    // (re)registra los archivos de audio
    void play(const std::string& name);   // efecto; admite el nombre sin extension
    void music(const std::string& name);  // en bucle
    void stopMusic();
    bool has(const std::string& name) const;
    static bool isAudioFile(const std::string& name);

private:
    struct Impl;
    Impl* p_ = nullptr;
    std::string resolve(const std::string& name) const;
};

}  // namespace forge
