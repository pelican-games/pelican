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
layout(location = 5) in vec4 inOutlineColor[];
layout(location = 6) in float inOutlineWidth[];

// 出力変数（フラグメントシェーダーへの入力）
layout(location = 0) out vec2 outFragmentUV;
layout(location = 1) out vec3 outWorldPos;
layout(location = 2) out vec3 outWorldNormal;
layout(location = 3) out vec3 outWorldTangent;
layout(location = 4) out vec3 outWorldBitangent;
layout(location = 5) out flat int outIsOutline;  // 新しい出力: アウトラインかどうか
layout(location = 6) out vec4 outOutlineColor;

// プッシュコンスタント（既存の構造体を拡張）
layout(push_constant) uniform PushConstant {
    mat4 view;
    mat4 proj;
} push;

void main() {
    // 三角形の法線を計算（フロントフェイス判定用）
    vec3 edge1 = inWorldPos[1] - inWorldPos[0];
    vec3 edge2 = inWorldPos[2] - inWorldPos[0];
    vec3 faceNormal = normalize(cross(edge1, edge2));
    
    // 視線方向を計算
    vec3 cameraPos = vec3(0.0, 0.0, 0.0); // カメラはビュー空間の原点
    vec3 viewDir = normalize((push.view * vec4(faceNormal, 0.0)).xyz);
    
    // フロントフェイスかどうか判定
    float NdotV = dot(normalize((push.view * vec4(faceNormal, 0.0)).xyz), vec3(0.0, 0.0, 1.0));
    
    // バックフェイスのみ描画（アウトラインとして）
    if (NdotV < 0.1) {  // しきい値を少し大きくして輪郭をより強調
        for (int i = 0; i < 3; i++) {
            outFragmentUV = inFragmentUV[i];
            outWorldPos = inWorldPos[i];
            outWorldNormal = inWorldNormal[i];
            outWorldTangent = inWorldTangent[i];
            outWorldBitangent = inWorldBitangent[i];
            outIsOutline = 1;  // アウトライン
            outOutlineColor = inOutlineColor[i];
            
            // スケーリングではなく頂点法線方向に押し出す
            vec4 pos = gl_in[i].gl_Position;
            vec3 normal = inWorldNormal[i];
            
            // NDC空間で一定幅になるよう調整
            vec3 viewNormal = normalize((push.view * vec4(normal, 0.0)).xyz);
            vec4 extrudedPos = pos + vec4(viewNormal.xy * inOutlineWidth[i] * pos.w, 0.0, 0.0);
            
            // // 深度値を遠くに移動（Z-fightingの防止）- 値を大きくする
            // extrudedPos.z += 0.005 * extrudedPos.w; // 0.001から0.005に増加
            
            gl_Position = extrudedPos;
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