//
//  RNJoltStaticBody.cpp
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNJoltStaticBody.h"
#include "RNJoltWorld.h"
#include "RNJoltInternals.h"


namespace RN
{
	RNDefineMeta(JoltStaticBody, JoltCollisionObject)
		
	JoltStaticBody::JoltStaticBody(JoltShape *shape) :
		_shape(shape->Retain()),
		_actor(nullptr)
	{
		JoltWorld *world = JoltWorld::GetSharedInstance();
		JPH::PhysicsSystem *physics = world->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		
		JPH::BodyCreationSettings settings(shape->GetJoltShape(), JPH::RVec3Arg(0.0f, 0.0f, 0.0f), JPH::QuatArg(0.0f, 0.0f, 0.0f, 1.0f), JPH::EMotionType::Static, world->GetObjectLayer(_collisionFilterGroup, _collisionFilterMask, 0));
		settings.mUserData = reinterpret_cast<uint64>(this);
		
		JPH::BodyID bodyID;
		if(world->IsLoadingLevel())
		{
			//When loading a level, schedule body to be bulk inserted into the physics scene
			JPH::Body *body = bodyInterface.CreateBody(settings);
			bodyID = body->GetID();
			world->AddBodyForLoadingLevel(body);
		}
		else
		{
			//Directly add to the scene if not loading a level
			bodyID = bodyInterface.CreateAndAddBody(settings, JPH::EActivation::DontActivate);
		}
		
		if(!bodyID.IsInvalid())
		{
			_actor = new JPH::BodyID();
			*_actor = bodyID;
		}
	}
		
	JoltStaticBody::~JoltStaticBody()
	{
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		
		bodyInterface.RemoveBody(*_actor);
		bodyInterface.DestroyBody(*_actor);
		
		if(_actor) delete _actor;
		_shape->Release();
	}
	
		
	JoltStaticBody *JoltStaticBody::WithShape(JoltShape *shape)
	{
		JoltStaticBody *body = new JoltStaticBody(shape);
		return body->Autorelease();
	}

	void JoltStaticBody::SetCollisionFilter(uint32 group, uint32 mask)
	{
		JoltCollisionObject::SetCollisionFilter(group, mask);
		JoltWorld::GetSharedInstance()->GetJoltInstance()->GetBodyInterface().SetObjectLayer(*_actor, JoltWorld::GetSharedInstance()->GetObjectLayer(_collisionFilterGroup, _collisionFilterMask, 0));
	}
	
	void JoltStaticBody::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		JoltCollisionObject::DidUpdate(changeSet);
		
		if(changeSet & SceneNode::ChangeSet::Position)
		{
			RN::Vector3 positionOffset = GetWorldRotation().GetRotatedVector(_positionOffset);
			Vector3 position = GetWorldPosition() - positionOffset;
			Quaternion rotation = GetWorldRotation() * _rotationOffset;
			rotation.Normalize();
			
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
				rotation.Normalize();
				
				JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
				JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
				
				bodyInterface.SetPositionAndRotation(*_actor, JPH::RVec3Arg(position.x, position.y, position.z), JPH::QuatArg(rotation.x, rotation.y, rotation.z, rotation.w), JPH::EActivation::DontActivate);
			}

			_owner = GetParent();
		}
	}

	void JoltStaticBody::UpdatePosition()
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

