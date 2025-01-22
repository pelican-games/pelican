#include "vulkan/model_db.hpp"
#include <pelican/pelican.hpp>

namespace pl {

ModelDataBase db;

Model loadModel(std::filesystem::path path) {
    return Model{ db.load_model(path) };
}

} // namespace pl
