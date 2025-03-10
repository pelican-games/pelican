#include <filesystem>
#define GLM_ENABLE_EXPERIMENTAL
#include <fastgltf/core.hpp>
#include <glm/gtx/quaternion.hpp>
#include <pelican/sceneLoader.hpp>

namespace pl {

glm::mat4x4 TransformInfo::getModelMatrix() const {
    return glm::translate(glm::identity<glm::mat4>(), translation) *
           glm::toMat4(rotation) *
           glm::scale(glm::identity<glm::mat4>(), scale);
}

SceneInfo loadScene(std::filesystem::path file_path) {
    SceneInfo scene;

    auto gltfFile = fastgltf::GltfFileStream(file_path);
    if (!gltfFile.isOpen()) {
        throw std::runtime_error("Failed to open glTF file: " + file_path.string());
    }

    fastgltf::Parser parser;
    fastgltf::Asset model;
    auto asset = parser.loadGltfBinary(gltfFile, "");
    model = std::move(asset.get());

    const auto &defaultScene = model.scenes[model.defaultScene.value_or(0)];
    const auto &rootNode = model.nodes[defaultScene.nodeIndices[0]];

    for (const auto collectionNodeIndex : rootNode.children) {
        const auto &collectionNode = model.nodes[collectionNodeIndex];

        for (const auto nodeIndex : collectionNode.children) {
            const auto &node = model.nodes[nodeIndex];

            pl::TransformInfo transform;
            auto trs = std::get<fastgltf::TRS>(node.transform);
            transform.translation = glm::vec3{trs.translation[0], trs.translation[1], trs.translation[2]};

            // gltf rotation is (x,y,z,w) order, glm::quat is (w,x,y,z) order.
            transform.rotation = glm::quat{trs.rotation[3], trs.rotation[0], trs.rotation[1], trs.rotation[2]};

            transform.scale = glm::vec3{trs.scale[0], trs.scale[1], trs.scale[2]};

            scene.objects[collectionNode.name.c_str()].push_back(transform);
        }
    }

    return scene;
}

} // namespace pl
