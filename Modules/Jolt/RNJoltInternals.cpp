//
//  RNJoltInternals.cpp
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNJoltInternals.h"
#include "RNJoltCollisionObject.h"
#include "RNJoltKinematicController.h"
#include "RNJoltWorld.h"

#include <Jolt/Physics/Body/BodyInterface.h>

namespace RN
{
	void JoltCharacterContactListener::OnAdjustBodyVelocity(const JPH::CharacterVirtual *inCharacter, const JPH::Body &inBody2, JPH::Vec3 &ioLinearVelocity, JPH::Vec3 &ioAngularVelocity)
	{
		/* Do nothing, the linear and angular velocity are already filled in */
	}

	bool JoltCharacterContactListener::OnContactValidate(const JPH::CharacterVirtual *inCharacter, const JPH::CharacterContact &inContact)
	{
		return true;
	}

	void JoltCharacterContactListener::OnContactAdded(const JPH::CharacterVirtual *inCharacter, const JPH::CharacterContact &inContact, JPH::CharacterContactSettings &ioSettings)
	{
		/* Default do nothing */
		JoltContactInfo info;

		JPH::PhysicsSystem *physics = controller->GetJoltWorld()->GetJoltInstance();
		JPH::BodyInterface &bodyInterface = physics->GetBodyInterface();
		info.collisionObject = reinterpret_cast<JoltCollisionObject *>(bodyInterface.GetUserData(inContact.mBodyB));
		{
			JPH::TransformedShape transformedShape = bodyInterface.GetTransformedShape(inContact.mBodyB);
			if(transformedShape.mShape)
			{
				JoltContactInfoShapeData::FillForTransformedShape(info, transformedShape, inContact.mSubShapeIDB);
			}
		}
		if(info.collisionObject) info.node = info.collisionObject->GetParent();
		if(info.node) info.node->Retain()->Autorelease();

		info.position = JoltConversions::ToPosition(inContact.mPosition);
		info.normal = JoltConversions::ToEngineVector(-inContact.mContactNormal);
		info.distance = 0.0f;
		if(controller->_contactCallback) controller->_contactCallback(info.collisionObject, info, JoltCollisionObject::ContactState::Begin);
	}

	void JoltCharacterContactListener::OnContactSolve(const JPH::CharacterVirtual *inCharacter, const JPH::BodyID &inBodyID2, const JPH::SubShapeID &inSubShapeID2, JPH::RVec3Arg inContactPosition, JPH::Vec3Arg inContactNormal, JPH::Vec3Arg inContactVelocity, const JPH::PhysicsMaterial *inContactMaterial, JPH::Vec3Arg inCharacterVelocity, JPH::Vec3 &ioNewCharacterVelocity)
	{
		/* Default do nothing */
	}
} // namespace RN
