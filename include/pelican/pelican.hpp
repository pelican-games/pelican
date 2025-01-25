#pragma once
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include "pelican/audio.hpp"
#include "pelican/input.hpp"
#include "pelican/boot.hpp"
#include "pelican/physics.hpp"
#include "pelican/renderer.hpp"
#include "pelican/sceneLoader.hpp"
#include "pelican/speaker.hpp"
#include "pelican/system.hpp"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
namespace pl {

Speaker generateSpeaker();
void setListenerPos(glm::vec3 pos);

} // namespace pl