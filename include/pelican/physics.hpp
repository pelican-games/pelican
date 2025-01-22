#ifndef PELICAN_PHYSICS_HPP
#define PELICAN_PHYSICS_HPP

#include <vector>
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>

namespace pl {

    class MagneticBall {
    public:
        btRigidBody* rigidbody;
        float mass;
        float radius;
        float magneticStrength;
        int polarity;

        MagneticBall(btDiscreteDynamicsWorld* dynamicsWorld, float strength, float mas, float rad, int pol);
        ~MagneticBall();
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

    unsigned int countBallsInDomain(const btVector3& leftBoundaryPoint,
        const btVector3& rightBoundaryPoint,
        const std::vector<MagneticBall>& balls);

}

#endif