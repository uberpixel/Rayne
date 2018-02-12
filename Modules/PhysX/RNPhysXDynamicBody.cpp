//
//  RNPhysXDynamicBody.cpp
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNPhysXDynamicBody.h"
#include "RNPhysXWorld.h"
#include "PxPhysicsAPI.h"
#include "RNPhysXInternals.h"

namespace RN
{
	RNDefineMeta(PhysXDynamicBody, PhysXCollisionObject)
		
		PhysXDynamicBody::PhysXDynamicBody(PhysXShape *shape, float mass) :
		_shape(shape->Retain()),
		_actor(nullptr)
	{
		physx::PxPhysics *physics = PhysXWorld::GetSharedInstance()->GetPhysXInstance();
		_actor = physics->createRigidDynamic(physx::PxTransform(physx::PxIdentity));

		if(shape->IsKindOfClass(PhysXCompoundShape::GetMetaClass()))
		{
			PhysXCompoundShape *compound = shape->Downcast<PhysXCompoundShape>();
			for(PhysXShape *tempShape : compound->_shapes)
			{
				_actor->attachShape(*tempShape->GetPhysXShape());
			}
		}
		else
		{
			_actor->attachShape(*shape->GetPhysXShape());
		}
		
		physx::PxRigidBodyExt::updateMassAndInertia(*_actor, mass);

		_actor->userData = this;
		_actor->setContactReportThreshold(0.0f);

		physx::PxScene *scene = PhysXWorld::GetSharedInstance()->GetPhysXScene();
		scene->addActor(*_actor);
	}
		
	PhysXDynamicBody::~PhysXDynamicBody()
	{
		physx::PxScene *scene = PhysXWorld::GetSharedInstance()->GetPhysXScene();
		scene->removeActor(*_actor);
		_actor->release();
		_shape->Release();
	}

	void PhysXDynamicBody::SetCollisionFilter(uint32 group, uint32 mask)
	{
		PhysXCollisionObject::SetCollisionFilter(group, mask);

		physx::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		filterData.word2 = _collisionFilterID;
		filterData.word3 = _collisionFilterIgnoreID;

		if(_shape->IsKindOfClass(PhysXCompoundShape::GetMetaClass()))
		{
			PhysXCompoundShape *compound = _shape->Downcast<PhysXCompoundShape>();
			for(PhysXShape *tempShape : compound->_shapes)
			{
				tempShape->GetPhysXShape()->setSimulationFilterData(filterData);
				tempShape->GetPhysXShape()->setQueryFilterData(filterData);
			}
		}
		else
		{
			_shape->GetPhysXShape()->setSimulationFilterData(filterData);
			_shape->GetPhysXShape()->setQueryFilterData(filterData);
		}
	}

	void PhysXDynamicBody::SetCollisionFilterID(uint32 id, uint32 ignoreid)
	{
		PhysXCollisionObject::SetCollisionFilterID(id, ignoreid);

		physx::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		filterData.word2 = _collisionFilterID;
		filterData.word3 = _collisionFilterIgnoreID;

		if(_shape->IsKindOfClass(PhysXCompoundShape::GetMetaClass()))
		{
			PhysXCompoundShape *compound = _shape->Downcast<PhysXCompoundShape>();
			for(PhysXShape *tempShape : compound->_shapes)
			{
				tempShape->GetPhysXShape()->setSimulationFilterData(filterData);
				tempShape->GetPhysXShape()->setQueryFilterData(filterData);
			}
		}
		else
		{
			_shape->GetPhysXShape()->setSimulationFilterData(filterData);
			_shape->GetPhysXShape()->setQueryFilterData(filterData);
		}
	}
	
		
	PhysXDynamicBody *PhysXDynamicBody::WithShape(PhysXShape *shape, float mass)
	{
		PhysXDynamicBody *body = new PhysXDynamicBody(shape, mass);
		return body->Autorelease();
	}
		
