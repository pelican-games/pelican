#version 450
#extension GL_ARB_separate_shader_objects : enable

//layout(set = 0, binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;
layout(location = 1) out vec2 fragmentUV;

void main() {
    outColor = vec4(1.0, 0.0, 0.0, 1.0);
}