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
		
		RNAPI virtual bool IsVisibleInCamera(Camera *camera);
		
		RNAPI void SetRange(float range);
		RNAPI void SetColor(const Vector3& color);
		RNAPI void SetAngle(float angle);
		RNAPI void SetShadow(bool shadow);
		
		virtual void Update(float delta);
		
		const Type LightType() const { return _lightType; }
		const Vector3& Color() const { return _color; }
		const float Range() const { return _range; }
		const float Angle() const { return _angle; }
		const bool Shadow() const { return _shadow; }
		
		class Camera *_shadowcam;
		class Camera *_lightcam;
		Array<Matrix> _shadowmats;
	
	private:
		Type _lightType;
		Vector3 _color;
		Vector3 _direction;
		float _range;
		float _angle;
		bool _shadow;
		
		RNDefineMeta(Light, Transform)
	};
}

#endif /* __RAYNE_LIGHT_H__ */
