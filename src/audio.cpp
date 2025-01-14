#include <pelican/Audio.hpp>
#include <stdexcept>
#include <AL/alut.h>

namespace pl {

    Audio::Audio()
        : bufferID(AL_NONE) // bufferID Çèâä˙âª
    {
    }

    void Audio::loadAudio(std::filesystem::path& path) {
        bufferID = alutCreateBufferFromFile(path.string().c_str());
        if (bufferID == AL_NONE) {
            throw std::runtime_error("Failed to load audio file: " + path.string());
        }
    }

    unsigned int Audio::getBufferID() const {
        return bufferID;
    }

} // namespace pl
