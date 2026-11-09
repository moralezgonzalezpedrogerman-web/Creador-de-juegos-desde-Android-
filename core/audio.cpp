#define MINIAUDIO_IMPLEMENTATION
#define MA_NO_ENCODING
#define MA_NO_GENERATION
#include "miniaudio.h"

#include "audio.h"
#include <algorithm>
#include <cstring>
#include <set>

namespace forge {

struct Audio::Impl {
    ma_engine engine;
    bool ok = false;
    bool musicOn = false;
    ma_sound music;
    std::set<std::string> names;
};

static std::string lower(std::string s) {
    for (auto& c : s) c = (char)tolower((unsigned char)c);
    return s;
}

bool Audio::isAudioFile(const std::string& name) {
    std::string n = lower(name);
    auto ends = [&](const char* e) { size_t l = strlen(e); return n.size() > l && n.compare(n.size() - l, l, e) == 0; };
    return ends(".mp3") || ends(".wav") || ends(".flac");
}

bool Audio::init(bool noDevice) {
    if (p_) return p_->ok;
    p_ = new Impl;
    ma_engine_config c = ma_engine_config_init();
    if (noDevice) { c.noDevice = MA_TRUE; c.channels = 2; c.sampleRate = 44100; }
    p_->ok = ma_engine_init(&c, &p_->engine) == MA_SUCCESS;
    return p_->ok;
}

void Audio::shutdown() {
    if (!p_) return;
    stopMusic();
    if (p_->ok) ma_engine_uninit(&p_->engine);
    delete p_;
    p_ = nullptr;
}

void Audio::setAssets(const Files& f) {
    if (!p_ || !p_->ok) return;
    ma_resource_manager* rm = ma_engine_get_resource_manager(&p_->engine);
    stopMusic();
    for (auto& n : p_->names) ma_resource_manager_unregister_data(rm, n.c_str());
    p_->names.clear();
    for (auto& a : f) {
        if (!isAudioFile(a.first) || a.second.empty()) continue;
        if (ma_resource_manager_register_encoded_data(rm, a.first.c_str(), a.second.data(), a.second.size()) == MA_SUCCESS)
            p_->names.insert(a.first);
    }
}

std::string Audio::resolve(const std::string& name) const {
    if (!p_ || name.empty()) return "";
    if (p_->names.count(name)) return name;
    for (auto& n : p_->names)  // "disparo" -> "disparo.wav"
        if (n.size() > name.size() && n.compare(0, name.size(), name) == 0 && n[name.size()] == '.') return n;
    return "";
}

bool Audio::has(const std::string& name) const { return !resolve(name).empty(); }

void Audio::play(const std::string& name) {
    if (!p_ || !p_->ok) return;
    std::string n = resolve(name);
    if (!n.empty()) ma_engine_play_sound(&p_->engine, n.c_str(), nullptr);
}

void Audio::music(const std::string& name) {
    if (!p_ || !p_->ok) return;
    std::string n = resolve(name);
    stopMusic();
    if (n.empty()) return;
    if (ma_sound_init_from_file(&p_->engine, n.c_str(), MA_SOUND_FLAG_STREAM, nullptr, nullptr, &p_->music) == MA_SUCCESS) {
        ma_sound_set_looping(&p_->music, MA_TRUE);
        ma_sound_start(&p_->music);
        p_->musicOn = true;
    }
}

void Audio::stopMusic() {
    if (p_ && p_->musicOn) {
        ma_sound_uninit(&p_->music);
        p_->musicOn = false;
    }
}

}  // namespace forge
