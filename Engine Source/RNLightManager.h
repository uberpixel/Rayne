//
//  RNLightManager.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_LIGHT_MANAGER_H__
#define __RAYNE_LIGHT_MANAGER_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNSceneNode.h"
#include "RNRect.h"
#include "RNPlane.h"
#include "RNSphere.h"
#include "RNShader.h"

namespace RN
{
	class Light;
	class Camera;
	class Renderer;
	class ShaderProgram;
	class Texture;
	
	class LightManager : public Object
	{
	public:
		friend class Camera;
		
		RNAPI LightManager();
		
		RNAPI virtual void UpdateProgram(Renderer *renderer, ShaderProgram *program) = 0;
		RNAPI virtual void AdjustProgramTypes(Shader *program, uint32 &types) = 0;
		
		RNAPI virtual void AddLight(Light *light) = 0;
		RNAPI virtual void CreateLightLists() = 0;
		
		RNAPI virtual size_t GetSpotlightCount() = 0;
		RNAPI virtual size_t GetPointLightCount() = 0;
		RNAPI virtual size_t GetDirectionalLightCount() = 0;
		
		Camera *GetCamera() const { return camera; }
		
		static LightManager *CreateDefaultLightManager();
		
	protected:
		Camera *camera;
		
		RNDefineMeta(LightManager, Object)
	};
	
	class ClusteredLightManager : public LightManager
	{
	public:		
		RNAPI ClusteredLightManager();
		RNAPI ~ClusteredLightManager();
		
		RNAPI void UpdateProgram(Renderer *renderer, ShaderProgram *program) final;
		RNAPI void AdjustProgramTypes(Shader *program, uint32 &types) final;
		
		RNAPI void AddLight(Light *light) final;
		RNAPI void CreateLightLists() final;
		
		RNAPI void SetClusterSize(const Vector3 &size);
		RNAPI void SetMaxDirectionalLights(size_t maxLights);
		RNAPI void SetMaxLightsPerCluster(size_t maxLights);
		
		size_t GetSpotlightCount() final { return _spotLightCount; }
		size_t GetPointLightCount() final { return _pointLightCount; }
		size_t GetDirectionalLightCount() final { return _directionalLightCount; }
		
		const Vector3 &GetClusterSize() const { return _clusterSize; }
		size_t GetMaxLightsPerCluster() const { return _maxLightsPerCluster; }
		size_t GetMaxDirectionalLights() const { return _maxLightsDirect; }
		
	private:
		void CullLights();
		
		void CreatePointSpotLightLists();
		void CreateDirectionalLightList();
		
		size_t _maxLightsDirect;
		size_t _maxLightsPerCluster;
		size_t _pointSpotLightCount;
		size_t _pointLightCount;
		size_t _spotLightCount;
		size_t _directionalLightCount;
		
		Vector3 _clusterSize;
		
		std::vector<Light *> _pointLights;
		std::vector<Light *> _spotLights;
		std::deque<Light *> _directionalLights;
		
		GLuint _lightTextures[4];
		GLuint _lightBuffers[4];
		
		size_t _lightPointDataSize;
		size_t _lightSpotDataSize;
		
		uint16 *_lightIndices;
		int32 *_lightOffsetCount;
		size_t _lightIndicesSize;
		size_t _lightOffsetCountSize;
		
		std::vector<Vector3> _lightDirectionalDirection;
		std::vector<Vector4> _lightDirectionalColor;
		std::vector<Matrix> _lightDirectionalMatrix;
		std::vector<Texture *> _lightDirectionalDepth;
		
		std::vector<Vector4> _lightSpotPosition;
		std::vector<Vector4> _lightSpotDirection;
		std::vector<Vector4> _lightSpotColor;
		std::vector<Matrix> _lightSpotMatrix;
		std::vector<Texture *> _lightSpotDepth;
		std::vector<Vector4> _lightSpotData;
		
		std::vector<Vector4> _lightPointPosition;
		std::vector<Vector4> _lightPointColor;
		std::vector<Texture *> _lightPointDepth;
		std::vector<Vector4> _lightPointData;
		
		RNDefineMeta(ClusteredLightManager, LightManager)
	};
}

#endif /* __RAYNE_LIGHT_MANAGER_H__ */
