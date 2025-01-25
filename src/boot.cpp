#include "pelican/boot.hpp"
#include "pelican/input.hpp"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include "vulkan/app.hpp"
#include <optional>
#include <map>

namespace pl {

    struct KeyState {
        bool is_down, is_up, is_pressed;
    };

    std::map<int, KeyState> keyStates;
    std::map<int, int> keyObjCount;

    Key::Key(int keycode) : keycode(keycode) {
        keyStates[keycode] = {};
        keyObjCount[keycode]++;
    }
    Key::~Key() {
        keyObjCount[keycode]--;
        if(keyObjCount[keycode] == 0) {
            keyStates.erase(keycode);
        }
    }
    
    bool Key::down() const {
        return keyStates[keycode].is_down;
    }
    bool Key::up() const{
        return keyStates[keycode].is_up;
    }
    bool Key::pressed() const{
        return keyStates[keycode].is_pressed;
    }

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

        renderer = std::make_unique<VulkanApp>(window, Windowwidth, Windowheight);
    }

    // 毎フレーム呼ばれる
    bool System::frameUpdate() {
        // if (!window || !renderer) {
        //     throw std::runtime_error("Window is not initialized.");
        // }

        renderer->drawFrame();

        // イベントポーリング
        glfwPollEvents();

        for(auto& [keycode, keyState] : keyStates) {
            auto old_state = keyState.is_pressed;
            keyState.is_pressed = glfwGetKey(window, keycode);
            keyState.is_down = keyState.is_pressed == GLFW_PRESS && old_state == GLFW_RELEASE;
            keyState.is_up = keyState.is_pressed == GLFW_RELEASE && old_state == GLFW_PRESS;
        }

        return !glfwWindowShouldClose(window);
    }

    // クリーンアップ処理
    System::~System() {
        if (window) {
            glfwDestroyWindow(window);
            glfwTerminate();
        }
    }
    
	Renderer& System::getDefaultRenderer() {
        return *renderer.get();
    }
}