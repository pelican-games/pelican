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

layout(location = 0) out vec3 geomPos;
layout(location = 1) out vec2 fragUV;

layout(push_constant) uniform PushConstant {
    mat4 view;
    mat4 proj;
} push;


void main() {
    mat4 instanceMatrix = mat4(inInstanceMatrix0, inInstanceMatrix1, inInstanceMatrix2, inInstanceMatrix3);
    gl_Position = push.proj * push.view * instanceMatrix * vec4(inPos, 1.0);
    geomPos = inPos;
    fragUV = inUV;
}