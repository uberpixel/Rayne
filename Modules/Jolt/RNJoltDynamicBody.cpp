//
//  RNJoltDynamicBody.cpp
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNJoltDynamicBody.h"
#include "RNJoltWorld.h"
#include "RNJoltInternals.h"

namespace RN
{
	RNDefineMeta(JoltDynamicBody, JoltCollisionObject)
		
		JoltDynamicBody::JoltDynamicBody(JoltShape *shape, float mass) :
		_shape(shape->Retain()),
		_actor(nullptr)
	{
		JoltWorld *world = JoltWorld::GetSharedInstance();
		JPH::PhysicsSystem *physics = world->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		
		JPH::BodyCreationSettings settings(shape->GetJoltShape(), JPH::RVec3Arg(0.0f, 0.0f, 0.0f), JPH::QuatArg(0.0f, 0.0f, 0.0f, 1.0f), JPH::EMotionType::Dynamic, world->GetObjectLayer(_collisionFilterGroup, _collisionFilterMask, 1));
		settings.mMassPropertiesOverride.mMass = mass;
		settings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
		settings.mUserData = reinterpret_cast<uint64>(this);
		JPH::BodyID bodyID = bodyInterface.CreateAndAddBody(settings, JPH::EActivation::DontActivate);
		
		if(!bodyID.IsInvalid())
		{
			_actor = new JPH::BodyID();
			*_actor = bodyID;
		}
	}
		
	JoltDynamicBody::~JoltDynamicBody()
	{
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		
		bodyInterface.RemoveBody(*_actor);
		bodyInterface.DestroyBody(*_actor);
		
		if(_actor) delete _actor;
		_shape->Release();
	}

	void JoltDynamicBody::SetCollisionFilter(uint32 group, uint32 mask)
	{
		JoltCollisionObject::SetCollisionFilter(group, mask);
		JoltWorld::GetSharedInstance()->GetJoltInstance()->GetBodyInterface().SetObjectLayer(*_actor, JoltWorld::GetSharedInstance()->GetObjectLayer(_collisionFilterGroup, _collisionFilterMask, 1));

		/*Jolt::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		filterData.word2 = _collisionFilterID;
		filterData.word3 = _collisionFilterIgnoreID;*/
	}

	void JoltDynamicBody::SetCollisionFilterID(uint32 id, uint32 ignoreid)
	{
		/*JoltCollisionObject::SetCollisionFilterID(id, ignoreid);

		Jolt::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		filterData.word2 = _collisionFilterID;
		filterData.word3 = _collisionFilterIgnoreID;

		if(_shape->IsKindOfClass(JoltCompoundShape::GetMetaClass()))
		{
			JoltCompoundShape *compound = _shape->Downcast<JoltCompoundShape>();
			for(JoltShape *tempShape : compound->_shapes)
			{
				tempShape->GetJoltShape()->setSimulationFilterData(filterData);
				tempShape->GetJoltShape()->setQueryFilterData(filterData);
			}
		}
		else
		{
			_shape->GetJoltShape()->setSimulationFilterData(filterData);
			_shape->GetJoltShape()->setQueryFilterData(filterData);
		}*/
	}
	
		
	JoltDynamicBody *JoltDynamicBody::WithShape(JoltShape *shape, float mass)
	{
		JoltDynamicBody *body = new JoltDynamicBody(shape, mass);
		return body->Autorelease();
	}
		
	void JoltDynamicBody::SetMass(float mass)
	{
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		
		//TODO: Need to recreate the body!? There doesn't appear to be a way to update the mass
	}

	float JoltDynamicBody::GetMass() const
	{
		//TODO: Probably need to store the mass separately?
		
		return 1.0f;//_actor->getMass();
	}

	void JoltDynamicBody::SetLinearVelocity(const Vector3 &velocity)
	{
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		
		bodyInterface.SetLinearVelocity(*_actor, JPH::Vec3Arg(velocity.x, velocity.y, velocity.z));
	}
	void JoltDynamicBody::SetAngularVelocity(const Vector3 &velocity)
	{
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		
		bodyInterface.SetAngularVelocity(*_actor, JPH::Vec3Arg(velocity.x, velocity.y, velocity.z));
	}
		
