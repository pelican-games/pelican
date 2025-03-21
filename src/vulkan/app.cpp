#include "app.hpp"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <vulkan/vulkan.hpp>

namespace pl {

VulkanApp::VulkanApp(GLFWwindow *window, unsigned int screenWidth, unsigned int screenHeight)
    : window(window), screenWidth{screenWidth}, screenHeight{screenHeight}, currentSubColor{1.0, 1.0, 1.0, 1.0} {
    std::cout << "Vulkan Header Version: " << VK_HEADER_VERSION << std::endl;

    // APIバージョンのプリント（ヘッダーで定義された最新のAPIバージョンを表示）
    std::cout << "Vulkan API Version: "
              << VK_VERSION_MAJOR(VK_HEADER_VERSION_COMPLETE)
              << "."
              << VK_VERSION_MINOR(VK_HEADER_VERSION_COMPLETE)
              << "."
              << VK_VERSION_PATCH(VK_HEADER_VERSION_COMPLETE)
              << std::endl;

    // インスタンスの初期化

    vk::ApplicationInfo appInfo{};
    appInfo.pApplicationName = "Vulkan Application";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_3;

    std::vector<const char*> requiredLayers;
#ifdef _DEBUG
    std::cout << "Vulkan Validation Layer Enabled" << std::endl;
    requiredLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif
    uint32_t instanceExtensionCount = 0;
    const char **requiredExtensions = glfwGetRequiredInstanceExtensions(&instanceExtensionCount);
    vk::InstanceCreateInfo instCreateInfo(
        {},
        &appInfo,
        requiredLayers.size(),
        requiredLayers.data(),
        instanceExtensionCount,
        requiredExtensions);

    instance = vk::createInstanceUnique(instCreateInfo);
    // 物理デバイスの初期化
    auto deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    vk::PhysicalDeviceFeatures deviceFeatures = {}; // DeviceFeaturesの設定
    deviceFeatures.geometryShader = VK_TRUE;
    deviceFeatures.samplerAnisotropy = VK_TRUE;
    deviceFeatures.sampleRateShading = VK_TRUE;

    // 物理デバイスの選択
    physicalDevice = pickPhysicalDevice(deviceExtensions, deviceFeatures);

    // サーフェスの作成
    createSurface();

    // デバイスの初期化
    std::vector<float> graphicQueuePriorities;
    std::vector<float> computeQueuePriorities;
    queueCreateInfos = findQueues(graphicQueuePriorities, computeQueuePriorities);

    vk::DeviceCreateInfo deviceCreateInfo(
        {},
        static_cast<uint32_t>(queueCreateInfos.size()),
        queueCreateInfos.data(),
        0,
        nullptr,
        deviceExtensions.size(),
        deviceExtensions.begin(),
        &deviceFeatures);

    vk::StructureChain createInfoChain{
        deviceCreateInfo,
        vk::PhysicalDeviceDynamicRenderingFeatures{true}};

    device = physicalDevice.createDeviceUnique(createInfoChain.get<vk::DeviceCreateInfo>());
    // キューの取得
    for (uint32_t i = 0; i < queueCreateInfos[0].queueCount; i++) { // グラフィックスキューの取得
        graphicsQueues.push_back(device->getQueue(queueCreateInfos[0].queueFamilyIndex, i));
    }

    // for(uint32_t i = 0 ; i<queueCreateInfos[1].queueCount ; i++) {//コンピュートキューの取得
    //     computeQueues.push_back(device->getQueue(queueCreateInfos[1].queueFamilyIndex, i));
    // }

    // コマンドプールの作成
    vk::CommandPoolCreateInfo graphicCommandPoolCreateInfo({vk::CommandPoolCreateFlagBits::eResetCommandBuffer}, queueCreateInfos[0].queueFamilyIndex);
    graphicCommandPool = device->createCommandPoolUnique(graphicCommandPoolCreateInfo);
    // vk::CommandPoolCreateInfo computeCommandPoolCreateInfo({}, queueCreateInfos[1].queueFamilyIndex);
    // computeCommandPool = device->createCommandPoolUnique(computeCommandPoolCreateInfo);

    // コマンドバッファの作成
    vk::CommandBufferAllocateInfo graphicCmdBufAllocInfo(graphicCommandPool.get(), vk::CommandBufferLevel::ePrimary, 1);
    graphicCommandBuffers = device->allocateCommandBuffersUnique(graphicCmdBufAllocInfo);
    // vk::CommandBufferAllocateInfo computeCmdBufAllocInfo(computeCommandPool.get(), vk::CommandBufferLevel::ePrimary, 1);
    // computeCommandBuffers = device->allocateCommandBuffersUnique(computeCmdBufAllocInfo);

    // イメージの作成
    // image = createImage(screenWidth, screenHeight, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

    uiimageDb.emplace(device.get(), physicalDevice, graphicsQueues[0], graphicCommandPoolCreateInfo.queueFamilyIndex);
    // シェーダーモジュールの作成
    vk::UniqueShaderModule vertShader2DModule = createShaderModule("src/shaders/shader2d.vert.spv");
    vk::UniqueShaderModule fragShader2DModule = createShaderModule("src/shaders/shader2d.frag.spv");
    // パイプラインの作成
    std::vector<vk::PipelineShaderStageCreateInfo> shader2DStages = {
        vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertShader2DModule.get(), "main"),
        vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragShader2DModule.get(), "main")};
    pipeline2DBuilder = std::make_unique<Pipeline2DBuilder>();
    pipeline2D = pipeline2DBuilder->buildPipeline(device.get(), pipeline2DLayout, {uiimageDb->descLayout.get()}, shader2DStages, screenWidth, screenHeight);

    // デスクリプタセットの作成
    std::vector<vk::DescriptorSetLayoutBinding> descriptorSetLayoutBindings = {
        vk::DescriptorSetLayoutBinding(0, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment),
        vk::DescriptorSetLayoutBinding(1, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment),
        vk::DescriptorSetLayoutBinding(2, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment),
        vk::DescriptorSetLayoutBinding(3, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment),
        vk::DescriptorSetLayoutBinding(4, vk::DescriptorType::eCombinedImageSampler, 1, vk::ShaderStageFlagBits::eFragment),
    };
    vk::DescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo({}, descriptorSetLayoutBindings.size(), descriptorSetLayoutBindings.data());
    descriptorSetLayout = device->createDescriptorSetLayoutUnique(descriptorSetLayoutCreateInfo);

    // シェーダーモジュールの作成
    vk::UniqueShaderModule vertShaderModule = createShaderModule("src/shaders/shader.vert.spv");
    vk::UniqueShaderModule geomShaderModule = createShaderModule("src/shaders/shader.geom.spv");
    vk::UniqueShaderModule fragShaderModule = createShaderModule("src/shaders/shader.frag.spv");

    // パイプラインの作成
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = {
        vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertShaderModule.get(), "main"),
        vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eGeometry, geomShaderModule.get(), "main"),
        vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragShaderModule.get(), "main")};

    pipelineBuilder = std::make_unique<PipelineBuilder>();
    std::vector<vk::DescriptorSetLayout> descSetLayout = {descriptorSetLayout.get()};
    pipeline = pipelineBuilder->buildPipeline(device.get(), pipelineLayout, descSetLayout, shaderStages, screenWidth, screenHeight);

    // スワップチェーンの作成
    createSwapchain();
    // スワップチェーンイメージ用フェンスの作成
    vk::FenceCreateInfo fenceCreateInfo{};
    swapchainImgFence = device->createFenceUnique(fenceCreateInfo);

    setViewport(0, 0, screenWidth, screenHeight);
}

VulkanApp::~VulkanApp() {
}

