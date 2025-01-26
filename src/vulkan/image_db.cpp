#include "image_db.hpp"
#include <iostream>
#include <stb_image.h>

namespace pl {

struct VulkanBuffer {
    vk::UniqueDeviceMemory mem;
    vk::UniqueBuffer buf;
};

struct VulkanImage {
    vk::UniqueDeviceMemory mem;
    vk::UniqueImage img;
};

VulkanBuffer createStagingBuffer(vk::Device device, vk::PhysicalDevice physDevice, void *pImgData, size_t imgDataSize) {
    vk::BufferCreateInfo imgStagingBufferCreateInfo;
    imgStagingBufferCreateInfo.size = imgDataSize;
    imgStagingBufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    imgStagingBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

    vk::UniqueBuffer imgStagingBuf = device.createBufferUnique(imgStagingBufferCreateInfo);

    vk::MemoryRequirements imgStagingBufMemReq = device.getBufferMemoryRequirements(imgStagingBuf.get());

    vk::MemoryAllocateInfo imgStagingBufMemAllocInfo;
    imgStagingBufMemAllocInfo.allocationSize = imgStagingBufMemReq.size;

    bool suitableMemoryTypeFound = false;
    auto memProps = physDevice.getMemoryProperties();
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) {
        if (imgStagingBufMemReq.memoryTypeBits & (1 << i) &&
            (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)) {
            imgStagingBufMemAllocInfo.memoryTypeIndex = i;
            suitableMemoryTypeFound = true;
            break;
        }
    }
    if (!suitableMemoryTypeFound) {
        throw std::runtime_error("適切なメモリタイプが存在しません。");
    }

    vk::UniqueDeviceMemory imgStagingBufMemory = device.allocateMemoryUnique(imgStagingBufMemAllocInfo);

    device.bindBufferMemory(imgStagingBuf.get(), imgStagingBufMemory.get(), 0);

    void *pImgStagingBufMem = device.mapMemory(imgStagingBufMemory.get(), 0, imgDataSize);

    std::memcpy(pImgStagingBufMem, pImgData, imgDataSize);

    vk::MappedMemoryRange flushMemoryRange;
    flushMemoryRange.memory = imgStagingBufMemory.get();
    flushMemoryRange.offset = 0;
    flushMemoryRange.size = imgDataSize;

    device.flushMappedMemoryRanges({flushMemoryRange});

    device.unmapMemory(imgStagingBufMemory.get());

    VulkanBuffer buf;
    buf.mem = std::move(imgStagingBufMemory);
    buf.buf = std::move(imgStagingBuf);

    return buf;
}

VulkanImage createImage(vk::Device device, vk::PhysicalDevice physDevice, int imgWidth, int imgHeight) {
    vk::ImageCreateInfo texImgCreateInfo;
    texImgCreateInfo.imageType = vk::ImageType::e2D;
    texImgCreateInfo.extent = vk::Extent3D(imgWidth, imgHeight, 1);
    texImgCreateInfo.mipLevels = 1;
    texImgCreateInfo.arrayLayers = 1;
    texImgCreateInfo.format = vk::Format::eR8G8B8A8Unorm;
    texImgCreateInfo.tiling = vk::ImageTiling::eOptimal;
    texImgCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
    texImgCreateInfo.usage = vk::ImageUsageFlagBits::eSampled | vk::ImageUsageFlagBits::eTransferDst;
    texImgCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    texImgCreateInfo.samples = vk::SampleCountFlagBits::e1;

    vk::UniqueImage texImage = device.createImageUnique(texImgCreateInfo);

    vk::MemoryRequirements texImgMemReq = device.getImageMemoryRequirements(texImage.get());

    vk::MemoryAllocateInfo texImgMemAllocInfo;
    texImgMemAllocInfo.allocationSize = texImgMemReq.size;

    bool suitableMemoryTypeFound = false;
    auto memProps = physDevice.getMemoryProperties();
    for (size_t i = 0; i < memProps.memoryTypeCount; i++) {
        if (texImgMemReq.memoryTypeBits & (1 << i) &&
            (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal)) {
            texImgMemAllocInfo.memoryTypeIndex = i;
            suitableMemoryTypeFound = true;
            break;
        }
    }

    if (!suitableMemoryTypeFound) {
        throw std::runtime_error("使用可能なメモリタイプがありません。");
    }

    vk::UniqueDeviceMemory texImgMem = device.allocateMemoryUnique(texImgMemAllocInfo);
    device.bindImageMemory(texImage.get(), texImgMem.get(), 0);

    VulkanImage img;
    img.mem = std::move(texImgMem);
    img.img = std::move(texImage);

    return img;
}

