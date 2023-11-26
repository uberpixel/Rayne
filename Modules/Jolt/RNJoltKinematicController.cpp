//
//  RNJoltKinematicController.cpp
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNJoltKinematicController.h"
#include "RNJoltWorld.h"
#include "RNJoltInternals.h"

#include <Jolt/Physics/Character/CharacterVirtual.h>

namespace RN
{
	RNDefineMeta(JoltKinematicController, JoltCollisionObject)
		
	JoltKinematicController::JoltKinematicController(float radius, float height, JoltMaterial *material, float stepOffset) : _fallSpeed(0.0f), _objectBelow(nullptr), _isFalling(false)
	{
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		
		_shape = JoltCapsuleShape::WithRadius(radius, height, material)->Retain();
		
		JPH::CharacterVirtualSettings settings;
		settings.mMaxSlopeAngle = 70.0f;
		settings.mMaxStrength = 10.0f;
		settings.mShape = _shape->GetJoltShape();
		//settings->mBackFaceMode = sBackFaceMode;
		//settings->mCharacterPadding = sCharacterPadding;
		//settings->mPenetrationRecoverySpeed = sPenetrationRecoverySpeed;
		//settings->mPredictiveContactDistance = sPredictiveContactDistance;
		settings.mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -radius); // Accept contacts that touch the lower sphere of the capsule
		_controller = new JPH::CharacterVirtual(&settings, JPH::RVec3::sZero(), JPH::Quat::sIdentity(), physics);
		//_controller->SetListener(this);

