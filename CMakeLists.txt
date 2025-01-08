cmake_minimum_required (VERSION 3.16)

project(pelican)

include(FetchContent)

add_library(pelican
    src/system.cpp
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


add_subdirectory(example)
