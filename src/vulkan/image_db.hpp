#include <filesystem>
#include <list>
#include <vulkan/vulkan.hpp>

namespace pl {

struct UIImageData {
    vk::UniqueDeviceMemory mem;
    vk::UniqueImage image;
    vk::UniqueImageView imageView;
    vk::UniqueSampler sampler;
    vk::UniqueDescriptorSet descSet;
    int width, height;
};

struct UIImageDataBase {
    vk::Device device;
    vk::PhysicalDevice physDevice;
    vk::Queue queue;
    uint32_t graphicsQueueFamilyIndex;
    
    vk::UniqueDescriptorSetLayout descLayout;
    vk::UniqueDescriptorPool descPool;
    std::vector<vk::UniqueDescriptorSet> descSets;
    
    std::list<pl::UIImageData> uiimages;

    UIImageDataBase(vk::Device device, vk::PhysicalDevice physDevice, vk::Queue queue, uint32_t graphicsQueueFamilyIndex);
    pl::UIImageData *load_image(std::filesystem::path file_path);
};

} // namespace pl
