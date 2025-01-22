#include "pelican/physics.hpp"

namespace pl {

    MagneticBall::MagneticBall(btDiscreteDynamicsWorld* world, float strength, float mas, float rad, int pol)
        : magneticStrength(strength), mass(mas), radius(rad), polarity(pol) {
        btCollisionShape* shape = new btSphereShape(radius);

        btVector3 localInertia(0, 0, 0);
        if (mass > 0) {
            shape->calculateLocalInertia(mass, localInertia);
        }

        btTransform startTransform;
        startTransform.setIdentity();

        btDefaultMotionState* motionState = new btDefaultMotionState(startTransform);
        btRigidBody::btRigidBodyConstructionInfo bodyCI(mass, motionState, shape, localInertia);
        rigidbody = new btRigidBody(bodyCI);
        world->addRigidBody(rigidbody);
    }

    MagneticBall::~MagneticBall() {
        delete rigidbody->getMotionState();
        delete rigidbody->getCollisionShape();
        delete rigidbody;
    }

    void setupWorld(btDiscreteDynamicsWorld*& dynamicsWorld, btBroadphaseInterface*& broadphase,
        btDefaultCollisionConfiguration*& collisionConfiguration, btCollisionDispatcher*& dispatcher,
        btSequentialImpulseConstraintSolver*& solver) {
        broadphase = new btDbvtBroadphase();
        collisionConfiguration = new btDefaultCollisionConfiguration();
        dispatcher = new btCollisionDispatcher(collisionConfiguration);
        solver = new btSequentialImpulseConstraintSolver();

        dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, broadphase, solver, collisionConfiguration);
        dynamicsWorld->setGravity(btVector3(0, 0, -9.8));
    }

    void resetBall(MagneticBall& ball, const btVector3& resetPoint) {
        btTransform newTransform;
        newTransform.setIdentity();
        newTransform.setOrigin(resetPoint);
        ball.rigidbody->setWorldTransform(newTransform);
        ball.rigidbody->setLinearVelocity(btVector3(0, 0, 0));
        ball.rigidbody->setAngularVelocity(btVector3(0, 0, 0));
    }

    unsigned int countBallsInDomain(const btVector3& leftBoundaryPoint, const btVector3& rightBoundaryPoint,
        const std::vector<MagneticBall>& balls) {
        unsigned int count = 0;
        for (const auto& ball : balls) {
            btVector3 pos = ball.rigidbody->getCenterOfMassPosition();
            if (pos.getX() >= leftBoundaryPoint.getX() && pos.getX() <= rightBoundaryPoint.getX() &&
                pos.getY() >= leftBoundaryPoint.getY() && pos.getY() <= rightBoundaryPoint.getY() &&
                pos.getZ() >= leftBoundaryPoint.getZ() && pos.getZ() <= rightBoundaryPoint.getZ()) {
                count++;
            }
        }
        return count;
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

}