pl::Model VulkanApp::loadModel(std::filesystem::path file_path, uint32_t max_object_num) {
    auto model = modelDb.load_model(file_path);

    for (auto &mesh : model->meshes) {
        std::vector<Vertex> meshVertices;
        std::vector<uint32_t> meshIndices;

        for (auto primitive : mesh.primitives) {
            meshVertices.insert(meshVertices.end(), primitive.vertices.begin(), primitive.vertices.end());
            meshIndices.insert(meshIndices.end(), primitive.indices.begin(), primitive.indices.end());
        }
        // インスタンスのあるオブジェクトの頂点バッファを作成
        modelDb.vertexBuffers.push_back(createBuffer({}, meshVertices.size() * sizeof(Vertex), vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eHostVisible));
        void *meshVertexBufMem = device->mapMemory(modelDb.vertexBuffers.back().second.get(), 0, meshVertices.size() * sizeof(Vertex));
        std::memcpy(meshVertexBufMem, meshVertices.data(), meshVertices.size() * sizeof(Vertex));
        device->unmapMemory(modelDb.vertexBuffers.back().second.get());

        // インスタンスのあるオブジェクトのインデックスバッファを作成
        modelDb.indexBuffers.push_back(createBuffer({}, meshIndices.size() * sizeof(uint32_t), vk::BufferUsageFlagBits::eIndexBuffer, vk::MemoryPropertyFlagBits::eHostVisible));
        void *meshIndexBufMem = device->mapMemory(modelDb.indexBuffers.back().second.get(), 0, meshIndices.size() * sizeof(uint32_t));
        std::memcpy(meshIndexBufMem, meshIndices.data(), meshIndices.size() * sizeof(uint32_t));
        device->unmapMemory(modelDb.indexBuffers.back().second.get());

        indexCounts.push_back(std::make_pair(meshIndices.size(), model->instanceAttributes.size()));
    }
    // インスタンスのあるオブジェクトのインスタンスバッファを作成
    model->instanceAttributes.resize(max_object_num);
    model->modelIndex = modelDb.instanceBuffers.size();
    modelDb.instanceBuffers.emplace_back(createBuffer({}, model->instanceAttributes.size() * sizeof(InstanceAttribute), vk::BufferUsageFlagBits::eVertexBuffer, vk::MemoryPropertyFlagBits::eHostVisible));
    transferTexture(*model);
    return Model{model};
}

// 物理デバイスの選択
vk::PhysicalDevice VulkanApp::pickPhysicalDevice(const std::vector<const char *> &deviceExtensions, vk::PhysicalDeviceFeatures deviceFeatures) {
    for (const auto &device : instance->enumeratePhysicalDevices()) {
        if (checkDeviceExtensionSupport(device, deviceExtensions) && checkDeviceFeatures(device, deviceFeatures)) {
            return device;
        }
    }
    throw std::runtime_error("適切な物理デバイスが見つかりませんでした");
}

// 物理デバイスのextensionをチェック
bool VulkanApp::checkDeviceExtensionSupport(vk::PhysicalDevice device, const std::vector<const char *> &deviceExtensions) {
    std::set<std::string> requiredExtensions{deviceExtensions.begin(),
                                             deviceExtensions.end()};
    for (const auto &extension : device.enumerateDeviceExtensionProperties()) {
        requiredExtensions.erase(extension.extensionName);
    }
    return requiredExtensions.empty();
}

// 物理デバイスのfeaturesをチェック
bool VulkanApp::checkDeviceFeatures(vk::PhysicalDevice device, vk::PhysicalDeviceFeatures requiredFeatures) {
    vk::PhysicalDeviceFeatures deviceFeatures = device.getFeatures();
    if (!deviceFeatures.robustBufferAccess && requiredFeatures.robustBufferAccess) {
        return false;
    } else if (!deviceFeatures.geometryShader && requiredFeatures.geometryShader) {
        return false;
    }
    return true;
}

// サーフェスの作成
void VulkanApp::createSurface() {
    auto result = glfwCreateWindowSurface(instance.get(), window, nullptr, &c_surface);
    if (result != VK_SUCCESS) {
        const char *err;
        glfwGetError(&err);
        std::cout << err << std::endl;
        glfwTerminate();
        throw std::runtime_error("サーフェスの作成に失敗しました");
    }
    surface = vk::UniqueSurfaceKHR{c_surface, instance.get()};
}

// キューの検索
std::vector<vk::DeviceQueueCreateInfo> VulkanApp::findQueues(std::vector<float> &graphicQueuePriorities, std::vector<float> &computeQueuePriorities) {
    std::vector<vk::QueueFamilyProperties> queueProps = physicalDevice.getQueueFamilyProperties();
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos(2, vk::DeviceQueueCreateInfo{});

    uint32_t graphicsQueueIndex = -1;
    uint32_t graphicsQueueCount = 0;
    uint32_t computeQueueIndex = -1;
    uint32_t computeQueueCount = 0;

    for (uint32_t i = 0; i < queueProps.size(); i++) { // キューを持つ数が最大のものを選択
        if (queueProps[i].queueFlags & vk::QueueFlagBits::eGraphics && physicalDevice.getSurfaceSupportKHR(i, surface.get())) {
            graphicsQueueCount = std::max(graphicsQueueCount, queueProps[i].queueCount);
            if (graphicsQueueCount == queueProps[i].queueCount) {
                graphicsQueueIndex = i;
            }
        }
        if (queueProps[i].queueFlags & vk::QueueFlagBits::eCompute) {
            computeQueueCount = std::max(computeQueueCount, queueProps[i].queueCount);
            if (computeQueueCount == queueProps[i].queueCount) {
                computeQueueIndex = i;
            }
        }
    }

    std::cout << "Graphics Queue Index: " << graphicsQueueCount << std::endl;

    if (graphicsQueueIndex >= 0) {
        for (uint32_t i = 0; i < graphicsQueueCount; i++) {
            float priority = static_cast<float>(i) / static_cast<float>(graphicsQueueCount);
            graphicQueuePriorities.push_back(priority);
            std::cout << "Graphics Queue Priority: " << priority << std::endl;
        }
        queueCreateInfos.at(0) = vk::DeviceQueueCreateInfo({},
                                                           graphicsQueueIndex,
                                                           graphicsQueueCount,
                                                           graphicQueuePriorities.data());
    } else {
        throw std::runtime_error("適切なキューが見つかりませんでした");
    }
    if (computeQueueIndex >= 0) {
        for (uint32_t i = 0; i < computeQueueCount; i++) {
            float priority = static_cast<float>(i) / static_cast<float>(computeQueueCount);
            computeQueuePriorities.push_back(priority);
            std::cout << "Compute Queue Priority: " << priority << std::endl;
        }
        queueCreateInfos.at(1) = vk::DeviceQueueCreateInfo({},
                                                           computeQueueIndex,
                                                           computeQueueCount,
                                                           computeQueuePriorities.data());
    } else {
        throw std::runtime_error("適切なキューが見つかりませんでした");
    }
    if (queueCreateInfos.at(1).queueFamilyIndex == queueCreateInfos.at(0).queueFamilyIndex) {
        queueCreateInfos.pop_back();
    }
    return queueCreateInfos;
}

// イメージの作成
std::pair<vk::UniqueImage, vk::UniqueDeviceMemory> VulkanApp::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::SampleCountFlagBits samples) {
    
    // サンプル数に応じたミップレベルの計算
    uint32_t mipLevels;
    if (samples == vk::SampleCountFlagBits::e1) {
        // 単一サンプル画像の場合はミップマップを計算
        mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
    } else {
        // マルチサンプル画像の場合はミップレベルを1に固定
        mipLevels = 1;
    }

    vk::ImageCreateInfo imageCreateInfo(
        {},
        vk::ImageType::e2D,
        format,
        vk::Extent3D(width, height, 1),
        mipLevels,  // 1からミップレベル数に変更
        1,
        samples,
        tiling,
        usage | vk::ImageUsageFlagBits::eTransferSrc, // ミップマップ生成に必要
        vk::SharingMode::eExclusive,
        0,
        nullptr,
        vk::ImageLayout::eUndefined);

    vk::UniqueImage image = device->createImageUnique(imageCreateInfo);

    // メモリの割り当て
    vk::MemoryRequirements memRequirements = device->getImageMemoryRequirements(image.get());
    vk::MemoryAllocateInfo allocInfo(
        memRequirements.size,
        findMemoryType(memRequirements.memoryTypeBits, vk::MemoryPropertyFlagBits::eDeviceLocal));
    vk::UniqueDeviceMemory imageMemory = device->allocateMemoryUnique(allocInfo);
    device->bindImageMemory(image.get(), imageMemory.get(), 0);

    return std::make_pair(std::move(image), std::move(imageMemory));
}

vk::UniqueImageView VulkanApp::createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags) {
    // イメージからミップレベルを取得
    vk::ImageSubresourceRange subresourceRange(
        aspectFlags,
        0,                                  // baseMipLevel
        VK_REMAINING_MIP_LEVELS,            // levelCount - すべてのレベルを使用
        0,                                  // baseArrayLayer
        1                                   // layerCount
    );
    
    vk::ImageViewCreateInfo viewCreateInfo(
        {},
        image,
        vk::ImageViewType::e2D,
        format,
        vk::ComponentMapping(),
        subresourceRange);

    return device->createImageViewUnique(viewCreateInfo);
}
// モデルデータベースからオブジェクトを生成
std::vector<pl::Object> modelToObjects(const pl::ModelDataBase &modelDb) {
    std::vector<pl::Object> objects;
    for (const auto &model : modelDb.models) {
        for (const auto &mesh : model.meshes) {
            pl::Object object;
            object.Instance = true;
            object.mesh = const_cast<pl::Mesh *>(&mesh);
            object.transform = pl::Transform();
            pl::InstanceAttribute attribute;
            attribute.model = glm::mat4(0.01f);
            object.instanceAttributes.push_back(attribute);
            objects.push_back(std::move(object));
        }
    }
    return objects;
}

