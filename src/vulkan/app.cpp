#include "app.hpp"
#include <GLFW/glfw3.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <vulkan/vulkan.hpp>

namespace pl {

VulkanApp::VulkanApp(GLFWwindow *window, unsigned int screenWidth, unsigned int screenHeight)
    : window(window), screenWidth{screenWidth}, screenHeight{screenHeight} {
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

    auto requiredLayers = {"VK_LAYER_KHRONOS_validation"};
    uint32_t instanceExtensionCount = 0;
    const char **requiredExtensions = glfwGetRequiredInstanceExtensions(&instanceExtensionCount);
    vk::InstanceCreateInfo instCreateInfo(
        {},
        &appInfo,
        requiredLayers.size(),
        requiredLayers.begin(),
        instanceExtensionCount,
        requiredExtensions);

    instance = vk::createInstanceUnique(instCreateInfo);
    // 物理デバイスの初期化
    auto deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME};
    vk::PhysicalDeviceFeatures deviceFeatures = {}; // DeviceFeaturesの設定
    deviceFeatures.geometryShader = VK_TRUE;

    // 物理デバイスの選択
    physicalDevice = pickPhysicalDevice(deviceExtensions, deviceFeatures);

    // サーフェスの作成
    createSurface();

    // デバイスの初期化
    std::vector<float> graphicQueuePriorities;
    std::vector<float> computeQueuePriorities;
    std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos = findQueues(graphicQueuePriorities, computeQueuePriorities);

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
    image = createImage(screenWidth, screenHeight, vk::Format::eR8G8B8A8Unorm, vk::ImageTiling::eOptimal, vk::ImageUsageFlagBits::eColorAttachment | vk::ImageUsageFlagBits::eSampled);

    // シェーダーモジュールの作成
    vk::UniqueShaderModule vertShaderModule = createShaderModule("src/shaders/shader.vert.spv");
    vk::UniqueShaderModule fragShaderModule = createShaderModule("src/shaders/shader.frag.spv");

