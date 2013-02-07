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
		
		RenderingLight Light();
		
	private:
		Vector3 _color;
		float _range;
	};
	
	RN_INLINE RenderingLight LightEntity::Light()
	{
		RenderingLight light;
		light.position = Position();
		light.color = _color;
		light.range = _range;
		
		return light;
	}
}

#endif /* __RAYNE_LIGHTENTITY_H__ */