	void JoltDynamicBody::SetDamping(float linear, float angular)
	{
		//TODO:: No idea how to do this!?
		
		/*_actor->setLinearDamping(linear);
		_actor->setAngularDamping(angular);
		_actor->setMaxAngularVelocity(PX_MAX_F32);*/
	}

	void JoltDynamicBody::SetMaxAngularVelocity(float max)
	{
		//_actor->setMaxAngularVelocity(max);
	}

	void JoltDynamicBody::SetMaxDepenetrationVelocity(float max)
	{
		//_actor->setMaxDepenetrationVelocity(max);
	}
		
	Vector3 JoltDynamicBody::GetLinearVelocity() const
	{
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		
		JPH::Vec3 velocity = bodyInterface.GetLinearVelocity(*_actor);
		return Vector3(velocity.GetX(), velocity.GetY(), velocity.GetZ());
	}
	Vector3 JoltDynamicBody::GetAngularVelocity() const
	{
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		
		JPH::Vec3 velocity = bodyInterface.GetAngularVelocity(*_actor);
		return Vector3(velocity.GetX(), velocity.GetY(), velocity.GetZ());
	}

	void JoltDynamicBody::SetEnableSleeping(bool sleeping)
	{
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		
		if(!sleeping) bodyInterface.ActivateBody(*_actor);
		else bodyInterface.DeactivateBody(*_actor);
	}

	bool JoltDynamicBody::GetIsSleeping() const
	{
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		
		return bodyInterface.IsActive(*_actor);
	}

	void JoltDynamicBody::LockMovement(RN::uint32 lockFlags)
	{
		//_actor->setRigidDynamicLockFlags(static_cast<Jolt::PxRigidDynamicLockFlags>(lockFlags));
	}

	void JoltDynamicBody::SetEnableCCD(bool enable)
	{
		//_actor->setRigidBodyFlag(Jolt::PxRigidBodyFlag::eENABLE_CCD, enable);
	}

	void JoltDynamicBody::SetEnableGravity(bool enable)
	{
		//_actor->setActorFlag(Jolt::PxActorFlag::eDISABLE_GRAVITY, !enable);
	}
	
	void JoltDynamicBody::SetSolverIterationCount(uint32 positionIterations, uint32 velocityIterations)
	{
		//_actor->setSolverIterationCounts(positionIterations, velocityIterations);
	}

	void JoltDynamicBody::ApplyForce(const Vector3 &force)
	{
		//_actor->addForce(Jolt::PxVec3(force.x, force.y, force.z));
	}
/*	void JoltDynamicBody::ApplyForce(const Vector3 &force, const Vector3 &origin)
	{
		_rigidBody->applyForce(btVector3(force.x, force.y, force.z), btVector3(origin.x, origin.y, origin.z));
	}*/
	void JoltDynamicBody::ClearForces()
	{
		//_actor->clearForce();
//		_actor->clearTorque();
	}
		
	void JoltDynamicBody::ApplyTorque(const Vector3 &torque)
	{
		//_actor->addTorque(Jolt::PxVec3(torque.x, torque.y, torque.z));
	}
	void JoltDynamicBody::ApplyTorqueImpulse(const Vector3 &torque)
	{
		//_actor->addTorque(Jolt::PxVec3(torque.x, torque.y, torque.z), Jolt::PxForceMode::eIMPULSE);
	}
	void JoltDynamicBody::ApplyImpulse(const Vector3 &impulse)
	{
		//_actor->addForce(Jolt::PxVec3(impulse.x, impulse.y, impulse.z), Jolt::PxForceMode::eIMPULSE);
	}
/*	void JoltDynamicBody::ApplyImpulse(const Vector3 &impulse, const Vector3 &origin)
	{
		_rigidBody->applyImpulse(btVector3(impulse.x, impulse.y, impulse.z), btVector3(origin.x, origin.y, origin.z));
	}*/

	void JoltDynamicBody::SetEnableKinematic(bool enable)
	{
		//_actor->setRigidBodyFlag(Jolt::PxRigidBodyFlag::eKINEMATIC, enable);
	}

	bool JoltDynamicBody::GetIsKinematic() const
	{
		return false;//_actor->getRigidBodyFlags() & Jolt::PxRigidBodyFlag::eKINEMATIC;
	}

