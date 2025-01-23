#pragma once
#include "geometry.hpp"
#include "pipelineBuilder.hpp"
#include <GLFW/glfw3.h>

namespace pl {

class VulkanApp {
        unsigned int screenWidth, screenHeight;
        GLFWwindow* window;

    public:
        VulkanApp(GLFWwindow* window, unsigned int screenWidth, unsigned int screenHeight);
        ~VulkanApp();

    private:
        vk::UniqueInstance instance;
        vk::PhysicalDevice physicalDevice;
        vk::UniqueDevice device;
        std::vector<vk::Queue> graphicsQueues;
        // std::vector<vk::Queue> computeQueues;

        vk::UniqueCommandPool graphicCommandPool;
        std::vector<vk::UniqueCommandBuffer> graphicCommandBuffers;
        // vk::UniqueCommandPool computeCommandPool;
        // std::vector<vk::UniqueCommandBuffer> computeCommandBuffers;
        
        std::vector<std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory>> vertexBuffers;
        std::vector<std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory>> indexBuffers;
        std::vector<std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory>> instanceBuffers;
        std::vector<std::pair<uint32_t, uint32_t>> indexCounts; //頂点数(のべ)とインスタンス数

        vk::UniquePipeline pipeline;
        std::unique_ptr<PipelineBuilder> pipelineBuilder;

        VkSurfaceKHR c_surface;
        vk::UniqueSurfaceKHR surface;
        vk::UniqueSwapchainKHR swapchain;
        std::vector<vk::Image> swapchainImages;
        std::vector<vk::UniqueImageView> swapchainImageViews;

        vk::UniqueFence swapchainImgFence;

        //イメージ
        vk::UniqueImage image;

        void initVulkan();
        void mainLoop();
        void cleanup();

        //vulkan初期化用関数
        vk::PhysicalDevice pickPhysicalDevice(const std::vector<const char*>& deviceExtensions, vk::PhysicalDeviceFeatures deviceFeatures);
        bool checkDeviceExtensionSupport(vk::PhysicalDevice device, const std::vector<const char*>& deviceExtensions);
        bool checkDeviceFeatures(vk::PhysicalDevice device, vk::PhysicalDeviceFeatures deviceFeatures);

        //サーフェスの作成
        void createSurface();

        std::vector<vk::DeviceQueueCreateInfo> findQueues(std::vector<float> &graphicQueuePriorities, std::vector<float> &computeQueuePriorities);
        uint32_t checkPresentationSupport(vk::SurfaceKHR surface);
        
        //イメージの作成
        vk::UniqueImage createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage);
        uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

        //シェーダーモジュールの作成
        vk::UniqueShaderModule createShaderModule(std::string filename);

        //スワップチェーンの作成
        void createSwapchain();

        //頂点バッファの作成
        std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory> createBuffer(vk::BufferCreateFlags flags, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
        void setBuffer(std::vector<Object> scene);

    public:
        //レンダリング
        void drawFrame(uint32_t imageIndex);
};

}