	void PhysXDynamicBody::SetMass(float mass)
	{
		physx::PxRigidBodyExt::updateMassAndInertia(*_actor, mass);
	}

	void PhysXDynamicBody::SetLinearVelocity(const Vector3 &velocity)
	{
		_actor->setLinearVelocity(physx::PxVec3(velocity.x, velocity.y, velocity.z));
	}
	void PhysXDynamicBody::SetAngularVelocity(const Vector3 &velocity)
	{
		_actor->setAngularVelocity(physx::PxVec3(velocity.x, velocity.y, velocity.z));
	}
		
	void PhysXDynamicBody::SetDamping(float linear, float angular)
	{
		_actor->setLinearDamping(linear);
		_actor->setAngularDamping(angular);
		_actor->setMaxAngularVelocity(PX_MAX_F32);
	}

	void PhysXDynamicBody::SetMaxAngularVelocity(float max)
	{
		_actor->setMaxAngularVelocity(max);
	}

	void PhysXDynamicBody::SetMaxDepenetrationVelocity(float max)
	{
		_actor->setMaxDepenetrationVelocity(max);
	}
		
	Vector3 PhysXDynamicBody::GetLinearVelocity() const
	{
		const physx::PxVec3& velocity = _actor->getLinearVelocity();
		return Vector3(velocity.x, velocity.y, velocity.z);
	}
	Vector3 PhysXDynamicBody::GetAngularVelocity() const
	{
		const physx::PxVec3& velocity = _actor->getAngularVelocity();
		return Vector3(velocity.x, velocity.y, velocity.z);
	}

	void PhysXDynamicBody::SetEnableCCD(bool enable)
	{
		_actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eENABLE_CCD, enable);
	}

	void PhysXDynamicBody::SetEnableGravity(bool enable)
	{
		_actor->setActorFlag(physx::PxActorFlag::eDISABLE_GRAVITY, !enable);
	}
		
		

	void PhysXDynamicBody::ApplyForce(const Vector3 &force)
	{
		_actor->addForce(physx::PxVec3(force.x, force.y, force.z));
	}
/*	void PhysXDynamicBody::ApplyForce(const Vector3 &force, const Vector3 &origin)
	{
		_rigidBody->applyForce(btVector3(force.x, force.y, force.z), btVector3(origin.x, origin.y, origin.z));
	}*/
	void PhysXDynamicBody::ClearForces()
	{
		_actor->clearForce();
//		_actor->clearTorque();
	}
		
	void PhysXDynamicBody::ApplyTorque(const Vector3 &torque)
	{
		_actor->addTorque(physx::PxVec3(torque.x, torque.y, torque.z));
	}
	void PhysXDynamicBody::ApplyTorqueImpulse(const Vector3 &torque)
	{
		_actor->addTorque(physx::PxVec3(torque.x, torque.y, torque.z), physx::PxForceMode::eIMPULSE);
	}
	void PhysXDynamicBody::ApplyImpulse(const Vector3 &impulse)
	{
		_actor->addForce(physx::PxVec3(impulse.x, impulse.y, impulse.z), physx::PxForceMode::eIMPULSE);
	}
