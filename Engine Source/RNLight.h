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

#define kRNShadowCameraPriority 1000

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
		RNAPI ~Light() override;
		
		RNAPI void ActivateSunShadows(bool shadow=true, float resolution=1024.0f, int splits=4, float distfac=0.05f, float biasfac=2.0f, float biasunits=512.0f);
		
		RNAPI void Render(Renderer *renderer, Camera *camera) override;
		RNAPI void Update(float delta) override;
		RNAPI bool CanUpdate(FrameID frame) override;
		
		RNAPI bool IsVisibleInCamera(Camera *camera) override;
		
		RNAPI void SetRange(float range);
		RNAPI void SetColor(const Color& color);
		RNAPI void SetAngle(float angle);
		RNAPI void SetIntensity(float intensity);
		RNAPI void SetShadowCamera(Camera *shadowCamera);
		RNAPI void SetLightCamera(Camera *lightCamera);
		
		const Color& Color() const { return _color; }
		const Vector3& ResultColor() { return _resultColor; }
		
		const Type LightType() const { return _lightType; }
		float Range() const { return _range; }
		float Angle() const { return _angle; }
		float Intensity() const { return _intensity; }
		bool Shadow() const { return _shadow; }
		
		Camera *ShadowCamera() const { return _shadowcam; }
		Camera *LightCamera() const { return _lightcam; }
		
		const std::vector<Matrix> ShadowMatrices() const { return _shadowmats; }
		const Array *ShadowCameras() const { return &_shadowcams; }
	
	private:
		void ReCalculateColor();
		
		Type _lightType;
		class Color _color;
		Vector3 _resultColor;
		Vector3 _direction;
		float _intensity;
		float _range;
		float _angle;
		
		Camera *_shadowcam;
		Camera *_lightcam;
		std::vector<Matrix> _shadowmats;
		Array _shadowcams;
		bool _shadow;
		int _shadowSplits;
		float _shadowDistFac;
		
		RNDefineMetaWithTraits(Light, SceneNode, MetaClassTraitCronstructable)
	};
}

#endif /* __RAYNE_LIGHT_H__ */
