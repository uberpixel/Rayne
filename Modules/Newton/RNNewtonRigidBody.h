//
//  RNNewtonRigidBody.h
//  Rayne-Newton
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_NEWTONRIGIDBODY_H_
#define __RAYNE_NEWTONRIGIDBODY_H_

#include "RNNewtonCollisionObject.h"

class NewtonBody;

namespace RN
{
	class NewtonShape;
	class NewtonRigidBody : public NewtonCollisionObject
	{
	public:
		NDAPI NewtonRigidBody(NewtonShape *shape, float mass);
		NDAPI ~NewtonRigidBody() override;
			
		NDAPI static NewtonRigidBody *WithShape(NewtonShape *shape, float mass);

		NDAPI void UpdatePosition() override;
			
		NDAPI void SetMass(float mass);
		NDAPI void SetLinearVelocity(const Vector3 &velocity);
		NDAPI void SetAngularVelocity(const Vector3 &velocity);
		NDAPI void SetDamping(float linear, float angular);
		NDAPI void SetEnableCCD(bool enable);
		NDAPI void SetEnableGravity(bool enable);
/*
		PXAPI void ApplyForce(const Vector3 &force);
		PXAPI void ApplyForce(const Vector3 &force, const Vector3 &origin);
		PXAPI void ClearForces();
			
		PXAPI void ApplyTorque(const Vector3 &torque);
		PXAPI void ApplyTorqueImpulse(const Vector3 &torque);
		PXAPI void ApplyImpulse(const Vector3 &impulse);
		PXAPI void ApplyImpulse(const Vector3 &impulse, const Vector3 &origin);*/
			
		NDAPI Vector3 GetLinearVelocity() const;
		NDAPI Vector3 GetAngularVelocity() const;

//		NDAPI bool SweepTest(std::vector<PhysXContactInfo> &contactInfo, const Vector3 &direction, const Vector3 &offsetPosition = Vector3(), const Quaternion &offsetRotation = Quaternion()) const;
//		NDAPI Quaternion RotationSweepTest(std::vector<PhysXContactInfo> &contactInfo, const Quaternion &targetRoation, float stepSize, float sweepSize, const Vector3 &offsetPosition = Vector3(), const Quaternion &offsetRotation = Quaternion()) const;

		NDAPI::NewtonBody *GetNewtonBody() const { return _body; }
			
	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
//		void UpdateFromMaterial(BulletMaterial *material) override;
			
	private:
		static void ForceAndTorqueCallback(const NewtonBody *body, float delta, int threadIndex);
		static void TransformCallback(const NewtonBody *body, const float *matrix, int threadIndex);

		NewtonShape *_shape;
		::NewtonBody *_body;

		bool _useGravity;
			
		RNDeclareMetaAPI(NewtonRigidBody, NDAPI)
	};
}

#endif /* defined(__RAYNE_NEWTONRIGIDBODY_H_) */