// コマンドバッファの作成

std::pair<vk::UniqueCommandPool, std::vector<vk::UniqueCommandBuffer>> VulkanApp::createCommandBuffers(vk::CommandPoolCreateFlagBits commandPoolFlag, vk::DeviceQueueCreateInfo queueCreateInfo, uint32_t commandBufferCount) {

    vk::CommandPoolCreateInfo commandPoolCreateInfo(commandPoolFlag, queueCreateInfo.queueFamilyIndex);
    vk::UniqueCommandPool commandPool = device->createCommandPoolUnique(commandPoolCreateInfo);

    vk::CommandBufferAllocateInfo commandBufferAllocateInfo(commandPool.get(), vk::CommandBufferLevel::ePrimary, commandBufferCount);
    std::vector<vk::UniqueCommandBuffer> commandBuffers = device->allocateCommandBuffersUnique(commandBufferAllocateInfo);

    return std::pair<vk::UniqueCommandPool, std::vector<vk::UniqueCommandBuffer>>(std::move(commandPool), std::move(commandBuffers));
}

vk::ImageMemoryBarrier VulkanApp::createImageMemoryBarrier(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask, vk::ImageAspectFlags aspectMask) {
    vk::ImageMemoryBarrier imageMemoryBarrier(
        srcAccessMask,
        dstAccessMask,
        oldLayout,
        newLayout,
        VK_QUEUE_FAMILY_IGNORED,
        VK_QUEUE_FAMILY_IGNORED,
        image,
        vk::ImageSubresourceRange(aspectMask, 0, 1, 0, 1));
    return imageMemoryBarrier;
}

void VulkanApp::generateMipmaps(vk::CommandBuffer cmdBuffer, vk::Image image, 
        int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
    // ブリットでミップマップを生成
    vk::ImageMemoryBarrier barrier;
    barrier.image = image;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.subresourceRange.levelCount = 1;

    int32_t mipWidth = texWidth;
    int32_t mipHeight = texHeight;

    for (uint32_t i = 1; i < mipLevels; i++) {
        barrier.subresourceRange.baseMipLevel = i - 1;
        barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrier.newLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrier.dstAccessMask = vk::AccessFlagBits::eTransferRead;

        cmdBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eTransfer, {},{}, {}, barrier
        );

        vk::ImageBlit blit;
        blit.srcOffsets[0] = vk::Offset3D(0, 0, 0);
        blit.srcOffsets[1] = vk::Offset3D(mipWidth, mipHeight, 1);
        blit.srcSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.srcSubresource.mipLevel = i - 1;
        blit.srcSubresource.baseArrayLayer = 0;
        blit.srcSubresource.layerCount = 1;
        blit.dstOffsets[0] = vk::Offset3D(0, 0, 0);
        blit.dstOffsets[1] = vk::Offset3D(mipWidth > 1 ? mipWidth / 2 : 1, 
                    mipHeight > 1 ? mipHeight / 2 : 1, 1);
        blit.dstSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        blit.dstSubresource.mipLevel = i;
        blit.dstSubresource.baseArrayLayer = 0;
        blit.dstSubresource.layerCount = 1;

        cmdBuffer.blitImage(
            image, vk::ImageLayout::eTransferSrcOptimal,
            image, vk::ImageLayout::eTransferDstOptimal,
            blit, vk::Filter::eLinear
        );

        barrier.oldLayout = vk::ImageLayout::eTransferSrcOptimal;
        barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrier.srcAccessMask = vk::AccessFlagBits::eTransferRead;
        barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

        cmdBuffer.pipelineBarrier(
            vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {},
            {}, {}, barrier
        );

        if (mipWidth > 1) mipWidth /= 2;
        if (mipHeight > 1) mipHeight /= 2;
    }

    // 最後のミップレベルをシェーダー読み取り用に変更
    barrier.subresourceRange.baseMipLevel = mipLevels - 1;
    barrier.oldLayout = vk::ImageLayout::eTransferDstOptimal;
    barrier.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    barrier.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
    barrier.dstAccessMask = vk::AccessFlagBits::eShaderRead;

    cmdBuffer.pipelineBarrier(
    vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {},
    {}, {}, barrier);
}

// テクスチャの転送
void VulkanApp::copyTexture(vk::CommandBuffer commandBuffer, pl::Material &material, vk::Image image, vk::Buffer stagingBuffer, vk::DeviceSize offset, uint32_t width, uint32_t height) {
    // ミップレベルの計算
    uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(width, height)))) + 1;
    
    // イメージのレイアウト変更
    vk::ImageMemoryBarrier imageMemoryBarrier = createImageMemoryBarrier(
        image, 
        vk::ImageLayout::eUndefined, 
        vk::ImageLayout::eTransferDstOptimal, 
        {}, 
        vk::AccessFlagBits::eTransferWrite,
        vk::ImageAspectFlagBits::eColor
    );
    
    // バリアを全ミップレベルに適用
    imageMemoryBarrier.subresourceRange.levelCount = mipLevels;
    
    commandBuffer.pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe, 
        vk::PipelineStageFlagBits::eTransfer, 
        {}, {}, {}, imageMemoryBarrier
    );

    // バッファからイメージへのコピー
    vk::BufferImageCopy bufferImageCopy(
        offset, 0, 0,
        vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1),
        vk::Offset3D(0, 0, 0),
        vk::Extent3D(width, height, 1)
    );
    commandBuffer.copyBufferToImage(
        stagingBuffer, image, vk::ImageLayout::eTransferDstOptimal, 1, &bufferImageCopy
    );
    
    // ミップマップの生成
    generateMipmaps(commandBuffer, image, width, height, mipLevels);
    
}

