//
//  RNLight.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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
		
		RNAPI Light(Type type = TypePointLight);
		RNAPI virtual ~Light();
		
		RNAPI virtual void Update(float delta);
		RNAPI virtual bool IsVisibleInCamera(Camera *camera);
		
		RNAPI void SetRange(float range);
		RNAPI void SetColor(Vector3 color);
		RNAPI void SetAngle(float angle);
		
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
