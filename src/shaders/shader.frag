#version 450
#extension GL_ARB_separate_shader_objects : enable

//layout(set = 0, binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec3 fragmentPosition;
layout(location = 1) in vec2 fragmentUV;
layout(location = 2) in vec3 fragmentNormal;



layout(set = 0, binding = 0) uniform sampler2D texSampler;
layout(set = 0, binding = 1) uniform sampler2D normalSampler;
layout(set = 0, binding = 2) uniform sampler2D specularSampler;
layout(set = 0, binding = 3) uniform sampler2D emissiveSampler;
layout(set = 0, binding = 4) uniform sampler2D ambientSampler;

void main() {
    outColor = texture(texSampler, fragmentUV);
    outColor = vec4(fragmentNormal, 1.0);
}