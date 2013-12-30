//
//  RNLight.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
		enum class Type
		{
			PointLight,
			SpotLight,
			DirectionalLight
		};
		
		RNAPI Light(Type type = Type::PointLight);
		RNAPI ~Light() override;
		
		RNAPI void ActivateDirectionalShadows(bool shadow=true, int resolution=1024, int splits=4, float distfac=0.05f, float biasfac=2.0f, float biasunits=512.0f);
		RNAPI void ActivatePointShadows(bool shadow=true, int resolution=1024);
		RNAPI void ActivateSpotShadows(bool shadow=true, int resolution=1024);
		
		RNAPI void Render(Renderer *renderer, Camera *camera) override;
		RNAPI void Update(float delta) override;
		
		RNAPI bool IsVisibleInCamera(Camera *camera) override;
		
		RNAPI void SetRange(float range);
		RNAPI void SetColor(const Color& color);
		RNAPI void SetAngle(float angle);
		RNAPI void SetIntensity(float intensity);
		RNAPI void SetLightCamera(Camera *lightCamera);
		
		const Color& GetColor() const { return _color; }
		const Vector3& GetResultColor() { return _resultColor; }
		
		const Type GetType() const { return _lightType; }
		float GetRange() const { return _range; }
		float GetAngle() const { return _angle; }
		float GetAngleCos() const { return _angleCos; }
		float GetIntensity() const { return _intensity; }
		
		bool Shadow() const { return _shadow; }
		
		Camera *GetShadowCamera() const { return _shadowcam; }
		Camera *GetLightCamera() const { return _lightcam; }
		
		const std::vector<Matrix> &GetShadowMatrices() const { return _shadowmats; }
		const Array *GetShadowCameras() const { return &_shadowcams; }
	
	private:
		void ReCalculateColor();
		void RemoveShadowCameras();
		
		Type _lightType;
		
		Observable<Color> _color;
		Vector3 _resultColor;
		Vector3 _direction;
		
		Observable<float> _intensity;
		Observable<float> _range;
		Observable<float> _angle;
		
		float _angleCos;
		
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
