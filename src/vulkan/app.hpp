#pragma once
#include "geometry.hpp"
#include "pipelineBuilder.hpp"
#include <GLFW/glfw3.h>
#include <pelican/renderer.hpp>
#include "model_db.hpp"

#include "pipeline2DBuilder.hpp"
#include "image_db.hpp"

namespace pl {



class VulkanApp : public pl::Renderer {
        unsigned int screenWidth, screenHeight;
        unsigned int virtualScreenWidth, virtualScreenHeight;
        GLFWwindow* window;

        vk::UniqueInstance instance;
        vk::PhysicalDevice physicalDevice;
        vk::UniqueDevice device;
        
        pl::ObjectDataBase objDb;
        pl::ModelDataBase modelDb;

        pl::VPMatrix vpMatrix;

    public:
        VulkanApp(GLFWwindow* window, unsigned int screenWidth, unsigned int screenHeight, unsigned int virtualScreenWidth, unsigned int virtualScreenHeight);
        ~VulkanApp() override;

    private:

        std::vector<vk::DeviceQueueCreateInfo> queueCreateInfos;
        std::vector<vk::Queue> graphicsQueues;
        // std::vector<vk::Queue> computeQueues;

        vk::UniqueCommandPool graphicCommandPool;
        std::vector<vk::UniqueCommandBuffer> graphicCommandBuffers;
        // vk::UniqueCommandPool computeCommandPool;
        // std::vector<vk::UniqueCommandBuffer> computeCommandBuffers;
        
        //std::vector<std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory>> vertexBuffers;
        //std::vector<std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory>> indexBuffers;
        //std::vector<std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory>> instanceBuffers;
        std::vector<std::pair<uint32_t, uint32_t>> indexCounts; //頂点数(のべ)とインスタンス数

        vk::UniquePipeline pipeline;
        vk::UniquePipelineLayout pipelineLayout;
        std::unique_ptr<PipelineBuilder> pipelineBuilder;
        
        std::optional<UIImageDataBase> uiimageDb;
        vk::UniquePipeline pipeline2D;
        vk::UniquePipelineLayout pipeline2DLayout;
        std::unique_ptr<Pipeline2DBuilder> pipeline2DBuilder;
        struct UIImageDrawInfo {
            UIImageData* data;
            Render2DPushConstantInfo push;
        };
        std::vector<UIImageDrawInfo> uiImageDrawInfos;

        VkSurfaceKHR c_surface;
        vk::UniqueSurfaceKHR surface;
        vk::UniqueSwapchainKHR swapchain;
        std::vector<vk::Image> swapchainImages;
        std::vector<vk::UniqueImageView> swapchainImageViews;

        vk::UniqueFence swapchainImgFence;

        vk::Viewport viewport3d;

        //デスクリプタセット
        vk::UniqueDescriptorSetLayout descriptorSetLayout;

        //イメージ
        std::vector<std::pair<vk::UniqueImage, vk::UniqueDeviceMemory>> image;

        //Gバッファ
        std::vector<std::pair<vk::UniqueImage, vk::UniqueDeviceMemory>> positionImage;
        std::vector<vk::UniqueImageView> positionImageView;
        std::vector<std::pair<vk::UniqueImage, vk::UniqueDeviceMemory>> normalImage;
        std::vector<vk::UniqueImageView> normalImageView;
        std::vector<std::pair<vk::UniqueImage, vk::UniqueDeviceMemory>> albedoImage;
        std::vector<vk::UniqueImageView> albedoImageView;
        std::vector<std::pair<vk::UniqueImage, vk::UniqueDeviceMemory>> depthImage;
        std::vector<vk::UniqueImageView> depthImageView;

        //std::vector<pl::Object> scene;

        //vulkan初期化用関数
        vk::PhysicalDevice pickPhysicalDevice(const std::vector<const char*>& deviceExtensions, vk::PhysicalDeviceFeatures deviceFeatures);
        bool checkDeviceExtensionSupport(vk::PhysicalDevice device, const std::vector<const char*>& deviceExtensions);
        bool checkDeviceFeatures(vk::PhysicalDevice device, vk::PhysicalDeviceFeatures deviceFeatures);

        //サーフェスの作成
        void createSurface();

        std::vector<vk::DeviceQueueCreateInfo> findQueues(std::vector<float> &graphicQueuePriorities, std::vector<float> &computeQueuePriorities);
        uint32_t checkPresentationSupport(vk::SurfaceKHR surface);
        
        //mipmapの生成
        void generateMipmaps(vk::CommandBuffer cmdBuffer, vk::Image image, 
            int32_t texWidth, int32_t texHeight, uint32_t mipLevels);

        //イメージの作成
        std::pair <vk::UniqueImage , vk::UniqueDeviceMemory> createImage(uint32_t width, uint32_t height, vk::Format format, vk::ImageTiling tiling, vk::ImageUsageFlags usage, vk::SampleCountFlagBits samples = vk::SampleCountFlagBits::e1);
        vk::UniqueImageView createImageView(vk::Image image, vk::Format format, vk::ImageAspectFlags aspectFlags);
        uint32_t findMemoryType(uint32_t typeFilter, vk::MemoryPropertyFlags properties);

        //シェーダーモジュールの作成
        vk::UniqueShaderModule createShaderModule(std::string filename);

        //スワップチェーンの作成
        void createSwapchain();

        std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory> createBuffer(vk::BufferCreateFlags flags, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties, vk::SharingMode sharingMode = vk::SharingMode::eExclusive);
        void setBuffer();

        //コマンドバッファの作成
        std::pair<vk::UniqueCommandPool, std::vector<vk::UniqueCommandBuffer>> createCommandBuffers(vk::CommandPoolCreateFlagBits commandPoolFlag, vk::DeviceQueueCreateInfo queueCreateInfo, uint32_t commandBufferCount);
        vk::ImageMemoryBarrier createImageMemoryBarrier(vk::Image image, vk::ImageLayout oldLayout, vk::ImageLayout newLayout, vk::AccessFlags srcAccessMask, vk::AccessFlags dstAccessMask, vk::ImageAspectFlags aspectMask = vk::ImageAspectFlagBits::eColor);
        
        //レンダリング用関数
        void copyTexture(vk::CommandBuffer commandBuffer, pl::Material& material, vk::Image image, vk::Buffer stagingBuffer, vk::DeviceSize offset, uint32_t width, uint32_t height);
        void transferTexture(const pl::ModelData&);
        void copyBufferToImage(vk::Buffer buffer, vk::Image image, uint32_t width, uint32_t height, vk::DeviceSize offset);
        void drawGBuffer(uint32_t objectIndex);  


    public:
        //レンダリング
        void drawFrame() override;

        void drawModel(const Model &model, glm::mat4x4 modelMatrix) override;//インスタンスバッファのためにインスタンス毎のモデル行列を受け取る

        void setCamera(glm::vec3 pos, glm::vec3 dir, glm::vec3 up) override;
        void setProjection(float horizontalAngle, float near, float far) override;
        pl::Model loadModel(std::filesystem::path file_path, uint32_t max_object_num) override;
        //void loadObject(std::filesystem::path file_path) override;
        void setViewport(int x, int y, int w, int h) override;
        void setLine(glm::vec4 color, float width) override;

        void drawUIImage(const UIImage &image, int x, int y, int texX, int texY, int texW, int texH, float scaleX, float scaleY);
        pl::UIImage loadUIImage(std::filesystem::path file_path);
        void loadIBL(std::filesystem::path file_path);
};

}
