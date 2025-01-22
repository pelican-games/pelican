#include <iostream>
#include <vector>
#include "pelican/physics.hpp"
#include <btBulletDynamicsCommon.h>

namespace pl {
    Physics::MagneticBall::MagneticBall(btDynamicsWorld* world,btRigidBody* body, float strength, float mas, float rad, int pol) {
        btCollisionShape* shape = new btSphereShape(radius);
        btVector3 localInertia(0, 0, 0);
        shape->calculateLocalInertia(mass, localInertia);
        btTransform startTransform;
        startTransform.setIdentity();
        btDefaultMotionState* motionState = new btDefaultMotionState(startTransform);
        btRigidBody::btRigidBodyConstructionInfo bodyCI(mass, motionState, shape, localInertia);
        rigidbody = new btRigidBody(bodyCI);
        mass = mas;
        magneticStrength = strength;
        polarity = pol;

        world->addRigidBody(rigidbody);
    }
    Physics::MagneticBall::~MagneticBall() {
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
        dynamicsWorld->setGravity(btVector3(0, 0, 0));
	}
	btRigidBody* createConvexHullRigidBody(const Mesh& mesh, float mass, const btVector3& position) {

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

    
    void resetBall(pl::Physics::MagneticBall ball,btVector3 resetPoint) {
        btTransform newTransform;
        newTransform.setIdentity();
        newTransform.setOrigin(resetPoint);
        ball.rigidbody->setWorldTransform(newTransform);
        ball.rigidbody->setLinearVelocity(btVector3(0, 0, 0));
        ball.rigidbody->setAngularVelocity(btVector3(0, 0, 0));
    }

    static unsigned int countBallsInDomain(btVector3 left_boundary_point, btVector3 right_boundary_point, std::vector<pl::Physics::MagneticBall> balls) {
        unsigned int count = 0;
        for (auto& ball : balls) {
            btVector3 pos = ball.rigidbody->getCenterOfMassPosition();
            if (pos.getX() >= left_boundary_point.getX() && pos.getX() <= right_boundary_point.getX() &&
                pos.getY() >= left_boundary_point.getY() && pos.getY() <= right_boundary_point.getY() &&
                pos.getZ() >= left_boundary_point.getZ() && pos.getZ() <= right_boundary_point.getZ()) {
                count++;
            }
        }
        return count;
    }

}