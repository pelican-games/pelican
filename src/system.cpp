#include <pelican/pelican.hpp>
#include <GLFW/glfw3.h>
#include <AL/alut.h>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

namespace pl {

void system_init() {
    glfwInit();
}

bool frame_update() {
    return true;
}

}
