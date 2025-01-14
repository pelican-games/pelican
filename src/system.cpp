#include <pelican/pelican.hpp>
#include <GLFW/glfw3.h>
#include <AL/alut.h>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <iostream>

namespace pl {

    // グローバル変数
    const int WIDTH = 800;
    const int HEIGHT = 600;
    GLFWwindow* window = nullptr;
    vk::Instance instance;

    // 1回だけ呼ばれる
    void system_init() {
        // GLFW初期化
        if (!glfwInit()) {
            throw std::runtime_error("Failed to initialize GLFW");
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        window = glfwCreateWindow(WIDTH, HEIGHT, "Pelican Vulkan Window", nullptr, nullptr);
        if (!window) {
            glfwTerminate();
            throw std::runtime_error("Failed to create GLFW window");
        }

        // Vulkanインスタンス作成
        try {
            vk::ApplicationInfo appInfo{};
            appInfo.pApplicationName = "Hello Vulkan";
            appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.pEngineName = "No Engine";
            appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
            appInfo.apiVersion = VK_API_VERSION_1_2;

            vk::InstanceCreateInfo createInfo{};
            createInfo.pApplicationInfo = &appInfo;

            // Vulkanインスタンスを作成
            instance = vk::createInstance(createInfo);
            std::cout << "Vulkan instance created successfully!" << std::endl;

        }
        catch (const std::exception& e) {
            std::cerr << e.what() << std::endl;
            glfwDestroyWindow(window);
            glfwTerminate();
            throw;
        }
    }

    // 毎フレーム呼ばれる
    bool frame_update() {
        if (!window) {
            throw std::runtime_error("Window is not initialized.");
        }

        // イベントポーリング
        glfwPollEvents();
        return !glfwWindowShouldClose(window);
    }

    // クリーンアップ処理
    void cleanup() {
        if (instance) {
            instance.destroy();
        }
        if (window) {
            glfwDestroyWindow(window);
            glfwTerminate();
        }
    }

} // namespace pl
