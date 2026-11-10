//
//  RNPhysXMaterial.cpp
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNPhysXMaterial.h"
#include "RNPhysXWorld.h"

#include "PxPhysicsAPI.h"

namespace RN
{
	RNDefineMeta(PhysXMaterial, Object)

	PhysXMaterial::PhysXMaterial(float staticFriction, float dynamicFriction, float restitution)
	{
		physx::PxPhysics *physics = PhysXWorld::GetSharedInstance()->GetPhysXInstance();
		_material = physics->createMaterial(staticFriction, dynamicFriction, restitution);
		_material->setFrictionCombineMode(physx::PxCombineMode::Enum::eMULTIPLY);
		_material->setRestitutionCombineMode(physx::PxCombineMode::Enum::eMAX);
		_material->setFlag(physx::PxMaterialFlag::eIMPROVED_PATCH_FRICTION, true);
	}

	PhysXMaterial::~PhysXMaterial()
	{
		// PhysX releases any remaining materials when the world shuts down.
		if(PhysXWorld::GetSharedInstance()) _material->release();
	}


	void PhysXMaterial::SetStaticFriction(float friction)
	{
		_material->setStaticFriction(friction);
	}

	void PhysXMaterial::SetDynamicFriction(float friction)
	{
		_material->setDynamicFriction(friction);
	}

	void PhysXMaterial::SetRestitution(float restitution)
	{
		_material->setRestitution(restitution);
	}


	float PhysXMaterial::GetStaticFriction() const
	{
		return _material->getStaticFriction();
	}

	float PhysXMaterial::GetDynamicFriction() const
	{
		return _material->getDynamicFriction();
	}

	float PhysXMaterial::GetRestitution() const
	{
		return _material->getRestitution();
	}
} // namespace RN
