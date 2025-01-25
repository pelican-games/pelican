#pragma once
#include "geometry.hpp"

namespace pl {

struct Render2DPushConstantInfo {
    glm::vec2 tl, sz, texclip_tl, texclip_sz;
};

class Pipeline2DBuilder {

  public:
    vk::UniquePipeline buildPipeline(vk::Device, vk::UniquePipelineLayout &pipelinelayout, std::vector<vk::PipelineShaderStageCreateInfo> shaderStages, uint32_t WIDTH, uint32_t HEIGHT);
};

} // namespace pl
