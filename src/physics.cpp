#include <iostream>
#include <vector>
#include "pelican/physics.hpp"
#include <btBulletDynamicsCommon.h>

namespace pl {

	void setupWorld(btDiscreteDynamicsWorld*& dynamicsWorld, btBroadphaseInterface*& broadphase,
		btDefaultCollisionConfiguration*& collisionConfiguration, btCollisionDispatcher*& dispatcher,
		btSequentialImpulseConstraintSolver*& solver) {

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

	void applyGravitationalForce(btDiscreteDynamicsWorld* dynamicsWorld, const std::vector<btRigidBody*>& spheres, float G, bool isAttraction) {

	}

    static unsigned int CountBallsInDomain(btVector3 left_boundary_point, btVector3 right_boundary_point, std::vector<pl::Physics::MagneticBall> balls) {
        unsigned int count = 0;
        for (const auto& ball : balls) {
            btVector3 pos = ball.rigidbody.getCenterOfMassPosition();
            if (pos.getX() >= left_boundary_point.getX() && pos.getX() <= right_boundary_point.getX() &&
                pos.getY() >= left_boundary_point.getY() && pos.getY() <= right_boundary_point.getY() &&
                pos.getZ() >= left_boundary_point.getZ() && pos.getZ() <= right_boundary_point.getZ()) {
                count++;
            }
        }
        return count;
    }
    btVector3 pos;
}