//
//  RNBulletMaterial.h
//  Rayne-Bullet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_BULLETMATERIAL_H_
#define __RAYNE_BULLETMATERIAL_H_

#include "RNBullet.h"

namespace RN
{
	class BulletMaterial : public Object
	{
	public:
		BTAPI BulletMaterial();
			
		BTAPI void SetLinearDamping(float damping);
		BTAPI void SetAngularDamping(float damping);
		BTAPI void SetFriction(float friction);
		BTAPI void SetRollingFriction(float friction);
		BTAPI void SetSpinningFriction(float friction);
		BTAPI void SetRestitution(float restitution);
			
		BTAPI float GetLinearDamping() const { return _linearDamping; }
		BTAPI float GetAngularDamping() const { return _angularDamping; }
		BTAPI float GetFriction() const { return _friction; }
		BTAPI float GetRollingFriction() const { return _rollingFriction; }
		BTAPI float GetSpinningFriction() const { return _spinningFriction; }
		BTAPI float GetRestitution() const { return _restitution; }
			
		Signal<void (BulletMaterial *)> signal;
			
	private:
		float _linearDamping;
		float _angularDamping;
			
		float _friction;
		float _rollingFriction;
		float _spinningFriction;
		float _restitution;
			
		RNDeclareMetaAPI(BulletMaterial, BTAPI)
	};
}

#endif /* defined(__RAYNE_BULLETMATERIAL_H_) */
