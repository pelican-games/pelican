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
    // 太陽光の宣言
    vec3 sunDirection = normalize(vec3(-1.0, -1.0, -1.0)); // 太陽光の方向
    vec3 sunColor = vec3(1.0, 1.0, 1.0); // 太陽光の色

    // マテリアルの取得
    vec3 ambient = vec3(0,0,0);
    vec3 albedo = texture(texSampler, fragmentUV).xyz;
    vec3 normal = fragmentNormal.rgb;
    vec3 specularColor = vec3(0.0, 0.0, 0.0);
    vec3 emissive = vec3(0.0, 0.0, 0.0);
    float shininess = 32.0;

    // 環境光
    vec3 ambientLight = ambient * albedo;

    vec3 N = normalize(fragmentNormal);
    vec3 V = normalize(-fragmentPosition); // 視線ベクトル
    vec3 L = normalize(sunDirection); // 光源ベクトル
    vec3 R = reflect(-L, N); // 反射ベクトル

    // 拡散反射
    float NdotL = max(dot(N, L), 0.0);
    vec3 diffuse = NdotL * albedo * sunColor;

    // 鏡面反射
    float RdotV = max(dot(R, V), 0.0);
    vec3 specular = pow(RdotV, shininess) * specularColor * sunColor;

    // 照明の合成
    vec3 color = ambientLight + diffuse + specular + emissive;

    // ガンマ補正
    color = pow(color, vec3(1.0 / 2.2));
    outColor = vec4(color, 1.0);
}