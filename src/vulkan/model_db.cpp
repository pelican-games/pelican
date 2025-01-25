#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "model_db.hpp"
#include <filesystem>
#include <iostream>
#include <tiny_gltf.h>

namespace pl {

template <class T>
glm::vec4 to_vec4(std::vector<T> v) {
    return glm::vec4(v[0], v[1], v[2], v[3]);
}

class ModelLoader {
    ModelDataBase &db;
    std::vector<pl::Material *> p_materials;
    pl::ModelData *p_model;
    tinygltf::Model model;

    template <uint32_t expectType, uint32_t expectComponentType, class F>
    void try_load_indices(const tinygltf::Primitive &primitive, F &&f) {
        const auto accessorIndex = primitive.indices;
        const auto &accessor = model.accessors[accessorIndex];
        if (!(accessor.type == expectType && accessor.componentType == expectComponentType))
            return;

        const auto bufferViewIndex = accessor.bufferView;
        const auto &bufferView = model.bufferViews[bufferViewIndex];
        const auto bufferIndex = bufferView.buffer;
        const auto &buffer = model.buffers[bufferIndex];
        const auto p = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
        const auto stride = accessor.ByteStride(bufferView);
        for (uint32_t i = 0; i < accessor.count; i++) {
            f(i, p + i * stride);
        }
    }

    template <uint32_t expectType, uint32_t expectComponentType, class F>
    void try_load_attribute(const char *name, const tinygltf::Primitive &primitive, F &&f) {
        if (primitive.attributes.find(name) == primitive.attributes.end())
            return;
        const auto accessorIndex = primitive.attributes.at(name);
        const auto &accessor = model.accessors[accessorIndex];
        if (!(accessor.type == expectType && accessor.componentType == expectComponentType))
            return;

        const auto bufferViewIndex = accessor.bufferView;
        const auto &bufferView = model.bufferViews[bufferViewIndex];
        const auto bufferIndex = bufferView.buffer;
        const auto &buffer = model.buffers[bufferIndex];
        const auto p = buffer.data.data() + bufferView.byteOffset + accessor.byteOffset;
        const auto stride = accessor.ByteStride(bufferView);

        for (uint32_t i = 0; i < accessor.count; i++) {
            f(i, p + i * stride);
        }
    }

    pl::Primitive load_primitive(const tinygltf::Primitive &primitive) {
        pl::Primitive primitiveData;

        // material
        if (primitive.material >= 0)
            primitiveData.material = p_materials[primitive.material];

        // indices
        if (primitive.indices < 0)
            throw std::runtime_error("model load error: no index buffer");
        const auto indCount = model.accessors[primitive.indices].count;
        primitiveData.indices.resize(indCount);
        try_load_indices<TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE>(
            primitive,
            [&primitiveData](uint32_t i, const uint8_t *p) {
                primitiveData.indices[i] = *reinterpret_cast<const uint8_t *>(p);
            });
        try_load_indices<TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT>(
            primitive,
            [&primitiveData](uint32_t i, const uint8_t *p) {
                primitiveData.indices[i] = *reinterpret_cast<const uint16_t *>(p);
            });
        try_load_indices<TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT>(
            primitive,
            [&primitiveData](uint32_t i, const uint8_t *p) {
                primitiveData.indices[i] = *reinterpret_cast<const uint32_t *>(p);
            });

        // attributes
        const auto vertCount = model.accessors[primitive.attributes.at("POSITION")].count;
        primitiveData.vertices.resize(vertCount);

        try_load_attribute<TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT>(
            "POSITION", primitive,
            [&primitiveData](uint32_t i, const uint8_t *p) {
                primitiveData.vertices[i].position = *reinterpret_cast<const glm::vec3 *>(p);
            });
        try_load_attribute<TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT>(
            "NORMAL", primitive,
            [&primitiveData](uint32_t i, const uint8_t *p) {
                primitiveData.vertices[i].normal = *reinterpret_cast<const glm::vec3 *>(p);
            });
        try_load_attribute<TINYGLTF_TYPE_VEC4, TINYGLTF_COMPONENT_TYPE_FLOAT>(
            "TANGENT", primitive,
            [&primitiveData](uint32_t i, const uint8_t *p) {
                primitiveData.vertices[i].tangent = *reinterpret_cast<const glm::vec4 *>(p);
            });
        try_load_attribute<TINYGLTF_TYPE_VEC2, TINYGLTF_COMPONENT_TYPE_FLOAT>(
            "TEXCOORD_0", primitive,
            [&primitiveData](uint32_t i, const uint8_t *p) {
                primitiveData.vertices[i].texCoord = *reinterpret_cast<const glm::vec2 *>(p);
            });
        try_load_attribute<TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT>(
            "COLOR_0", primitive,
            [&primitiveData](uint32_t i, const uint8_t *p) {
                primitiveData.vertices[i].color = glm::vec4(*reinterpret_cast<const glm::vec3 *>(p), 1.0f);
            });
        try_load_attribute<TINYGLTF_TYPE_VEC4, TINYGLTF_COMPONENT_TYPE_FLOAT>(
            "COLOR_0", primitive,
            [&primitiveData](uint32_t i, const uint8_t *p) {
                primitiveData.vertices[i].color = *reinterpret_cast<const glm::vec4 *>(p);
            });
        try_load_attribute<TINYGLTF_TYPE_VEC4, TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE>(
            "JOINTS_0", primitive,
            [&primitiveData](uint32_t i, const uint8_t *p) {
                primitiveData.vertices[i].joint = *reinterpret_cast<const glm::u8vec4 *>(p);
            });
        try_load_attribute<TINYGLTF_TYPE_VEC4, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT>(
            "JOINTS_0", primitive,
            [&primitiveData](uint32_t i, const uint8_t *p) {
                primitiveData.vertices[i].joint = *reinterpret_cast<const glm::u16vec4 *>(p);
            });
        try_load_attribute<TINYGLTF_TYPE_VEC4, TINYGLTF_COMPONENT_TYPE_FLOAT>(
            "WEIGHTS_0", primitive,
            [&primitiveData](uint32_t i, const uint8_t *p) {
                primitiveData.vertices[i].weight = *reinterpret_cast<const glm::vec4 *>(p);
            });

        return primitiveData;
    }

