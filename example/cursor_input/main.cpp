#include <iostream>
#include <pelican/pelican.hpp>
#include <thread>

int main() {
    pl::Cursor cursor;
    try {
        pl::System sys(800, 600);

        pl::Renderer &renderer = sys.getDefaultRenderer();
        std::cout << "Renderer loaded" << std::endl;
        pl::Model test_model = renderer.loadModel("example/assets/simple_box.glb");
        std::cout << "Model loaded" << std::endl;

        while (sys.frameUpdate()) {
            renderer.setCamera(glm::vec3(15.0, 0.0, 0.0), glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
            renderer.setProjection(45.0);

            auto pos = cursor.getPos();
            renderer.drawModel(
                test_model,
                glm::translate(
                    glm::identity<glm::mat4>(),
                    {0, (pos.y - 300.0f) / 100, -(pos.x - 400.0f) / 100}) *
                    glm::scale(
                        glm::identity<glm::mat4>(),
                        cursor.left_pressed() ? glm::vec3{1.0, 1.0, 1.0} : glm::vec3{0.5, 0.5, 0.5}));
        }
    } catch (std::exception &e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
