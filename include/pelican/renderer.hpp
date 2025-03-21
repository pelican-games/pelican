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
    std::vector<std::vector<glm::vec3>> getVertices() const;
};

struct UIImageData;
struct UIImage {
    pl::UIImageData *pDat;
};

class Renderer {
  public:
    virtual void drawModel(const Model &model, glm::mat4x4 modelMatrix) = 0;
    virtual void setCamera(glm::vec3 pos, glm::vec3 dir, glm::vec3 up) = 0;
    virtual void setProjection(float horizontalAngle, float near = 0.1f, float far = 100.0f) = 0;
    virtual pl::Model loadModel(std::filesystem::path file_path, uint32_t max_object_num = 128) = 0;
    //virtual void loadObject(std::filesystem::path file_path) = 0;

    virtual void drawUIImage(const UIImage &image, int x, int y, int texX, int texY, int texW, int texH, float scaleX, float scaleY) = 0;
    virtual pl::UIImage loadUIImage(std::filesystem::path file_path) = 0;
    virtual void loadIBL(std::filesystem::path file_path) = 0;
    virtual void drawFrame() = 0;
    virtual void setViewport(int x, int y, int w, int h) = 0;
    virtual void setLine(glm::vec4 color, float width) = 0;
    virtual void setSubColor(glm::vec4 subColor = {1.0, 1.0, 1.0, 1.0}) = 0;
    virtual ~Renderer() = default;
};

} // namespace pl

#endif // RENDERER_HPP
