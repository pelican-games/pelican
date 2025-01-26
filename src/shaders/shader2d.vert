#version 450
#extension GL_ARB_separate_shader_objects : enable

vec2 positions[6] = vec2[](
    vec2(0.0, 0.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 0.0),
    vec2(0.0, 1.0),
    vec2(1.0, 1.0)
);

layout(push_constant) uniform PushConstant {
    vec2 tl;
    vec2 sz;
    vec2 texclip_tl;
    vec2 texclip_sz;
} push;

layout(location = 0) out vec2 texCoord;

void main() {
    gl_Position = vec4(push.tl + positions[gl_VertexIndex] * push.sz, 0.0, 1.0);
    texCoord = push.texclip_tl + positions[gl_VertexIndex] * push.texclip_sz;
}