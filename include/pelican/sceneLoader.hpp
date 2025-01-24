#pragma once

#include <vector>
#include <map>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

namespace pl {

struct TransformInfo {
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;

    glm::mat4x4 getModelMatrix() const;
};

struct SceneInfo {
    std::map<std::string, std::vector<TransformInfo>> objects;
};

SceneInfo loadScene(std::filesystem::path file_path);

}
