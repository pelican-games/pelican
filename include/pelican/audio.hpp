#ifndef PELICAN_AUDIO_HPP
#define PELICAN_AUDIO_HPP

#include <filesystem>

namespace pl {

    class Audio {
    public:
        void initAudio();
        void loadAudio(std::filesystem::path path,std::string audio_name);
        unsigned int getBufferID();
        void destAudio();
    private:
        unsigned int bufferID = 0;
    };

} // namespace pl

#endif // PELICAN_AUDIO_HPP
