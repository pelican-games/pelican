#include <pelican/Audio.hpp>
#include <stdexcept>
#include <AL/alut.h>
#include <iostream>
#include <cassert>
#include <fstream>

namespace pl {
    void Audio::initAudio() {
        alutInit(nullptr, nullptr);
    }
    void Audio::loadAudio(std::filesystem::path path,std::string audio_name) {
        if(!std::filesystem::exists(path / audio_name)) {
            std::ofstream err("error.txt");
            err << path /audio_name << " not found";
        }

        bufferID = alutCreateBufferFromFile((path /audio_name).string().c_str());
        if (bufferID == AL_NONE) {
           throw std::runtime_error("Failed to load audio file: " + path.string());
        }
    }
    unsigned int Audio::getBufferID() {
        return bufferID;
    }
    void Audio::destAudio() {
        alutSleep(1);
        alutExit();
    }

} // namespace pl
