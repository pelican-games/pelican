#include <pelican/pelican.hpp>
int main() {
    pl::system_init();
    while(pl::frame_update()) {}
    return 0;
}
