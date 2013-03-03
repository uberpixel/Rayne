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
#include "RNMaterial.h"
#include "RNMesh.h"

namespace RN
{
	class LightEntity : public Entity
	{
	friend class RenderingPipeline;
	public:
		LightEntity();
		LightEntity(LightEntity *other);
		
		virtual ~LightEntity();
		
		virtual void PostUpdate();
		virtual bool IsVisibleInCamera(Camera *camera);
		
		void SetRange(float range);
		void SetColor(Vector3 color);
		
		const Past<Vector3>& Color() { return _color; }
		const Past<float> Range() { return _range; }
		
	private:
		Past<Vector3> _color;
		Past<float> _range;
	};
}

#endif /* __RAYNE_LIGHTENTITY_H__ */
