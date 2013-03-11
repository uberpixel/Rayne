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
		enum Type
		{
			TypePointLight = 0,
			TypeSpotLight = 1,
			TypeDirectionalLight = 2
		};
		
		LightEntity(Type type = TypePointLight);
		LightEntity(LightEntity *other);
		
		virtual ~LightEntity();
		
		virtual void PostUpdate();
		virtual bool IsVisibleInCamera(Camera *camera);
		
		void SetRange(float range);
		void SetColor(Vector3 color);
		void SetDirection(Vector3 direction);
		void SetAngle(float angle);
		
		const Type LightType() { return _lightType; }
		const Past<Vector3>& Color() { return _color; }
		const Past<float> Range() { return _range; }
		const Past<Vector3>& Direction() { return _direction; }
		const Past<float> Angle() { return _angle; }
		
	private:
		Type _lightType;
		Past<Vector3> _color;
		Past<float> _range;
		Past<Vector3> _direction;
		Past<float> _angle;
	};
}

#endif /* __RAYNE_LIGHTENTITY_H__ */
