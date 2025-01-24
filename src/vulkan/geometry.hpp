#pragma once
#include <glm/gtc/quaternion.hpp>
#include <vulkan/vulkan.hpp>

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

struct Material {
    glm::vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;

    vk::UniqueImage baseColorTexture;
    vk::UniqueImageView baseColorTextureView;
    vk::UniqueSampler baseColorTextureSampler;

    vk::UniqueImage metallicRoughnessTexture;
    vk::UniqueImageView metallicRoughnessTextureView;
    vk::UniqueSampler metallicRoughnessTextureSampler;

    vk::UniqueImage normalTexture;
    vk::UniqueImageView normalTextureView;
    vk::UniqueSampler normalTextureSampler;

    vk::UniqueImage occlusionTexture;
    vk::UniqueImageView occlusionTextureView;
    vk::UniqueSampler occlusionTextureSampler;

    vk::UniqueImage emissiveTexture;
    vk::UniqueImageView emissiveTextureView;
    vk::UniqueSampler emissiveTextureSampler;
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

