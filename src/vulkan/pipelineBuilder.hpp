#pragma once
#include "geometry.hpp"

class PipelineBuilder{

    public:
    
    void clear();
    PipelineBuilder(){clear();};
    vk::UniquePipeline buildPipeline(vk::Device , std::vector<vk::PipelineShaderStageCreateInfo> shaderStages, uint32_t WIDTH, uint32_t HEIGHT);

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