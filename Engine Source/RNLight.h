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
	struct ShadowSplit
	{
		ShadowSplit(size_t updateInterval = 1, size_t updateOffset = 0) :
			biasFactor(2.0f),
			biasUnits(512.0f),
			updateInterval(updateInterval),
			updateOffset(updateOffset)
		{}
		
		float biasFactor;
		float biasUnits;
		size_t updateInterval;
		size_t updateOffset;
	};
	
	struct ShadowParameter
	{
		ShadowParameter(size_t resolution = 1024) :
			resolution(resolution),
			distanceBlendFactor(0.05f),
			shadowTarget(nullptr)
		{
			splits.push_back(ShadowSplit());
		}
		
		ShadowParameter(Camera *target, size_t resolution = 1024) :
			resolution(resolution),
			distanceBlendFactor(0.05f),
			shadowTarget(target)
		{
			splits.push_back(ShadowSplit(1, 0));
			splits.push_back(ShadowSplit(2, 0));
			splits.push_back(ShadowSplit(2, 1));
			splits.push_back(ShadowSplit(3, 0));
		}
		
		size_t resolution;
		
		std::vector<ShadowSplit> splits;
		float distanceBlendFactor;
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
		RNAPI void UpdateShadowParameters(const ShadowParameter &parameter);
		
		RNAPI void Render(Renderer *renderer, Camera *camera) override;
		RNAPI void Update(float delta) override;
		
		RNAPI bool IsVisibleInCamera(Camera *camera) override;
		
		RNAPI void SetRange(float range);
		RNAPI void SetColor(const Color& color);
		RNAPI void SetAngle(float angle);
		RNAPI void SetIntensity(float intensity);
		
		const Color& GetColor() const { return _color; }
		const Vector3& GetFinalColor() { return _finalColor; }
		
		const Type GetType() const { return _lightType; }
		float GetRange() const { return _range; }
		float GetAngle() const { return _angle; }
		float GetAngleCos() const { return _angleCos; }
		float GetIntensity() const { return _intensity; }
		
		ShadowParameter GetShadowParameters() const {return _shadowParameter;}
		
		const std::vector<Matrix> &GetShadowMatrices() const { return _shadowCameraMatrices; }
		const Array *GetShadowDepthCameras() const { return &_shadowDepthCameras; }
	
	private:
		void ReCalculateColor();
		void RemoveShadowCameras();
		
		bool ActivateDirectionalShadows();
		bool ActivatePointShadows();
		bool ActivateSpotShadows();
		
		Type _lightType;
		
		Observable<Color, Light> _color;
		Vector3 _finalColor;
		Vector3 _direction;
		
		Observable<float, Light> _intensity;
		Observable<float, Light> _range;
		Observable<float, Light> _angle;
		
		float _angleCos;
		
		Camera *_shadowTarget;
		std::vector<Matrix> _shadowCameraMatrices;
		Array _shadowDepthCameras;
		bool _suppressShadows;
		ShadowParameter _shadowParameter;
		
		RNDeclareMetaWithTraits(Light, SceneNode, MetaClassTraitCronstructable)
	};
}

#endif /* __RAYNE_LIGHT_H__ */
