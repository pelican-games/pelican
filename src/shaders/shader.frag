#version 450
#extension GL_ARB_separate_shader_objects : enable

//layout(set = 0, binding = 1) uniform sampler2D texSampler;

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 fragmentUV;
layout(location = 1) in vec3 fragmentPos;
layout(location = 2) in vec3 fragmentNormal;
layout(location = 3) in vec3 fragmentTangent;
layout(location = 4) in vec3 fragmentBitangent;


layout(set = 0, binding = 0) uniform sampler2D texSampler;
layout(set = 0, binding = 1) uniform sampler2D specularSampler;
layout(set = 0, binding = 2) uniform sampler2D normalSampler;
layout(set = 0, binding = 3) uniform sampler2D ambientSampler;
layout(set = 0, binding = 4) uniform sampler2D emissiveSampler;

struct PointLight {
    vec3 position;
    vec3 color;
    float intensity;
};

const PointLight pointLight = PointLight(
    vec3(5.0, 5.0, 5.0), // 光源位置
    vec3(0.0078, 1.0, 0.7176), // 光源色
    4.0                  // 光源強度
);

void main() {

    vec4 texColor = texture(texSampler, fragmentUV);
    vec4 normalColor = texture(normalSampler, fragmentUV);
    vec4 specularColor = texture(specularSampler, fragmentUV);
    vec4 emissiveColor = texture(emissiveSampler, fragmentUV);
    vec4 ambientColor = texture(ambientSampler, fragmentUV);

        // TBN行列の構築
    mat3 TBN = mat3(
        normalize(fragmentTangent),
        normalize(fragmentBitangent),
        normalize(fragmentNormal)
    );
    
    // 法線マッピングの適用
    vec3 normal;
    if (length(normalColor.rgb) > 0.5) {
        // 法線マップが有効な場合
        normal = normalize(TBN * (normalColor.xyz * 2.0 - 1.0));
    } else {
        // 法線マップがない場合
        normal = normalize(fragmentNormal);
    }

    // 光源方向と距離
    vec3 lightDir = pointLight.position - fragmentPos;
    float lightDistance = length(lightDir);
    lightDir = normalize(lightDir);
    
    // 視線方向
    vec3 viewDir = normalize(vec3(0,0,0) - fragmentPos);
    
    // 半球ベクトル (Blinn-Phong用)
    vec3 halfwayDir = normalize(lightDir + viewDir);
    
    // 環境光成分
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * pointLight.color * texColor.rgb;
    
    // 拡散反射成分
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * pointLight.color * texColor.rgb;
    
    // 鏡面反射成分
    float specularStrength = specularColor.r; // 鏡面反射の強さ
    float shininess = 32.0; // 光沢度
    float spec = pow(max(dot(normal, halfwayDir), 0.0), shininess);
    vec3 specular = specularStrength * spec * pointLight.color;
    
    // 光の減衰
    float attenuation = 1.0 / (1.0 + 0.09 * lightDistance + 0.032 * (lightDistance * lightDistance));
    
    // エミッシブマップの追加
    vec3 emission = emissiveColor.rgb * emissiveColor.a;
    
    // 最終色の計算
    vec3 result = ambient + (diffuse + specular) * attenuation * pointLight.intensity + emission;
    
    // 出力色
    outColor = vec4(result, texColor.a);
}