//
//  RNLightEntity.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_LIGHTENTITY_H__
#define __RAYNE_LIGHTENTITY_H__

#include "RNBase.h"
#include "RNEntity.h"
#include "RNRendering.h"
#include "RNMaterial.h"
#include "RNMesh.h"

namespace RN
{
	class LightEntity : public Entity
	{
	public:
		LightEntity();
		LightEntity(LightEntity *other);
		
		virtual ~LightEntity();
		
		virtual void Update(float delta);
		virtual void PostUpdate();
		
	private:
		Vector3 _position;
		Vector3 _color;
		float _range;
		Vector3 _direction;
		float _angle;
	};
}

#endif /* __RAYNE_LIGHTENTITY_H__ */
