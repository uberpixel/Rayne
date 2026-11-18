//
//  RNJoltStaticBody.cpp
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNJoltStaticBody.h"
#include "RNJoltInternals.h"
#include "RNJoltWorld.h"


namespace RN
{
	RNDefineMeta(JoltStaticBody, JoltCollisionObject)

	JoltStaticBody::JoltStaticBody(JoltShape *shape, JoltWorld *owningWorld) :
		JoltCollisionObject(owningWorld),
		_shape(shape->Retain()),
		_actor(nullptr)
	{
		JoltWorld *world = GetJoltWorld();
		JPH::PhysicsSystem *physics = world->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();

		JPH::BodyCreationSettings settings(shape->GetJoltShape(), JPH::RVec3::sZero(), JPH::QuatArg(0.0f, 0.0f, 0.0f, 1.0f), JPH::EMotionType::Static, world->GetObjectLayer(_collisionFilterGroup, _collisionFilterMask, 0));
		settings.mEnhancedInternalEdgeRemoval = true;
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
		JoltWorld *world = GetJoltWorld();
		JPH::PhysicsSystem *physics = world->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();

		if(_actor)
		{
			world->RemoveConstraintsForBody(*_actor);
			bodyInterface.RemoveBody(*_actor);
			bodyInterface.DestroyBody(*_actor);
			delete _actor;
		}
		_shape->Release();
	}


	JoltStaticBody *JoltStaticBody::WithShape(JoltShape *shape, JoltWorld *world)
	{
		JoltStaticBody *body = new JoltStaticBody(shape, world);
		return body->Autorelease();
	}

	void JoltStaticBody::SetCollisionFilter(uint32 group, uint32 mask)
	{
		JoltCollisionObject::SetCollisionFilter(group, mask);
		GetJoltWorld()->GetJoltInstance()->GetBodyInterface().SetObjectLayer(*_actor, GetJoltWorld()->GetObjectLayer(_collisionFilterGroup, _collisionFilterMask, 0));
	}

	void JoltStaticBody::SetFriction(float friction)
	{
		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		bodyInterface.SetFriction(*_actor, friction);
	}

	void JoltStaticBody::SetRestitution(float restitution)
	{
		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		bodyInterface.SetRestitution(*_actor, restitution);
	}

	void JoltStaticBody::InvalidateContactCache()
	{
		if(!_actor) return;

		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		bodyInterface.InvalidateContactCache(*_actor);
	}

	void JoltStaticBody::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		JoltCollisionObject::DidUpdate(changeSet);

		const bool shouldUpdatePose = (changeSet & SceneNode::ChangeSet::Position) ||
#if RN_ENABLE_LAZY_FRAME_ROOT_TRANSFORMS
			(changeSet & SceneNode::ChangeSet::Parent) ||
#endif
			((changeSet & SceneNode::ChangeSet::Attachments) && !_owner && GetParent());
		if(shouldUpdatePose && _actor)
			SetBodyTransformFromNode(*_actor, GetJoltWorld()->GetJoltInstance()->GetBodyInterface());

		if(changeSet & SceneNode::ChangeSet::Attachments) _owner = GetParent();
	}

	void JoltStaticBody::UpdatePosition()
	{
		if(!GetParent())
		{
			return;
		}

		JPH::PhysicsSystem *physics = GetJoltWorld()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();

		JPH::RVec3 position;
		JPH::Quat rotation;
		bodyInterface.GetPositionAndRotation(*_actor, position, rotation);

		RN::Quaternion rotationResult = JoltNativeConversions::ToSceneRotation(rotation, GetJoltWorld()) * _rotationOffset.GetConjugated();
		RN::Vector3 positionOffset = rotationResult.GetRotatedVector(_positionOffset);
		JoltNativeConversions::SetAttachmentTransform(this, position, positionOffset, rotationResult, GetJoltWorld());
	}
} // namespace RN