		/*_callback = new JoltKinematicControllerCallback();

		Jolt::PxCapsuleControllerDesc desc;
		desc.height = height;
		desc.radius = radius;
		desc.position.set(0.0f, 10.0, 0.0f);
		desc.stepOffset = stepOffset;
		desc.material = _material->GetJoltMaterial();
		desc.reportCallback = _callback;
		desc.behaviorCallback = nullptr;//_callback;
		desc.userData = this;

		Jolt::PxControllerManager *manager = JoltWorld::GetSharedInstance()->GetJoltControllerManager();
		_controller = static_cast<Jolt::PxCapsuleController*>(manager->createController(desc));*/
	}
	
	JoltKinematicController::~JoltKinematicController()
	{
		SafeRelease(_shape);
		delete _controller;
		//if(_callback) delete _callback;
	}
		
	void JoltKinematicController::Move(const Vector3 &direction, float delta)
	{
		if(delta < k::EpsilonFloat || direction.GetLength() < k::EpsilonFloat)
		{
			return;
		}

		/*Jolt::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		filterData.word2 = _collisionFilterID;
		filterData.word3 = _collisionFilterIgnoreID;
		Jolt::PxControllerFilters controllerFilter(&filterData, _callback, _callback);
		Jolt::PxControllerCollisionFlags collisionFlags = _controller->move(Jolt::PxVec3(direction.x, direction.y, direction.z), 0.0f, delta, controllerFilter);
*/
		
		JPH::PhysicsSystem *physics = JoltWorld::GetSharedInstance()->GetJoltInstance();
		
		_controller->SetLinearVelocity(JPH::Vec3Arg(direction.x, direction.y, direction.z));
		_controller->Update(delta, physics->GetGravity(), physics->GetDefaultBroadPhaseLayerFilter(JoltObjectLayers::MOVING), physics->GetDefaultLayerFilter(JoltObjectLayers::MOVING), {}, {}, *JoltWorld::GetSharedInstance()->_internals->tempAllocator);
		
		UpdatePosition();
	}

	void JoltKinematicController::Gravity(float gforce, float delta)
	{
		JoltContactInfo contact = SweepTest(RN::Vector3(0.0f, -10000.0f, 0.0f));
		float groundDistance = _controller->GetPosition().GetY() - contact.position.y + GetFeetOffset().y;
		float fallDistance = 0.0f;
		_fallSpeed += gforce * delta;
		if((groundDistance + _fallSpeed * delta) < k::EpsilonFloat || (!_isFalling && groundDistance + gforce * 2.0f * delta < 0.0f))
		{
			_fallSpeed = 0.0f;
			_isFalling = false;
			fallDistance = -groundDistance;
		}
		else
		{
			fallDistance = _fallSpeed * delta;
			_isFalling = true;
		}
		
		_objectBelow = contact.node;
		if(std::abs(fallDistance) > k::EpsilonFloat)
		{
			Move(RN::Vector3(0.0f, fallDistance, 0.0f), delta);
		}
	}

	std::vector<JoltContactInfo> JoltKinematicController::SweepTestAll(const Vector3 &direction, const Vector3 &offset) const
	{
		/*const Jolt::PxExtendedVec3 &position = _controller->getPosition();
		Jolt::PxScene *scene = JoltWorld::GetSharedInstance()->GetJoltScene();
		float length = direction.GetLength();
		Vector3 normalizedDirection = direction.GetNormalized();
		const Jolt::PxU32 bufferSize = 2048;
		Jolt::PxSweepHit hitBuffer[bufferSize];
		Jolt::PxSweepBuffer hit(hitBuffer, bufferSize);
		Jolt::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		filterData.word2 = _collisionFilterID;
		filterData.word3 = _collisionFilterIgnoreID;
		JoltQueryFilterCallback filterCallback;
		Jolt::PxShape *shape;
		_controller->getActor()->getShapes(&shape, 1);
		shape->setFlag(Jolt::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		Quaternion orientation(RN::Vector3(0.0f, 0.0f, 90.0f));
		scene->sweep(shape->getGeometry().any(), Jolt::PxTransform(Jolt::PxVec3(position.x + offset.x, position.y + offset.y, position.z + offset.z), Jolt::PxQuat(orientation.x, orientation.y, orientation.z, orientation.w)), Jolt::PxVec3(normalizedDirection.x, normalizedDirection.y, normalizedDirection.z), length, hit, Jolt::PxHitFlags(Jolt::PxHitFlag::eDEFAULT), Jolt::PxQueryFilterData(filterData, Jolt::PxQueryFlag::eDYNAMIC|Jolt::PxQueryFlag::eSTATIC|Jolt::PxQueryFlag::ePREFILTER|Jolt::PxQueryFlag::eNO_BLOCK), &filterCallback);
		shape->setFlag(Jolt::PxShapeFlag::eSCENE_QUERY_SHAPE, true);*/
		
		std::vector<JoltContactInfo> contacts;

		/*if(hit.getNbTouches() == 0)
			return contacts;
		
		for(uint32 i = 0; i < hit.nbTouches; i++)
		{
			Jolt::PxSweepHit currentHit = hit.touches[i];
			
			JoltContactInfo contact;
			contact.distance = currentHit.distance;
			contact.node = nullptr;
			contact.collisionObject = nullptr;

			contact.position = Vector3(currentHit.position.x, currentHit.position.y, currentHit.position.z);
			contact.normal = Vector3(currentHit.normal.x, currentHit.normal.y, currentHit.normal.z);
			if(currentHit.actor)
			{
				JoltCollisionObject *collisionObject = static_cast<JoltCollisionObject*>(currentHit.actor->userData);
				contact.collisionObject = collisionObject;
				if(collisionObject->GetParent())
				{
					contact.node = collisionObject->GetParent();
					if(contact.node) contact.node->Retain()->Autorelease();
				}
			}

			contacts.push_back(contact);
		}*/
		
		return contacts;
	}

	JoltContactInfo JoltKinematicController::SweepTest(const Vector3 &direction, const Vector3 &offset) const
	{
		/*const Jolt::PxExtendedVec3 &position = _controller->getPosition();
		Jolt::PxScene *scene = JoltWorld::GetSharedInstance()->GetJoltScene();
		float length = direction.GetLength();
		Vector3 normalizedDirection = direction.GetNormalized();
		Jolt::PxSweepBuffer hit;
		Jolt::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		filterData.word2 = _collisionFilterID;
		filterData.word3 = _collisionFilterIgnoreID;
		JoltQueryFilterCallback filterCallback;
		Jolt::PxShape *shape;
		_controller->getActor()->getShapes(&shape, 1);
		shape->setFlag(Jolt::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		Quaternion orientation(RN::Vector3(0.0f, 0.0f, 90.0f));
		bool didHit = scene->sweep(shape->getGeometry().any(), Jolt::PxTransform(Jolt::PxVec3(position.x + offset.x, position.y + offset.y, position.z + offset.z), Jolt::PxQuat(orientation.x, orientation.y, orientation.z, orientation.w)), Jolt::PxVec3(normalizedDirection.x, normalizedDirection.y, normalizedDirection.z), length, hit, Jolt::PxHitFlags(Jolt::PxHitFlag::eDEFAULT), Jolt::PxQueryFilterData(filterData, Jolt::PxQueryFlag::eDYNAMIC|Jolt::PxQueryFlag::eSTATIC|Jolt::PxQueryFlag::ePREFILTER), &filterCallback);
		shape->setFlag(Jolt::PxShapeFlag::eSCENE_QUERY_SHAPE, true);*/
		
		JoltContactInfo contact;
		contact.distance = -1.0f;
		contact.node = nullptr;
		contact.collisionObject = nullptr;

		/*if(!didHit)
			return contact;

		Jolt::PxSweepHit closestHit = hit.block;
		contact.distance = closestHit.distance;
		contact.position = Vector3(closestHit.position.x, closestHit.position.y, closestHit.position.z);
		contact.normal = Vector3(closestHit.normal.x, closestHit.normal.y, closestHit.normal.z);
		if(closestHit.actor)
		{
			JoltCollisionObject *collisionObject = static_cast<JoltCollisionObject*>(closestHit.actor->userData);
			contact.collisionObject = collisionObject;
			if(collisionObject->GetParent())
			{
				contact.node = collisionObject->GetParent();
				if(contact.node) contact.node->Retain()->Autorelease();
			}
		}*/
		return contact;
	}

	JoltContactInfo JoltKinematicController::OverlapTest() const
	{
		/*const Jolt::PxExtendedVec3 &position = _controller->getPosition();
		Jolt::PxScene *scene = JoltWorld::GetSharedInstance()->GetJoltScene();
		Jolt::PxOverlapBuffer hit;
		Jolt::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		filterData.word2 = _collisionFilterID;
		filterData.word3 = _collisionFilterIgnoreID;
		JoltQueryFilterCallback filterCallback;
		Jolt::PxShape *shape;
		_controller->getActor()->getShapes(&shape, 1);
		shape->setFlag(Jolt::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		Quaternion orientation(RN::Vector3(0.0f, 0.0f, 90.0f));
		scene->overlap(shape->getGeometry().any(), Jolt::PxTransform(Jolt::PxVec3(position.x, position.y, position.z), Jolt::PxQuat(orientation.x, orientation.y, orientation.z, orientation.w)), hit, Jolt::PxQueryFilterData(filterData, Jolt::PxQueryFlag::eDYNAMIC|Jolt::PxQueryFlag::eSTATIC|Jolt::PxQueryFlag::ePREFILTER), &filterCallback);
		shape->setFlag(Jolt::PxShapeFlag::eSCENE_QUERY_SHAPE, true);*/
		
		JoltContactInfo contact;
		contact.distance = -1.0f;
		contact.node = nullptr;
		contact.collisionObject = nullptr;

		/*if(hit.getNbAnyHits() == 0)
			return contact;

		Jolt::PxOverlapHit closestHit = hit.getAnyHit(0);
		contact.distance = 0.0f;
		contact.position = Vector3(position.x, position.y, position.z);
		contact.normal = Vector3(0.0f, 0.0f, 0.0f);
		if(closestHit.actor)
		{
			JoltCollisionObject *collisionObject = static_cast<JoltCollisionObject*>(closestHit.actor->userData);
			contact.collisionObject = collisionObject;
			if(collisionObject->GetParent())
			{
				contact.node = collisionObject->GetParent();
				if(contact.node) contact.node->Retain()->Autorelease();
			}
		}*/
		return contact;
	}

	std::vector<JoltContactInfo> JoltKinematicController::OverlapTestAll() const
	{
		/*const Jolt::PxExtendedVec3 &position = _controller->getPosition();
		Jolt::PxScene *scene = JoltWorld::GetSharedInstance()->GetJoltScene();
		Jolt::PxOverlapHit hitBuffer[256];
		Jolt::PxOverlapBuffer hit(hitBuffer, 255);
		Jolt::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		filterData.word2 = _collisionFilterID;
		filterData.word3 = _collisionFilterIgnoreID;
		JoltQueryFilterCallback filterCallback;
		Jolt::PxShape *shape;
		_controller->getActor()->getShapes(&shape, 1);
		shape->setFlag(Jolt::PxShapeFlag::eSCENE_QUERY_SHAPE, false);
		Quaternion orientation(RN::Vector3(0.0f, 0.0f, 90.0f));
		scene->overlap(shape->getGeometry().any(), Jolt::PxTransform(Jolt::PxVec3(position.x, position.y, position.z), Jolt::PxQuat(orientation.x, orientation.y, orientation.z, orientation.w)), hit, Jolt::PxQueryFilterData(filterData, Jolt::PxQueryFlag::eDYNAMIC|Jolt::PxQueryFlag::eSTATIC|Jolt::PxQueryFlag::ePREFILTER|Jolt::PxQueryFlag::eNO_BLOCK), &filterCallback);
		shape->setFlag(Jolt::PxShapeFlag::eSCENE_QUERY_SHAPE, true);
*/
		std::vector<JoltContactInfo> contacts;

	/*	if(hit.getNbAnyHits() == 0)
			return contacts;
		
		for(uint32 i = 0; i < hit.getNbAnyHits(); i++)
		{
			Jolt::PxOverlapHit currentHit = hit.getAnyHit(i);
			
			JoltContactInfo contact;
			contact.distance = 0.0f;
			contact.node = nullptr;
			contact.collisionObject = nullptr;

			contact.position = Vector3(position.x, position.y, position.z);
			contact.normal = Vector3(0.0f, 0.0f, 0.0f);
			if(currentHit.actor)
			{
				JoltCollisionObject *collisionObject = static_cast<JoltCollisionObject*>(currentHit.actor->userData);
				contact.collisionObject = collisionObject;
				if(collisionObject->GetParent())
				{
					contact.node = collisionObject->GetParent();
					if(contact.node) contact.node->Retain()->Autorelease();
				}
			}

			contacts.push_back(contact);
		}*/

        return contacts;
	}

	bool JoltKinematicController::Resize(float height, bool checkIfBlocked)
	{
		/*bool isBlocked = false;
		float oldHeight = _controller->getHeight();
		if(checkIfBlocked && height > oldHeight)
		{
			float radius = _controller->getRadius();
			float dh = std::max(height - oldHeight - 2.0f * radius, 0.0f);
			Jolt::PxCapsuleGeometry geom(radius, dh * 0.5f);
			float temporaryShapeHeight = dh + radius * 2.0f;
			float offset = height - temporaryShapeHeight * 0.5f;

			Jolt::PxExtendedVec3 position = _controller->getPosition();
			Jolt::PxVec3 pos((float)position.x, (float)position.y + offset, (float)position.z);
			Jolt::PxQuat orientation(k::Pi_2, Jolt::PxVec3(0.0f, 0.0f, 1.0f));

			Jolt::PxFilterData filterData;
			filterData.word0 = _collisionFilterGroup;
			filterData.word1 = _collisionFilterMask;
			filterData.word2 = _collisionFilterID;
			filterData.word3 = _collisionFilterIgnoreID;
			Jolt::PxOverlapBuffer hit;
			JoltQueryFilterCallback filterCallback;
			JoltWorld *JoltWorld = JoltWorld::GetSharedInstance();
			if(JoltWorld->GetJoltScene()->overlap(geom, Jolt::PxTransform(pos, orientation), hit, Jolt::PxQueryFilterData(filterData, Jolt::PxQueryFlag::eANY_HIT|Jolt::PxQueryFlag::eSTATIC|Jolt::PxQueryFlag::eDYNAMIC|Jolt::PxQueryFlag::ePREFILTER), &filterCallback)) isBlocked = true;
		}
		
		if(!isBlocked)
		{
			_controller->resize(height);
		}
		
		return !isBlocked;*/
		
		return false;
	}

	void JoltKinematicController::SetCollisionFilter(uint32 group, uint32 mask)
	{
		JoltCollisionObject::SetCollisionFilter(group, mask);

/*		Jolt::PxShape *shape;
		_controller->getActor()->getShapes(&shape, 1);

		Jolt::PxFilterData filterData;
		filterData.word0 = _collisionFilterGroup;
		filterData.word1 = _collisionFilterMask;
		filterData.word2 = _collisionFilterID;
		filterData.word3 = _collisionFilterIgnoreID;
		shape->setSimulationFilterData(filterData);
		shape->setQueryFilterData(filterData);
		shape->setFlag(Jolt::PxShapeFlag::eSIMULATION_SHAPE, false);*/

//		_controller->invalidateCache();
	}

	Vector3 JoltKinematicController::GetFeetOffset() const
	{
/*		Jolt::PxVec3 footPosition = Jolt::toVec3(_controller->getFootPosition());
		Jolt::PxVec3 position = Jolt::toVec3(_controller->getPosition());
		Jolt::PxVec3 offset = footPosition - position;*/
		return Vector3();//Vector3(offset.x, offset.y, offset.z);
	}

	void JoltKinematicController::Jump(float force)
	{
		_fallSpeed = force;
		_isFalling = true;
	}

	void JoltKinematicController::DidUpdate(SceneNode::ChangeSet changeSet)
	{
		JoltCollisionObject::DidUpdate(changeSet);
		
		if(changeSet & SceneNode::ChangeSet::Position)
		{
			Vector3 position = GetWorldPosition() - _positionOffset;
			_controller->SetPosition(JPH::RVec3Arg(position.x, position.y, position.z));
		}

		if(changeSet & SceneNode::ChangeSet::Attachments)
		{
			if(!_owner && GetParent())
			{
				Vector3 position = GetWorldPosition() - _positionOffset;
				_controller->SetPosition(JPH::RVec3Arg(position.x, position.y, position.z));
			}

			_owner = GetParent();
		}
	}

	void JoltKinematicController::UpdatePosition()
	{
		if(!_owner)
		{
			return;
		}
		
		JPH::RVec3 position = _controller->GetPosition();
		SetWorldPosition(Vector3(position.GetX(), position.GetY(), position.GetZ()) + _positionOffset);
	}
}
