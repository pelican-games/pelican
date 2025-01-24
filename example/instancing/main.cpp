#include <pelican/pelican.hpp>
#include <iostream>
int main() {
    try {
        pl::System sys(800, 600);

        pl::Renderer& renderer = sys.getDefaultRenderer();
        renderer.setup();

        auto scene = pl::loadScene("example/assets/instanceTest2.glb");

        for(auto& [name, objs] : scene.objects) {
            std::cout << name << ":\n";
            for(auto& obj : objs) {
                std::cout << "  translate: (" << obj.translation[0] << ", " << obj.translation[1] << ", " << obj.translation[2] << ")" << std::endl;
                std::cout << "  rotation: (" << obj.rotation[0] << ", " << obj.rotation[1] << ", " << obj.rotation[2] << ", " << obj.rotation[3] << ")" << std::endl;
                std::cout << "  scale: (" << obj.scale[0] << ", " << obj.scale[1] << ", " << obj.scale[2] << ")" << std::endl;
            }
        }
    } catch(std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
