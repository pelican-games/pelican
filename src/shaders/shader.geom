#version 450
layout(triangles) in;

layout(location = 0) in vec3 geomPos[];
layout(location = 1) in vec2 geomUV[];

layout(triangle_strip, max_vertices = 3) out;

layout(location = 0) out vec3 fragPos;
layout(location = 2) out vec3 fragNormal;

void main() {
    // 三角形の3頂点からベクトルを計算
    vec3 edge1 = geomPos[1] - geomPos[0];
    vec3 edge2 = geomPos[2] - geomPos[0];

    // 法線を計算し正規化
    vec3 normal = normalize(cross(edge1, edge2)) * vec3(-1);

    for (int i = 0; i < 3; i++) {
        vec4 offsetPosition = gl_in[i].gl_Position;
        gl_Position = gl_in[i].gl_Position;
        fragNormal = normal;  // 入力色をそのまま出力
        //fragDepth = geomPos[i];  // デプス値を計算して出力
        //fragIndex = geomIndex[i];  // インデックスを出力
        //fragObjPos = geomObjPos[i];  // オブジェクト座標を出力
        EmitVertex();
    }
    EndPrimitive();

}
