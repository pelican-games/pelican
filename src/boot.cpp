#include "pelican/boot.hpp"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>

namespace pl {
    // 1回だけ呼ばれる
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

    // 毎フレーム呼ばれる
    bool frame_update(GLFWwindow* window) {
        if (!window) {
            throw std::runtime_error("Window is not initialized.");
        }

        // イベントポーリング
        glfwPollEvents();
        return !glfwWindowShouldClose(window);
    }

    // クリーンアップ処理
    void cleanup(GLFWwindow* window) {
        if (window) {
            glfwDestroyWindow(window);
            glfwTerminate();
        }
    }
}