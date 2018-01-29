//
//  RNNewtonRigidBody.cpp
//  Rayne-Newton
//
//  Copyright 2018 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNNewtonRigidBody.h"
#include "RNNewtonWorld.h"
#include "Newton.h"

namespace RN
{
	RNDefineMeta(NewtonRigidBody, NewtonCollisionObject)

	void NewtonRigidBody::ForceAndTorqueCallback(const NewtonBody *body, float delta, int threadIndex)
	{
		NewtonRigidBody *userObject = static_cast<NewtonRigidBody*>(NewtonBodyGetUserData(body));
		if(userObject->_useGravity)
		{
			NewtonWorld *newtonWorld = NewtonWorld::GetSharedInstance();
			Vector3 gravity = newtonWorld->GetGravity();

			float Ixx;
			float Iyy;
			float Izz;
			float mass;
			NewtonBodyGetMass(body, &mass, &Ixx, &Iyy, &Izz);
			gravity *= mass;
			NewtonBodySetForce(body, &gravity.x);
		}

		userObject->UpdatePosition();
	}

	void NewtonRigidBody::TransformCallback(const NewtonBody *body, const float *matrix, int threadIndex)
	{
		NewtonRigidBody *userObject = static_cast<NewtonRigidBody*>(NewtonBodyGetUserData(body));
		userObject->UpdatePosition();
	}
		
	NewtonRigidBody::NewtonRigidBody(NewtonShape *shape, float mass) :
		_shape(shape->Retain()),
		_body(nullptr)
	{
		Matrix initialPose;
		::NewtonWorld *newtonInstance = NewtonWorld::GetSharedInstance()->GetNewtonInstance();
		_body = NewtonCreateDynamicBody(newtonInstance, shape->GetNewtonShape(), initialPose.m);
		NewtonBodySetUserData(_body, this);
		SetMass(mass);

		if(mass > k::EpsilonFloat)
		{
			NewtonBodySetForceAndTorqueCallback(_body, NewtonRigidBody::ForceAndTorqueCallback);
			NewtonBodySetTransformCallback(_body, TransformCallback);
		}
			
	}
		
	NewtonRigidBody::~NewtonRigidBody()
	{
		NewtonDestroyBody(_body);
		_shape->Release();
	}
	
		
	NewtonRigidBody *NewtonRigidBody::WithShape(NewtonShape *shape, float mass)
	{
		NewtonRigidBody *body = new NewtonRigidBody(shape, mass);
		return body->Autorelease();
	}
		
	void NewtonRigidBody::SetMass(float mass)
	{
		float inertia[3] = { 1, 1, 1 };
		if(mass > k::EpsilonFloat)
		{
			float origin[3];
			NewtonConvexCollisionCalculateInertialMatrix(_shape->GetNewtonShape(), inertia, origin);
			NewtonBodySetCentreOfMass(_body, origin);
		}
		
		NewtonBodySetMassMatrix(_body, mass, inertia[0], inertia[1], inertia[2]);
	}

	void NewtonRigidBody::SetLinearVelocity(const Vector3 &velocity)
	{
		NewtonBodySetVelocity(_body, &velocity.x);
	}

	void NewtonRigidBody::SetAngularVelocity(const Vector3 &velocity)
	{
		NewtonBodySetOmega(_body, &velocity.x);
	}
		
	void NewtonRigidBody::SetDamping(float linear, float angular)
	{
		NewtonBodySetLinearDamping(_body, linear);

		float angularValues[3] = {angular , angular , angular};
		NewtonBodySetAngularDamping(_body, angularValues);
	}
		
	Vector3 NewtonRigidBody::GetLinearVelocity() const
	{
		RN::Vector3 velocity;
		NewtonBodyGetVelocity(_body, &velocity.x);
		return velocity;
	}
	Vector3 NewtonRigidBody::GetAngularVelocity() const
	{
		RN::Vector3 velocity;
		NewtonBodyGetOmega(_body, &velocity.x);
		return velocity;
	}

	void NewtonRigidBody::SetEnableCCD(bool enable)
	{
		NewtonBodySetContinuousCollisionMode(_body, enable ? 1 : 0);
	}

