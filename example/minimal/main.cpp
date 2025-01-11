#include <pelican/pelican.hpp>
#include <iostream>
int main() {
    try {
        pl::system_init();
        while(pl::frame_update()) {}
    } catch(std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
    }
    return 0;
}