void copyImgData(vk::Device device, uint32_t graphicsQueueFamilyIndex, vk::Queue queue, vk::Image texImage, vk::Buffer imgStagingBuf, int width, int height) {
    vk::CommandPoolCreateInfo tmpCmdPoolCreateInfo;
    tmpCmdPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    tmpCmdPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
    vk::UniqueCommandPool tmpCmdPool = device.createCommandPoolUnique(tmpCmdPoolCreateInfo);

    vk::CommandBufferAllocateInfo tmpCmdBufAllocInfo;
    tmpCmdBufAllocInfo.commandPool = tmpCmdPool.get();
    tmpCmdBufAllocInfo.commandBufferCount = 1;
    tmpCmdBufAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    std::vector<vk::UniqueCommandBuffer> tmpCmdBufs = device.allocateCommandBuffersUnique(tmpCmdBufAllocInfo);

    vk::CommandBufferBeginInfo cmdBeginInfo;
    cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    tmpCmdBufs[0]->begin(cmdBeginInfo);

    {
        vk::ImageMemoryBarrier barrior;
        barrior.oldLayout = vk::ImageLayout::eUndefined;
        barrior.newLayout = vk::ImageLayout::eTransferDstOptimal;
        barrior.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrior.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrior.image = texImage;
        barrior.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrior.subresourceRange.baseMipLevel = 0;
        barrior.subresourceRange.levelCount = 1;
        barrior.subresourceRange.baseArrayLayer = 0;
        barrior.subresourceRange.layerCount = 1;
        barrior.srcAccessMask = {};
        barrior.dstAccessMask = vk::AccessFlagBits::eTransferWrite;
        tmpCmdBufs[0]->pipelineBarrier(vk::PipelineStageFlagBits::eTopOfPipe, vk::PipelineStageFlagBits::eTransfer, {}, {}, {}, {barrior});
    }

    {
        vk::BufferImageCopy imgCopyRegion;
        imgCopyRegion.bufferOffset = 0;
        imgCopyRegion.imageSubresource.aspectMask = vk::ImageAspectFlagBits::eColor;
        imgCopyRegion.imageSubresource.mipLevel = 0;
        imgCopyRegion.imageSubresource.baseArrayLayer = 0;
        imgCopyRegion.imageSubresource.layerCount = 1;
        imgCopyRegion.imageOffset = vk::Offset3D{0, 0, 0};
        imgCopyRegion.imageExtent = vk::Extent3D{uint32_t(width), uint32_t(height), 1};

        imgCopyRegion.bufferRowLength = 0;
        imgCopyRegion.bufferImageHeight = 0;

        tmpCmdBufs[0]->copyBufferToImage(imgStagingBuf, texImage, vk::ImageLayout::eTransferDstOptimal, {imgCopyRegion});
    }

    {
        vk::ImageMemoryBarrier barrior;
        barrior.oldLayout = vk::ImageLayout::eTransferDstOptimal;
        barrior.newLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
        barrior.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrior.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrior.image = texImage;
        barrior.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        barrior.subresourceRange.baseMipLevel = 0;
        barrior.subresourceRange.levelCount = 1;
        barrior.subresourceRange.baseArrayLayer = 0;
        barrior.subresourceRange.layerCount = 1;
        barrior.srcAccessMask = vk::AccessFlagBits::eTransferWrite;
        barrior.dstAccessMask = vk::AccessFlagBits::eShaderRead;
        tmpCmdBufs[0]->pipelineBarrier(vk::PipelineStageFlagBits::eTransfer, vk::PipelineStageFlagBits::eFragmentShader, {}, {}, {}, {barrior});
    }

    tmpCmdBufs[0]->end();

    vk::CommandBuffer submitCmdBuf[1] = {tmpCmdBufs[0].get()};
    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = submitCmdBuf;

    queue.submit({submitInfo});
    queue.waitIdle();
}

vk::UniqueSampler createSampler(vk::Device device) {
    vk::SamplerCreateInfo samplerCreateInfo;
    samplerCreateInfo.magFilter = vk::Filter::eLinear;
    samplerCreateInfo.minFilter = vk::Filter::eLinear;
    samplerCreateInfo.addressModeU = vk::SamplerAddressMode::eRepeat;
    samplerCreateInfo.addressModeV = vk::SamplerAddressMode::eRepeat;
    samplerCreateInfo.addressModeW = vk::SamplerAddressMode::eRepeat;
    samplerCreateInfo.anisotropyEnable = false;
    samplerCreateInfo.maxAnisotropy = 1.0f;
    samplerCreateInfo.borderColor = vk::BorderColor::eIntOpaqueBlack;
    samplerCreateInfo.unnormalizedCoordinates = false;
    samplerCreateInfo.compareEnable = false;
    samplerCreateInfo.compareOp = vk::CompareOp::eAlways;
    samplerCreateInfo.mipmapMode = vk::SamplerMipmapMode::eLinear;
    samplerCreateInfo.mipLodBias = 0.0f;
    samplerCreateInfo.minLod = 0.0f;
    samplerCreateInfo.maxLod = 0.0f;
    return device.createSamplerUnique(samplerCreateInfo);
}

