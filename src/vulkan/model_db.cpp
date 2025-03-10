#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "model_db.hpp"
#include <chrono>
#include <fastgltf/core.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <fastgltf/tools.hpp>
#include <filesystem>
#include <iostream>
#include <stb_image.h>

namespace pl {

glm::vec4 to_vec4(const fastgltf::math::nvec4 &v) {
    return glm::vec4(v[0], v[1], v[2], v[3]);
}

class ModelLoader {
    ModelDataBase &db;
    std::vector<pl::Material *> p_materials;
    pl::ModelData *p_model;
    fastgltf::Asset model;

    template <fastgltf::AccessorType expectType, fastgltf::ComponentType expectComponentType, class T, class F>
    void try_load_indices(const fastgltf::Primitive &primitive, F &&f) {
        const auto accessorIndex = primitive.indicesAccessor.value();
        const auto &accessor = model.accessors[accessorIndex];
        // if (!(accessor.type == expectType && accessor.componentType == expectComponentType))
        //     return;

        fastgltf::iterateAccessorWithIndex<T>(model, accessor, f);
    }

    template <fastgltf::AccessorType expectType, fastgltf::ComponentType expectComponentType, class T, class F>
    void try_load_attribute(const char *name, const fastgltf::Primitive &primitive, F &&f) {
        const auto it = primitive.findAttribute(name);
        if (it == primitive.attributes.cend())
            return;
        const auto accessorIndex = it->accessorIndex;
        const auto &accessor = model.accessors[accessorIndex];
        if (!(accessor.type == expectType && accessor.componentType == expectComponentType))
            return;

        fastgltf::iterateAccessorWithIndex<T>(model, accessor, f);
    }

    pl::Primitive load_primitive(const fastgltf::Primitive &primitive) {
        pl::Primitive primitiveData;

        // material
        if (primitive.materialIndex.has_value())
            primitiveData.material = p_materials[primitive.materialIndex.value()];

        // indices
        if (!primitive.indicesAccessor.has_value())
            throw std::runtime_error("model load error: no index buffer");
        const auto indCount = model.accessors[primitive.indicesAccessor.value()].count;
        primitiveData.indices.resize(indCount);

        try_load_indices<fastgltf::AccessorType::Scalar, fastgltf::ComponentType::UnsignedByte, uint8_t>(
            primitive,
            [&primitiveData](const uint8_t p, size_t i) {
                primitiveData.indices[i] = p;
            });
        try_load_indices<fastgltf::AccessorType::Scalar, fastgltf::ComponentType::UnsignedShort, uint16_t>(
            primitive,
            [&primitiveData](const uint16_t p, size_t i) {
                primitiveData.indices[i] = p;
            });
        try_load_indices<fastgltf::AccessorType::Scalar, fastgltf::ComponentType::UnsignedInt, uint32_t>(
            primitive,
            [&primitiveData](const uint32_t p, size_t i) {
                primitiveData.indices[i] = p;
            });

        // attributes
        const auto vertCount = model.accessors[primitive.findAttribute("POSITION")->accessorIndex].count;
        primitiveData.vertices.resize(vertCount);

        try_load_attribute<fastgltf::AccessorType::Vec3, fastgltf::ComponentType::Float, fastgltf::math::fvec3>(
            "POSITION", primitive,
            [&primitiveData](fastgltf::math::fvec3 p, size_t i) {
                primitiveData.vertices[i].position = glm::vec3{p.x(), p.y(), p.z()};
            });
        try_load_attribute<fastgltf::AccessorType::Vec3, fastgltf::ComponentType::Float, fastgltf::math::fvec3>(
            "NORMAL", primitive,
            [&primitiveData](fastgltf::math::fvec3 p, size_t i) {
                primitiveData.vertices[i].normal = {p[0], p[1], p[2]};
            });
        try_load_attribute<fastgltf::AccessorType::Vec4, fastgltf::ComponentType::Float, fastgltf::math::fvec4>(
            "TANGENT", primitive,
            [&primitiveData](fastgltf::math::fvec4 p, size_t i) {
                primitiveData.vertices[i].tangent = {p[0], p[1], p[2], p[3]};
            });
        try_load_attribute<fastgltf::AccessorType::Vec2, fastgltf::ComponentType::Float, fastgltf::math::fvec2>(
            "TEXCOORD_0", primitive,
            [&primitiveData](fastgltf::math::fvec2 p, size_t i) {
                primitiveData.vertices[i].texCoord = {p[0], p[1]};
            });
        try_load_attribute<fastgltf::AccessorType::Vec3, fastgltf::ComponentType::Float, fastgltf::math::fvec3>(
            "COLOR_0", primitive,
            [&primitiveData](fastgltf::math::fvec3 p, size_t i) {
                primitiveData.vertices[i].color = {p[0], p[1], p[2], 1.0f};
            });
        try_load_attribute<fastgltf::AccessorType::Vec4, fastgltf::ComponentType::Float, fastgltf::math::fvec4>(
            "COLOR_0", primitive,
            [&primitiveData](fastgltf::math::fvec4 p, size_t i) {
                primitiveData.vertices[i].color = {p[0], p[1], p[2], p[3]};
            });
        try_load_attribute<fastgltf::AccessorType::Vec4, fastgltf::ComponentType::UnsignedByte, fastgltf::math::u8vec4>(
            "JOINTS_0", primitive,
            [&primitiveData](fastgltf::math::u8vec4 p, size_t i) {
                primitiveData.vertices[i].joint = {p[0], p[1], p[2], p[3]};
            });
        try_load_attribute<fastgltf::AccessorType::Vec4, fastgltf::ComponentType::UnsignedShort, fastgltf::math::u16vec4>(
            "JOINTS_0", primitive,
            [&primitiveData](fastgltf::math::u16vec4 p, size_t i) {
                primitiveData.vertices[i].joint = {p[0], p[1], p[2], p[3]};
            });
        try_load_attribute<fastgltf::AccessorType::Vec4, fastgltf::ComponentType::Float, fastgltf::math::fvec4>(
            "WEIGHTS_0", primitive,
            [&primitiveData](fastgltf::math::fvec4 p, size_t i) {
                primitiveData.vertices[i].weight = {p[0], p[1], p[2], p[3]};
            });

        return primitiveData;
    }

