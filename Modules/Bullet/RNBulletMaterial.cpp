//
//  RNBulletMaterial.cpp
//  Rayne-Bullet
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNBulletMaterial.h"

namespace RN
{
	RNDefineMeta(BulletMaterial, Object)
		
	BulletMaterial::BulletMaterial()
	{
		_linearDamping  = 0.0f;
		_angularDamping = 0.0f;
			
		_friction = 0.5f;
		_rollingFriction = 0.003f;
		_restitution = 0.1f;
	}
		
		
		
	void BulletMaterial::SetLinearDamping(float damping)
	{
		_linearDamping = damping;
		signal.Emit(this);
	}
		
	void BulletMaterial::SetAngularDamping(float damping)
	{
		_angularDamping = damping;
		signal.Emit(this);
	}
		
	void BulletMaterial::SetFriction(float friction)
	{
		_friction = friction;
		signal.Emit(this);
	}

	void BulletMaterial::SetRollingFriction(float friction)
	{
		_rollingFriction = friction;
		signal.Emit(this);
	}
		
	void BulletMaterial::SetRestitution(float restitution)
	{
		_restitution = restitution;
		signal.Emit(this);
	}
}
