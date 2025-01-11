#include <filesystem>
#include <glm/glm.hpp>

namespace pl {

void system_init();
bool frame_update();

class Key {
    bool down();
    bool up();
    bool pressed();
};

class Cursor {
    void setPos();
    void getPos();
};

class Model {};
Model loadModel(std::filesystem::path path);

class Renderer {
public:
    void drawModel(const Model& model, glm::mat4x4 modelMatrix);
    void setCamera(glm::vec3 pos, glm::vec3 dir, glm::vec3 up);
    void setProjection(float horizontalAngle);
};
Renderer& getDefaultRenderer();

class Audio {};
Audio loadAudio(std::filesystem::path path);

class Speaker {
public:
    void set(const Audio& audio);
    void play();
    void stop();
    void setVolume(float volume);
    void setLoop(bool isLoopEnable);
    void setPos(glm::vec3 pos);
};
Speaker generateSpeaker();
void setListenerPos(glm::vec3 pos);

}
