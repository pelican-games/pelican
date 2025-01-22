#include <pelican/Audio.hpp>
#include <stdexcept>
#include <AL/alut.h>
#include <iostream>
#include <cassert>

namespace pl {

    void Audio::loadAudio(std::filesystem::path& path) {
        bufferID = alutCreateBufferFromFile(path.string().c_str());
        if (bufferID == AL_NONE) {
           throw std::runtime_error("Failed to load audio file: " + path.string());
        }
        ALuint audio_source;
        alGenSources(1, &audio_source);
        alSourcei(audio_source, AL_BUFFER, bufferID);

    }

    unsigned int Audio::getBufferID() const {
        return bufferID;
    }

} // namespace pl
