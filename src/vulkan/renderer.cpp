#include <pelican/pelican.hpp>

namespace pl {

Renderer defaultRenderer;

Renderer& getDefaultRenderer() {
    return defaultRenderer;
}

void Renderer::drawModel(const Model& model, glm::mat4x4 modelMatrix) {

}
void Renderer::setCamera(glm::vec3 pos, glm::vec3 dir, glm::vec3 up) {

}
void Renderer::setProjection(float horizontalAngle) {

}

}
