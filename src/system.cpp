#include "pelican/system.hpp"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <AL/alut.h>
#include <vulkan/vulkan.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include "pelican/audio.hpp"
#include "pelican/speaker.hpp"
#include "pelican/physics.hpp"

namespace pl {
    void startGame() {

    }

    void pauseGame() {

    }

    void restartGame() {

    }

    void endGame() {

    }

    void loadStage(std::string stageinfopath) {
        std::filesystem::path path = std::filesystem::current_path() / stageinfopath;

        if (!std::filesystem::exists(path)) {
            std::cerr << "Error: Stage file does not exists." << std::endl;
            return;
        }
        
    }
    void restartStage(std::string stageinfopath) {
        std::filesystem::path path = std::filesystem::current_path() / stageinfopath;
        if (!std::filesystem::exists(path)) {
            std::cerr << "Error: Stage file does not exists." << std::endl;
            return;
        }
    }

    void saveGamestate(std::string savepath) {
        std::filesystem::path path = std::filesystem::current_path() / savepath;
        if (!std::filesystem::exists(path)) {
            std::cerr << "Error: Stage file does not exists." << std::endl;
            return;
        }
    }
    void loadGamestate(std::string loadpath) {
        std::filesystem::path path = std::filesystem::current_path() / loadpath;
        if (!std::filesystem::exists(path)) {
            std::cerr << "Error: Stage file does not exists." << std::endl;
            return;
        }

    }
} // namespace pl