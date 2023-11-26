//
//  RNJoltMaterial.h
//  Rayne-Jolt
//
//  Copyright 2023 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_JOLTMATERIAL_H_
#define __RAYNE_JOLTMATERIAL_H_

#include "RNJolt.h"

namespace JPH
{
	class PhysicsMaterial;
}

namespace RN
{
	class JoltMaterial : public Object
	{
	public:
		JTAPI JoltMaterial(float staticFriction = 0.5f, float dynamicFriction = 0.5f, float restitution = 0.1f);
		JTAPI ~JoltMaterial();
			
		JTAPI void SetStaticFriction(float friction);
		JTAPI void SetDynamicFriction(float friction);
		JTAPI void SetRestitution(float restitution);
			
		JTAPI float GetStaticFriction() const;
		JTAPI float GetDynamicFriction() const;
		JTAPI float GetRestitution() const;

		JTAPI JPH::PhysicsMaterial *GetJoltMaterial() const { return _material; }
			
	private:
		JPH::PhysicsMaterial *_material;
			
		RNDeclareMetaAPI(JoltMaterial, JTAPI)
	};
}

#endif /* defined(__RAYNE_JOLTMATERIAL_H_) */
