#ifndef PELICAN_BOOT_HPP
#define PELICAN_BOOT_HPP
#include<iostream>
#include<GLFW/glfw3.h>

namespace pl {
	GLFWwindow* systemInit(unsigned int Windowheight,unsigned int Windowwidth);

	void systemClean(GLFWwindow* &window);

	bool frameUpdate(GLFWwindow* &window);

}
#endif

