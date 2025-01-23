#pragma once
#include "geometry.hpp"
#include <list>
#include <filesystem>

namespace pl {

struct ObjectDataBase {
    std::vector<pl::Object> objects;
};

struct ModelData {
    std::vector<pl::Mesh> meshes;

    void add_object(ObjectDataBase& objDb);
};

struct ModelDataBase {

std::list<pl::Material> materials;
std::list<pl::ModelData> models;

const pl::ModelData* load_model(std::filesystem::path file_path);

};
    
}
