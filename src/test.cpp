#include "pelican/pelican.hpp"
#include <iostream>
#include <vector>
#include <AL/alut.h>
#include <thread>
#include <GLFW/glfw3.h>
#include <filesystem>
#include "tiny_gltf.h"
#include <btBulletDynamicsCommon.h>
struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 tangent;

    //�ϗv�f
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
bool isAttraction = true; // ���͂��L�����ǂ������Ǘ�����t���O

bool allSpheresClose(const std::vector<btRigidBody*>& spheres, float threshold) {
    btVector3 avgPosition(0, 0, 0);
    for (const auto& sphere : spheres) {
        avgPosition += sphere->getCenterOfMassPosition();
    }
    avgPosition /= spheres.size();

    for (const auto& sphere : spheres) {
        btVector3 pos = sphere->getCenterOfMassPosition();
        if ((pos - avgPosition).length2() > threshold * threshold) {
            return false; // 1�ł�臒l�ȏ�ɗ���Ă���ꍇ
        }
    }
    return true;
}

// ���͂܂��͐˗͂�K�p����֐�
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
                force = -force; // �˗͂̏ꍇ�A�͂̕������t�ɂ���
            }

            // i�ɗ͂�������
            spheres[i]->applyCentralForce(force);

            // j�ɔ���p��������
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
// �����_���[��Geomerty.hpp��ǂݍ���ł��܂�
btRigidBody* createConvexHullRigidBody(const Mesh& mesh, float mass, const btVector3& position) {
    // Convex Hull Shape���쐬
    btConvexHullShape* convexShape = new btConvexHullShape();

    // Mesh�̂��ׂĂ�Primitive������
    for (const Primitive& primitive : mesh.primitives) {
        const std::vector<Vertex>& vertices = primitive.vertices;

        // ���_��Convex Hull Shape�ɒǉ�
        for (const Vertex& vertex : vertices) {
            btVector3 btVertex(vertex.position.x, vertex.position.y, vertex.position.z);
            convexShape->addPoint(btVertex, false); // �Čv�Z�͍Ō�ɂ܂Ƃ߂čs��
        }
    }

    // Convex Hull Shape�̍Čv�Z
    convexShape->optimizeConvexHull();
    convexShape->recalcLocalAabb();
    
    // ���[�V�����X�e�[�g���쐬�i�����ʒu��ݒ�j
    btDefaultMotionState* motionState = new btDefaultMotionState(btTransform(btQuaternion(0, 0, 0, 1), position));

    // ���ʂƊ������v�Z
    btVector3 localInertia(0, 0, 0);
    if (mass > 0.0f) {
        convexShape->calculateLocalInertia(mass, localInertia);
    }

    // ���̂̏���ݒ�
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

    sphereRigidBody->setRestitution(0.0f); // ���S��e���Փ�
    sphereRigidBody->setFriction(0.0f);    // ���C�𖳌���
    sphereRigidBody->setLinearVelocity(velocity);

    dynamicsWorld->addRigidBody(sphereRigidBody);
    return sphereRigidBody;
}
void placeSpheresInCircle(btDiscreteDynamicsWorld* dynamicsWorld,
    std::vector<btRigidBody*>& spheres,
    int numSpheres, float circleRadius,
    float sphereRadius, float sphereMass) {
    for (int i = 0; i < numSpheres; ++i) {
        // ���Ԋu�̊p�x���v�Z
        float angle = 2.0f * M_PI * i / numSpheres;

        // sin, cos ���g�p���ĉ~����̍��W���v�Z
        float x = circleRadius * std::cos(angle);
        float y = circleRadius * std::sin(angle);
        float z = sphereRadius; // �����͋��̔��a�ɐݒ�

        // �������x�̓[��
        btVector3 position(x, y, z);
        btVector3 velocity(0, 0, 0);

        // �����쐬���Ēǉ�
        spheres.push_back(addSphere(dynamicsWorld, sphereRadius, sphereMass, position, velocity));
    }
}
// �J�X�^���Փ˃t�B���^�����O
class CustomCollisionFilterCallback : public btOverlapFilterCallback {
public:
    // �Ԃ�l��true�̏ꍇ�A�Փ˂�L����
    bool needBroadphaseCollision(btBroadphaseProxy* proxy0, btBroadphaseProxy* proxy1) const override {
        // �Փ˂𖳌�������ꍇ��false��Ԃ�
        return false; // �����ŋ����m�̏Փ˂����S������
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
    // Bullet Physics�̃Z�b�g�A�b�v
    btDiscreteDynamicsWorld* dynamicsWorld;
    btBroadphaseInterface* broadphase;
    btDefaultCollisionConfiguration* collisionConfiguration;
    btCollisionDispatcher* dispatcher;
    btSequentialImpulseConstraintSolver* solver;

    setupWorld(dynamicsWorld, broadphase, collisionConfiguration, dispatcher, solver);
    addGround(dynamicsWorld);

    std::vector<btRigidBody*> spheres;
    const int numSpheres = 20;         // ���̐�
    const float circleRadius = 10.0f;  // �~�̔��a
    const float sphereRadius = 0.2f;   // ���̔��a
    const float sphereMass = 10.0f;    // ���̎���

    // �~�`�ɋ���z�u
    placeSpheresInCircle(dynamicsWorld, spheres, numSpheres, circleRadius, sphereRadius, sphereMass);

    // �f���p��Mesh�f�[�^�i�K�؂ɏ���������K�v������܂��j
    Mesh demoMesh = {
        {
            { // Primitive 1
                {   // ���_�f�[�^
                    {{0, 0, 0}, {0, 0, 1}, {0, 0, 0, 0}, {0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {1, 0, 0, 0}},
                    {{1, 0, 0}, {0, 0, 1}, {0, 0, 0, 0}, {0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {1, 0, 0, 0}},
                    {{0, 1, 0}, {0, 0, 1}, {0, 0, 0, 0}, {0, 0}, {1, 1, 1, 1}, {0, 0, 0, 0}, {1, 0, 0, 0}}
                },
                {0, 1, 2} // �C���f�b�N�X
            }
        }
    };

    // �ʕ�̍��̂��쐬���ăV�~�����[�V�����ɒǉ�
    float convexMass = 5.0f; // �ʕ�̂̎���
    btVector3 initialPosition(5, 5, 5); // �����ʒu
    btRigidBody* convexRigidBody = createConvexHullRigidBody(demoMesh, convexMass, initialPosition);
    dynamicsWorld->addRigidBody(convexRigidBody);

    // �V�~�����[�V�����̎��s
    const int steps = 10 * 50;         // �V�~�����[�V�����X�e�b�v
    const float G = 100.0f;            // ���L���͒萔
    const float closeThreshold = 1.0f; // �ߐڂ�臒l

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
}
