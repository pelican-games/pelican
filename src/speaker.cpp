#include <pelican/Speaker.hpp>
#include <stdexcept>
#include <AL/al.h>
#include <AL/alut.h>

namespace pl {

    Speaker::Speaker() {
        alGenSources(1, &sourceID);
        if (sourceID == 0) {
            throw std::runtime_error("Failed to generate OpenAL source");
        }
    }

    Speaker::~Speaker() {
        alDeleteSources(1, &sourceID);
    }

    void Speaker::set(Audio audio) {
        alSourcei(sourceID, AL_BUFFER, audio.getBufferID());
    }

    void Speaker::play() {
        alSourcePlay(sourceID);
    }

    void Speaker::stop() {
        alSourceStop(sourceID);
    }

    void Speaker::setVolume(float volume) {
        alSourcef(sourceID, AL_GAIN, volume);
    }

    void Speaker::setLoop(bool loop) {
        alSourcei(sourceID, AL_LOOPING, loop ? AL_TRUE : AL_FALSE);
    }

    void Speaker::setPos(const glm::vec3& pos) {
        alSource3f(sourceID, AL_POSITION, pos.x, pos.y, pos.z);
    }

} // namespace pl
