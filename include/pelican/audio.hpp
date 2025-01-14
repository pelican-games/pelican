#ifndef PELICAN_AUDIO_HPP
#define PELICAN_AUDIO_HPP

#include <filesystem>

namespace pl {

    struct Audio {
    public:
        Audio();

        void loadAudio(std::filesystem::path& path);
        unsigned int getBufferID() const;
    private:
        unsigned int bufferID = 0;
    };

} // namespace pl

#endif // PELICAN_AUDIO_HPP
