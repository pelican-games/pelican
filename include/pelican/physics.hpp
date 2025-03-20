#ifndef PELICAN_PHYSICS_HPP
#define PELICAN_PHYSICS_HPP

#include <vector>
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>
#include <memory>

namespace pl {
    constexpr float coulomb_Constant = 0.89875;
    class MagneticBall {
    public:
        btRigidBody* rigidbody;
        float mass;
        float radius;
        float magneticStrength;
        int polarity;
        btVector3 addtionalAccel;

        MagneticBall(btDiscreteDynamicsWorld* dynamicsWorld, float strength, float mas, float rad, int pol);
        ~MagneticBall();
        
        void setAddtionalAccelaration(btVector3 a);
    };

    struct Boxdata {
        btVector3 translation;
        btQuaternion rotation;
        btVector3 scale;
    };

    void setupWorld(btDiscreteDynamicsWorld*& dynamicsWorld,
        btBroadphaseInterface*& broadphase,
        btDefaultCollisionConfiguration*& collisionConfiguration,
        btCollisionDispatcher*& dispatcher,
        btSequentialImpulseConstraintSolver*& solver);

    void resetBall(MagneticBall& ball, const btVector3& resetPoint);

    void cleanupWorld(btDiscreteDynamicsWorld* dynamicsWorld,
        std::vector<btRigidBody*>& bodies,
        btBroadphaseInterface* broadphase,
        btDefaultCollisionConfiguration* collisionConfiguration,
        btCollisionDispatcher* dispatcher,
        btSequentialImpulseConstraintSolver* solver);

    void setGravityInDomain(
        btDiscreteDynamicsWorld* dynamicsWorld,
        const btVector3& rectCenter, const btVector3& rectScale, const btQuaternion rectRotation,
        const std::vector<std::unique_ptr<pl::MagneticBall>>& balls,
        const btVector3 accel);
    void applyGravity(const std::vector<std::unique_ptr<pl::MagneticBall>>& balls);

    void moveKinematicModel(btRigidBody* rigidBody, const btVector3& newPosition);

    bool isObjectInDirection(btDiscreteDynamicsWorld* dynamicsWorld, const btVector3& origin, const btVector3& direction, float distance);

    std::vector<int> ballIndexesInDomain(const btVector3& leftBoundaryPoint, const btVector3& rightBoundaryPoint, const std::vector<std::unique_ptr<pl::MagneticBall>>& balls);
    
    size_t countBallsInDomain(const btVector3& rectCenter, const btVector3& rectScale, const btQuaternion rectRotation, const std::vector<std::unique_ptr<pl::MagneticBall>>& balls);

    void calculateMagneticForce(std::vector<std::unique_ptr<pl::MagneticBall>>& balls);

    btRigidBody* createKinematicBodies(std::vector<Boxdata> modeldata);
}

#endif