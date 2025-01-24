#include <filesystem>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <pelican/sceneLoader.hpp>
#include <tiny_gltf.h>

namespace pl {

glm::mat4x4 TransformInfo::getModelMatrix() const {
    return glm::translate(glm::identity<glm::mat4>(), translation) *
           glm::toMat4(rotation) *
           glm::scale(glm::identity<glm::mat4>(), scale);
}

SceneInfo loadScene(std::filesystem::path file_path) {
    SceneInfo scene;

    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err, warn;
    if (!loader.LoadBinaryFromFile(&model, &err, &warn, file_path.string())) {
        throw std::runtime_error(std::string("gltf load error: ") + err);
    }

    const auto &defaultScene = model.scenes[model.defaultScene >= 0 ? model.defaultScene : 0];
    const auto &rootNode = model.nodes[defaultScene.nodes[0]];

    for (const auto collectionNodeIndex : rootNode.children) {
        const auto &collectionNode = model.nodes[collectionNodeIndex];

        for(const auto nodeIndex : collectionNode.children) {
            const auto &node = model.nodes[nodeIndex];

            pl::TransformInfo transform;
            if (node.translation.size() == 3)
                transform.translation = glm::vec3{node.translation[0], node.translation[1], node.translation[2]};
            else
                transform.translation = glm::vec3(0, 0, 0);
    
            if (node.rotation.size() == 4)
                // gltf rotation is (x,y,z,w) order, glm::quat is (w,x,y,z) order.
                transform.rotation = glm::quat{float(node.rotation[3]), float(node.rotation[0]), float(node.rotation[1]), float(node.rotation[2])};
            else
                transform.rotation = glm::identity<glm::quat>();
    
            if (node.scale.size() == 3)
                transform.scale = glm::vec3{node.scale[0], node.scale[1], node.scale[2]};
            else
                transform.scale = glm::vec3(1, 1, 1);
    
            scene.objects[collectionNode.name].push_back(transform);
        }
    }

    return scene;
}

} // namespace pl
