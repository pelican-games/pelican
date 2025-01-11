#include <pelican/pelican.hpp>
#include <iostream>
int main() {
    try {
        pl::system_init();

        pl::Renderer& renderer = pl::getDefaultRenderer();
        pl::Model test_model = pl::loadModel("example/assets/fox.glb");
        while(pl::frame_update()) {
            renderer.setCamera(glm::vec3(2.0, 0.0, 0.0), glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
            renderer.drawModel(test_model, glm::mat4x4());
        }
    } catch(std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
