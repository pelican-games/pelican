#include <pelican/pelican.hpp>
#include <GLFW/glfw3.h>
#include <AL/alut.h>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>

namespace pl {

GLFWwindow *mainWindow;

// 1回だけ呼ばれる
void system_init() {
    glfwInit();

    // ウィンドウの作成とか
    // Vulkanその他の初期化とか
}

// 毎フレーム呼ばれる
bool frame_update() {
    // 描画とか
    // キー入力状態の取得とか

    glfwPollEvents();
    return !glfwWindowShouldClose(mainWindow);
}

}
