//
//  RNLightManager.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
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

namespace RN
{
	class Light;
	class Camera;
	class Renderer;
		
	class LightManager
	{
	public:
		friend class Renderer;
		friend class Renderer32;
		
		LightManager();
		~LightManager();
		
		
		void AddLight(Light *light);
		bool CreateLightLists(Camera *camera);
		
		int CreatePointLightList(Camera *camera);
		int CreateSpotLightList(Camera *camera);
		int CreateDirectionalLightList(Camera *camera);
		
	private:
		void CullLights(Camera *camera);
		
		int _maxLightFastPath;
		
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
		std::vector<Texture *> _lightSpotDepth;
		std::vector<Vector4> _lightSpotData;
		
		std::vector<Vector4> _lightPointPosition;
		std::vector<Vector4> _lightPointColor;
		std::vector<Texture *> _lightPointDepth;
		std::vector<Vector4> _lightPointData;
	};
}

#endif /* __RAYNE_LIGHT_MANAGER_H__ */
