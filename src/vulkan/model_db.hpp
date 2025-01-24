#pragma once
#include "geometry.hpp"
#include <list>
#include <filesystem>

namespace pl {

struct ObjectDataBase {
    std::vector<pl::Object> objects;

    const pl::ObjectDataBase* load_object(std::filesystem::path file_path);
};

struct ModelData {
    std::vector<pl::Mesh> meshes;//現状は1つのみ
    std::vector<pl::InstanceAttribute> instanceAttributes;
    uint32_t modelIndex;
    void add_object(ObjectDataBase& objDb);
};

struct ModelDataBase {
    std::vector<std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory>> vertexBuffers;
    std::vector<std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory>> indexBuffers;
    std::vector<std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory>> instanceBuffers;
    std::list<pl::Material> materials;
    std::list<pl::ModelData> models;

    const pl::ModelData* load_model(std::filesystem::path file_path);
};
    
}
