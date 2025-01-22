cmake_minimum_required (VERSION 3.16)

project(pelican)

include(FetchContent)

add_library(pelican
	STATIC
    src/system.cpp
    src/vulkan/model_db.cpp
	src/audio.cpp
	src/speaker.cpp
)
target_include_directories(pelican PUBLIC include)
set_target_properties(pelican PROPERTIES CXX_STANDARD 20)

if(MSVC)
target_compile_options(pelican PUBLIC "/utf-8")
endif()

# Libralies

## glm
find_package(glm CONFIG REQUIRED)
target_link_libraries(pelican PUBLIC glm::glm)

## OpenAL-soft
find_package(OpenAL CONFIG REQUIRED)
target_link_libraries(pelican PRIVATE OpenAL::OpenAL)

## FreeALUT
find_package(FreeALUT CONFIG REQUIRED)
target_link_libraries(pelican PRIVATE FreeALUT::alut)

## GLFW
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(pelican PRIVATE glfw)

## Vulkan
find_package(Vulkan REQUIRED)
target_include_directories(pelican PRIVATE ${Vulkan_INCLUDE_DIRS})
target_link_libraries(pelican PRIVATE ${Vulkan_LIBRARIES})

## tinygltf
find_path(TINYGLTF_INCLUDE_DIRS "tiny_gltf.h")
target_include_directories(pelican PRIVATE ${TINYGLTF_INCLUDE_DIRS})

add_subdirectory(example)

## Bullet Physics
find_package(Bullet CONFIG REQUIRED)
# specific set: BulletSoftBody, BulletDynamics, BulletInverseDynamics,
#               BulletCollision, Bullet3Common, LinearMath
target_link_libraries(pelican PRIVATE ${BULLET_LIBRARIES})
# Add the test application
add_executable(test_app src/test.cpp)
target_include_directories(test_app PRIVATE include)
target_link_libraries(test_app PRIVATE pelican OpenAL::OpenAL FreeALUT::alut glfw Vulkan::Vulkan)
set_target_properties(test_app PROPERTIES CXX_STANDARD 20)


