#include <pelican/pelican.hpp>
#include <iostream>
int main() {
    try {
        pl::System sys(800, 600);

        pl::Renderer& renderer = sys.getDefaultRenderer();
        pl::Model test_model = renderer.loadModel("example/assets/test.glb");
        renderer.loadObject("example/assets/test.glb");
        renderer.setup();

        while(sys.frameUpdate()) {
            renderer.setCamera(glm::vec3(2.0, 0.0, 0.0), glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
            renderer.setObjectData();
        }
    } catch(std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
