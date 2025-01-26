#include <pelican/pelican.hpp>
#include <iostream>
int main() {
    try {
        pl::System sys(800, 600);

        pl::Renderer& renderer = sys.getDefaultRenderer();
        std::cout << "Renderer loaded" << std::endl;
        pl::Model test_model = renderer.loadModel("example/assets/DamagedHelmet.glb");

        std::cout << "Model loaded" << std::endl;

        float theta = 0;

        while(sys.frameUpdate()) {
            theta += 0.001f;

            renderer.setCamera(glm::vec3(cos(theta) * 4.0f, sin(theta) * 4.0f, 0.0), glm::vec3(-cos(theta), -sin(theta), 0.0), glm::vec3(0.0, 0.0, 1.0));
            renderer.setProjection(45.0);
            renderer.drawModel(test_model, glm::identity<glm::mat4>());
        }
    } catch(std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
