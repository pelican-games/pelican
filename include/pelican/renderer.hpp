#ifndef PELICAN_RENDERER_HPP
#define PELICAN_RENDERER_HPP

#include <memory>
#include <glm/glm.hpp>
#include <filesystem>
//#include "geometry.hpp"

namespace pl {

struct ModelData;
struct Model {
    pl::ModelData *pDat;
};

class Renderer {
  public:
    virtual void drawModel(const Model &model, glm::mat4x4 modelMatrix) = 0;
    //virtual void setObjectData() = 0;
    virtual void setCamera(glm::vec3 pos, glm::vec3 dir, glm::vec3 up) = 0;
    virtual void setProjection(float horizontalAngle) = 0;
    virtual pl::Model loadModel(std::filesystem::path file_path) = 0;
    //virtual void loadObject(std::filesystem::path file_path) = 0;
    virtual void drawFrame() = 0;
    virtual void setup() = 0;
    virtual ~Renderer() = default;
};

} // namespace pl

#endif // RENDERER_HPP
