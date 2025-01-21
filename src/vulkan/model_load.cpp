#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "geometry.hpp"
#include <filesystem>
#include <tiny_gltf.h>

template <uint32_t expectType, uint32_t expectComponentType, class F>
void try_load_indices(const tinygltf::Model &model, const tinygltf::Primitive &primitive, F &&f) {
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
void try_load_attribute(const char *name, const tinygltf::Model &model, const tinygltf::Primitive &primitive, F &&f) {
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

pl::Primitive load_primitive(const tinygltf::Model &model, const tinygltf::Primitive &primitive) {
    pl::Primitive primitiveData;

    // material
    if (primitive.material >= 0)
        primitiveData.materialIndex = primitive.material;

    // indices
    if (primitive.indices < 0)
        throw std::runtime_error("model load error: no index buffer");
    const auto indCount = model.accessors[primitive.indices].count;
    primitiveData.indices.resize(indCount);
    try_load_indices<TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE>(
        model, primitive,
        [&primitiveData](uint32_t i, const uint8_t *p) {
            primitiveData.indices[i] = *reinterpret_cast<const uint8_t *>(p);
        });
    try_load_indices<TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT>(
        model, primitive,
        [&primitiveData](uint32_t i, const uint8_t *p) {
            primitiveData.indices[i] = *reinterpret_cast<const uint16_t *>(p);
        });
    try_load_indices<TINYGLTF_TYPE_SCALAR, TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT>(
        model, primitive,
        [&primitiveData](uint32_t i, const uint8_t *p) {
            primitiveData.indices[i] = *reinterpret_cast<const uint32_t *>(p);
        });

    // attributes
    const auto vertCount = model.accessors[primitive.attributes.at("POSITION")].count;
    primitiveData.vertices.resize(vertCount);

    try_load_attribute<TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT>(
        "POSITION", model, primitive,
        [&primitiveData](uint32_t i, const uint8_t *p) {
            primitiveData.vertices[i].position = *reinterpret_cast<const glm::vec3 *>(p);
        });
    try_load_attribute<TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT>(
        "NORMAL", model, primitive,
        [&primitiveData](uint32_t i, const uint8_t *p) {
            primitiveData.vertices[i].normal = *reinterpret_cast<const glm::vec3 *>(p);
        });
    try_load_attribute<TINYGLTF_TYPE_VEC4, TINYGLTF_COMPONENT_TYPE_FLOAT>(
        "TANGENT", model, primitive,
        [&primitiveData](uint32_t i, const uint8_t *p) {
            primitiveData.vertices[i].tangent = *reinterpret_cast<const glm::vec4 *>(p);
        });
    try_load_attribute<TINYGLTF_TYPE_VEC2, TINYGLTF_COMPONENT_TYPE_FLOAT>(
        "TEXCOORD_0", model, primitive,
        [&primitiveData](uint32_t i, const uint8_t *p) {
            primitiveData.vertices[i].texCoord = *reinterpret_cast<const glm::vec2 *>(p);
        });
    try_load_attribute<TINYGLTF_TYPE_VEC3, TINYGLTF_COMPONENT_TYPE_FLOAT>(
        "COLOR_0", model, primitive,
        [&primitiveData](uint32_t i, const uint8_t *p) {
            primitiveData.vertices[i].color = glm::vec4(*reinterpret_cast<const glm::vec3 *>(p), 1.0f);
        });
    try_load_attribute<TINYGLTF_TYPE_VEC4, TINYGLTF_COMPONENT_TYPE_FLOAT>(
        "COLOR_0", model, primitive,
        [&primitiveData](uint32_t i, const uint8_t *p) {
            primitiveData.vertices[i].color = *reinterpret_cast<const glm::vec4 *>(p);
        });
    try_load_attribute<TINYGLTF_TYPE_VEC4, TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE>(
        "JOINTS_0", model, primitive,
        [&primitiveData](uint32_t i, const uint8_t *p) {
            primitiveData.vertices[i].joint = *reinterpret_cast<const glm::u8vec4 *>(p);
        });
    try_load_attribute<TINYGLTF_TYPE_VEC4, TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT>(
        "JOINTS_0", model, primitive,
        [&primitiveData](uint32_t i, const uint8_t *p) {
            primitiveData.vertices[i].joint = *reinterpret_cast<const glm::u16vec4 *>(p);
        });
    try_load_attribute<TINYGLTF_TYPE_VEC4, TINYGLTF_COMPONENT_TYPE_FLOAT>(
        "WEIGHTS_0", model, primitive,
        [&primitiveData](uint32_t i, const uint8_t *p) {
            primitiveData.vertices[i].weight = *reinterpret_cast<const glm::vec4 *>(p);
        });
}

void load_node(pl::Mesh &meshData, const tinygltf::Model &model, const tinygltf::Node &node) {
    for (const auto childNodeIndex : node.children) {
        load_node(meshData, model, model.nodes[childNodeIndex]);
    }

    if (node.mesh >= 0) {
        const auto &mesh = model.meshes[node.mesh];
        for (const auto &primitive : mesh.primitives) {
            const auto primitiveData = load_primitive(model, primitive);

            meshData.primitives.push_back(primitiveData);
        }
    }
}

pl::Mesh load_model(std::filesystem::path file_path) {
    pl::Mesh mesh;

    tinygltf::TinyGLTF loader;
    tinygltf::Model model;
    std::string err, warn;
    if (!loader.LoadBinaryFromFile(&model, &err, &warn, file_path.string())) {
        throw std::runtime_error(std::string("gltf load error: ") + err);
    }

    const auto &defaultScene = model.scenes[model.defaultScene >= 0 ? model.defaultScene : 0];
    for (const auto nodeIndex : defaultScene.nodes) {
        load_node(mesh, model, model.nodes[nodeIndex]);
    }
}
