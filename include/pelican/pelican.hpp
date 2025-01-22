#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include "pelican/audio.hpp"
#include "pelican/speaker.hpp"
#include "pelican/renderer.hpp"
#include "pelican/boot.hpp"
#include "pelican/physics.hpp"
#include "pelican/system.hpp"
#include <GLFW/glfw3.h>
namespace pl {


    class Key {
        bool down();
        bool up();
        bool pressed();
    };

    class Cursor {
        void setPos();
        void getPos();
    };

    struct ModelData;
    struct Model {
        const pl::ModelData* pDat;
    };
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