#version 450
#extension GL_ARB_separate_shader_objects : enable
// Vertex shader
layout(location = 0) in vec3 inPos;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec4 inTangent;
layout(location = 3) in vec2 inUV;
layout(location = 4) in vec4 inColor;
layout(location = 5) in uvec4 inBoneIndices;
layout(location = 6) in vec4 inBoneWeights;

layout(location = 7) in vec4 inInstanceMatrix0;
layout(location = 8) in vec4 inInstanceMatrix1;
layout(location = 9) in vec4 inInstanceMatrix2;
layout(location = 10) in vec4 inInstanceMatrix3;

layout(push_constant) uniform PushConstant {
    mat4 view;
    mat4 proj;
    vec4 outlineColor;
    float outlineWidth;
} push;

layout(location = 0) out vec2 fragmentUV;
layout(location = 1) out vec3 worldPos;
layout(location = 2) out vec3 worldNormal;
layout(location = 3) out vec3 worldTangent;
layout(location = 4) out vec3 worldBitangent;

void main() {

    mat4 instanceMatrix = mat4(inInstanceMatrix0, inInstanceMatrix1, inInstanceMatrix2, inInstanceMatrix3);
    gl_Position = push.proj * push.view * instanceMatrix * vec4(inPos, 1.0);

    // 法線をワールド空間に変換
    mat3 normalMatrix = transpose(inverse(mat3(instanceMatrix)));
    worldNormal = normalize(normalMatrix * inNormal);

    if(length(inTangent) == 0.0) {
        // 法線から接線を計算
        vec3 c1 = cross(worldNormal, vec3(0.0, 0.0, 1.0));
        vec3 c2 = cross(worldNormal, vec3(0.0, 1.0, 0.0));
        worldTangent = normalize(length(c1) > length(c2) ? c1 : c2);
        worldBitangent = normalize(cross(worldNormal, worldTangent));
    }
    else {
        worldTangent = normalize(normalMatrix * inTangent.xyz);
        worldBitangent = normalize(cross(worldNormal, worldTangent) * inTangent.w);
    }
    
    worldPos = (instanceMatrix * vec4(inPos, 1.0)).xyz;
    
    fragmentUV = inUV;
}