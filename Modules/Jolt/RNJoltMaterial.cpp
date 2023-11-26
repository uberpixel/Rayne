//
//  RNJoltMaterial.cpp
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNJoltMaterial.h"
#include "RNJoltWorld.h"
#include "RNJoltInternals.h"


namespace RN
{
	RNDefineMeta(JoltMaterial, Object)
		
	JoltMaterial::JoltMaterial(float staticFriction, float dynamicFriction, float restitution)
	{
		_material = new JPH::PhysicsMaterial();
		_material->AddRef();
		/*_material->
		_material = physics->createMaterial(staticFriction, dynamicFriction, restitution);
		_material->setFrictionCombineMode(Jolt::PxCombineMode::Enum::eMULTIPLY);
		//_material->setRestitutionCombineMode(PxCombineMode::Enum combMode);
		_material->setFlag(Jolt::PxMaterialFlag::eIMPROVED_PATCH_FRICTION, true);*/
	}
		
	JoltMaterial::~JoltMaterial()
	{
		_material->Release();
	}

		
	void JoltMaterial::SetStaticFriction(float friction)
	{
		//_material->setStaticFriction(friction);
	}
		
	void JoltMaterial::SetDynamicFriction(float friction)
	{
		//_material->setDynamicFriction(friction);
	}

	void JoltMaterial::SetRestitution(float restitution)
	{
		//_material->setRestitution(restitution);
	}



	float JoltMaterial::GetStaticFriction() const
	{
		return 0.0f;//_material->getStaticFriction();
	}

	float JoltMaterial::GetDynamicFriction() const
	{
		return 0.0f;//_material->getDynamicFriction();
	}

	float JoltMaterial::GetRestitution() const
	{
		return 0.1f;//_material->getRestitution();
	}
}
