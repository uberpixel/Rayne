//
//  RNBulletRigidBody.h
//  Rayne-Bullet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_BULLETRIGIDBODY_H_
#define __RAYNE_BULLETRIGIDBODY_H_

#include "RNBullet.h"
#include "RNBulletCollisionObject.h"
#include "RNBulletShape.h"

class btRigidBody;
namespace RN
{
	class BulletRigidBodyMotionState;
	class BulletRigidBody : public BulletCollisionObject
	{
	public:
		BTAPI BulletRigidBody(BulletShape *shape, float mass);
		BTAPI BulletRigidBody(BulletShape *shape, float mass, const Vector3 &inertia);
			
		BTAPI ~BulletRigidBody() override;
			
		BTAPI static BulletRigidBody *WithShape(BulletShape *shape, float mass);
		BTAPI static BulletRigidBody *WithShapeAndInertia(BulletShape *shape, float mass, const Vector3 &inertia);
			
		BTAPI void SetMass(float mass);
		BTAPI void SetMass(float mass, const Vector3 &inertia);
		BTAPI void SetLinearVelocity(const Vector3 &velocity);
		BTAPI void SetAngularVelocity(const Vector3 &velocity);
		BTAPI void SetCCDMotionThreshold(float threshold);
		BTAPI void SetCCDSweptSphereRadius(float radius);
		BTAPI void SetGravity(const Vector3 &gravity);
		BTAPI void SetDamping(float linear, float angular);
		BTAPI void SetAllowDeactivation(bool canDeactivate);
        BTAPI void Activate();
			
		BTAPI void ApplyForce(const Vector3 &force);
		BTAPI void ApplyForce(const Vector3 &force, const Vector3 &origin);
		BTAPI void ClearForces();
			
		BTAPI void ApplyTorque(const Vector3 &torque);
		BTAPI void ApplyTorqueImpulse(const Vector3 &torque);
		BTAPI void ApplyImpulse(const Vector3 &impulse);
		BTAPI void ApplyImpulse(const Vector3 &impulse, const Vector3 &origin);
			
		BTAPI Vector3 GetLinearVelocity() const;
		BTAPI Vector3 GetAngularVelocity() const;
			
		BTAPI Vector3 GetCenterOfMass() const;
		BTAPI Matrix GetCenterOfMassTransform() const;
			
		BTAPI btCollisionObject *GetBulletCollisionObject() const override;
		BTAPI btRigidBody *GetBulletRigidBody() { return _rigidBody; }

		BTAPI void SetPositionOffset(RN::Vector3 offset) final;
			
	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
		void UpdateFromMaterial(BulletMaterial *material) override;
		
		void InsertIntoWorld(BulletWorld *world) override;
		void RemoveFromWorld(BulletWorld *world) override;
			
	private:
		BulletShape *_shape;
		btRigidBody *_rigidBody;
		BulletRigidBodyMotionState *_motionState;
			
		RNDeclareMetaAPI(BulletRigidBody, BTAPI)
	};
}

#endif /* defined(__RAYNE_BULLETRIGIDBODY_H_) */
