#include "vulkan/model_db.hpp"
#include <pelican/pelican.hpp>

namespace pl {

std::vector<std::vector<glm::vec3>> Model::getVertices() const {
    std::vector<std::vector<glm::vec3>> vertices;
    for (const auto &mesh : pDat->meshes) {
        for (const auto &primitive : mesh.primitives) {
            std::vector<glm::vec3> primitiveVertices(primitive.vertices.size());
            for (int i = 0; i < primitive.vertices.size(); i++)
                primitiveVertices[i] = primitive.vertices[i].position;
            vertices.push_back(std::move(primitiveVertices));
        }
    }

    return vertices;
}

} // namespace pl