	void NewtonRigidBody::SetEnableGravity(bool enable)
	{
		_useGravity = enable;
	}
		
		
/*
	void PhysXDynamicBody::ApplyForce(const Vector3 &force)
	{
		_rigidBody->applyCentralForce(btVector3(force.x, force.y, force.z));
	}
	void PhysXDynamicBody::ApplyForce(const Vector3 &force, const Vector3 &origin)
	{
		_rigidBody->applyForce(btVector3(force.x, force.y, force.z), btVector3(origin.x, origin.y, origin.z));
	}
	void PhysXDynamicBody::ClearForces()
	{
		_rigidBody->clearForces();
	}
		
	void PhysXDynamicBody::ApplyTorque(const Vector3 &torque)
	{
		_rigidBody->applyTorque(btVector3(torque.x, torque.y, torque.z));
	}
	void PhysXDynamicBody::ApplyTorqueImpulse(const Vector3 &torque)
	{
		_rigidBody->applyTorqueImpulse(btVector3(torque.x, torque.y, torque.z));
	}
	void PhysXDynamicBody::ApplyImpulse(const Vector3 &impulse)
	{
		_rigidBody->applyCentralImpulse(btVector3(impulse.x, impulse.y, impulse.z));
	}
	void PhysXDynamicBody::ApplyImpulse(const Vector3 &impulse, const Vector3 &origin)
	{
		_rigidBody->applyImpulse(btVector3(impulse.x, impulse.y, impulse.z), btVector3(origin.x, origin.y, origin.z));
	}*/

	
/*	bool NewtonRigidBody::SweepTest(std::vector<PhysXContactInfo> &contactInfo, const Vector3 &direction, const Vector3 &offsetPosition, const Quaternion &offsetRotation) const
	{
		physx::PxTransform pose = _actor->getGlobalPose();
		pose.p += physx::PxVec3(offsetPosition.x, offsetPosition.y, offsetPosition.z);
		pose.q = physx::PxQuat(offsetRotation.x, offsetRotation.y, offsetRotation.z, offsetRotation.w) * pose.q;

		physx::PxScene *scene = PhysXWorld::GetSharedInstance()->GetPhysXScene();
		float length = direction.GetLength();
		physx::PxVec3 normalizedDirection = physx::PxVec3(direction.x, direction.y, direction.z);
		if(normalizedDirection.magnitude() < k::EpsilonFloat)
			normalizedDirection = physx::PxVec3(0.0f, 0.0f, -1.0f);
		normalizedDirection.normalize();
		physx::PxSweepBuffer hit;
		physx::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		filterData.word2 = _collisionFilterID;
		filterData.word3 = _collisionFilterIgnoreID;

		PhysXQueryFilterCallback filterCallback;

		if(_shape->IsKindOfClass(PhysXCompoundShape::GetMetaClass()))
		{
			PhysXCompoundShape *compound = _shape->Downcast<PhysXCompoundShape>();
			for(PhysXShape *tempShape : compound->_shapes)
			{
				physx::PxShape *shape = tempShape->GetPhysXShape();
				shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
			}
			for(PhysXShape *tempShape : compound->_shapes)
			{
				physx::PxShape *shape = tempShape->GetPhysXShape();
				scene->sweep(shape->getGeometry().any(), pose, normalizedDirection, length, hit, physx::PxHitFlags(physx::PxHitFlag::eDEFAULT), physx::PxQueryFilterData(filterData, physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::ePREFILTER), &filterCallback);

				for(int i = 0; i < hit.getNbAnyHits(); i++)
				{
					PhysXContactInfo contact;
					contact.distance = hit.getAnyHit(i).distance;
					contact.normal = Vector3(hit.getAnyHit(i).normal.x, hit.getAnyHit(i).normal.y, hit.getAnyHit(i).normal.z);
					contact.position = Vector3(hit.getAnyHit(i).position.x, hit.getAnyHit(i).position.y, hit.getAnyHit(i).position.z);
					contact.node = static_cast<SceneNode*>(hit.getAnyHit(i).actor->userData);
					contactInfo.push_back(contact);
				}
			}
			for(PhysXShape *tempShape : compound->_shapes)
			{
				physx::PxShape *shape = tempShape->GetPhysXShape();
				shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, true);
			}
		}
		else
		{
			physx::PxShape *shape = _shape->GetPhysXShape();
			shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
			scene->sweep(shape->getGeometry().any(), pose, normalizedDirection, length, hit, physx::PxHitFlags(physx::PxHitFlag::eDEFAULT), physx::PxQueryFilterData(filterData, physx::PxQueryFlag::eDYNAMIC | physx::PxQueryFlag::eSTATIC | physx::PxQueryFlag::ePREFILTER), &filterCallback);
			shape->setFlag(physx::PxShapeFlag::eSCENE_QUERY_SHAPE, true);

			for(int i = 0; i <hit.getNbAnyHits(); i++)
			{
				PhysXContactInfo contact;
				contact.distance = hit.getAnyHit(i).distance;
				contact.normal = Vector3(hit.getAnyHit(i).normal.x, hit.getAnyHit(i).normal.y, hit.getAnyHit(i).normal.z);
				contact.position = Vector3(hit.getAnyHit(i).position.x, hit.getAnyHit(i).position.y, hit.getAnyHit(i).position.z);
				contact.node = static_cast<SceneNode*>(hit.getAnyHit(i).actor->userData);
				contactInfo.push_back(contact);
			}
		}

		return (contactInfo.size() > 0);
	}

	Quaternion NewtonRigidBody::RotationSweepTest(std::vector<PhysXContactInfo> &contactInfo, const Quaternion &targetRoation, float stepSize, float sweepSize, const Vector3 &offsetPosition, const Quaternion &offsetRotation) const
	{
		Quaternion startRotation = offsetRotation*GetWorldRotation();
		Quaternion rotationDiff = targetRoation / startRotation;
		rotationDiff.Normalize();
		Vector4 axisAngleDiff = rotationDiff.GetAxisAngle();
		if(axisAngleDiff.w < stepSize)
			return GetWorldRotation();

		uint32 maxSteps = axisAngleDiff.w / stepSize + 1;
		float actualStepSize = 1.0f / static_cast<float>(maxSteps);
		float slerpFactor = actualStepSize;

		Quaternion lastRotation = startRotation;
		Quaternion newRotation = startRotation.GetLerpSpherical(targetRoation, slerpFactor).GetNormalized();
		Vector3 lastDirection = lastRotation.GetRotatedVector(Vector3(0.0f, 0.0f, -1.0f));
		Vector3 newDirection = newRotation.GetRotatedVector(Vector3(0.0f, 0.0f, -1.0f));
		Vector3 directionDiff = newDirection - lastDirection;
		directionDiff.Normalize(sweepSize);

		while(slerpFactor < 1.0f)
		{
			bool isBlocked = SweepTest(contactInfo, directionDiff * 2.0f, offsetPosition - directionDiff, newRotation / GetWorldRotation());
			if(isBlocked)
			{
				return lastRotation;
			}

			slerpFactor += actualStepSize;
			lastRotation = newRotation;
			newRotation = startRotation.GetLerpSpherical(targetRoation, slerpFactor).GetNormalized();

			lastDirection = lastRotation.GetRotatedVector(Vector3(0.0f, 0.0f, -1.0f));
			newDirection = newRotation.GetRotatedVector(Vector3(0.0f, 0.0f, -1.0f));
			directionDiff = newDirection - lastDirection;
			directionDiff.Normalize(sweepSize);
		}

		return newRotation;
	}*/
		
