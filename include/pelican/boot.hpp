#ifndef PELICAN_BOOT_HPP
#define PELICAN_BOOT_HPP
#include<iostream>
#include<GLFW/glfw3.h>

namespace pl {
	class Boot {
	public:
		GLFWwindow* systemInit(unsigned int Windowheight,unsigned int Windowwidth);
		void systemClean();
		bool frameUpdate();

	};
}
#endif

