//
//  RNLight.h
//  Rayne
//
//  Copyright 2017 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_LIGHT_H__
#define __RAYNE_LIGHT_H__

#include "../Base/RNBase.h"
#include "../Rendering/RNTexture.h"
#include "RNSceneNode.h"

namespace RN
{
	struct ShadowSplit
	{
		ShadowSplit(size_t updateInterval = 1, size_t updateOffset = 0, float biasFactor = 2.0f, float biasUnit = 512.0f, float distance = 0.0f) :
			biasFactor(biasFactor),
			biasUnits(biasUnit),
			maxDistance(distance),
			updateInterval(updateInterval),
			updateOffset(updateOffset)
		{}
		
		float biasFactor;
		float biasUnits;
		float maxDistance;
		size_t updateInterval;
		size_t updateOffset;
	};
	
	struct ShadowParameter
	{
		ShadowParameter(size_t resolution = 512) :
			resolution(resolution),
			depthTextureFormat(Texture::Format::Depth_32F),
			clipNear(0.0f),
			clipFar(100000.0f),
			directionalShadowDistance(5000.0f),
			distanceBlendFactor(0.05f),
			maxShadowDist(1.0f),
			shadowTarget(nullptr)
		{
			splits.push_back(ShadowSplit());
		}
		
		ShadowParameter(Camera *target, size_t resolution = 1024) :
			resolution(resolution),
			depthTextureFormat(Texture::Format::Depth_32F),
			clipNear(0.0f),
			clipFar(100000.0f),
			directionalShadowDistance(5000.0f),
			distanceBlendFactor(0.05f),
			maxShadowDist(1.0f),
			shadowTarget(target)
		{
			splits.push_back(ShadowSplit(1, 0, 3.0f, 256.0f));
			splits.push_back(ShadowSplit(2, 0, 3.0f, 512.0f));
			splits.push_back(ShadowSplit(2, 1, 3.0f, 1024.0f));
			splits.push_back(ShadowSplit(3, 0, 3.0f, 1024.0f));
		}
		
		size_t resolution;
		Texture::Format depthTextureFormat;
		float clipNear;
		float clipFar;
		float directionalShadowDistance;
		
		std::vector<ShadowSplit> splits;
		float distanceBlendFactor;
		float maxShadowDist;
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
		RNAPI Light(const Light *other);
		RNAPI ~Light() override;
		
		RNAPI bool ActivateShadows(const ShadowParameter &parameter = ShadowParameter(), bool useMultiview = false);
		RNAPI void DeactivateShadows();
		bool HasShadows() const { return (_shadowDepthCameras.GetCount() > 0 && !_suppressShadows); }
		RNAPI void SetSuppressShadows(bool suppress);
		RNAPI void UpdateShadowParameters(const ShadowParameter &parameter);
		
		RNAPI void Render(Renderer *renderer, Camera *camera) const override;
		RNAPI void Update(float delta) override;
		RNAPI void DidUpdate(ChangeSet change) override;
		
		RNAPI bool CanRender(Renderer *renderer, Camera *camera) const override;
		
		RNAPI void SetType(Type type);
		RNAPI void SetRange(float range);
		RNAPI void SetColor(const Color &color);
		RNAPI void SetAngle(float angle);
		RNAPI void SetIntensity(float intensity);
		
		const Color &GetColor() const { return _color; }
		const Vector3 &GetFinalColor() { return _finalColor; }
		
		const Type GetType() const { return _lightType; }
		float GetRange() const { return _range; }
		float GetAngle() const { return _angle; }
		float GetAngleCos() const { return _angleCos; }
		float GetIntensity() const { return _intensity; }
		
		ShadowParameter GetShadowParameters() const {return _shadowParameter;}
		
		const std::vector<Matrix> &GetShadowMatrices() const { return _shadowCameraMatrices; }
		const Array *GetShadowDepthCameras() const { return &_shadowDepthCameras; }
		Texture *GetShadowDepthTexture() const { return _shadowDepthTexture; }
		
		IntrusiveList<Light>::Member _lightSceneEntry; //TODO: Make private but keep accessible to user made scene implementations
	
	private:
		void ReCalculateColor();
		void RemoveShadowCameras();
		void SetRangeInternal(float range);
		
		bool ActivateDirectionalShadows(bool useMultiview);
		bool ActivatePointShadows(bool useMultiview);
		bool ActivateSpotShadows(bool useMultiview);
		
		void UpdateShadows();
		
		Type _lightType;
		
		Color _color;
		Vector3 _finalColor;
		
		float _intensity;
		float _range;
		float _angle;
		
		float _angleCos;
		
		Camera *_shadowTarget;
		std::vector<Matrix> _shadowCameraMatrices;
		Array _shadowDepthCameras;
		Texture *_shadowDepthTexture;
		bool _suppressShadows;
		
		ShadowParameter _shadowParameter;
		Camera *_multiviewShadowParentCamera;

		__RNDeclareMetaInternal(Light)
	};
}

#endif /* __RAYNE_LIGHT_H__ */
