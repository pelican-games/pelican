cmake_minimum_required (VERSION 3.16)

project(pelican)

include(FetchContent)

add_library(pelican
	STATIC
    src/system.cpp
    src/model.cpp
    src/vulkan/model_db.cpp
    src/vulkan/app.cpp
    src/vulkan/pipelineBuilder.cpp
    src/vulkan/pipeline2DBuilder.cpp
    src/vulkan/image_db.cpp
	src/audio.cpp
	src/speaker.cpp
    src/physics.cpp 
    src/boot.cpp
    src/sceneLoader.cpp
)
target_include_directories(pelican PUBLIC 
    include
    ${CMAKE_SOURCE_DIR}/include
)
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
target_link_libraries(pelican PUBLIC OpenAL::OpenAL)

## FreeALUT
find_package(FreeALUT CONFIG REQUIRED)
target_link_libraries(pelican PUBLIC FreeALUT::alut)

## GLFW
find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(pelican PUBLIC glfw)

## Vulkan
find_package(Vulkan REQUIRED)
target_include_directories(pelican PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(pelican PUBLIC ${Vulkan_LIBRARIES})

#fastgltf
find_package(fastgltf CONFIG REQUIRED)
target_link_libraries(pelican PRIVATE fastgltf::fastgltf)

#libpng
find_package(PNG REQUIRED)
target_link_libraries(pelican PRIVATE PNG::PNG)

#stb
find_package(Stb REQUIRED)
target_include_directories(pelican PRIVATE ${Stb_INCLUDE_DIR})

add_subdirectory(example)

## Bullet Physics
find_package(Bullet CONFIG REQUIRED)
# specific set: BulletSoftBody, BulletDynamics, BulletInverseDynamics,
#               BulletCollision, Bullet3Common, LinearMath
target_link_libraries(pelican PUBLIC ${BULLET_LIBRARIES})