	void NewtonRigidBody::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		NewtonCollisionObject::DidUpdate(changeSet);
		
		if(changeSet & SceneNode::ChangeSet::Position)
		{
			Vector3 position = GetWorldPosition() - _offset;
			Quaternion rotation = GetWorldRotation();
			Matrix poseMatrix = Matrix::WithTranslation(position) * Matrix::WithRotation(rotation);
			NewtonBodySetMatrix(_body, poseMatrix.m);
		}

		if(changeSet & SceneNode::ChangeSet::Attachments)
		{
			if(!_owner && GetParent())
			{
				Vector3 position = GetWorldPosition() - _offset;
				Quaternion rotation = GetWorldRotation();
				Matrix poseMatrix = Matrix::WithTranslation(position) * Matrix::WithRotation(rotation);
				NewtonBodySetMatrix(_body, poseMatrix.m);
			}

			_owner = GetParent();
		}
	}

/*	void NewtonRigidBody::UpdateFromMaterial(BulletMaterial *material)
	{
		_rigidBody->setFriction(material->GetFriction());
		_rigidBody->setRollingFriction(material->GetRollingFriction());
		_rigidBody->setSpinningFriction(material->GetSpinningFriction());
		_rigidBody->setRestitution(material->GetRestitution());
		_rigidBody->setDamping(material->GetLinearDamping(), material->GetAngularDamping());
	}*/

	void NewtonRigidBody::UpdatePosition()
	{
		if(!_owner)
		{
			return;
		}

		Vector3 position;
		float q[4];
		NewtonBodyGetPosition(_body, &position.x);
		NewtonBodyGetRotation(_body, q);
		Quaternion rotation(q[1], q[2], q[3], q[0]);

		SetWorldPosition(position + _offset);
		SetWorldRotation(rotation);
	}
}
