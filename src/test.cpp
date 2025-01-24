#include "pelican/pelican.hpp"
#include <iostream>
#include <vector>
#include <AL/alut.h>
#include <thread>
#include <GLFW/glfw3.h>
#include <filesystem>
#include "tiny_gltf.h"
#include <btBulletDynamicsCommon.h>
using namespace pl;
constexpr unsigned int WIDTH = 800, HEIGHT = 1200;
/*struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 tangent;

    //可変要素
    glm::vec2 texCoord;
    glm::vec4 color;
    glm::uvec4 joint;
    glm::vec4 weight;

    static vk::VertexInputBindingDescription getBindingDescription() {
        return vk::VertexInputBindingDescription(0, sizeof(Vertex), vk::VertexInputRate::eVertex);
    }

    static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, position)),
            vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)),
            vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, tangent)),
            vk::VertexInputAttributeDescription(3, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord)),
            vk::VertexInputAttributeDescription(4, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, color)),
            vk::VertexInputAttributeDescription(5, 0, vk::Format::eR32G32B32A32Uint, offsetof(Vertex, joint)),
            vk::VertexInputAttributeDescription(6, 0, vk::Format::eR32G32B32A32Sfloat, offsetof(Vertex, weight))
        };
    }
};

struct Material {
    glm::vec4 baseColorFactor;
    float metallicFactor;
    float roughnessFactor;

    vk::UniqueImage baseColorTexture;
    vk::UniqueImageView baseColorTextureView;
    vk::UniqueSampler baseColorTextureSampler;

    vk::UniqueImage metallicRoughnessTexture;
    vk::UniqueImageView metallicRoughnessTextureView;
    vk::UniqueSampler metallicRoughnessTextureSampler;

    vk::UniqueImage normalTexture;
    vk::UniqueImageView normalTextureView;
    vk::UniqueSampler normalTextureSampler;

    vk::UniqueImage occlusionTexture;
    vk::UniqueImageView occlusionTextureView;
    vk::UniqueSampler occlusionTextureSampler;

    vk::UniqueImage emissiveTexture;
    vk::UniqueImageView emissiveTextureView;
    vk::UniqueSampler emissiveTextureSampler;
};

struct Primitive {
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    uint32_t materialIndex;
};

struct Mesh {
    std::vector<Primitive> primitives;
};

struct Transform {
    glm::vec3 translation;
    glm::quat rotation;
    glm::vec3 scale;
};

struct Object {
    bool Instance;
    Mesh mesh;
    std::vector<glm::mat4> model;

    static vk::VertexInputBindingDescription getBindingDescription() {
        return vk::VertexInputBindingDescription(1, sizeof(glm::vec4) * 4, vk::VertexInputRate::eInstance);
    }

    static std::vector<vk::VertexInputAttributeDescription> getAttributeDescriptions() {
        return {
            vk::VertexInputAttributeDescription(7, 1, vk::Format::eR32G32B32A32Sfloat, 0),
            vk::VertexInputAttributeDescription(8, 1, vk::Format::eR32G32B32A32Sfloat, sizeof(glm::vec4)),
            vk::VertexInputAttributeDescription(9, 1, vk::Format::eR32G32B32A32Sfloat, 2 * sizeof(glm::vec4)),
            vk::VertexInputAttributeDescription(10, 1, vk::Format::eR32G32B32A32Sfloat, 3 * sizeof(glm::vec4))
        };
    }
};
constexpr float M_PI = 3.14159265398979;
bool isAttraction = true; // 引力が有効かどうかを管理するフラグ

bool allSpheresClose(const std::vector<btRigidBody*>& spheres, float threshold) {
    btVector3 avgPosition(0, 0, 0);
    for (const auto& sphere : spheres) {
        avgPosition += sphere->getCenterOfMassPosition();
    }
    avgPosition /= spheres.size();

    for (const auto& sphere : spheres) {
        btVector3 pos = sphere->getCenterOfMassPosition();
        if ((pos - avgPosition).length2() > threshold * threshold) {
            return false; // 1つでも閾値以上に離れている場合
        }
    }
    return true;
}

// 引力または斥力を適用する関数
void applyGravitationalForce(btDiscreteDynamicsWorld* dynamicsWorld, const std::vector<btRigidBody*>& spheres, float G, bool isAttraction) {
    for (size_t i = 0; i < spheres.size(); ++i) {
        for (size_t j = i + 1; j < spheres.size(); ++j) {
            btVector3 posI = spheres[i]->getCenterOfMassPosition();
            btVector3 posJ = spheres[j]->getCenterOfMassPosition();

            btVector3 direction = posJ - posI;
            float distanceSquared = direction.length2();
            if (distanceSquared < 1e-4f) continue;

            direction.normalize();
            float forceMagnitude = G / distanceSquared;
            btVector3 force = direction * forceMagnitude;

            if (!isAttraction) {
                force = -force; // 斥力の場合、力の方向を逆にする
            }

            // iに力を加える
            spheres[i]->applyCentralForce(force);

            // jに反作用を加える
            spheres[j]->applyCentralForce(-force);
        }
    }
}

void setupWorld(btDiscreteDynamicsWorld*& dynamicsWorld, btBroadphaseInterface*& broadphase,
    btDefaultCollisionConfiguration*& collisionConfiguration, btCollisionDispatcher*& dispatcher,
    btSequentialImpulseConstraintSolver*& solver) {
    broadphase = new btDbvtBroadphase();
    collisionConfiguration = new btDefaultCollisionConfiguration();
    dispatcher = new btCollisionDispatcher(collisionConfiguration);
    solver = new btSequentialImpulseConstraintSolver();

    dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0, 0, 0));
}

//レンダラーのGeomerty.hppを読み込んでやります
btRigidBody* createConvexHullRigidBody(const Mesh& mesh, float mass, const btVector3& position) {
    // Convex Hull Shapeを作成
    btConvexHullShape* convexShape = new btConvexHullShape();

    // MeshのすべてのPrimitiveを処理
    for (const Primitive& primitive : mesh.primitives) {
        const std::vector<Vertex>& vertices = primitive.vertices;

        // 頂点をConvex Hull Shapeに追加
        for (const Vertex& vertex : vertices) {
            btVector3 btVertex(vertex.position.x, vertex.position.y, vertex.position.z);
            convexShape->addPoint(btVertex, false); // 再計算は最後にまとめて行う
        }
    }

    // Convex Hull Shapeの再計算
    convexShape->optimizeConvexHull();
    convexShape->recalcLocalAabb();
    
    // モーションステートを作成（初期位置を設定）
    btDefaultMotionState* motionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), position));

    // 質量と慣性を計算
    btVector3 localInertia(0, 0, 0);
    if (mass > 0.0f) {
        convexShape->calculateLocalInertia(mass, localInertia);
    }

    // 剛体の情報を設定
    btRigidBody::btRigidBodyConstructionInfo rbInfo(mass, motionState, convexShape, localInertia);
    btRigidBody* rigidBody = new btRigidBody(rbInfo);

    return rigidBody;
}

btRigidBody* addSphere(btDiscreteDynamicsWorld* dynamicsWorld, float radius, float mass,
    const btVector3& position, const btVector3& velocity) {
    btCollisionShape* sphereShape = new btSphereShape(radius);

    btDefaultMotionState* motionState = new btDefaultMotionState(
        btTransform(btQuaternion(0, 0, 0, 1), position));
    btVector3 inertia(0, 0, 0);
    if (mass > 0.0) {
        sphereShape->calculateLocalInertia(mass, inertia);
    }
    btRigidBody::btRigidBodyConstructionInfo sphereRigidBodyCI(mass, motionState, sphereShape, inertia);
    btRigidBody* sphereRigidBody = new btRigidBody(sphereRigidBodyCI);

    sphereRigidBody->setActivationState(DISABLE_DEACTIVATION);

    sphereRigidBody->setRestitution(0.0f); // 完全非弾性衝突
    sphereRigidBody->setFriction(0.0f);    // 摩擦を無効化
    sphereRigidBody->setLinearVelocity(velocity);

    dynamicsWorld->addRigidBody(sphereRigidBody);
    return sphereRigidBody;
}
void placeSpheresInCircle(btDiscreteDynamicsWorld* dynamicsWorld,
    std::vector<btRigidBody*>& spheres,
    int numSpheres, float circleRadius,
    float sphereRadius, float sphereMass) {
    for (int i = 0; i < numSpheres; ++i) {
        // 等間隔の角度を計算
        float angle = 2.0f * M_PI * i / numSpheres;

        // sin, cos を使用して円周上の座標を計算
        float x = circleRadius * std::cos(angle);
        float y = circleRadius * std::sin(angle);
        float z = sphereRadius; // 高さは球の半径に設定

        // 初期速度はゼロ
        btVector3 position(x, y, z);
        btVector3 velocity(0, 0, 0);

        // 球を作成して追加
        spheres.push_back(addSphere(dynamicsWorld, sphereRadius, sphereMass, position, velocity));
    }
}
// カスタム衝突フィルタリング
class CustomCollisionFilterCallback : public btOverlapFilterCallback {
public:
    // 返り値がtrueの場合、衝突を有効化
    bool needBroadphaseCollision(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1) const override {
        // 衝突を無効化する場合はfalseを返す
        return false; // ここで球同士の衝突を完全無効化
    }
};

void addGround(btDiscreteDynamicsWorld* dynamicsWorld) {
    btCollisionShape* groundShape = new btStaticPlaneShape(btVector3(0, 0, 1), 0);

    btDefaultMotionState* groundMotionState = new btDefaultMotionState(
        btTransform(btQuaternion(0, 0, 0, 1), btVector3(0, 0, 0)));
    btRigidBody::btRigidBodyConstructionInfo groundRigidBodyCI(0, groundMotionState, groundShape);
    btRigidBody* groundRigidBody = new btRigidBody(groundRigidBodyCI);

    groundRigidBody->setRestitution(0.5f);
    dynamicsWorld->addRigidBody(groundRigidBody);
}

void cleanupWorld(btDiscreteDynamicsWorld* dynamicsWorld, std::vector<btRigidBody*>& bodies,
    btBroadphaseInterface* broadphase, btDefaultCollisionConfiguration* collisionConfiguration,
    btCollisionDispatcher* dispatcher, btSequentialImpulseConstraintSolver* solver) {
    for (auto body : bodies) {
        delete body->getMotionState();
        dynamicsWorld->removeRigidBody(body);
        delete body;
    }

    delete dynamicsWorld;
    delete solver;
    delete dispatcher;
    delete collisionConfiguration;
    delete broadphase;
}


int main() {
    // Bullet Physicsのセットアップ
    btDiscreteDynamicsWorld* dynamicsWorld;
    btBroadphaseInterface* broadphase;
    btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btSequentialImpulseConstraintSolver* solver;

    setupWorld(dynamicsWorld, broadphase, collisionConfiguration, dispatcher, solver);
    addGround(dynamicsWorld);

    std::vector<btRigidBody*> spheres;
    const int numSpheres = 20;         // 球の数
    const float circleRadius = 10.0f;  // 円の半径
    const float sphereRadius = 0.2f;   // 球の半径
    const float sphereMass = 10.0f;    // 球の質量

    // 円形に球を配置
    placeSpheresInCircle(dynamicsWorld, spheres, numSpheres, circleRadius, sphereRadius, sphereMass);

    // デモ用のMeshデータ（適切に初期化する必要があります）
    Mesh demoMesh = {
        {
            { // Primitive 1
                {   // 頂点データ
                    {{0, 0, 0}, {0, 0, 1}, {0, 0, 0, 0}, {0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {1, 0, 0, 0}},
                    {{1, 0, 0}, {0, 0, 1}, {0, 0, 0, 0}, {0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {1, 0, 0, 0}},
                    {{0, 1, 0}, {0, 0, 1}, {0, 0, 0, 0}, {0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {1, 0, 0, 0}}
                },
                {0, 1, 2} // インデックス
            }
        }
    };

    // 凸包の剛体を作成してシミュレーションに追加
    float convexMass = 5.0f; // 凸包剛体の質量
    btVector3 initialPosition(5, 5, 5); // 初期位置
    btRigidBody* convexRigidBody = createConvexHullRigidBody(demoMesh, convexMass, initialPosition);
    dynamicsWorld->addRigidBody(convexRigidBody);

    // シミュレーションの実行
    const int steps = 10 * 50;         // シミュレーションステップ
    const float G = 100.0f;            // 万有引力定数
    const float closeThreshold = 1.0f; // 近接の閾値

    for (int i = 0; i < steps; ++i) {
        if (isAttraction && allSpheresClose(spheres, closeThreshold)) {
            for (int j = 0; j < 1000; j++) {
                std::cout << "repulsion\n";
            }
            isAttraction = false;
        }

        applyGravitationalForce(dynamicsWorld, spheres, G, isAttraction);
        dynamicsWorld->stepSimulation(1 / 60.f, 10);

        std::cout << "Step " << i << ":\n";
        for (size_t j = 0; j < spheres.size(); ++j) {
            btTransform trans;
            spheres[j]->getMotionState()->getWorldTransform(trans);
            auto origin = trans.getOrigin();
            auto velocity = spheres[j]->getLinearVelocity();
            std::cout << "  Sphere " << j << " - Position: (" << origin.getX() << ", " << origin.getY() << ", " << origin.getZ()
                << "), Velocity: (" << velocity.getX() << ", " << velocity.getY() << ", " << velocity.getZ() << ")\n";
        }
    }

    cleanupWorld(dynamicsWorld, spheres, broadphase, collisionConfiguration, dispatcher, solver);
    return 0;
}*/

