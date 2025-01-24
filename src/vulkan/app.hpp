#pragma once
#include "geometry.hpp"
#include "pipelineBuilder.hpp"
#include <GLFW/glfw3.h>
#include <pelican/renderer.hpp>
#include "model_db.hpp"

namespace pl {

class VulkanApp : public pl::Renderer {
        unsigned int screenWidth, screenHeight;
        GLFWwindow* window;

        pl::ObjectDataBase objDb;
        pl::ModelDataBase modelDb;

        pl::VPMatrix vpMatrix;

        std::vector<Model> drawGeometry;

    public:
        VulkanApp(GLFWwindow* window, unsigned int screenWidth, unsigned int screenHeight);
        ~VulkanApp() override;

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
        
        //std::vector<std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory>> vertexBuffers;
        //std::vector<std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory>> indexBuffers;
        //std::vector<std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory>> instanceBuffers;
        std::vector<std::pair<uint32_t, uint32_t>> indexCounts; //頂点数(のべ)とインスタンス数

        vk::UniquePipeline pipeline;
        vk::UniquePipelineLayout pipelineLayout;
        std::unique_ptr<PipelineBuilder> pipelineBuilder;

        VkSurfaceKHR c_surface;
        vk::UniqueSurfaceKHR surface;
        vk::UniqueSwapchainKHR swapchain;
        std::vector<vk::Image> swapchainImages;
        std::vector<vk::UniqueImageView> swapchainImageViews;

        vk::UniqueFence swapchainImgFence;

        //イメージ
        vk::UniqueImage image;

        //std::vector<pl::Object> scene;

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

        std::pair<vk::UniqueBuffer, vk::UniqueDeviceMemory> createBuffer(vk::BufferCreateFlags flags, vk::DeviceSize size, vk::BufferUsageFlags usage, vk::MemoryPropertyFlags properties);
        void setBuffer();

        

        //レンダリング
        void drawGBuffer(uint32_t objectIndex);  

    public:
        //レンダリング
        void drawFrame() override;

        void drawModel(const Model &model, glm::mat4x4 modelMatrix) override;//インスタンスバッファのためにインスタンス毎のモデル行列を受け取る

        void setObjectData();
        void setCamera(glm::vec3 pos, glm::vec3 dir, glm::vec3 up) override;
        void setProjection(float horizontalAngle) override;
        pl::Model loadModel(std::filesystem::path file_path) override;
        //void loadObject(std::filesystem::path file_path) override;
        //セットアップ2
        void setup();
};

}
