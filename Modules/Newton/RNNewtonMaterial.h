//
//  RNPhysXMaterial.h
//  Rayne-PhysX
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_PHYSXMATERIAL_H_
#define __RAYNE_PHYSXMATERIAL_H_

#include "RNNewton.h"

namespace physx
{
	class PxMaterial;
}

namespace RN
{
	class PhysXMaterial : public Object
	{
	public:
		PXAPI PhysXMaterial();
			
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
