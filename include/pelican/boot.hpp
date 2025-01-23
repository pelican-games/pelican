#ifndef PELICAN_BOOT_HPP
#define PELICAN_BOOT_HPP
#include<iostream>
#include<GLFW/glfw3.h>

namespace pl {

class System {
	GLFWwindow* window;
public:
	System(unsigned int Windowheight,unsigned int Windowwidth);
	~System();

	bool frameUpdate();
};

}
#endif

