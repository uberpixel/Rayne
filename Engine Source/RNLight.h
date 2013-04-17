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
#include "RNColor.h"
#include "RNSceneNode.h"
#include "RNCamera.h"

namespace RN
{
	class Light : public SceneNode
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
		RNAPI void SetColor(const Color& color);
		RNAPI void SetAngle(float angle);
		RNAPI void SetIntensity(float intensity);
		
		RNAPI virtual void Render(Renderer *renderer, Camera *camera);
		
		const Color& Color() const { return _color; }
		const Vector3& ResultColor() { return _resultColor; }
		
		const Type LightType() const { return _lightType; }
		float Range() const { return _range; }
		float Angle() const { return _angle; }
		float Intensity() const { return _intensity; }
		
		const Vector3& Direction();
		
	private:
		void ReCalculateColor();
		
		Type _lightType;
		class Color _color;
		Vector3 _resultColor;
		Vector3 _direction;
		float _intensity;
		float _range;
		float _angle;
		
		RNDefineMeta(Light, SceneNode)
	};
}

#endif /* __RAYNE_LIGHT_H__ */
