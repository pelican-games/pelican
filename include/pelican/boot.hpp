#ifndef PELICAN_BOOT_HPP
#define PELICAN_BOOT_HPP
#include<iostream>
#include<GLFW/glfw3.h>
#include<pelican/renderer.hpp>
#include <chrono>

namespace pl {

class System {
	GLFWwindow* window;
	std::unique_ptr<Renderer> renderer;
	
	std::chrono::system_clock::time_point base_time;
	uint32_t frame_count;

public:
	System(unsigned int Windowheight,unsigned int Windowwidth);
	~System();

	bool frameUpdate();
	Renderer& getDefaultRenderer();
};

}
#endif