    // パイプラインの作成
    std::vector<vk::PipelineShaderStageCreateInfo> shaderStages = {
        vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eVertex, vertShaderModule.get(), "main"),
        vk::PipelineShaderStageCreateInfo({}, vk::ShaderStageFlagBits::eFragment, fragShaderModule.get(), "main")};

    pipelineBuilder = std::make_unique<PipelineBuilder>();
    pipeline = pipelineBuilder->buildPipeline(device.get(), pipelineLayout, shaderStages, screenWidth, screenHeight);

    // スワップチェーンの作成
    createSwapchain();
    // スワップチェーンイメージ用フェンスの作成
    vk::FenceCreateInfo fenceCreateInfo{};
    swapchainImgFence = device->createFenceUnique(fenceCreateInfo);
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
vk::UniqueImage VulkanApp::createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage) {
    vk::ImageCreateInfo imageCreateInfo(
        {},
        vk::ImageType::e2D,
        format,
        vk::Extent3D(width, height, 1),
        1,
        1,
        vk::SampleCountFlagBits::e1,
        tiling,
        usage,
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

    return image;
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
}

std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory> VulkanApp::createBuffer(vk::BufferCreateFlags flags, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties) {
    vk::BufferCreateInfo bufferCreateInfo(
        flags,
        size,
        usage,
        vk::SharingMode::eExclusive,
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
    device->resetFences({swapchainImgFence.get()});
    vk::ResultValue acquireResult = device->acquireNextImageKHR(swapchain.get(), UINT64_MAX, {}, swapchainImgFence.get());

    if (acquireResult.result != vk::Result::eSuccess) {
        throw std::runtime_error("スワップチェーンイメージの取得に失敗しました");
    }
    uint32_t imageIndex = acquireResult.value;

    if (device->waitForFences({swapchainImgFence.get()}, VK_TRUE, UINT64_MAX) != vk::Result::eSuccess) {
        throw std::runtime_error("スワップチェーンイメージの取得に失敗しました");
    }

    std::vector<vk::RenderingAttachmentInfo> colorAttachments = {
        vk::RenderingAttachmentInfo(
            swapchainImageViews.at(imageIndex).get(), // imageView
            vk::ImageLayout::eColorAttachmentOptimal, // imageLayout
            vk::ResolveModeFlagBits::eNone,           // resolveMode
            {},                                       // resolveImageView
            vk::ImageLayout::eUndefined,              // resolveImageLayout
            vk::AttachmentLoadOp::eClear,             // loadOp
            vk::AttachmentStoreOp::eStore,            // storeOp
            vk::ClearValue{}                          // clearValue
            )};

    vk::RenderingInfo renderingInfo(
        {},                                              // flags
        vk::Rect2D({0, 0}, {screenWidth, screenHeight}), // renderArea
        1,                                               // layerCount
        0,                                               // viewMask
        colorAttachments.size(),                         // colorAttachmentCount
        colorAttachments.data(),                         // pColorAttachments
        nullptr,                                         // pDepthAttachment
        nullptr                                          // pStencilAttachment
    );

    graphicCommandBuffers.at(0)->reset();

    vk::CommandBufferBeginInfo beginInfo;
    graphicCommandBuffers.at(0)->begin(beginInfo);

    // vk::ImageMemoryBarrier firstMemoryBarrier;
    // firstMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eNone;
    // firstMemoryBarrier.dstAccessMask = vk::AccessFlagBits::eNone;
    // firstMemoryBarrier.oldLayout = vk::ImageLayout::eUndefined;
    // firstMemoryBarrier.newLayout = vk::ImageLayout::eColorAttachmentOptimal;
    // firstMemoryBarrier.image = swapchainImages[imageIndex];
    // firstMemoryBarrier.setSubresourceRange({ vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1 });

    // graphicCommandBuffers.at(0)->pipelineBarrier(
    //     vk::PipelineStageFlagBits::eBottomOfPipe,
    //     vk::PipelineStageFlagBits::eTopOfPipe,
    //     {},
    //     {},
    //     {},
    //     firstMemoryBarrier
    // );

    graphicCommandBuffers.at(0)->beginRendering(renderingInfo);

    vk::ClearValue clearValue(vk::ClearColorValue(std::array<float, 4>{0.0f, 0.0f, 0.0f, 1.0f}));

    graphicCommandBuffers.at(0)->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());

    graphicCommandBuffers.at(0)->pushConstants(pipelineLayout.get(), vk::ShaderStageFlagBits::eVertex, 0, sizeof(vpMatrix), &vpMatrix);

    for (auto& model : modelDb.models) {
        auto& [instanceBuf, instanceBufMem] = modelDb.instanceBuffers.at(model.modelIndex);

        const auto instanceBufSize = model.instanceAttributes.size() * sizeof(InstanceAttribute);
        const auto pInstanceBuf = device->mapMemory(instanceBufMem.get(), 0, instanceBufSize, {});
        std::memcpy(pInstanceBuf, model.instanceAttributes.data(), instanceBufSize);

        vk::MappedMemoryRange range;
        range.memory = instanceBufMem.get();
        range.offset = 0;
        range.size = instanceBufSize;

        device->flushMappedMemoryRanges({range});
        device->unmapMemory(instanceBufMem.get());

        graphicCommandBuffers.at(0)->bindVertexBuffers(0, {modelDb.vertexBuffers.at(model.modelIndex).first.get(), instanceBuf.get()}, {0, 0});
        graphicCommandBuffers.at(0)->bindIndexBuffer(modelDb.indexBuffers.at(model.modelIndex).first.get(), 0, vk::IndexType::eUint32);

        uint32_t indexCount = 0;
        for (auto &mesh : model.meshes) {
            for (auto &primitive : mesh.primitives) {
                indexCount += primitive.indices.size();
            }
        }
        graphicCommandBuffers.at(0)->drawIndexed(indexCount, model.instanceAttributes.size(), 0, 0, 0);

        model.instanceAttributes.clear();
    }

    graphicCommandBuffers.at(0)->endRendering();

    vk::ImageMemoryBarrier imageMemoryBarrier;
    imageMemoryBarrier.srcAccessMask = vk::AccessFlagBits::eColorAttachmentWrite;
    imageMemoryBarrier.oldLayout = vk::ImageLayout::eUndefined;
    imageMemoryBarrier.newLayout = vk::ImageLayout::ePresentSrcKHR;
    imageMemoryBarrier.image = swapchainImages[imageIndex];
    imageMemoryBarrier.setSubresourceRange({vk::ImageAspectFlagBits::eColor, 0, 1, 0, 1});

    graphicCommandBuffers.at(0)->pipelineBarrier(
        vk::PipelineStageFlagBits::eColorAttachmentOutput,
        vk::PipelineStageFlagBits::eBottomOfPipe,
        {},
        {},
        {},
        imageMemoryBarrier);
    graphicCommandBuffers.at(0)->end();

    vk::CommandBuffer submitCommandBuffer = graphicCommandBuffers.at(0).get();
    vk::SubmitInfo submitInfo(
        {},
        {},
        {submitCommandBuffer},
        {});

    // device->waitIdle();
    // graphicsQueues.at(0).waitIdle();

    graphicsQueues.at(0).submit({submitInfo});

    vk::PresentInfoKHR presentInfo;

    auto presentSwapchains = {swapchain.get()};
    auto imgIndices = {imageIndex};

    presentInfo.swapchainCount = presentSwapchains.size();
    presentInfo.pSwapchains = presentSwapchains.begin();
    presentInfo.pImageIndices = imgIndices.begin();

    graphicsQueues.at(0).presentKHR(presentInfo);

    graphicsQueues.at(0).waitIdle();
}

void VulkanApp::drawFrame() {
    drawGBuffer(0);
}

void VulkanApp::setCamera(glm::vec3 pos, glm::vec3 dir, glm::vec3 up) {
    vpMatrix.view = glm::lookAt(pos, dir, up);
}

void VulkanApp::setProjection(float horizontalAngle) {
    vpMatrix.projection = glm::perspective(glm::radians(horizontalAngle), static_cast<float>(screenWidth) / static_cast<float>(screenHeight), 0.1f, 100.0f);
}

void VulkanApp::drawModel(const Model &model, glm::mat4x4 modelMatrix) {
    InstanceAttribute attr;
    attr.model = modelMatrix;
    model.pDat->instanceAttributes.push_back(attr);
}

} // namespace pl
