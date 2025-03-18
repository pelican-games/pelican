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
    100                  // 光源強度
);

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
    // テクスチャデータの取得
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
    vec3 N;
    if (length(normalColor.rgb) > 0.5) {
        // 法線マップが有効な場合
        N = normalize(TBN * (normalColor.xyz * 2.0 - 1.0));
    } else {
        // 法線マップがない場合
        N = normalize(fragmentNormal);
    }

        // glTF-PBRの標準に合わせて修正
    float metallic = specularColor.b;   // Rチャンネルに金属度（一般的なglTF仕様）
    float roughness = specularColor.g;  // Gチャンネルに粗さ（一般的なglTF仕様）
    // 正しいメタリック値を青チャンネルで表示
outColor = vec4(0, 0, metallic, 1.0);
    
    // 値の範囲確認用デバッグ（コメント解除して確認）
    // outColor = vec4(vec3(metallic), 1.0);  // 金属度の可視化
    // return;

    // 低すぎるとスペキュラーがほぼ見えないため、最小値を設定
    roughness = max(roughness, 0.01);
    
    // アルベドガンマ補正（PBR計算はリニア空間で行うため）
    vec3 albedo = pow(texColor.rgb, vec3(2.2));

    // 視線方向（ビュー空間ではカメラは原点）
    vec3 V = normalize(-fragmentPos);
    
    // 光源パラメータ調整（強めの光を使用）
    vec3 L = normalize(pointLight.position - fragmentPos);
    vec3 H = normalize(V + L);
    float distance = length(pointLight.position - fragmentPos);
    
    // 減衰を調整（より遠くまで届くように）
    float attenuation = 1.0 / (1.0 + 0.01 * distance + 0.001 * (distance * distance));
    vec3 radiance = pointLight.color * pointLight.intensity * attenuation;
    
    // 金属度に基づく基礎反射率（F0）の計算
    // 非金属: 0.04, 金属: albedo
    vec3 F0 = vec3(0.04); 
    F0 = mix(F0, albedo, metallic);
    
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
    vec3 Lo = (kD * albedo / PI + specular) * radiance * NdotL;
    
    // 環境光と金属度に基づく環境反射の模擬
    vec3 ambient = vec3(0.01) * albedo * (1.0 - metallic) + vec3(0.2) * F0 * (1.0 - roughness);
    
    // エミッシブ
    vec3 emission = emissiveColor.rgb * emissiveColor.a;
    
    // 最終カラーの計算
    vec3 color = ambient + Lo + emission;
    
    // デバッグ出力（必要に応じて有効化）
    // outColor = vec4(specular * 5.0, 1.0); // スペキュラーを強調表示
    // return;
    
    // トーンマッピング（HDRをLDRに変換）- ACES近似
    color = color / (color + vec3(1.0));
    
    // ガンマ補正（リニアからsRGB空間へ）
    color = pow(color, vec3(1.0/2.2));
    
    // 出力
    outColor = vec4(color, texColor.a);
}