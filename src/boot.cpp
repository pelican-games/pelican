#include "pelican/boot.hpp"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include "vulkan/app.hpp"
#include <optional>

namespace pl {
    std::optional<VulkanApp> vkApp;

    // 1回だけ呼ばれる
    System::System(unsigned int Windowwidth, unsigned int Windowheight) {
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(Windowwidth, Windowheight, "Pelican Vulkan Window", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        vkApp.emplace(window, Windowwidth, Windowheight);
    }

    // 毎フレーム呼ばれる
    bool System::frameUpdate() {
        if (!window || !vkApp) {
            throw std::runtime_error("Window is not initialized.");
        }

        vkApp->drawFrame();

        // イベントポーリング
        glfwPollEvents();
        return !glfwWindowShouldClose(window);
    }

    // クリーンアップ処理
    System::~System() {
        if (window) {
            glfwDestroyWindow(window);
            glfwTerminate();
        }
    }
}