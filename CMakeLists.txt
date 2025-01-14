cmake_minimum_required (VERSION 3.16)

project(pelican)

include(FetchContent)

add_library(pelican
	STATIC
    src/system.cpp
	src/audio.cpp
	src/speaker.cpp
)
target_include_directories(pelican PUBLIC include)
set_target_properties(pelican PROPERTIES CXX_STANDARD 20)


# Libralies

## glm
FetchContent_Declare(
	glm
	GIT_REPOSITORY	https://github.com/g-truc/glm.git
	GIT_TAG 	bf71a834948186f4097caa076cd2663c69a10e1e #refs/tags/1.0.1
)
FetchContent_MakeAvailable(glm)
target_link_libraries(pelican PUBLIC glm::glm)

## OpenAL-soft
FetchContent_Declare(
	OpenAL
	GIT_REPOSITORY	https://github.com/kcat/openal-soft
)
FetchContent_MakeAvailable(OpenAL)
target_link_libraries(pelican PRIVATE OpenAL)
set(OPENAL_LIBRARY ${openal_BINARY_DIR}/Release/OpenAL32.lib)
set(OPENAL_INCLUDE_DIR ${openal_SOURCE_DIR}/include/AL)

## FreeALUT
FetchContent_Declare(
	freealut
	GIT_REPOSITORY	https://github.com/vancegroup/freealut
)
FetchContent_MakeAvailable(freealut)
target_link_libraries(pelican PRIVATE alut)

## GLFW
FetchContent_Declare(
	glfw
	GIT_REPOSITORY	https://github.com/glfw/glfw
)
FetchContent_MakeAvailable(glfw)
target_link_libraries(pelican PRIVATE glfw)

## Vulkan
find_package(Vulkan REQUIRED)
target_include_directories(pelican PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(pelican PRIVATE ${Vulkan_LIBRARIES})

add_subdirectory(example)

# Add the test application
add_executable(test_app src/test.cpp)
target_include_directories(test_app PRIVATE include)
target_link_libraries(test_app PRIVATE pelican OpenAL alut glfw Vulkan::Vulkan)
set_target_properties(test_app PROPERTIES CXX_STANDARD 20)