    void load_node(pl::Mesh &meshData, const tinygltf::Node &node) {
        // transform
        pl::Transform transform;
        if (node.translation.size() == 3)
            transform.translation = glm::vec3{node.translation[0], node.translation[1], node.translation[2]};
        else
            transform.translation = glm::vec3(0, 0, 0);

        if (node.rotation.size() == 4)
            transform.rotation = glm::quat{static_cast<float>(node.rotation[0]), static_cast<float>(node.rotation[1]), static_cast<float>(node.rotation[2]), static_cast<float>(node.rotation[3])};
        else
            transform.rotation = glm::quat(0, 0, 0, 1);

        if (node.scale.size() == 3)
            transform.scale = glm::vec3{node.scale[0], node.scale[1], node.scale[2]};
        else
            transform.translation = glm::vec3(1, 1, 1);

        for (const auto childNodeIndex : node.children) {
            load_node(meshData, model.nodes[childNodeIndex]);
        }

        if (node.mesh >= 0) {
            const auto &mesh = model.meshes[node.mesh];
            for (const auto &primitive : mesh.primitives) {
                const auto primitiveData = load_primitive(primitive);

                meshData.primitives.push_back(primitiveData);
            }
        }
    }

    std::optional<pl::TextureRaw> load_texture(const int index) {
        if(index < 0)
            return std::nullopt;

        const auto &tex = model.textures[index];

        TextureRaw rawData;
        switch (model.samplers[tex.sampler].magFilter) {
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
            rawData.magFilter = pl::FilterType::Nearest;
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
            rawData.magFilter = pl::FilterType::Linear;
            break;
        default:
            rawData.magFilter = pl::FilterType::Linear;
            break;
        }
        switch (model.samplers[tex.sampler].minFilter) {
        case TINYGLTF_TEXTURE_FILTER_NEAREST:
            rawData.minFilter = pl::FilterType::Nearest;
            break;
        case TINYGLTF_TEXTURE_FILTER_LINEAR:
            rawData.minFilter = pl::FilterType::Linear;
            break;
        default:
            rawData.minFilter = pl::FilterType::Linear;
            break;
        }
        const auto& image = model.images[tex.source];
        rawData.width = image.width;
        rawData.height = image.height;
        rawData.data = image.image;
        return rawData;
    }

    pl::Material *load_material(const tinygltf::Material &material) {
        pl::Material materialData;

        if (material.pbrMetallicRoughness.baseColorFactor.size() == 4)
            materialData.baseColorFactor = to_vec4(material.pbrMetallicRoughness.baseColorFactor);
        materialData.metallicFactor = material.pbrMetallicRoughness.metallicFactor;
        materialData.roughnessFactor = material.pbrMetallicRoughness.roughnessFactor;

        materialData.baseColorTextureRaw = load_texture(material.pbrMetallicRoughness.baseColorTexture.index);
        materialData.normalTextureRaw = load_texture(material.normalTexture.index);
        materialData.emissiveTextureRaw = load_texture(material.emissiveTexture.index);
        materialData.occlusionTextureRaw = load_texture(material.occlusionTexture.index);

        db.materials.emplace_back(std::move(materialData));
        return &db.materials.back();
    }

  public:
    ModelLoader(ModelDataBase &db, std::filesystem::path file_path) : db{db} {
        pl::Mesh mesh;

        tinygltf::TinyGLTF loader;
        std::string err, warn;
        if (!loader.LoadBinaryFromFile(&model, &err, &warn, file_path.string())) {
            throw std::runtime_error(std::string("gltf load error: ") + err);
        }

        p_materials.resize(model.materials.size());
        for (int i = 0; const auto &material : model.materials) {
            p_materials[i] = load_material(material);
            i++;
        }

        const auto &defaultScene = model.scenes[model.defaultScene >= 0 ? model.defaultScene : 0];
        for (const auto nodeIndex : defaultScene.nodes) {
            load_node(mesh, model.nodes[nodeIndex]);
        }

        pl::ModelData model;
        model.meshes.push_back(mesh);

        db.models.emplace_front(std::move(model));
        p_model = &db.models.front();
    }

    pl::ModelData *getModel() const { return p_model; }
};
pl::ModelData *ModelDataBase::load_model(std::filesystem::path file_path) {
    ModelLoader loader{*this, file_path};
    return loader.getModel();
}

} // namespace pl