    void load_node(pl::Mesh &meshData, const fastgltf::Node &node) {
        auto trs = std::get<fastgltf::TRS>(node.transform);

        // transform
        pl::Transform transform;
        transform.translation = glm::vec3{trs.translation[0], trs.translation[1], trs.translation[2]};
        transform.rotation = glm::quat{trs.rotation[0], trs.rotation[1], trs.rotation[2], trs.rotation[3]};
        transform.scale = glm::vec3{trs.scale[0], trs.scale[1], trs.scale[2]};

        for (const auto childNodeIndex : node.children) {
            load_node(meshData, model.nodes[childNodeIndex]);
        }

        if (node.meshIndex.has_value()) {
            const auto &mesh = model.meshes[node.meshIndex.value()];
            for (const auto &primitive : mesh.primitives) {
                const auto primitiveData = load_primitive(primitive);

                meshData.primitives.push_back(primitiveData);
            }
        }
    }

    std::optional<pl::TextureRaw> load_texture(const fastgltf::Texture &tex) {
        TextureRaw rawData;
        if (tex.samplerIndex)
            rawData.magFilter = pl::FilterType::Linear;
        else
            switch (model.samplers[tex.samplerIndex.value()].magFilter.value_or(fastgltf::Filter::Linear)) {
            case fastgltf::Filter::Nearest:
                rawData.magFilter = pl::FilterType::Nearest;
                break;
            case fastgltf::Filter::Linear:
                rawData.magFilter = pl::FilterType::Linear;
                break;
            default:
                rawData.magFilter = pl::FilterType::Linear;
                break;
            }

        if (tex.samplerIndex)
            rawData.minFilter = pl::FilterType::Linear;
        else
            switch (model.samplers[tex.samplerIndex.value()].minFilter.value_or(fastgltf::Filter::Linear)) {
            case fastgltf::Filter::Nearest:
                rawData.minFilter = pl::FilterType::Nearest;
                break;
            case fastgltf::Filter::Linear:
                rawData.minFilter = pl::FilterType::Linear;
                break;
            default:
                rawData.minFilter = pl::FilterType::Linear;
                break;
            }
        const auto &image = model.images[tex.imageIndex.value()];

        std::visit(
            fastgltf::visitor{
                [&](const fastgltf::sources::BufferView &view) {
                    auto &bufferView = model.bufferViews[view.bufferViewIndex];
                    auto &buffer = model.buffers[bufferView.bufferIndex];
                    std::visit(
                        fastgltf::visitor{
                            [&](fastgltf::sources::Array &vector) {
                                int w, h, ch;
                                auto data = stbi_load_from_memory(
                                    reinterpret_cast<const stbi_uc *>(vector.bytes.data() + bufferView.byteOffset),
                                    bufferView.byteLength,
                                    &w, &h, &ch, STBI_rgb_alpha);
                                rawData.width = w;
                                rawData.height = h;
                                rawData.data.resize(w * h * 4);
                                std::memcpy(rawData.data.data(), data, rawData.data.size());
                                rawData.bits = 8;

                                stbi_image_free(data);
                            },
                            [](auto &arg) { throw std::runtime_error("unsupported model structure"); },
                        },
                        buffer.data);
                },
                [](auto &arg) { throw std::runtime_error("unsupported model structure"); },
            },
            image.data);
        return rawData;
    }

