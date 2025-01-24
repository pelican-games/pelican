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
#include "pelican/sceneLoader.hpp"
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

    Speaker generateSpeaker();
    void setListenerPos(glm::vec3 pos);

}