//
//  RNLight.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_LIGHT_H__
#define __RAYNE_LIGHT_H__

#include "RNBase.h"
#include "RNTransform.h"
#include "RNCamera.h"

namespace RN
{
	class Light : public Transform
	{
	friend class Renderer;
	public:
		enum Type
		{
			TypePointLight = 0,
			TypeSpotLight = 1,
			TypeDirectionalLight = 2
		};
		
		Light(Type type = TypePointLight);
		virtual ~Light();
		
		virtual void Update(float delta);
		virtual bool IsVisibleInCamera(Camera *camera);
		
		void SetRange(float range);
		void SetColor(Vector3 color);
		void SetAngle(float angle);
		
		const Type LightType() { return _lightType; }
		const Vector3& Color() { return _color; }
		const Vector3& Direction() { return _direction; }
		const float Range() { return _range; }
		const float Angle() { return _angle; }
	
		
	private:
		Type _lightType;
		Vector3 _color;
		Vector3 _direction;
		float _range;
		float _angle;
		
		RNDefineMeta(Light, Transform)
	};
}

#endif /* __RAYNE_LIGHT_H__ */
