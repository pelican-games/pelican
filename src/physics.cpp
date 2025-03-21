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

        addtionalAccel = {0, 0, -9.8};
    }

    MagneticBall::~MagneticBall() {
        delete rigidbody->getMotionState();
        delete rigidbody->getCollisionShape();
        delete rigidbody;
    }

    void MagneticBall::setAddtionalAccelaration(btVector3 a) {
        addtionalAccel = a;
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

    void resetBall(MagneticBall& ball, const btVector3& resetPoint) {
        btTransform newTransform;
        newTransform.setIdentity();
        newTransform.setOrigin(resetPoint);
        ball.rigidbody->setWorldTransform(newTransform);
        ball.rigidbody->setLinearVelocity(btVector3(0, 0, 0));
        ball.rigidbody->setAngularVelocity(btVector3(0, 0, 0));
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
    std::vector<int> ballIndexesInDomain(const btVector3& leftBoundaryPoint, const btVector3& rightBoundaryPoint, const std::vector<std::unique_ptr<pl::MagneticBall>>& balls) {
        std::vector<int> indexes;
        for (size_t i = 0; i < balls.size(); i++) {
            btVector3 pos = balls[i]->rigidbody->getCenterOfMassPosition();
            if (pos.getX() >= leftBoundaryPoint.getX() && pos.getX() <= rightBoundaryPoint.getX() &&
                pos.getY() >= leftBoundaryPoint.getY() && pos.getY() <= rightBoundaryPoint.getY() &&
                pos.getZ() >= leftBoundaryPoint.getZ() && pos.getZ() <= rightBoundaryPoint.getZ()) {
                indexes.emplace_back(i);
            }
        }
        return indexes;
    }

    size_t countBallsInDomain(const btVector3& rectCenter, const btVector3& rectScale, const btQuaternion rectRotation, const std::vector<std::unique_ptr<pl::MagneticBall>>& balls) {
        size_t count = 0;
        for (size_t i = 0; i < balls.size(); i++) {
            btVector3 pos = balls[i]->rigidbody->getCenterOfMassPosition();

            pos -= rectCenter;

            const auto p2 = quatRotate(rectRotation.inverse(), pos);

            if (abs(p2.x()) < rectScale.x() / 2 &&
                abs(p2.y()) < rectScale.y() / 2 &&
                abs(p2.z()) < rectScale.z() / 2)
                count++;
        }
        return count;
    }

    void setGravityInDomain(
        btDiscreteDynamicsWorld* dynamicsWorld,
        const btVector3& rectCenter, const btVector3& rectScale, const btQuaternion rectRotation,
        const std::vector<std::unique_ptr<pl::MagneticBall>>& balls,
        const btVector3 accel) {
        for (auto& ball : balls) {
            btVector3 pos = ball->rigidbody->getCenterOfMassPosition();

            pos -= rectCenter;

            const auto p2 = quatRotate(rectRotation.inverse(), pos);

            if (abs(p2.x()) < rectScale.x() / 2 &&
                abs(p2.y()) < rectScale.y() / 2 &&
                abs(p2.z()) < rectScale.z() / 2)
                ball->setAddtionalAccelaration(accel);
        }
    }
    void applyGravity(const std::vector<std::unique_ptr<pl::MagneticBall>>& balls) {
        for (auto& ball : balls) {
            ball->rigidbody->applyCentralForce(ball->addtionalAccel);
        }
    }

    bool isObjectInDirection(btDiscreteDynamicsWorld* dynamicsWorld, const btVector3& origin, const btVector3& direction, float distance) {
        btVector3 end = origin + direction.normalized() * distance;
        btCollisionWorld::ClosestRayResultCallback rayCallback(origin, end);

        dynamicsWorld->rayTest(origin, end, rayCallback);

        return rayCallback.hasHit();
    }

    void moveKinematicModel(btRigidBody* rigidBody, const btVector3& newPosition) {
        if (rigidBody->isKinematicObject()) {
            btTransform transform;
            rigidBody->getMotionState()->getWorldTransform(transform);
            transform.setOrigin(newPosition);
            rigidBody->getMotionState()->setWorldTransform(transform);
            rigidBody->setWorldTransform(transform);
        }
    }

    void calculateMagneticForce(std::vector<std::unique_ptr<pl::MagneticBall>>& balls) {
        for (int i = 0; i < balls.size(); i++) {
            for (int j = i + 1; j < balls.size(); j++) {
                btVector3 posi = balls[i]->rigidbody->getCenterOfMassPosition();
                btVector3 posj = balls[j]->rigidbody->getCenterOfMassPosition();

                btVector3 directionVector = posj - posi;
                btVector3 direction = directionVector;
                direction.normalize();
                btScalar distance = directionVector.length2();

                btVector3 magneticForce = coulomb_Constant * balls[i]->magneticStrength * balls[j]->magneticStrength 
                    * balls[i]->polarity * balls[j]->polarity * directionVector;
                balls[i]->rigidbody->applyCentralForce(-magneticForce);
                balls[j]->rigidbody->applyCentralForce(magneticForce);
            }
        }
    }

    btRigidBody* createKinematicBodies(std::vector<Boxdata> modeldata) {
        btCompoundShape* kinematicBodies = new btCompoundShape();
        for (const auto& boxdata : modeldata) {
            btBoxShape* boxshape = new btBoxShape(boxdata.scale);
            btTransform transform;
            transform.setIdentity();
            transform.setOrigin(boxdata.translation);
            transform.setRotation(boxdata.rotation);

            kinematicBodies->addChildShape(transform, boxshape);
        }
        btTransform initialtransform;
        initialtransform.setIdentity();

        btDefaultMotionState* motionstate = new btDefaultMotionState(initialtransform);
        
        btRigidBody::btRigidBodyConstructionInfo rigidBodyCI(0, motionstate, kinematicBodies, btVector3(0, 0, 0));
        btRigidBody* rigidBody = new btRigidBody(rigidBodyCI);

        rigidBody->setCollisionFlags(rigidBody->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
        rigidBody->setActivationState(DISABLE_DEACTIVATION);

        return rigidBody;
    }

}