void VulkanApp::transferTexture(const pl::ModelData &model) {
    std::vector<uint8_t> textureData;
    std::vector<vk::DescriptorPoolSize> descriptorPoolSizes;
    std::vector<vk::DescriptorImageInfo> imageInfos;

    // ダミーテクスチャデータ（各種テクスチャタイプに対応）
    uint8_t defaultBaseColorPixels[4] = { 255, 255, 255, 255 };        // 白色 (baseColor用)
    uint8_t defaultMetallicRoughnessPixels[4] = { 0, 128, 0, 255 };    // 非金属、中程度粗さ
    uint8_t defaultNormalPixels[4] = { 128, 128, 255, 255 };           // 上向き法線
    uint8_t defaultOcclusionPixels[4] = { 255, 255, 255, 255 };        // 遮蔽なし
    uint8_t defaultEmissivePixels[4] = { 0, 0, 0, 255 };               // 発光なし

    for (auto p_material : model.used_materials) {
        auto &material = *p_material;

        std::cout << "Color: " << material.baseColorTextureRaw.has_value() << std::endl;
        std::cout << "Metalic: " << material.metallicRoughnessTextureRaw.has_value() << std::endl;
        std::cout << "Normal: " << material.normalTextureRaw.has_value() << std::endl;
        std::cout << "Occlusion: " << material.occlusionTextureRaw.has_value() << std::endl;
        std::cout << "Emissive: " << material.emissiveTextureRaw.has_value() << std::endl;

        // 物理デバイスプロパティを一度だけ取得（全テクスチャで共通利用）
        vk::PhysicalDeviceProperties deviceProperties = physicalDevice.getProperties();
        float maxAnisotropy = deviceProperties.limits.maxSamplerAnisotropy;

        // === BaseColorテクスチャの処理 ===
        if (material.baseColorTextureRaw.has_value()) {
            // 通常のテクスチャ処理
            textureData.insert(textureData.end(), material.baseColorTextureRaw->data.begin(), material.baseColorTextureRaw->data.end());
            
            uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(
                std::max(material.baseColorTextureRaw->width, material.baseColorTextureRaw->height)))) + 1;

            if (material.baseColorTextureRaw->bits == 8) {
                material.baseColorTexture = createImage(material.baseColorTextureRaw->width, material.baseColorTextureRaw->height, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
                material.baseColorTextureView = createImageView(material.baseColorTexture.first.get(), vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
            } else if (material.baseColorTextureRaw->bits == 16) {
                material.baseColorTexture = createImage(material.baseColorTextureRaw->width, material.baseColorTextureRaw->height, vk::Format::eR16G16B16A16Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
                material.baseColorTextureView = createImageView(material.baseColorTexture.first.get(), vk::Format::eR16G16B16A16Unorm, vk::ImageAspectFlagBits::eColor);
            }

            vk::SamplerCreateInfo samplerCreateInfo(
                {},
                material.baseColorTextureRaw->toVkFilter(material.baseColorTextureRaw->magFilter),
                material.baseColorTextureRaw->toVkFilter(material.baseColorTextureRaw->minFilter),
                vk::SamplerMipmapMode::eLinear,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                0.0f,                         // mipLodBias
                VK_TRUE,                      // anisotropyEnable を TRUE に
                maxAnisotropy,                // maxAnisotropy を物理デバイスの上限値に
                VK_FALSE,
                vk::CompareOp::eAlways,
                0.0f,                         // minLod
                static_cast<float>(mipLevels), // maxLod - ミップレベル数を設定
                vk::BorderColor::eFloatOpaqueBlack,
                VK_FALSE);
            material.baseColorTextureSampler = device->createSamplerUnique(samplerCreateInfo);
        } else {
            // ダミーテクスチャを作成
            std::cout << "BaseColorテクスチャにダミーテクスチャを使用します" << std::endl;
            
            // データを追加
            textureData.insert(textureData.end(), defaultBaseColorPixels, defaultBaseColorPixels + 4);
            
            // 1x1ピクセルのテクスチャを作成
            material.baseColorTexture = createImage(1, 1, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, 
                                                   vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
            material.baseColorTextureView = createImageView(material.baseColorTexture.first.get(), 
                                                           vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
            
            // サンプラー作成
            vk::SamplerCreateInfo samplerCreateInfo(
                {},
                vk::Filter::eLinear,         // magFilter
                vk::Filter::eLinear,         // minFilter
                vk::SamplerMipmapMode::eLinear,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                0.0f,                        // mipLodBias
                VK_TRUE,                     // anisotropyEnable
                maxAnisotropy,               // maxAnisotropy
                VK_FALSE,
                vk::CompareOp::eAlways,
                0.0f,                        // minLod
                1.0f,                        // maxLod - ダミーテクスチャは1レベルのみ
                vk::BorderColor::eFloatOpaqueBlack,
                VK_FALSE);
            material.baseColorTextureSampler = device->createSamplerUnique(samplerCreateInfo);
        }
        
        // 共通処理：イメージ情報をデスクリプタに追加
        imageInfos.push_back(vk::DescriptorImageInfo(
            material.baseColorTextureSampler.get(),
            material.baseColorTextureView.get(),
            vk::ImageLayout::eShaderReadOnlyOptimal));

        // === MetallicRoughnessテクスチャの処理 ===
        if (material.metallicRoughnessTextureRaw.has_value()) {
            // 通常のテクスチャ処理
            textureData.insert(textureData.end(), material.metallicRoughnessTextureRaw->data.begin(), material.metallicRoughnessTextureRaw->data.end());
            
            uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(
                std::max(material.metallicRoughnessTextureRaw->width, material.metallicRoughnessTextureRaw->height)))) + 1;
            
            if (material.metallicRoughnessTextureRaw->bits == 8) {
                material.metallicRoughnessTexture = createImage(material.metallicRoughnessTextureRaw->width, material.metallicRoughnessTextureRaw->height, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
                material.metallicRoughnessTextureView = createImageView(material.metallicRoughnessTexture.first.get(), vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
            } else if (material.metallicRoughnessTextureRaw->bits == 16) {
                material.metallicRoughnessTexture = createImage(material.metallicRoughnessTextureRaw->width, material.metallicRoughnessTextureRaw->height, vk::Format::eR16G16B16A16Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
                material.metallicRoughnessTextureView = createImageView(material.metallicRoughnessTexture.first.get(), vk::Format::eR16G16B16A16Unorm, vk::ImageAspectFlagBits::eColor);
            }

            vk::SamplerCreateInfo samplerCreateInfo(
                {},
                material.metallicRoughnessTextureRaw->toVkFilter(material.metallicRoughnessTextureRaw->magFilter),
                material.metallicRoughnessTextureRaw->toVkFilter(material.metallicRoughnessTextureRaw->minFilter),
                vk::SamplerMipmapMode::eLinear,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                0.0f,                         // mipLodBias
                VK_TRUE,                      // anisotropyEnable
                maxAnisotropy,                // maxAnisotropy
                VK_FALSE,
                vk::CompareOp::eAlways,
                0.0f,                         // minLod
                static_cast<float>(mipLevels), // maxLod
                vk::BorderColor::eFloatOpaqueBlack,
                VK_FALSE);
            material.metallicRoughnessTextureSampler = device->createSamplerUnique(samplerCreateInfo);
        } else {
            // ダミーテクスチャを作成
            std::cout << "MetallicRoughnessテクスチャにダミーテクスチャを使用します" << std::endl;
            
            // データを追加
            textureData.insert(textureData.end(), defaultMetallicRoughnessPixels, defaultMetallicRoughnessPixels + 4);
            
            // 1x1ピクセルのテクスチャを作成
            material.metallicRoughnessTexture = createImage(1, 1, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, 
                                                           vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
            material.metallicRoughnessTextureView = createImageView(material.metallicRoughnessTexture.first.get(), 
                                                                   vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
            
            // サンプラー作成
            vk::SamplerCreateInfo samplerCreateInfo(
                {},
                vk::Filter::eLinear,         // magFilter
                vk::Filter::eLinear,         // minFilter
                vk::SamplerMipmapMode::eLinear,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                0.0f,                        // mipLodBias
                VK_TRUE,                     // anisotropyEnable
                maxAnisotropy,               // maxAnisotropy
                VK_FALSE,
                vk::CompareOp::eAlways,
                0.0f,                        // minLod
                1.0f,                        // maxLod - ダミーテクスチャは1レベルのみ
                vk::BorderColor::eFloatOpaqueBlack,
                VK_FALSE);
            material.metallicRoughnessTextureSampler = device->createSamplerUnique(samplerCreateInfo);
        }
        
        // 共通処理：イメージ情報をデスクリプタに追加
        imageInfos.push_back(vk::DescriptorImageInfo(
            material.metallicRoughnessTextureSampler.get(),
            material.metallicRoughnessTextureView.get(),
            vk::ImageLayout::eShaderReadOnlyOptimal));

        // === Normalテクスチャの処理 ===
        if (material.normalTextureRaw.has_value()) {
            // 通常のテクスチャ処理
            textureData.insert(textureData.end(), material.normalTextureRaw->data.begin(), material.normalTextureRaw->data.end());
            
            uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(
                std::max(material.normalTextureRaw->width, material.normalTextureRaw->height)))) + 1;
            
            if (material.normalTextureRaw->bits == 8) {
                material.normalTexture = createImage(material.normalTextureRaw->width, material.normalTextureRaw->height, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
                material.normalTextureView = createImageView(material.normalTexture.first.get(), vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
            } else if (material.normalTextureRaw->bits == 16) {
                material.normalTexture = createImage(material.normalTextureRaw->width, material.normalTextureRaw->height, vk::Format::eR16G16B16A16Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
                material.normalTextureView = createImageView(material.normalTexture.first.get(), vk::Format::eR16G16B16A16Unorm, vk::ImageAspectFlagBits::eColor);
            }

            vk::SamplerCreateInfo samplerCreateInfo(
                {},
                material.normalTextureRaw->toVkFilter(material.normalTextureRaw->magFilter),
                material.normalTextureRaw->toVkFilter(material.normalTextureRaw->minFilter),
                vk::SamplerMipmapMode::eLinear,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                0.0f,                         // mipLodBias
                VK_TRUE,                      // anisotropyEnable
                maxAnisotropy,                // maxAnisotropy
                VK_FALSE,
                vk::CompareOp::eAlways,
                0.0f,                         // minLod
                static_cast<float>(mipLevels), // maxLod
                vk::BorderColor::eFloatOpaqueBlack,
                VK_FALSE);
            material.normalTextureSampler = device->createSamplerUnique(samplerCreateInfo);
        } else {
            // ダミーテクスチャを作成
            std::cout << "Normalテクスチャにダミーテクスチャを使用します" << std::endl;
            
            // データを追加
            textureData.insert(textureData.end(), defaultNormalPixels, defaultNormalPixels + 4);
            
            // 1x1ピクセルのテクスチャを作成
            material.normalTexture = createImage(1, 1, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, 
                                               vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
            material.normalTextureView = createImageView(material.normalTexture.first.get(), 
                                                       vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
            
            // サンプラー作成
            vk::SamplerCreateInfo samplerCreateInfo(
                {},
                vk::Filter::eLinear,         // magFilter
                vk::Filter::eLinear,         // minFilter
                vk::SamplerMipmapMode::eLinear,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                0.0f,                        // mipLodBias
                VK_TRUE,                     // anisotropyEnable
                maxAnisotropy,               // maxAnisotropy
                VK_FALSE,
                vk::CompareOp::eAlways,
                0.0f,                        // minLod
                1.0f,                        // maxLod - ダミーテクスチャは1レベルのみ
                vk::BorderColor::eFloatOpaqueBlack,
                VK_FALSE);
            material.normalTextureSampler = device->createSamplerUnique(samplerCreateInfo);
        }
        
        // 共通処理：イメージ情報をデスクリプタに追加
        imageInfos.push_back(vk::DescriptorImageInfo(
            material.normalTextureSampler.get(),
            material.normalTextureView.get(),
            vk::ImageLayout::eShaderReadOnlyOptimal));

        // === Occlusionテクスチャの処理 ===
        if (material.occlusionTextureRaw.has_value()) {
            // 通常のテクスチャ処理
            textureData.insert(textureData.end(), material.occlusionTextureRaw->data.begin(), material.occlusionTextureRaw->data.end());
            
            uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(
                std::max(material.occlusionTextureRaw->width, material.occlusionTextureRaw->height)))) + 1;
            
            if (material.occlusionTextureRaw->bits == 8) {
                material.occlusionTexture = createImage(material.occlusionTextureRaw->width, material.occlusionTextureRaw->height, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
                material.occlusionTextureView = createImageView(material.occlusionTexture.first.get(), vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
            } else if (material.occlusionTextureRaw->bits == 16) {
                material.occlusionTexture = createImage(material.occlusionTextureRaw->width, material.occlusionTextureRaw->height, vk::Format::eR16G16B16A16Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
                material.occlusionTextureView = createImageView(material.occlusionTexture.first.get(), vk::Format::eR16G16B16A16Unorm, vk::ImageAspectFlagBits::eColor);
            }

            vk::SamplerCreateInfo samplerCreateInfo(
                {},
                material.occlusionTextureRaw->toVkFilter(material.occlusionTextureRaw->magFilter),
                material.occlusionTextureRaw->toVkFilter(material.occlusionTextureRaw->minFilter),
                vk::SamplerMipmapMode::eLinear,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                0.0f,                         // mipLodBias
                VK_TRUE,                      // anisotropyEnable
                maxAnisotropy,                // maxAnisotropy
                VK_FALSE,
                vk::CompareOp::eAlways,
                0.0f,                         // minLod
                static_cast<float>(mipLevels), // maxLod
                vk::BorderColor::eFloatOpaqueBlack,
                VK_FALSE);
            material.occlusionTextureSampler = device->createSamplerUnique(samplerCreateInfo);
        } else {
            // ダミーテクスチャを作成
            std::cout << "Occlusionテクスチャにダミーテクスチャを使用します" << std::endl;
            
            // データを追加
            textureData.insert(textureData.end(), defaultOcclusionPixels, defaultOcclusionPixels + 4);
            
            // 1x1ピクセルのテクスチャを作成
            material.occlusionTexture = createImage(1, 1, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, 
                                                  vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
            material.occlusionTextureView = createImageView(material.occlusionTexture.first.get(), 
                                                          vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
            
            // サンプラー作成
            vk::SamplerCreateInfo samplerCreateInfo(
                {},
                vk::Filter::eLinear,         // magFilter
                vk::Filter::eLinear,         // minFilter
                vk::SamplerMipmapMode::eLinear,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                0.0f,                        // mipLodBias
                VK_TRUE,                     // anisotropyEnable
                maxAnisotropy,               // maxAnisotropy
                VK_FALSE,
                vk::CompareOp::eAlways,
                0.0f,                        // minLod
                1.0f,                        // maxLod - ダミーテクスチャは1レベルのみ
                vk::BorderColor::eFloatOpaqueBlack,
                VK_FALSE);
            material.occlusionTextureSampler = device->createSamplerUnique(samplerCreateInfo);
        }
        
        // 共通処理：イメージ情報をデスクリプタに追加
        imageInfos.push_back(vk::DescriptorImageInfo(
            material.occlusionTextureSampler.get(),
            material.occlusionTextureView.get(),
            vk::ImageLayout::eShaderReadOnlyOptimal));

        // === Emissiveテクスチャの処理 ===
        if (material.emissiveTextureRaw.has_value()) {
            // 通常のテクスチャ処理
            textureData.insert(textureData.end(), material.emissiveTextureRaw->data.begin(), material.emissiveTextureRaw->data.end());
            
            uint32_t mipLevels = static_cast<uint32_t>(std::floor(std::log2(
                std::max(material.emissiveTextureRaw->width, material.emissiveTextureRaw->height)))) + 1;
            
            if (material.emissiveTextureRaw->bits == 8) {
                material.emissiveTexture = createImage(material.emissiveTextureRaw->width, material.emissiveTextureRaw->height, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
                material.emissiveTextureView = createImageView(material.emissiveTexture.first.get(), vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
            } else if (material.emissiveTextureRaw->bits == 16) {
                material.emissiveTexture = createImage(material.emissiveTextureRaw->width, material.emissiveTextureRaw->height, vk::Format::eR16G16B16A16Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
                material.emissiveTextureView = createImageView(material.emissiveTexture.first.get(), vk::Format::eR16G16B16A16Unorm, vk::ImageAspectFlagBits::eColor);
            }

            vk::SamplerCreateInfo samplerCreateInfo(
                {},
                material.emissiveTextureRaw->toVkFilter(material.emissiveTextureRaw->magFilter),
                material.emissiveTextureRaw->toVkFilter(material.emissiveTextureRaw->minFilter),
                vk::SamplerMipmapMode::eLinear,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                0.0f,                         // mipLodBias
                VK_TRUE,                      // anisotropyEnable
                maxAnisotropy,                // maxAnisotropy
                VK_FALSE,
                vk::CompareOp::eAlways,
                0.0f,                         // minLod
                static_cast<float>(mipLevels), // maxLod
                vk::BorderColor::eFloatOpaqueBlack,
                VK_FALSE);
            material.emissiveTextureSampler = device->createSamplerUnique(samplerCreateInfo);
        } else {
            // ダミーテクスチャを作成
            std::cout << "Emissiveテクスチャにダミーテクスチャを使用します" << std::endl;
            
            // データを追加
            textureData.insert(textureData.end(), defaultEmissivePixels, defaultEmissivePixels + 4);
            
            // 1x1ピクセルのテクスチャを作成
            material.emissiveTexture = createImage(1, 1, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, 
                                                 vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst);
            material.emissiveTextureView = createImageView(material.emissiveTexture.first.get(), 
                                                         vk::Format::eR8G8B8A8Unorm, vk::ImageAspectFlagBits::eColor);
            
            // サンプラー作成
            vk::SamplerCreateInfo samplerCreateInfo(
                {},
                vk::Filter::eLinear,         // magFilter
                vk::Filter::eLinear,         // minFilter
                vk::SamplerMipmapMode::eLinear,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                vk::SamplerAddressMode::eRepeat,
                0.0f,                        // mipLodBias
                VK_TRUE,                     // anisotropyEnable
                maxAnisotropy,               // maxAnisotropy
                VK_FALSE,
                vk::CompareOp::eAlways,
                0.0f,                        // minLod
                1.0f,                        // maxLod - ダミーテクスチャは1レベルのみ
                vk::BorderColor::eFloatOpaqueBlack,
                VK_FALSE);
            material.emissiveTextureSampler = device->createSamplerUnique(samplerCreateInfo);
        }
        
        // 共通処理：イメージ情報をデスクリプタに追加
        imageInfos.push_back(vk::DescriptorImageInfo(
            material.emissiveTextureSampler.get(),
            material.emissiveTextureView.get(),
            vk::ImageLayout::eShaderReadOnlyOptimal));

        // デスクリプタプールの作成と割り当て
        descriptorPoolSizes.push_back(vk::DescriptorPoolSize(vk::DescriptorType::eCombinedImageSampler, 5));

        vk::DescriptorPoolCreateInfo descriptorPoolCreateInfo(vk::DescriptorPoolCreateFlagBits::eFreeDescriptorSet, 1, descriptorPoolSizes.size(), descriptorPoolSizes.data());
        material.descPool = device->createDescriptorPoolUnique(descriptorPoolCreateInfo);

        vk::DescriptorSetAllocateInfo descriptorSetAllocateInfo(material.descPool.get(), 1, &descriptorSetLayout.get());
        auto tmpDescSet = device->allocateDescriptorSetsUnique(descriptorSetAllocateInfo);
        material.descSet = std::move(tmpDescSet[0]);

        vk::WriteDescriptorSet writeDescriptorSet(
            material.descSet.get(),
            0,
            0,
            imageInfos.size(),
            vk::DescriptorType::eCombinedImageSampler,
            imageInfos.data(),
            nullptr,
            nullptr);

        device->updateDescriptorSets(1, &writeDescriptorSet, 0, nullptr);

        // テクスチャデータがある場合のみステージングバッファを作成
        if (!textureData.empty()) {
            std::cout << "Texture Data Size: " << textureData.size() * sizeof(uint8_t) << std::endl;
            auto stagingBuffer = createBuffer({}, textureData.size() * sizeof(uint8_t), vk::BufferUsageFlagBits::eTransferSrc, vk::MemoryPropertyFlagBits::eHostVisible, vk::SharingMode::eExclusive);
            void *stagingBufferMem = device->mapMemory(stagingBuffer.second.get(), 0, textureData.size() * sizeof(uint8_t));
            std::memcpy(stagingBufferMem, textureData.data(), textureData.size() * sizeof(uint8_t));
            device->unmapMemory(stagingBuffer.second.get());

            // コマンドバッファの準備
            std::pair<vk::UniqueCommandPool, std::vector<vk::UniqueCommandBuffer>> commandBuffers = createCommandBuffers(vk::CommandPoolCreateFlagBits::eResetCommandBuffer, queueCreateInfos[0], 1);
            vk::CommandBufferBeginInfo beginInfo;
            beginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;
            commandBuffers.second[0]->begin(beginInfo);

            vk::DeviceSize offset = 0;

            // 各テクスチャのコピーとミップマップ生成
            if (material.baseColorTexture.first) {
                int width = material.baseColorTextureRaw.has_value() ? material.baseColorTextureRaw->width : 1;
                int height = material.baseColorTextureRaw.has_value() ? material.baseColorTextureRaw->height : 1;
                copyTexture(commandBuffers.second[0].get(), material, material.baseColorTexture.first.get(), 
                           stagingBuffer.first.get(), offset, width, height);
                offset += material.baseColorTextureRaw.has_value() ? 
                         material.baseColorTextureRaw->data.size() * sizeof(uint8_t) : 4;
            }
            
            if (material.metallicRoughnessTexture.first) {
                int width = material.metallicRoughnessTextureRaw.has_value() ? material.metallicRoughnessTextureRaw->width : 1;
                int height = material.metallicRoughnessTextureRaw.has_value() ? material.metallicRoughnessTextureRaw->height : 1;
                copyTexture(commandBuffers.second[0].get(), material, material.metallicRoughnessTexture.first.get(), 
                           stagingBuffer.first.get(), offset, width, height);
                offset += material.metallicRoughnessTextureRaw.has_value() ? 
                         material.metallicRoughnessTextureRaw->data.size() * sizeof(uint8_t) : 4;
            }
            
            if (material.normalTexture.first) {
                int width = material.normalTextureRaw.has_value() ? material.normalTextureRaw->width : 1;
                int height = material.normalTextureRaw.has_value() ? material.normalTextureRaw->height : 1;
                copyTexture(commandBuffers.second[0].get(), material, material.normalTexture.first.get(), 
                           stagingBuffer.first.get(), offset, width, height);
                offset += material.normalTextureRaw.has_value() ? 
                         material.normalTextureRaw->data.size() * sizeof(uint8_t) : 4;
            }
            
            if (material.occlusionTexture.first) {
                int width = material.occlusionTextureRaw.has_value() ? material.occlusionTextureRaw->width : 1;
                int height = material.occlusionTextureRaw.has_value() ? material.occlusionTextureRaw->height : 1;
                copyTexture(commandBuffers.second[0].get(), material, material.occlusionTexture.first.get(), 
                           stagingBuffer.first.get(), offset, width, height);
                offset += material.occlusionTextureRaw.has_value() ? 
                         material.occlusionTextureRaw->data.size() * sizeof(uint8_t) : 4;
            }
            
            if (material.emissiveTexture.first) {
                int width = material.emissiveTextureRaw.has_value() ? material.emissiveTextureRaw->width : 1;
                int height = material.emissiveTextureRaw.has_value() ? material.emissiveTextureRaw->height : 1;
                copyTexture(commandBuffers.second[0].get(), material, material.emissiveTexture.first.get(), 
                           stagingBuffer.first.get(), offset, width, height);
                offset += material.emissiveTextureRaw.has_value() ? 
                         material.emissiveTextureRaw->data.size() * sizeof(uint8_t) : 4;
            }

            // コマンド実行
            commandBuffers.second[0]->end();

            vk::CommandBuffer submitCmdBuf[1] = {commandBuffers.second[0].get()};
            vk::SubmitInfo submitInfo(0, nullptr, nullptr, 1, submitCmdBuf, 0, nullptr);
            graphicsQueues[0].submit(submitInfo, nullptr);
            graphicsQueues[0].waitIdle();
        }
        
        // 転送後にテクスチャデータをクリア（オプション）
        imageInfos.clear();
        textureData.clear();
    }
}

// オブジェクトのダンプ
void dumpMesh(const Mesh &mesh) {
    std::cout << "start dumpMesh" << std::endl;
    std::cout << "Mesh:" << std::endl;
    for (const auto &primitive : mesh.primitives) {
        std::cout << "  Primitive:" << std::endl;
        std::cout << "    Vertices:" << std::endl;
        for (const auto &vertex : primitive.vertices) {
            std::cout << "      Position: (" << vertex.position.x << ", " << vertex.position.y << ", " << vertex.position.z << ")" << std::endl;
            std::cout << "      Normal: (" << vertex.normal.x << ", " << vertex.normal.y << ", " << vertex.normal.z << ")" << std::endl;
            std::cout << "      TexCoord: (" << vertex.texCoord.x << ", " << vertex.texCoord.y << ")" << std::endl;
        }
        std::cout << "    Indices:" << std::endl;
        for (const auto &index : primitive.indices) {
            std::cout << "      " << index << std::endl;
        }
    }
}

void dumpObject(const Object &object) {
    std::cout << "start dumpObject" << std::endl;
    std::cout << "Object:" << std::endl;
    std::cout << "  Mesh: " << object.mesh << std::endl;
    dumpMesh(*object.mesh);
    std::cout << "  Transform:" << std::endl;
    std::cout << "    Translation: (" << object.transform.translation.x << ", " << object.transform.translation.y << ", " << object.transform.translation.z << ")" << std::endl;
    std::cout << "    Rotation: (" << object.transform.rotation.x << ", " << object.transform.rotation.y << ", " << object.transform.rotation.z << ", " << object.transform.rotation.w << ")" << std::endl;
    std::cout << "    Scale: (" << object.transform.scale.x << ", " << object.transform.scale.y << ", " << object.transform.scale.z << ")" << std::endl;
}

// メモリタイプの検索
uint32_t VulkanApp::findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties) {
    vk::PhysicalDeviceMemoryProperties memProperties = physicalDevice.getMemoryProperties();
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    throw std::runtime_error("適切なメモリタイプが見つかりませんでした");
}

vk::UniqueShaderModule VulkanApp::createShaderModule(std::string filename) {
    size_t spvFileSz = std::filesystem::file_size(filename);

    std::ifstream spvFile(filename, std::ios::binary);
    if (!spvFile.is_open()) {
        throw std::runtime_error("シェーダーファイルを開けませんでした: " + filename);
    }

    std::vector<char> spvFileData(spvFileSz);
    spvFile.read(spvFileData.data(), spvFileSz);
    if (!spvFile) {
        throw std::runtime_error("シェーダーファイルの読み込みに失敗しました: " + filename);
    }

    vk::ShaderModuleCreateInfo shaderCreateInfo{};
    shaderCreateInfo.codeSize = spvFileSz;
    shaderCreateInfo.pCode = reinterpret_cast<const uint32_t *>(spvFileData.data());

    return device->createShaderModuleUnique(shaderCreateInfo);
}

// スワップチェーンの作成
void VulkanApp::createSwapchain() {
    vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface.get());
    std::vector<vk::SurfaceFormatKHR> surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface.get());
    std::vector<vk::PresentModeKHR> surfacePresentModes = physicalDevice.getSurfacePresentModesKHR(surface.get());

    vk::SurfaceFormatKHR swapchainFormat = surfaceFormats[0];
    vk::PresentModeKHR presentMode = surfacePresentModes[0];

    vk::SwapchainCreateInfoKHR swapchainCreateInfo(
        {},
        surface.get(),
        surfaceCapabilities.minImageCount + 1,
        swapchainFormat.format,
        swapchainFormat.colorSpace,
        surfaceCapabilities.currentExtent,
        1,
        vk::ImageUsageFlagBits::eColorAttachment,
        vk::SharingMode::eExclusive,
        0,
        nullptr,
        surfaceCapabilities.currentTransform,
        vk::CompositeAlphaFlagBitsKHR::eOpaque,
        presentMode,
        VK_TRUE,
        nullptr);
    swapchain = device->createSwapchainKHRUnique(swapchainCreateInfo);
    swapchainImages = device->getSwapchainImagesKHR(swapchain.get());

    for (uint32_t i = 0; i < swapchainImages.size(); i++) {
        vk::ImageViewCreateInfo imageViewCreateInfo(
            {},
            swapchainImages[i],
            vk::ImageViewType::e2D,
            swapchainFormat.format,
            vk::ComponentMapping(),
            vk::ImageSubresourceRange(vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1));
        swapchainImageViews.push_back(device->createImageViewUnique(imageViewCreateInfo));
    }

    for (uint32_t i = 0; i < swapchainImages.size(); i++) {
        positionImage.push_back(createImage(screenWidth, screenHeight, vk::Format::eR32G32B32A32Sfloat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::SampleCountFlagBits::e4));
        positionImageView.push_back(createImageView(positionImage[i].first.get(), vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor));
        
        normalImage.push_back(createImage(screenWidth, screenHeight, vk::Format::eR32G32B32A32Sfloat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::SampleCountFlagBits::e4));
        normalImageView.push_back(createImageView(normalImage[i].first.get(), vk::Format::eR32G32B32A32Sfloat, vk::ImageAspectFlagBits::eColor));
        
        // スワップチェーンと同じフォーマットを使用（ここを修正）
        albedoImage.push_back(createImage(screenWidth, screenHeight, swapchainFormat.format, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled, vk::SampleCountFlagBits::e4));
        albedoImageView.push_back(createImageView(albedoImage[i].first.get(), swapchainFormat.format, vk::ImageAspectFlagBits::eColor));
        
        depthImage.push_back(createImage(screenWidth, screenHeight, vk::Format::eD32Sfloat, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eDepthStencilAttachment, vk::SampleCountFlagBits::e4));
        depthImageView.push_back(createImageView(depthImage[i].first.get(), vk::Format::eD32Sfloat, vk::ImageAspectFlagBits::eDepth));
    }
}

std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory> VulkanApp::createBuffer(
    vk::BufferCreateFlags flags,
    vk::DeviceSize size,
    vk::BufferUsageFlags usage,
    vk::MemoryPropertyFlags properties,
    vk::SharingMode sharingMode) {

    vk::BufferCreateInfo bufferCreateInfo(
        flags,
        size,
        usage,
        sharingMode,
        0,
        nullptr);

    vk::UniqueBuffer buffer = device->createBufferUnique(bufferCreateInfo);

    vk::MemoryRequirements memRequirements = device->getBufferMemoryRequirements(buffer.get());

    vk::MemoryAllocateInfo allocInfo;
    allocInfo.allocationSize = memRequirements.size;

    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

    vk::UniqueDeviceMemory bufferMemory = device->allocateMemoryUnique(allocInfo);
    device->bindBufferMemory(buffer.get(), bufferMemory.get(), 0);

    return std::make_pair(std::move(buffer), std::move(bufferMemory));
}

void VulkanApp::drawGBuffer(uint32_t objectIndex) {
    // フェンスをリセットしてスワップチェーンイメージを取得
    device->resetFences({swapchainImgFence.get()});
    vk::ResultValue acquireResult = device->acquireNextImageKHR(swapchain.get(), UINT64_MAX, {}, swapchainImgFence.get());

    if (acquireResult.result != vk::Result::eSuccess && acquireResult.result != vk::Result::eSuboptimalKHR) {
        throw std::runtime_error("スワップチェーンイメージの取得に失敗しました: " + vk::to_string(acquireResult.result));
    }
    uint32_t imageIndex = acquireResult.value;

    if (auto result = device->waitForFences({swapchainImgFence.get()}, VK_TRUE, UINT64_MAX); result != vk::Result::eSuccess) {
        throw std::runtime_error("スワップチェーンイメージの取得がタイムアウトしました: " + vk::to_string(acquireResult.result));
    }

    // コマンドバッファの準備
    graphicCommandBuffers.at(0)->reset();
    vk::CommandBufferBeginInfo beginInfo;
    graphicCommandBuffers.at(0)->begin(beginInfo);

    // ===== レンダリング前にバリアを適用 =====
    
    // 深度イメージの初期化バリア
    vk::ImageMemoryBarrier depthMemoryBarrier = createImageMemoryBarrier(
        depthImage.at(imageIndex).first.get(), 
        vk::ImageLayout::eUndefined, 
        vk::ImageLayout::eDepthStencilAttachmentOptimal, 
        vk::AccessFlagBits::eNone, 
        vk::AccessFlagBits::eDepthStencilAttachmentWrite, 
        vk::ImageAspectFlagBits::eDepth
    );
    
    // MSAAカラーターゲットの初期化バリア
    vk::ImageMemoryBarrier albedoImageBarrier = createImageMemoryBarrier(
        albedoImage.at(imageIndex).first.get(), 
        vk::ImageLayout::eUndefined, 
        vk::ImageLayout::eColorAttachmentOptimal, 
        vk::AccessFlagBits::eNone, 
        vk::AccessFlagBits::eColorAttachmentWrite, 
        vk::ImageAspectFlagBits::eColor
    );
    
    // スワップチェーンをリゾルブ先として準備
    vk::ImageMemoryBarrier swapchainBarrier = createImageMemoryBarrier(
        swapchainImages.at(imageIndex), 
        vk::ImageLayout::eUndefined, 
        vk::ImageLayout::eColorAttachmentOptimal, 
        vk::AccessFlagBits::eNone, 
        vk::AccessFlagBits::eColorAttachmentWrite, 
        vk::ImageAspectFlagBits::eColor
    );

    // すべてのバリアを一度に適用
    std::array<vk::ImageMemoryBarrier, 3> initialBarriers = {
        depthMemoryBarrier,
        albedoImageBarrier,
        swapchainBarrier
    };
    
    graphicCommandBuffers.at(0)->pipelineBarrier(
        vk::PipelineStageFlagBits::eTopOfPipe,
        vk::PipelineStageFlagBits::eEarlyFragmentTests | vk::PipelineStageFlagBits::eColorAttachmentOutput,
        {},
        {},
        {},
        initialBarriers
    );

    // ビューポート設定
    graphicCommandBuffers.at(0)->setViewport(0, {viewport3d});

    // ===== MSAAレンダリング設定 =====
    
    // MSAAカラーアタッチメントにリゾルブ設定 (平均化でスワップチェーンに解決)
    std::vector<vk::RenderingAttachmentInfo> colorAttachments = {
        vk::RenderingAttachmentInfo(
            albedoImageView.at(imageIndex).get(),          // MSAA対応のカラーアタッチメント
            vk::ImageLayout::eColorAttachmentOptimal,      // imageLayout
            vk::ResolveModeFlagBits::eAverage,             // リゾルブモード
            swapchainImageViews.at(imageIndex).get(),      // リゾルブ先 (1サンプル)
            vk::ImageLayout::eColorAttachmentOptimal,      // リゾルブ先レイアウト
            vk::AttachmentLoadOp::eClear,                  // loadOp
            vk::AttachmentStoreOp::eStore,                 // storeOp 
            vk::ClearValue(vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f})) // 黒で初期化
        )
    };

    // MSAA対応の深度アタッチメント
    std::vector<vk::RenderingAttachmentInfo> depthAttachments = {
        vk::RenderingAttachmentInfo(
            depthImageView.at(imageIndex).get(),             // imageView
            vk::ImageLayout::eDepthStencilAttachmentOptimal, // imageLayout
            vk::ResolveModeFlagBits::eNone,                  // resolveMode
            {},                                              // resolveImageView
            vk::ImageLayout::eUndefined,                     // resolveImageLayout
            vk::AttachmentLoadOp::eClear,                    // loadOp
            vk::AttachmentStoreOp::eStore,                   // storeOp
            vk::ClearValue{1.0f}                             // clearValue
        )
    };

    vk::RenderingInfo renderingInfo(
        {},                                              // flags
        vk::Rect2D({0, 0}, {screenWidth, screenHeight}), // renderArea
        1,                                               // layerCount
        0,                                               // viewMask
        colorAttachments.size(),                         // colorAttachmentCount
        colorAttachments.data(),                         // pColorAttachments
        depthAttachments.data(),                         // pDepthAttachment
        nullptr                                          // pStencilAttachment
    );

    // ===== 3Dシーンのレンダリング =====
    graphicCommandBuffers.at(0)->beginRendering(renderingInfo);

    graphicCommandBuffers.at(0)->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());
    graphicCommandBuffers.at(0)->pushConstants(
        pipelineLayout.get(), 
        vk::ShaderStageFlagBits::eVertex | vk::ShaderStageFlagBits::eGeometry | vk::ShaderStageFlagBits::eFragment, 
        0, 
        sizeof(vpMatrix), 
        &vpMatrix
    );

    // モデル描画
    for (auto &model : modelDb.models) {
        if (model.instanceAttributes.empty())
            continue;
        
        auto &[instanceBuf, instanceBufMem] = modelDb.instanceBuffers.at(model.modelIndex);

        // インスタンスデータの転送
        const auto instanceBufSize = model.instanceAttributes.size() * sizeof(InstanceAttribute);
        const auto pInstanceBuf = device->mapMemory(instanceBufMem.get(), 0, instanceBufSize, {});
        std::memcpy(pInstanceBuf, model.instanceAttributes.data(), instanceBufSize);
        
        vk::MappedMemoryRange range;
        range.memory = instanceBufMem.get();
        range.offset = 0;
        range.size = instanceBufSize;
        device->flushMappedMemoryRanges({range});
        device->unmapMemory(instanceBufMem.get());

        // バインドと描画
        graphicCommandBuffers.at(0)->bindVertexBuffers(
            0, 
            {modelDb.vertexBuffers.at(model.modelIndex).first.get(), instanceBuf.get()}, 
            {0, 0}
        );
        graphicCommandBuffers.at(0)->bindIndexBuffer(
            modelDb.indexBuffers.at(model.modelIndex).first.get(), 
            0, 
            vk::IndexType::eUint32
        );
        graphicCommandBuffers.at(0)->bindDescriptorSets(
            vk::PipelineBindPoint::eGraphics, 
            pipelineLayout.get(), 
            0, 
            {model.meshes[0].primitives[0].material->descSet.get()}, 
            {}
        );

        // インデックスカウント計算
        uint32_t indexCount = 0;
        for (auto &mesh : model.meshes) {
            for (auto &primitive : mesh.primitives) {
                indexCount += primitive.indices.size();
            }
        }
        
        graphicCommandBuffers.at(0)->drawIndexed(
            indexCount, 
            model.instanceAttributes.size(), 
            0, 0, 0
        );

        model.instanceAttributes.clear();
    }
    
    graphicCommandBuffers.at(0)->endRendering();

    // ===== UI描画（MSAAなし、直接スワップチェーンに描画） =====
    {
        std::vector<vk::RenderingAttachmentInfo> uiColorAttachments = {
            vk::RenderingAttachmentInfo(
                swapchainImageViews.at(imageIndex).get(), // imageView
                vk::ImageLayout::eColorAttachmentOptimal, // imageLayout
                vk::ResolveModeFlagBits::eNone,           // resolveMode
                {},                                       // resolveImageView
                vk::ImageLayout::eUndefined,              // resolveImageLayout
                vk::AttachmentLoadOp::eLoad,              // loadOp - 前の内容を維持
                vk::AttachmentStoreOp::eStore,            // storeOp
                vk::ClearValue{}                          // clearValue
            )
        };

        vk::RenderingInfo uiRenderingInfo(
            {},                                              // flags
            vk::Rect2D({0, 0}, {screenWidth, screenHeight}), // renderArea
            1,                                               // layerCount
            0,                                               // viewMask
            uiColorAttachments.size(),                       // colorAttachmentCount
            uiColorAttachments.data(),                       // pColorAttachments
            nullptr,                                         // pDepthAttachment - UI描画では不要
            nullptr                                          // pStencilAttachment
        );
        
        graphicCommandBuffers.at(0)->beginRendering(uiRenderingInfo);
        
        // UI描画
        graphicCommandBuffers.at(0)->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline2D.get());
        for (const auto &drawInfo : uiImageDrawInfos) {
            graphicCommandBuffers.at(0)->bindDescriptorSets(
                vk::PipelineBindPoint::eGraphics, 
                pipeline2DLayout.get(), 
                0, 
                {drawInfo.data->descSet.get()}, 
                {}
            );
            graphicCommandBuffers.at(0)->pushConstants(
                pipeline2DLayout.get(), 
                vk::ShaderStageFlagBits::eVertex, 
                0, 
                sizeof(Render2DPushConstantInfo), 
                &drawInfo.push
            );
            graphicCommandBuffers.at(0)->draw(6, 1, 0, 0);
        }
        uiImageDrawInfos.clear();
        
        graphicCommandBuffers.at(0)->endRendering();
    }

    // ===== レンダリング後のバリア =====
    // スワップチェーンイメージをプレゼント用に遷移
    vk::ImageMemoryBarrier presentBarrier = createImageMemoryBarrier(
        swapchainImages.at(imageIndex), 
        vk::ImageLayout::eColorAttachmentOptimal, 
        vk::ImageLayout::ePresentSrcKHR, 
        vk::AccessFlagBits::eColorAttachmentWrite, 
        vk::AccessFlagBits::eMemoryRead,
        vk::ImageAspectFlagBits::eColor
    );

    graphicCommandBuffers.at(0)->pipelineBarrier(
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eBottomOfPipe,
        {},
        {},
        {},
        presentBarrier
    );

    // コマンドバッファ完了
    graphicCommandBuffers.at(0)->end();

    // ===== コマンドバッファの実行とプレゼント =====
    vk::CommandBuffer submitCommandBuffer = graphicCommandBuffers.at(0).get();
    vk::SubmitInfo submitInfo({}, {}, {submitCommandBuffer}, {});

    graphicsQueues.at(0).submit({submitInfo});

    // プレゼント設定
    vk::PresentInfoKHR presentInfo;
    auto presentSwapchains = {swapchain.get()};
    auto imgIndices = {imageIndex};
    presentInfo.swapchainCount = presentSwapchains.size();
    presentInfo.pSwapchains = presentSwapchains.begin();
    presentInfo.pImageIndices = imgIndices.begin();

    // プレゼント実行
    graphicsQueues.at(0).presentKHR(presentInfo);
    graphicsQueues.at(0).waitIdle();
}

void VulkanApp::drawFrame() {
    drawGBuffer(0);
}

void VulkanApp::setCamera(glm::vec3 pos, glm::vec3 dir, glm::vec3 up) {
    vpMatrix.view = glm::lookAt(pos, pos + dir, up);
}

void VulkanApp::setProjection(float horizontalAngle, float near, float far) {
    vpMatrix.projection = glm::perspective(glm::radians(horizontalAngle), static_cast<float>(viewport3d.width) / static_cast<float>(viewport3d.height), near, far);
}

void VulkanApp::setLine(glm::vec4 color, float width) {
    vpMatrix.lineColor = color;
    vpMatrix.lineWidth = width;
}

void VulkanApp::setSubColor(glm::vec4 color) {
    currentSubColor = color;
}

void VulkanApp::drawModel(const Model &model, glm::mat4x4 modelMatrix) {
    InstanceAttribute attr;
    attr.model = modelMatrix;
    attr.lineColor = vpMatrix.lineColor;
    attr.lineWidth = vpMatrix.lineWidth;
    attr.subColor = currentSubColor;
    model.pDat->instanceAttributes.push_back(attr);
}

pl::UIImage VulkanApp::loadUIImage(std::filesystem::path file_path) {
    return pl::UIImage{uiimageDb->load_image(file_path)};
}
void VulkanApp::drawUIImage(const UIImage &image, int x, int y, int texX, int texY, int texW, int texH, float scaleX, float scaleY) {
    Render2DPushConstantInfo push;
    push.tl = {x * 2.0f / screenWidth - 1.0f, y * 2.0f / screenHeight - 1.0f};
    push.sz = {2.0f * scaleX * texW / screenWidth, 2.0f * scaleY * texH / screenHeight};
    push.texclip_tl = {texX / image.pDat->width, texY / image.pDat->height};
    push.texclip_sz = {texW / image.pDat->width, texH / image.pDat->height};
    uiImageDrawInfos.push_back({image.pDat, push});
}

void VulkanApp::loadIBL(std::filesystem::path file_path){
    
}

void VulkanApp::setViewport(int x, int y, int w, int h) {
    viewport3d.x = x;
    viewport3d.y = y;
    viewport3d.width = w;
    viewport3d.height = h;
    viewport3d.minDepth = 0.0;
    viewport3d.maxDepth = 1.0;
}

} // namespace pl
