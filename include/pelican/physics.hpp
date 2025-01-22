#ifndef PELICAN_PHYSICS_HPP
#define PELICAN_PHYSICS_HPP
#include <iostream>
#include <vector>
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>

namespace pl {
	class Physics {
	public:
		class MagneticBall {
		public:
			btRigidBody* rigidbody;
			float mass;
			float radius;
			float magneticStrength;
			int polarity;
			MagneticBall(btDynamicsWorld* world,btRigidBody* body, float strength, float mas,float rad,int pol)
				:rigidbody(body), magneticStrength(strength),mass(mas),radius(rad), polarity(pol) {}
			~MagneticBall();
		};
		void setupWorld(btDiscreteDynamicsWorld*& dynamicsWorld, btBroadphaseInterface*& broadphase,
			btDefaultCollisionConfiguration*& collisionConfiguration, btCollisionDispatcher*& dispatcher,
			btSequentialImpulseConstraintSolver*& solver);

		void resetBall(MagneticBall ball);
		btRigidBody* createConvexHullRigidBody(const Mesh& mesh, float mass, const btVector3& position);

		btRigidBody* addSphere(btDiscreteDynamicsWorld* dynamicsWorld, float radius, float mass,
			const btVector3& position, const btVector3& velocity);

		void addGround(btDiscreteDynamicsWorld* dynamicsWorld);

		void cleanupWorld(btDiscreteDynamicsWorld* dynamicsWorld, std::vector<btRigidBody*>& bodies,
			btBroadphaseInterface* broadphase, btDefaultCollisionConfiguration* collisionConfiguration,
			btCollisionDispatcher* dispatcher, btSequentialImpulseConstraintSolver* solver);

		unsigned int countBallsInDomain(btVector3 left_boundary_point, btVector3 right_boundary_point,std::vector<MagneticBall> balls);
	};
}

#endif