#version 450
#extension GL_ARB_separate_shader_objects : enable

// 入力は頂点シェーダーからの三角形
layout(triangles) in;
// 出力は最大6つの頂点を持つトライアングルストリップ
layout(triangle_strip, max_vertices = 6) out;

// 入力変数（頂点シェーダーからの出力を受け取る）
layout(location = 0) in vec2 inFragmentUV[];
layout(location = 1) in vec3 inWorldPos[];
layout(location = 2) in vec3 inWorldNormal[];
layout(location = 3) in vec3 inWorldTangent[];
layout(location = 4) in vec3 inWorldBitangent[];

// 出力変数（フラグメントシェーダーへの入力）
layout(location = 0) out vec2 outFragmentUV;
layout(location = 1) out vec3 outWorldPos;
layout(location = 2) out vec3 outWorldNormal;
layout(location = 3) out vec3 outWorldTangent;
layout(location = 4) out vec3 outWorldBitangent;
layout(location = 5) out flat int outIsOutline;  // 新しい出力: アウトラインかどうか

// プッシュコンスタント（既存の構造体を拡張）
layout(push_constant) uniform PushConstant {
    mat4 view;
    mat4 proj;
    vec4 outlineColor;    // 追加: アウトライン色
    float outlineWidth;   // 追加: アウトライン幅
} push;

void main() {
    // 1. まず拡大したモデル（アウトライン）を描画
    vec3 centerPos = (inWorldPos[0] + inWorldPos[1] + inWorldPos[2]) / 3.0;
    
    // 三角形の法線を計算（フロントフェイス判定用）
    vec3 edge1 = inWorldPos[1] - inWorldPos[0];
    vec3 edge2 = inWorldPos[2] - inWorldPos[0];
    vec3 faceNormal = normalize(cross(edge1, edge2));
    
    // 視線方向を計算
    vec3 cameraPos = vec3(push.view[0][3], push.view[1][3], push.view[2][3]);
    vec3 viewDir = normalize(cameraPos - centerPos);
    
    // フロントフェイスかどうか判定
    float NdotV = dot(faceNormal, viewDir);
    
    // バックフェイスのみ描画（アウトラインとして）
    if (NdotV < 0.0) {  // バックフェイスの場合
        for (int i = 0; i < 3; i++) {
            outFragmentUV = inFragmentUV[i];
            outWorldPos = inWorldPos[i];
            outWorldNormal = inWorldNormal[i];
            outWorldTangent = inWorldTangent[i];
            outWorldBitangent = inWorldBitangent[i];
            outIsOutline = 1;  // アウトライン
            
            // スケーリングでサイズを大きくする
            vec3 dirFromCenter = normalize(inWorldPos[i] - centerPos);
            vec3 scaledPos = centerPos + (inWorldPos[i] - centerPos) * (1.0 + push.outlineWidth);
            gl_Position = push.proj * push.view * vec4(scaledPos, 1.0);
            
            // 深度値を意図的に遠くに移動（Z-fightingの防止）
            vec4 position = gl_Position;
            position.z += 0.001 * position.w; // 正の値で背面に移動
            
            gl_Position = position;
            EmitVertex();
        }
        EndPrimitive();
    }
    
    // 2. 次に通常のモデルを描画
    for (int i = 0; i < 3; i++) {
        outFragmentUV = inFragmentUV[i];
        outWorldPos = inWorldPos[i];
        outWorldNormal = inWorldNormal[i];
        outWorldTangent = inWorldTangent[i];
        outWorldBitangent = inWorldBitangent[i];
        outIsOutline = 0;  // 通常のポリゴン
        gl_Position = gl_in[i].gl_Position;
        EmitVertex();
    }
    EndPrimitive();
}