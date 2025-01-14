#ifndef PELICAN_SPEAKER_HPP
#define PELICAN_SPEAKER_HPP

#include <glm/glm.hpp>
#include <pelican/Audio.hpp>

namespace pl {

    class Speaker {
    public:
        Speaker();
        ~Speaker();

        void set(Audio audio);
        void play();
        void stop();
        void setVolume(float volume);
        void setLoop(bool loop);
        void setPos(const glm::vec3& pos);

    private:
        unsigned int sourceID;
    };

} // namespace pl

#endif // PELICAN_SPEAKER_HPP
