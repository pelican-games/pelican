#include "pipeline2DBuilder.hpp"
#include "geometry.hpp"

namespace pl {

vk::UniquePipeline Pipeline2DBuilder::buildPipeline(vk::Device device, vk::UniquePipelineLayout &pipelineLayout, std::initializer_list<vk::DescriptorSetLayout> descLayouts, std::vector<vk::PipelineShaderStageCreateInfo> shaderStagesInput, uint32_t WIDTH, uint32_t HEIGHT) {
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages;
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    vk::PipelineViewportStateCreateInfo viewportState;
    vk::PipelineRasterizationStateCreateInfo rasterizer;
    vk::PipelineDepthStencilStateCreateInfo depthStencil;
    vk::PipelineMultisampleStateCreateInfo multisampling;
    vk::PipelineColorBlendStateCreateInfo colorBlending;

    // 入力シェーダーステージを設定
    shaderStages = shaderStagesInput;

    vertexInputInfo = vk::PipelineVertexInputStateCreateInfo();
    // InputAssemblyStateの設定
    inputAssembly = vk::PipelineInputAssemblyStateCreateInfo(
        {},                                   // flags
        vk::PrimitiveTopology::eTriangleList, // topology
        VK_FALSE                              // primitiveRestartEnable
    );

    vk::Viewport viewport(
        0.0f, 0.0f,
        static_cast<float>(WIDTH), static_cast<float>(HEIGHT),
        0.0f, 1.0f);

    vk::Rect2D scissor(
        {0, 0},
        {WIDTH, HEIGHT});

    viewportState = vk::PipelineViewportStateCreateInfo(
        {},        // flags
        1,         // viewportCount
        &viewport, // pViewports
        1,         // scissorCount
        &scissor   // pScissors
    );

    rasterizer = vk::PipelineRasterizationStateCreateInfo(
        {},                          // flags
        VK_FALSE,                    // depthClampEnable
        VK_FALSE,                    // rasterizerDiscardEnable
        vk::PolygonMode::eFill,      // polygonMode
        vk::CullModeFlagBits::eNone, // cullMode
        vk::FrontFace::eClockwise,   // frontFace
        VK_FALSE,                    // depthBiasEnable
        0.0f,                        // depthBiasConstantFactor
        0.0f,                        // depthBiasClamp
        0.0f,                        // depthBiasSlopeFactor
        1.0f                         // lineWidth
    );

    multisampling = vk::PipelineMultisampleStateCreateInfo(
        {},                          // flags
        vk::SampleCountFlagBits::e1, // rasterizationSamples
        VK_FALSE,                    // sampleShadingEnable
        1.0f,                        // minSampleShading
        nullptr,                     // pSampleMask
        VK_FALSE,                    // alphaToCoverageEnable
        VK_FALSE                     // alphaToOneEnable
    );

    vk::PipelineColorBlendAttachmentState colorBlendAttachment(
        VK_TRUE,                            // blendEnable
        vk::BlendFactor::eSrcAlpha,         // srcColorBlendFactor
        vk::BlendFactor::eOneMinusSrcAlpha, // dstColorBlendFactor
        vk::BlendOp::eAdd,                  // colorBlendOp
        vk::BlendFactor::eOne,              // srcAlphaBlendFactor
        vk::BlendFactor::eZero,             // dstAlphaBlendFactor
        vk::BlendOp::eAdd,                  // alphaBlendOp
        vk::ColorComponentFlags(
            vk::ColorComponentFlagBits::eR |
            vk::ColorComponentFlagBits::eG |
            vk::ColorComponentFlagBits::eB |
            vk::ColorComponentFlagBits::eA) // colorWriteMask
    );

    colorBlending = vk::PipelineColorBlendStateCreateInfo(
        {},                      // flags
        VK_FALSE,                // logicOpEnable
        vk::LogicOp::eCopy,      // logicOp
        1,                       // attachmentCount
        &colorBlendAttachment,   // pAttachments
        {0.0f, 0.0f, 0.0f, 0.0f} // blendConstants
    );

    // DynamicStateの設定
    std::vector<vk::DynamicState> dynamicStates = {
        vk::DynamicState::eViewport,
        vk::DynamicState::eScissor};

    vk::PipelineDynamicStateCreateInfo dynamicState(
        {},                   // flags
        dynamicStates.size(), // dynamicStateCount
        dynamicStates.data()  // pDynamicStates
    );

    // PipelineLayoutの設定

    std::vector<vk::PushConstantRange> pushConstantRanges = {
        vk::PushConstantRange(
            vk::ShaderStageFlagBits::eVertex,    // stageFlags
            0,                                   // offset
            sizeof(pl::Render2DPushConstantInfo) // size
            ),
    };

    vk::PipelineLayoutCreateInfo pipelineLayoutInfo(
        {},                        // flags
        descLayouts.size(),        // setLayoutCount
        descLayouts.begin(),       // pSetLayouts
        pushConstantRanges.size(), // pushConstantRangeCount
        pushConstantRanges.data()  // pPushConstantRanges
    );

    pipelineLayout = device.createPipelineLayoutUnique(pipelineLayoutInfo);

    // RenderingCreateInfoの設定
    std::vector<vk::Format> colorAttachmentFormats = {vk::Format::eB8G8R8A8Unorm};

    vk::PipelineRenderingCreateInfo renderingCreateInfo(
        0,                             // viewMask
        colorAttachmentFormats.size(), // colorAttachmentCount
        colorAttachmentFormats.data(), // pColorAttachmentFormats
        vk::Format::eUndefined,        // depthAttachmentFormat
        vk::Format::eUndefined         // stencilAttachmentFormat
    );

    vk::GraphicsPipelineCreateInfo pipelineCreateInfo(
        {},                   // flags
        shaderStages.size(),  // stageCount
        shaderStages.data(),  // pStages
        &vertexInputInfo,     // pVertexInputState
        &inputAssembly,       // pInputAssemblyState
        VK_NULL_HANDLE,       // pTessellationState
        &viewportState,       // pViewportState
        &rasterizer,          // pRasterizationState
        &multisampling,       // pMultisampleState
        nullptr,              // pDepthStencilState
        &colorBlending,       // pColorBlendState
        VK_NULL_HANDLE,       // pDynamicState
        pipelineLayout.get(), // layout
        VK_NULL_HANDLE,       // renderPass
        0,                    // subpass
        VK_NULL_HANDLE,       // basePipelineHandle
        -1                    // basePipelineIndex
    );
    pipelineCreateInfo.setPNext(&renderingCreateInfo);

    vk::UniquePipeline pipeline = device.createGraphicsPipelineUnique(VK_NULL_HANDLE, pipelineCreateInfo).value;
    return pipeline;
}

} // namespace pl
