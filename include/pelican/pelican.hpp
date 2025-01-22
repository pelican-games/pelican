#include <filesystem>
#include <glm/glm.hpp>
#include <pelican/audio.hpp>
#include <pelican/speaker.hpp>

namespace pl {

    void system_init();
    bool frame_update();
    void cleanup();

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

    Speaker generateSpeaker();
    void setListenerPos(glm::vec3 pos);

}