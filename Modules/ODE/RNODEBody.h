//
//  RNODEBody.h
//  Rayne-ODE
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_ODEBODY_H_
#define __RAYNE_ODEBODY_H_

#include "RNODE.h"
#include "RNODECollisionObject.h"
#include "RNODEShape.h"

struct dxBody;

namespace RN
{
	class ODEBody : public ODECollisionObject
	{
	public:
		ODEAPI ODEBody(ODEShape *shape, float mass);
		ODEAPI ODEBody(ODEShape *shape, float mass, const Vector3 &inertia);
			
		ODEAPI ~ODEBody() override;
			
		ODEAPI static ODEBody *WithShape(ODEShape *shape, float mass);
			
		ODEAPI void SetMass(float mass);
		ODEAPI void SetMass(float mass, const Vector3 &inertia);
		ODEAPI void SetLinearVelocity(const Vector3 &velocity);
		ODEAPI void SetAngularVelocity(const Vector3 &velocity);
		ODEAPI void SetCCDMotionThreshold(float threshold);
		ODEAPI void SetCCDSweptSphereRadius(float radius);
		ODEAPI void SetGravity(const Vector3 &gravity);
		ODEAPI void SetDamping(float linear, float angular);
		ODEAPI void SetAllowDeactivation(bool canDeactivate);
			
		ODEAPI void ApplyForce(const Vector3 &force);
		ODEAPI void ApplyForce(const Vector3 &force, const Vector3 &origin);
		ODEAPI void ClearForces();
			
		ODEAPI void ApplyTorque(const Vector3 &torque);
		ODEAPI void ApplyTorqueImpulse(const Vector3 &torque);
		ODEAPI void ApplyImpulse(const Vector3 &impulse);
		ODEAPI void ApplyImpulse(const Vector3 &impulse, const Vector3 &origin);
			
		ODEAPI Vector3 GetLinearVelocity() const;
		ODEAPI Vector3 GetAngularVelocity() const;
			
		ODEAPI Vector3 GetCenterOfMass() const;
		ODEAPI Matrix GetCenterOfMassTransform() const;
			
//		ODEAPI btCollisionObject *GetODECollisionObject() const override;
//		ODEAPI btRigidBody *GetODERigidBody() { return _rigidBody; }

		ODEAPI void SetPositionOffset(RN::Vector3 offset) final;
		ODEAPI void AccelerateToTarget(const Vector3 &position, const Quaternion &rotation, float delta);

		ODEAPI void Update(float delta) final;
			
	protected:
		void DidUpdate(SceneNode::ChangeSet changeSet) override;
//		void UpdateFromMaterial(BulletMaterial *material) override;
		
		void InsertIntoWorld(ODEWorld *world) override;
		void RemoveFromWorld(ODEWorld *world) override;
			
	private:
		ODEShape *_shape;
		dxBody *_body;
		dxGeom *_geometry;
		float _mass;
//		BulletRigidBodyMotionState *_motionState;
			
		RNDeclareMetaAPI(ODEBody, ODEAPI)
	};
}

#endif /* defined(__RAYNE_ODEBODY_H_) */
