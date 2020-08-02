//
//  RNPhysXMaterial.h
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PHYSXMATERIAL_H_
#define __RAYNE_PHYSXMATERIAL_H_

#include "RNPhysX.h"

namespace physx
{
	class PxMaterial;
}

namespace RN
{
	class PhysXMaterial : public Object
	{
	public:
		PXAPI PhysXMaterial(float staticFriction = 0.5f, float dynamicFriction = 0.5f, float restitution = 0.1f);
			
		PXAPI void SetStaticFriction(float friction);
		PXAPI void SetDynamicFriction(float friction);
		PXAPI void SetRestitution(float restitution);
			
		PXAPI float GetStaticFriction() const;
		PXAPI float GetDynamicFriction() const;
		PXAPI float GetRestitution() const;

		PXAPI physx::PxMaterial *GetPhysXMaterial() const { return _material; }
			
	private:
		physx::PxMaterial *_material;
			
		RNDeclareMetaAPI(PhysXMaterial, PXAPI)
	};
}

#endif /* defined(__RAYNE_PHYSXMATERIAL_H_) */
