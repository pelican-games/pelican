#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) out vec4 outColor;

layout(location = 0) in vec2 fragmentUV;
layout(location = 1) in vec3 fragmentPos;
layout(location = 2) in vec3 fragmentNormal;
layout(location = 3) in vec3 fragmentTangent;
layout(location = 4) in vec3 fragmentBitangent;
layout(location = 5) in flat int isOutline;
layout(location = 6) in vec4 inOutlineColor;
layout(location = 7) in vec4 inSubColor;

layout(set = 0, binding = 0) uniform sampler2D texSampler;
layout(set = 0, binding = 1) uniform sampler2D specularSampler;
layout(set = 0, binding = 2) uniform sampler2D normalSampler;
layout(set = 0, binding = 3) uniform sampler2D ambientSampler;
layout(set = 0, binding = 4) uniform sampler2D emissiveSampler;

// IBL関連
layout(set = 0, binding = 5) uniform samplerCube irradianceMap;
layout(set = 0, binding = 6) uniform samplerCube prefilteredMap;
layout(set = 0, binding = 7) uniform sampler2D brdfLUT;

// 光源情報をハードコーディング
const int LIGHT_COUNT = 3; // 光源の数

// ポイントライトの構造体定義
struct PointLight {
    vec3 position;   // ワールド空間での位置
    vec3 color;      // 色
    float intensity; // 強度
};

// 複数の光源をハードコーディングで定義
const PointLight lights[LIGHT_COUNT] = PointLight[](
    // 光源1: 正面上
    PointLight(
        vec3(0.0, 5.0, 5.0),  // 位置
        vec3(1.0, 1.0, 1.0),  // 暖色系の光
        10                  // 強度
    ),
    // 光源2: 左側
    PointLight(
        vec3(-5.0, 2.0, 0.0), // 位置
        vec3(0.0, 0.0, 0.0),  // 青っぽい光
        60.0                  // 強度
    ),
    // 光源3: 右側
    PointLight(
        vec3(0.0, 5.0, -2.0), // 位置
        vec3(1.0, 1.0, 1.0),  // 赤っぽい光
        10.0                  // 強度
    )
);

layout(push_constant) uniform PushConstant {
    mat4 view;
    mat4 proj;
} push;

const float PI = 3.14159265359;

// PBR関数群
// 法線分布関数（GGX/Trowbridge-Reitz）
float DistributionGGX(vec3 N, vec3 H, float roughness) {
    float a = roughness * roughness;
    float a2 = a * a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH * NdotH;
    
    float nom = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;
    
    return nom / max(denom, 0.0000001);
}

// 幾何遮蔽関数（Smith's Schlick-GGX）
float GeometrySchlickGGX(float NdotV, float roughness) {
    float r = (roughness + 1.0);
    float k = (r * r) / 8.0;

    float nom = NdotV;
    float denom = NdotV * (1.0 - k) + k;
    
    return nom / max(denom, 0.0000001);
}

// スミス法による幾何遮蔽関数
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness) {
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);
    
    return ggx1 * ggx2;
}

// フレネル方程式（Schlickの近似）
vec3 fresnelSchlick(float cosTheta, vec3 F0) {
    return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

void main() {
    if(isOutline == 1) {
        outColor = inOutlineColor;
        return;
    }

    // テクスチャデータの取得
    vec4 texColor = texture(texSampler, fragmentUV) * inSubColor;
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
    vec3 N;
    if (length(normalColor.rgb) > 0.5) {
        // 法線マップが有効な場合
        N = normalize(TBN * (normalColor.xyz * 2.0 - 1.0));
    } else {
        // 法線マップがない場合
        N = normalize(fragmentNormal);
    }

    // glTF-PBRの標準に合わせて修正
    float metallic = 1-specularColor.b;   // Bチャンネルに金属度
    float roughness = specularColor.g;  // Gチャンネルに粗さ

    // 低すぎるとスペキュラーがほぼ見えないため、最小値を設定
    roughness = max(roughness, 0.01);
    
    // アルベドガンマ補正（PBR計算はリニア空間で行うため）
    vec3 albedo = pow(texColor.rgb, vec3(2.2));

    // 視線方向（ビュー空間ではカメラは原点）
    vec3 V = normalize(-fragmentPos);
    
    // 金属度に基づく基礎反射率（F0）の計算
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
    
    // すべての光源からの寄与を計算
    vec3 Lo = vec3(0.0);
    
    // 各光源からの寄与を蓄積
    for (int i = 0; i < LIGHT_COUNT; i++) {
        // ワールド空間の光源位置をビュー空間に変換
        vec3 lightPos = (push.view * vec4(lights[i].position, 1.0)).xyz;
        vec3 lightColor = lights[i].color;
        float lightIntensity = lights[i].intensity;
        
        // 光源方向と距離
        vec3 L = normalize(lightPos - fragmentPos);
        vec3 H = normalize(V + L);
        float distance = length(lightPos - fragmentPos);
        
        // 減衰計算
        float attenuation = 1.0 / (1.0 + 0.01 * distance + 0.001 * (distance * distance));
        vec3 radiance = lightColor * lightIntensity * attenuation;
        
        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G = GeometrySmith(N, V, L, roughness);
        vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);
        
        // 反射率と拡散率の計算
        vec3 kS = F;
        vec3 kD = vec3(1.0) - kS;
        kD *= 1.0 - metallic; // 金属はディフューズ成分を持たない
        
        // スペキュラー項の計算
        vec3 numerator = NDF * G * F;
        float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
        vec3 specular = numerator / denominator;
        
        // 拡散反射と鏡面反射の合成
        float NdotL = max(dot(N, L), 0.0);
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;
    }
    
    // 環境光の詳細制御
    float ambientIntensity = 1;  // わずかな環境光を追加
    float reflectionIntensity = 1;

    // 拡散環境光
    vec3 ambientDiffuse = vec3(ambientIntensity) * albedo * (1.0 - metallic);
    // 反射環境光
    vec3 ambientSpecular = vec3(reflectionIntensity) * F0 * (1.0 - roughness);
    // 合成
    vec3 ambient = ambientDiffuse + ambientSpecular;
    
    // エミッシブ
    vec3 emission = emissiveColor.rgb * 1;
    
    // 最終カラーの計算
    vec3 color = ambient + Lo + emission;
    
    // トーンマッピング（HDRをLDRに変換）- ACES近似
    color = color / (color + vec3(1.0));
    
    // ガンマ補正（リニアからsRGB空間へ）
    color = pow(color, vec3(1.0/2.2));
    
    // 出力
    outColor = vec4(color, 1.0);
}