    pl::Material *load_material(const fastgltf::Material &material) {
        pl::Material materialData;

        materialData.baseColorFactor = to_vec4(material.pbrData.baseColorFactor);
        materialData.metallicFactor = material.pbrData.metallicFactor;
        materialData.roughnessFactor = material.pbrData.roughnessFactor;

        if (material.pbrData.baseColorTexture.has_value())
            materialData.baseColorTextureRaw = load_texture(model.textures[material.pbrData.baseColorTexture->textureIndex]);
        if (material.normalTexture.has_value())
            materialData.normalTextureRaw = load_texture(model.textures[material.normalTexture->textureIndex]);
        if (material.emissiveTexture.has_value())
            materialData.emissiveTextureRaw = load_texture(model.textures[material.emissiveTexture->textureIndex]);
        if (material.occlusionTexture.has_value())
            materialData.occlusionTextureRaw = load_texture(model.textures[material.occlusionTexture->textureIndex]);

        db.materials.emplace_back(std::move(materialData));
        return &db.materials.back();
    }

  public:
    ModelLoader(ModelDataBase &db, std::filesystem::path file_path) : db{db} {
        pl::Mesh mesh;

        auto time_s = std::chrono::system_clock::now();

        std::cout << (std::chrono::system_clock::now() - time_s) << std::endl;

        auto gltfFile = fastgltf::GltfFileStream(file_path);
        if (!gltfFile.isOpen()) {
            throw std::runtime_error("Failed to open glTF file: " + file_path.string());
        }

        std::cout << (std::chrono::system_clock::now() - time_s) << std::endl;

        fastgltf::Parser parser;
        auto asset = parser.loadGltfBinary(gltfFile, "");
        if (asset.error() != fastgltf::Error::None) {
            throw std::runtime_error("Failed to load glTF: " + std::string(fastgltf::getErrorMessage(asset.error())));
        }
        model = std::move(asset.get());

        std::cout << (std::chrono::system_clock::now() - time_s) << std::endl;

        pl::ModelData modelDat;
        p_materials.resize(model.materials.size());
        for (int i = 0; const auto &material : model.materials) {
            p_materials[i] = load_material(material);
            modelDat.used_materials.push_back(p_materials[i]);
            i++;
        }

        std::cout << (std::chrono::system_clock::now() - time_s) << std::endl;

        const auto &defaultScene = model.scenes[model.defaultScene.value_or(0)];
        for (const auto nodeIndex : defaultScene.nodeIndices) {
            load_node(mesh, model.nodes[nodeIndex]);
        }

        std::cout << (std::chrono::system_clock::now() - time_s) << std::endl;

        modelDat.meshes.push_back(mesh);

        std::cout << (std::chrono::system_clock::now() - time_s) << std::endl;

        db.models.emplace_front(std::move(modelDat));

        std::cout << (std::chrono::system_clock::now() - time_s) << std::endl;

        p_model = &db.models.front();
    }

    pl::ModelData *getModel() const { return p_model; }
};
pl::ModelData *ModelDataBase::load_model(std::filesystem::path file_path) {
    ModelLoader loader{*this, file_path};
    return loader.getModel();
}

} // namespace pl