	void JoltDynamicBody::SetKinematicTarget(const Vector3 &position, const Quaternion &rotation)
	{
		/*RN::Vector3 positionOffset = GetWorldRotation().GetRotatedVector(_positionOffset);
		Quaternion targetRotation = rotation * _rotationOffset;
		_actor->setKinematicTarget(Jolt::PxTransform(position.x - positionOffset.x, position.y - positionOffset.y, position.z - positionOffset.z, Jolt::PxQuat(targetRotation.x, targetRotation.y, targetRotation.z, targetRotation.w)));*/
	}

/*	void JoltDynamicBody::AccelerateToTarget(const Vector3 &position, const Quaternion &rotation, float delta)
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
		
		_actor->addForce(Jolt::PxVec3(linearForce.x, linearForce.y, linearForce.z), Jolt::PxForceMode::eACCELERATION);
		_actor->addTorque(Jolt::PxVec3(angularForce.x, angularForce.y, angularForce.z), Jolt::PxForceMode::eACCELERATION);
	}
	
	bool JoltDynamicBody::SweepTest(std::vector<JoltContactInfo> &contactInfo, const Vector3 &direction, const Vector3 &offsetPosition, const Quaternion &offsetRotation, float inflation) const
	{
		Jolt::PxTransform pose = _actor->getGlobalPose();
		pose.p += Jolt::PxVec3(offsetPosition.x, offsetPosition.y, offsetPosition.z);
		pose.q = Jolt::PxQuat(offsetRotation.x, offsetRotation.y, offsetRotation.z, offsetRotation.w) * pose.q;

		Jolt::PxScene *scene = JoltWorld::GetSharedInstance()->GetJoltScene();
		float length = direction.GetLength();
		Jolt::PxVec3 normalizedDirection = Jolt::PxVec3(direction.x, direction.y, direction.z);
		if(normalizedDirection.magnitude() < k::EpsilonFloat)
			normalizedDirection = Jolt::PxVec3(0.0f, 0.0f, -1.0f);
		normalizedDirection.normalize();
		Jolt::PxSweepBuffer hit;
		Jolt::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		filterData.word2 = _collisionFilterID;
		filterData.word3 = _collisionFilterIgnoreID;

		JoltQueryFilterCallback filterCallback;

		if(_shape->IsKindOfClass(JoltCompoundShape::GetMetaClass()))
		{
			JoltCompoundShape *compound = _shape->Downcast<JoltCompoundShape>();
			for(JoltShape *tempShape : compound->_shapes)
			{
				Jolt::PxShape *shape = tempShape->GetJoltShape();
				shape->setFlag(Jolt::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
			}
			for(JoltShape *tempShape : compound->_shapes)
			{
				Jolt::PxShape *shape = tempShape->GetJoltShape();
				scene->sweep(shape->getGeometry().any(), pose, normalizedDirection, length, hit, Jolt::PxHitFlags(Jolt::PxHitFlag::eDEFAULT), Jolt::PxQueryFilterData(filterData, Jolt::PxQueryFlag::eDYNAMIC | Jolt::PxQueryFlag::eSTATIC | Jolt::PxQueryFlag::ePREFILTER|Jolt::PxQueryFlag::eNO_BLOCK), &filterCallback, nullptr, inflation);

				for(int i = 0; i < hit.getNbAnyHits(); i++)
				{
					JoltContactInfo contact;
					contact.distance = hit.getAnyHit(i).distance;
					contact.normal = Vector3(hit.getAnyHit(i).normal.x, hit.getAnyHit(i).normal.y, hit.getAnyHit(i).normal.z);
					contact.position = Vector3(hit.getAnyHit(i).position.x, hit.getAnyHit(i).position.y, hit.getAnyHit(i).position.z);
					contact.node = nullptr;
					JoltCollisionObject *attachment = static_cast<JoltCollisionObject*>(hit.getAnyHit(i).actor->userData);
					contact.collisionObject = attachment;
					if(attachment)
					{
						contact.node = attachment->GetParent();
						if(contact.node) contact.node->Retain()->Autorelease();
					}
					contactInfo.push_back(contact);
				}
			}
			for(JoltShape *tempShape : compound->_shapes)
			{
				Jolt::PxShape *shape = tempShape->GetJoltShape();
				shape->setFlag(Jolt::PxShapeFlag::eSCENE_QUERY_SHAPE, true);
			}
		}
		else if(_shape->GetJoltShape())
		{
			Jolt::PxShape *shape = _shape->GetJoltShape();
			shape->setFlag(Jolt::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
			scene->sweep(shape->getGeometry().any(), pose, normalizedDirection, length, hit, Jolt::PxHitFlags(Jolt::PxHitFlag::eDEFAULT), Jolt::PxQueryFilterData(filterData, Jolt::PxQueryFlag::eDYNAMIC | Jolt::PxQueryFlag::eSTATIC | Jolt::PxQueryFlag::ePREFILTER|Jolt::PxQueryFlag::eNO_BLOCK), &filterCallback, nullptr, inflation);
			shape->setFlag(Jolt::PxShapeFlag::eSCENE_QUERY_SHAPE, true);

			for(int i = 0; i < hit.getNbAnyHits(); i++)
			{
				JoltContactInfo contact;
				contact.distance = hit.getAnyHit(i).distance;
				contact.normal = Vector3(hit.getAnyHit(i).normal.x, hit.getAnyHit(i).normal.y, hit.getAnyHit(i).normal.z);
				contact.position = Vector3(hit.getAnyHit(i).position.x, hit.getAnyHit(i).position.y, hit.getAnyHit(i).position.z);
				contact.node = nullptr;
				JoltCollisionObject *attachment = static_cast<JoltCollisionObject*>(hit.getAnyHit(i).actor->userData);
				contact.collisionObject = attachment;
				if(attachment)
				{
					contact.node = attachment->GetParent();
					if(contact.node) contact.node->Retain()->Autorelease();
				}
				contactInfo.push_back(contact);
			}
		}

		return (contactInfo.size() > 0);
	}

	Quaternion JoltDynamicBody::RotationSweepTest(std::vector<JoltContactInfo> &contactInfo, const Quaternion &targetRoation, float stepSize, float sweepSize, const Vector3 &offsetPosition, const Quaternion &offsetRotation) const
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
		
	void JoltDynamicBody::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		JoltCollisionObject::DidUpdate(changeSet);
		
		if(changeSet & SceneNode::ChangeSet::Position)
		{
			RN::Vector3 positionOffset = GetWorldRotation().GetRotatedVector(_positionOffset);
			Vector3 position = GetWorldPosition() - positionOffset;
			Quaternion rotation = GetWorldRotation() * _rotationOffset;
			
			JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
			JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
			
			bodyInterface.SetPositionAndRotation(*_actor, JPH::RVec3Arg(position.x, position.y, position.z), JPH::QuatArg(rotation.x, rotation.y, rotation.z, rotation.w), JPH::EActivation::DontActivate);
		}

		if(changeSet & SceneNode::ChangeSet::Attachments)
		{
			if(!_owner && GetParent())
			{
				RN::Vector3 positionOffset = GetWorldRotation().GetRotatedVector(_positionOffset);
				Vector3 position = GetWorldPosition() - positionOffset;
				Quaternion rotation = GetWorldRotation() * _rotationOffset;
				
				JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
				JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
				
				bodyInterface.SetPositionAndRotation(*_actor, JPH::RVec3Arg(position.x, position.y, position.z), JPH::QuatArg(rotation.x, rotation.y, rotation.z, rotation.w), JPH::EActivation::DontActivate);
			}

			_owner = GetParent();
		}
	}

/*	void JoltDynamicBody::UpdateFromMaterial(BulletMaterial *material)
	{
		_rigidBody->setFriction(material->GetFriction());
		_rigidBody->setRollingFriction(material->GetRollingFriction());
		_rigidBody->setSpinningFriction(material->GetSpinningFriction());
		_rigidBody->setRestitution(material->GetRestitution());
		_rigidBody->setDamping(material->GetLinearDamping(), material->GetAngularDamping());
	}*/

	void JoltDynamicBody::UpdatePosition()
	{
		if(!_owner)
		{
			return;
		}
		
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		
		JPH::RVec3 position;
		JPH::Quat rotation;
		bodyInterface.GetPositionAndRotation(*_actor, position, rotation);

		RN::Quaternion rotationResult = Quaternion(rotation.GetX(), rotation.GetY(), rotation.GetZ(), rotation.GetW()) * _rotationOffset.GetConjugated();
		RN::Vector3 positionOffset = rotationResult.GetRotatedVector(_positionOffset);
		SetWorldPosition(Vector3(position.GetX(), position.GetY(), position.GetZ()) + positionOffset);
		SetWorldRotation(rotationResult);
	}
}