int main() {
    pl::System sys(HEIGHT,WIDTH);
    pl::Renderer& renderer = sys.getDefaultRenderer();
    std::cout << "Renderer loaded" << std::endl;
    pl::Model test_model = renderer.loadModel("example/assets/test.glb");
    std::cout << "Model loaded" << std::endl;
    //renderer.loadObject("example/assets/test.glb");
    std::cout << "Model loaded" << std::endl;
    renderer.setup();

    std::cout << "Game is Start now!" << std::endl;
    btDiscreteDynamicsWorld* dynamicsWorld;
    btBroadphaseInterface* broadphase;
    btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btSequentialImpulseConstraintSolver* solver;
    pl::setupWorld(dynamicsWorld, broadphase, collisionConfiguration, dispatcher, solver);
    pl::MagneticBall ball(dynamicsWorld, 1.0f, 1.0f, 1.0f, 0);
    btVector3 initialVelocity(5.0f, 0.0f, 0.0f);
    ball.rigidbody->setLinearVelocity(initialVelocity);
    while (sys.frameUpdate()) {
        btVector3 force(10.0f, 0, 0);
        ball.rigidbody->applyCentralForce(force);
        btTransform transform;
        ball.rigidbody->getMotionState()->getWorldTransform(transform);
        std::cout << transform.getOrigin().getX() << " " << transform.getOrigin().getY() << " " << transform.getOrigin().getZ() << std::endl;
        dynamicsWorld->stepSimulation(1 / 60.f, 10);
        renderer.setCamera(glm::vec3(2.0, 0.0, 0.0), glm::vec3(-1.0, 0.0, 0.0), glm::vec3(0.0, 1.0, 0.0));
        renderer.setObjectData();
    }
    std::cout << "Game is correctly End." << std::endl;
}