vk::UniqueDescriptorSetLayout createDescLayout(vk::Device device) {
    vk::DescriptorSetLayoutBinding descSetLayoutBinding[1];
    descSetLayoutBinding[0].binding = 0;
    descSetLayoutBinding[0].descriptorType = vk::DescriptorType::eCombinedImageSampler;
    descSetLayoutBinding[0].descriptorCount = 1;
    descSetLayoutBinding[0].stageFlags = vk::ShaderStageFlagBits::eFragment;

    vk::DescriptorSetLayoutCreateInfo descSetLayoutCreateInfo{};
    descSetLayoutCreateInfo.bindingCount = std::size(descSetLayoutBinding);
    descSetLayoutCreateInfo.pBindings = descSetLayoutBinding;

    return device.createDescriptorSetLayoutUnique(descSetLayoutCreateInfo);
}

constexpr uint32_t maxImages = 64;

vk::UniqueDescriptorPool createDescPool(vk::Device device) {
    vk::DescriptorPoolSize descPoolSize[1];
    descPoolSize[0].type = vk::DescriptorType::eCombinedImageSampler;
    descPoolSize[0].descriptorCount = maxImages;

    vk::DescriptorPoolCreateInfo descPoolCreateInfo;
    descPoolCreateInfo.poolSizeCount = std::size(descPoolSize);
    descPoolCreateInfo.pPoolSizes = descPoolSize;
    descPoolCreateInfo.maxSets = maxImages;

    return device.createDescriptorPoolUnique(descPoolCreateInfo);
}

std::vector<vk::UniqueDescriptorSet> allocDescSet(vk::Device device, vk::DescriptorPool descPool, vk::DescriptorSetLayout layout) {
    vk::DescriptorSetAllocateInfo descSetAllocInfo;

    std::vector<vk::DescriptorSetLayout> descSetLayouts(maxImages, layout);

    descSetAllocInfo.descriptorPool = descPool;
    descSetAllocInfo.descriptorSetCount = descSetLayouts.size();
    descSetAllocInfo.pSetLayouts = descSetLayouts.data();

    return device.allocateDescriptorSetsUnique(descSetAllocInfo);
}

vk::UniqueImageView createImageView(vk::Device device, vk::Image texImage) {
    vk::ImageViewCreateInfo texImgViewCreateInfo;
    texImgViewCreateInfo.image = texImage;
    texImgViewCreateInfo.viewType = vk::ImageViewType::e2D;
    texImgViewCreateInfo.format = vk::Format::eR8G8B8A8Unorm;
    texImgViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
    texImgViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
    texImgViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
    texImgViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
    texImgViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
    texImgViewCreateInfo.subresourceRange.baseMipLevel = 0;
    texImgViewCreateInfo.subresourceRange.levelCount = 1;
    texImgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
    texImgViewCreateInfo.subresourceRange.layerCount = 1;
    return device.createImageViewUnique(texImgViewCreateInfo);
}

void updateDescSetAsTextureImage(vk::Device device, vk::DescriptorSet descSet, vk::ImageView texImageView, vk::Sampler sampler) {
    vk::WriteDescriptorSet writeTexDescSet;
    writeTexDescSet.dstSet = descSet;
    writeTexDescSet.dstBinding = 0;
    writeTexDescSet.dstArrayElement = 0;
    writeTexDescSet.descriptorType = vk::DescriptorType::eCombinedImageSampler;

    vk::DescriptorImageInfo descImgInfo[1];
    descImgInfo[0].imageView = texImageView;
    descImgInfo[0].imageLayout = vk::ImageLayout::eShaderReadOnlyOptimal;
    descImgInfo[0].sampler = sampler;

    writeTexDescSet.descriptorCount = std::size(descImgInfo);
    writeTexDescSet.pImageInfo = descImgInfo;

    device.updateDescriptorSets({writeTexDescSet}, {});
}

UIImageDataBase::UIImageDataBase(vk::Device device, vk::PhysicalDevice physDevice, vk::Queue queue, uint32_t graphicsQueueFamilyIndex)
    : device(device), physDevice(physDevice), queue(queue), graphicsQueueFamilyIndex(graphicsQueueFamilyIndex) {
    descPool = createDescPool(device);
    descLayout = createDescLayout(device);
    descSets = allocDescSet(device, descPool.get(), descLayout.get());
}

pl::UIImageData *UIImageDataBase::load_image(std::filesystem::path file_path) {
    int w, h, ch;
    auto pData = stbi_load(file_path.string().c_str(), &w, &h, &ch, STBI_rgb_alpha);
    size_t imgDataSize = 4 * w * h;

    auto stagingBuf = createStagingBuffer(device, physDevice, pData, imgDataSize);
    stbi_image_free(pData);

    auto img = createImage(device, physDevice, w, h);
    copyImgData(device, graphicsQueueFamilyIndex, queue, img.img.get(), stagingBuf.buf.get(), w, h);

    auto imgView = createImageView(device, img.img.get());
    auto sampler = createSampler(device);
    updateDescSetAsTextureImage(device, descSets.back().get(), imgView.get(), sampler.get());

    UIImageData data;
    data.width = w;
    data.height = h;
    data.mem = std::move(img.mem);
    data.image = std::move(img.img);
    data.imageView = std::move(imgView);
    data.sampler = std::move(sampler);
    data.descSet = std::move(descSets.back());
    descSets.pop_back();

    uiimages.emplace_front(std::move(data));
    return &uiimages.front();
}

} // namespace pl
