#include "pelican/boot.hpp"
#include "pelican/input.hpp"
#include <GLFW/glfw3.h>
#include <vulkan/vulkan.hpp>
#include "vulkan/app.hpp"
#include <optional>
#include <thread>
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

    pl::Cursor::Point cursorPos;
    std::optional<pl::Cursor::Point> targetCursorPos;

    KeyState cursorLeft, cursorRight;
    pl::Cursor::Point Cursor::getPos() const {
        return cursorPos;
    }
    void Cursor::setPos(pl::Cursor::Point pos) {
        targetCursorPos = pos;
    }

    bool Cursor::left_down() const {
        return cursorLeft.is_down;
    }
    bool Cursor::left_up() const{
        return cursorLeft.is_up;
    }
    bool Cursor::left_pressed() const{
        return cursorLeft.is_pressed;
    }
    
    bool Cursor::right_down() const {
        return cursorRight.is_down;
    }
    bool Cursor::right_up() const{
        return cursorRight.is_up;
    }
    bool Cursor::right_pressed() const{
        return cursorRight.is_pressed;
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

        base_time = std::chrono::system_clock::now();
    }

    // 毎フレーム呼ばれる
    bool System::frameUpdate() {
        // if (!window || !renderer) {
        //     throw std::runtime_error("Window is not initialized.");
        // }

        renderer->drawFrame();

        {
            const auto now_time = std::chrono::system_clock::now();
            frame_count++;
            auto sleep_time_millis = (frame_count * 1000.0f / 60.0f) - std::chrono::duration_cast<std::chrono::milliseconds>(now_time - base_time).count();
            if(sleep_time_millis < 1) {
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
                frame_count = 0;
                base_time = now_time;
            } else {
                std::this_thread::sleep_for(std::chrono::milliseconds(int(sleep_time_millis)));
                if(frame_count > 60) {
                    frame_count = 0;
                    base_time = now_time;
                }
            }
        }

        // イベントポーリング
        glfwPollEvents();

        if(targetCursorPos) {
            glfwSetCursorPos(window, targetCursorPos->x, targetCursorPos->y);
            cursorPos = {int(targetCursorPos->x), int(targetCursorPos->y)};
            targetCursorPos.reset();
        } else 
        {
            double cx, cy;
            glfwGetCursorPos(window, &cx, &cy);
            cursorPos = {int(cx), int(cy)};
        }

        {
            auto old_state = cursorLeft.is_pressed;
            cursorLeft.is_pressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
            cursorLeft.is_down = cursorLeft.is_pressed == GLFW_PRESS && old_state == GLFW_RELEASE;
            cursorLeft.is_up = cursorLeft.is_pressed == GLFW_RELEASE && old_state == GLFW_PRESS;
        }
        {
            auto old_state = cursorRight.is_pressed;
            cursorRight.is_pressed = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_RIGHT);
            cursorRight.is_down = cursorRight.is_pressed == GLFW_PRESS && old_state == GLFW_RELEASE;
            cursorRight.is_up = cursorRight.is_pressed == GLFW_RELEASE && old_state == GLFW_PRESS;
        }

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