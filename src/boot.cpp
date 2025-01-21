#include "pelican/boot.hpp"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

namespace pl {
    // 1�񂾂��Ă΂��
    GLFWwindow* systemInit(unsigned int Windowheight, unsigned int Windowwidth) {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        GLFWwindow* window = glfwCreateWindow(Windowheight, Windowwidth, "Pelican Vulkan Window", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }
    }

    // ���t���[���Ă΂��
    bool frame_update(GLFWwindow* window) {
        if (!window) {
            throw std::runtime_error("Window is not initialized.");
        }

        // �C�x���g�|�[�����O
        glfwPollEvents();
        return !glfwWindowShouldClose(window);
    }

    // �N���[���A�b�v����
    void cleanup(GLFWwindow* window) {
        if (window) {
            glfwDestroyWindow(window);
            glfwTerminate();
        }
    }
}