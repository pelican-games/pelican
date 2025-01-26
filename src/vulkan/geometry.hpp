#pragma once
#include <glm/gtc/quaternion.hpp>
#include <vulkan/vulkan.hpp>
#include <optional>

namespace pl {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 tangent;
    
    //可変要素
    glm::vec2 texCoord;
    glm::vec4 color;
    glm::uvec4 joint;
    glm::vec4 weight;

    static vk::VertexInputBindingDescription getBindingDescription() {
        return vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex);
    }

    static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)),
            vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, tangent)),
            vk::VertexInputAttributeDescription(3, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord)),
            vk::VertexInputAttributeDescription(4, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, color)),
            vk::VertexInputAttributeDescription(5, 0, vk::Format::eR32G32B32A32Uint, offsetof(Vertex, joint)),
            vk::VertexInputAttributeDescription(6, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, weight))
        };
    }
};

enum class FilterType {
    Nearest, Linear, Cubic,
};

struct TextureRaw {
    vk::Filter toVkFilter(FilterType filter) {
        switch (filter) {
        case FilterType::Nearest:
            return vk::Filter::eNearest;
        case FilterType::Linear:
            return vk::Filter::eLinear;
        case FilterType::Cubic:
            return vk::Filter::eCubicIMG;
        default:
            return vk::Filter::eLinear;
        }
    }
    FilterType magFilter, minFilter;
    int width, height;
    int bits;
    std::vector<uint8_t> data;
};

struct Material {
    glm::vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;

    std::optional<TextureRaw> baseColorTextureRaw;
    std::pair<vk::UniqueImage, vk::UniqueDeviceMemory> baseColorTexture;
    vk::UniqueImageView baseColorTextureView;
    vk::UniqueSampler baseColorTextureSampler;

    std::optional<TextureRaw> metallicRoughnessTextureRaw;
    std::pair<vk::UniqueImage, vk::UniqueDeviceMemory> metallicRoughnessTexture;
    vk::UniqueImageView metallicRoughnessTextureView;
    vk::UniqueSampler metallicRoughnessTextureSampler;

    std::optional<TextureRaw> normalTextureRaw;
    std::pair<vk::UniqueImage, vk::UniqueDeviceMemory> normalTexture;
    vk::UniqueImageView normalTextureView;
    vk::UniqueSampler normalTextureSampler;

    std::optional<TextureRaw> occlusionTextureRaw;
    std::pair<vk::UniqueImage, vk::UniqueDeviceMemory> occlusionTexture;
    vk::UniqueImageView occlusionTextureView;
    vk::UniqueSampler occlusionTextureSampler;

    std::optional<TextureRaw> emissiveTextureRaw;
    std::pair<vk::UniqueImage, vk::UniqueDeviceMemory> emissiveTexture;
    vk::UniqueImageView emissiveTextureView;
    vk::UniqueSampler emissiveTextureSampler;

    vk::UniqueDescriptorPool descPool;
    vk::UniqueDescriptorSet descSet;

    vk::DescriptorSetLayoutBinding getDescriptorSetLayoutBinding(uint32_t binding) {
        uint32_t count = 0;
        if(baseColorTextureRaw.has_value()){
            count++;
        }
        if(metallicRoughnessTextureRaw.has_value()){
            count++;
        }
        if(normalTextureRaw.has_value()){
            count++;
        }
        if(occlusionTextureRaw.has_value()){
            count++;
        }
        if(emissiveTextureRaw.has_value()){
            count++;
        }
        return {
            vk::DescriptorSetLayoutBinding(binding, vk::DescriptorType::eCombinedImageSampler, count, vk::ShaderStageFlagBits::eFragment)
        };
    }

    vk::DescriptorPoolSize getDescriptorPoolSize() {
        uint32_t count = 0;
        if(baseColorTextureRaw.has_value()){
            count++;
        }
        if(metallicRoughnessTextureRaw.has_value()){
            count++;
        }
        if(normalTextureRaw.has_value()){
            count++;
        }
        if(occlusionTextureRaw.has_value()){
            count++;
        }
        if(emissiveTextureRaw.has_value()){
            count++;
        }
        return {
            vk::DescriptorType::eCombinedImageSampler, count
        };
    }
};

struct Primitive {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    Material* material;
};

struct Mesh {
    std::vector<Primitive> primitives;
};

struct Transform {
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
};

struct InstanceAttribute {
    glm::mat4 model;
};

struct InstanceUpdate {
    std::vector<InstanceAttribute> sphere;
    std::vector<InstanceAttribute> module;
};

struct Object {
    bool Instance;
    Mesh* mesh;
    std::vector<InstanceAttribute> instanceAttributes;
    Transform transform;

    static vk::VertexInputBindingDescription getBindingDescription() {
        return vk::VertexInputBindingDescription(1, sizeof(glm::vec4) * 4, vk::VertexInputRate::eInstance);
    }

    static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription(7, 1, vk::Format::eR32G32B32A32Sfloat, 0),
            vk::VertexInputAttributeDescription(8, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4)),
            vk::VertexInputAttributeDescription(9, 1, vk::Format::eR32G32B32A32Sfloat, 2 * sizeof(glm::vec4)),
            vk::VertexInputAttributeDescription(10, 1, vk::Format::eR32G32B32A32Sfloat, 3 * sizeof(glm::vec4))
        };
    }
};

struct VPMatrix {
    glm::mat4 view;
    glm::mat4 projection;
};

}