/*	void PhysXDynamicBody::ApplyImpulse(const Vector3 &impulse, const Vector3 &origin)
	{
		_rigidBody->applyImpulse(btVector3(impulse.x, impulse.y, impulse.z), btVector3(origin.x, origin.y, origin.z));
	}*/

	void PhysXDynamicBody::SetEnableKinematic(bool enable)
	{
		_actor->setRigidBodyFlag(physx::PxRigidBodyFlag::eKINEMATIC, enable);
	}

	void PhysXDynamicBody::SetKinematicTarget(const Vector3 &position, const Quaternion &rotation)
	{
		_actor->setKinematicTarget(physx::PxTransform(position.x, position.y, position.z, physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)));
	}

	void PhysXDynamicBody::AccelerateToTarget(const Vector3 &position, const Quaternion &rotation, float delta)
	{
		//Linear velocity
		RN::Vector3 speed = position - GetWorldPosition();
		speed /= delta;

		//Angular velocity
		RN::Quaternion startRotation = GetWorldRotation();
		if(rotation.GetDotProduct(startRotation) > 0.0f)
			startRotation = startRotation.GetConjugated();
		RN::Quaternion rotationSpeed = rotation*startRotation;
		RN::Vector4 axisAngleSpeed = rotationSpeed.GetAxisAngle();
		if(axisAngleSpeed.w > 180.0f)
			axisAngleSpeed.w -= 360.0f;
		RN::Vector3 angularVelocity(axisAngleSpeed.x, axisAngleSpeed.y, axisAngleSpeed.z);
		angularVelocity *= axisAngleSpeed.w*M_PI;
		angularVelocity /= 180.0f;
		angularVelocity /= delta;

		RN::Vector3 linearForce = speed - GetLinearVelocity();
		linearForce /= delta;
//		linearForce *= _actor->getMass();
		linearForce *= 10.0f;
		RN::Vector3 angularForce = angularVelocity - GetAngularVelocity();
		angularForce /= delta;
//		angularForce *= _mass;
		angularForce *= 10.0f;

		if(linearForce.GetLength() > 5000.0f)
			linearForce.Normalize(5000.0f);

		if(angularForce.GetLength() > 15000.0f)
			angularForce.Normalize(15000.0f);
		
		_actor->addForce(physx::PxVec3(linearForce.x, linearForce.y, linearForce.z), physx::PxForceMode::eACCELERATION);
		_actor->addTorque(physx::PxVec3(angularForce.x, angularForce.y, angularForce.z), physx::PxForceMode::eACCELERATION);
	}
	
	bool PhysXDynamicBody::SweepTest(std::vector<PhysXContactInfo> &contactInfo, const Vector3 &direction, const Vector3 &offsetPosition, const Quaternion &offsetRotation) const
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

	Quaternion PhysXDynamicBody::RotationSweepTest(std::vector<PhysXContactInfo> &contactInfo, const Quaternion &targetRoation, float stepSize, float sweepSize, const Vector3 &offsetPosition, const Quaternion &offsetRotation) const
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
	}
		
	void PhysXDynamicBody::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		PhysXCollisionObject::DidUpdate(changeSet);
		
		if(changeSet & SceneNode::ChangeSet::Position)
		{
			Vector3 position = GetWorldPosition() - _offset;
			Quaternion rotation = GetWorldRotation();
			_actor->setGlobalPose(physx::PxTransform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)));
		}

		if(changeSet & SceneNode::ChangeSet::Attachments)
		{
			if(!_owner && GetParent())
			{
				Vector3 position = GetWorldPosition() - _offset;
				Quaternion rotation = GetWorldRotation();
				_actor->setGlobalPose(physx::PxTransform(physx::PxVec3(position.x, position.y, position.z), physx::PxQuat(rotation.x, rotation.y, rotation.z, rotation.w)));
			}

			_owner = GetParent();
		}
	}

/*	void PhysXDynamicBody::UpdateFromMaterial(BulletMaterial *material)
	{
		_rigidBody->setFriction(material->GetFriction());
		_rigidBody->setRollingFriction(material->GetRollingFriction());
		_rigidBody->setSpinningFriction(material->GetSpinningFriction());
		_rigidBody->setRestitution(material->GetRestitution());
		_rigidBody->setDamping(material->GetLinearDamping(), material->GetAngularDamping());
	}*/

	void PhysXDynamicBody::UpdatePosition()
	{
		if(!_owner)
		{
			return;
		}

		const physx::PxTransform &transform = _actor->getGlobalPose();
		SetWorldPosition(Vector3(transform.p.x, transform.p.y, transform.p.z) + _offset);
		SetWorldRotation(Quaternion(transform.q.x, transform.q.y, transform.q.z, transform.q.w));
	}
}
