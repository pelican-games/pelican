#pragma once
#include "geometry.hpp"

namespace pl {

class PipelineBuilder {                                                                                                                                                                                                                                
  public:
    void clear();
    PipelineBuilder() { clear(); };
    vk::UniquePipeline buildPipeline(vk::Device device, vk::UniquePipelineLayout& pipelineLayout,std::vector<vk::DescriptorSetLayout> descriptorSetLayouts, std::vector<vk::PipelineShaderStageCreateInfo> shaderStagesInput, uint32_t WIDTH, uint32_t HEIGHT);

    vk::UniquePipeline gBufPipelinr(vk::Device, vk::UniquePipelineLayout& pipelinelayout, std::vector<vk::PipelineShaderStageCreateInfo> shaderStages, uint32_t WIDTH, uint32_t HEIGHT);
    vk::UniquePipeline shadingPipeline(vk::Device, vk::UniquePipelineLayout& pipelinelayout, std::vector<vk::PipelineShaderStageCreateInfo> shaderStages, uint32_t WIDTH, uint32_t HEIGHT);

  private:
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    vk::PipelineViewportStateCreateInfo viewportState;
    vk::PipelineRasterizationStateCreateInfo rasterizer;
    vk::PipelineDepthStencilStateCreateInfo depthStencil;
    vk::PipelineMultisampleStateCreateInfo multisampling;
    vk::PipelineColorBlendStateCreateInfo colorBlending;
    vk::PipelineDynamicStateCreateInfo dynamicState;
    vk::UniquePipelineLayout pipelineLayout;
};

} // namespace pl
