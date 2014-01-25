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
	struct ShadowParameter
	{
		ShadowParameter(size_t tresolution = 1024) :
			resolution(tresolution),
			splits(4),
			distanceBlendFactor(0.05f),
			biasFactor(2.0f),
			biasUnits(512.0f),
			shadowTarget(nullptr)
		{}
		
		ShadowParameter(Camera *target, size_t tresolution = 1024) :
			resolution(tresolution),
			splits(4),
			distanceBlendFactor(0.05f),
			biasFactor(2.0f),
			biasUnits(512.0f),
			shadowTarget(target)
		{}
		
		size_t resolution;
		
		size_t splits;
		float distanceBlendFactor;
		float biasFactor;
		float biasUnits;
		Camera *shadowTarget;
	};
	
	class Light : public SceneNode
	{
	public:
		friend class Renderer;
		
		enum class Type
		{
			PointLight,
			SpotLight,
			DirectionalLight
		};
		
		RNAPI Light(Type type = Type::PointLight);
		RNAPI ~Light() override;
		
		RNAPI bool ActivateShadows(const ShadowParameter &parameter = ShadowParameter());
		RNAPI void DeactivateShadows();
		RNAPI bool HasShadows() const { return (_shadowDepthCameras.GetCount() > 0 && !_suppressShadows); }
		RNAPI void SetSuppressShadows(bool suppress);
		
		RNAPI void Render(Renderer *renderer, Camera *camera) override;
		RNAPI void Update(float delta) override;
		
		RNAPI bool IsVisibleInCamera(Camera *camera) override;
		
		RNAPI void SetRange(float range);
		RNAPI void SetColor(const Color& color);
		RNAPI void SetAngle(float angle);
		RNAPI void SetIntensity(float intensity);
		
		const Color& GetColor() const { return _color; }
		const Vector3& GetResultColor() { return _resultColor; }
		
		const Type GetType() const { return _lightType; }
		float GetRange() const { return _range; }
		float GetAngle() const { return _angle; }
		float GetAngleCos() const { return _angleCos; }
		float GetIntensity() const { return _intensity; }
		
		const std::vector<Matrix> &GetShadowMatrices() const { return _shadowCameraMatrices; }
		const Array *GetShadowCameras() const { return &_shadowDepthCameras; }
	
	private:
		void ReCalculateColor();
		void RemoveShadowCameras();
		
		bool ActivateDirectionalShadows(const ShadowParameter &parameter);
		bool ActivatePointShadows(const ShadowParameter &parameter);
		bool ActivateSpotShadows(const ShadowParameter &parameter);
		
		Type _lightType;
		
		Observable<Color> _color;
		Vector3 _resultColor;
		Vector3 _direction;
		
		Observable<float> _intensity;
		Observable<float> _range;
		Observable<float> _angle;
		
		float _angleCos;
		
		Camera *_shadowTarget;
		std::vector<Matrix> _shadowCameraMatrices;
		Array _shadowDepthCameras;
		bool _suppressShadows;
		ShadowParameter _shadowParameter;
		
		RNDefineMetaWithTraits(Light, SceneNode, MetaClassTraitCronstructable)
	};
}

#endif /* __RAYNE_LIGHT_H__ */
