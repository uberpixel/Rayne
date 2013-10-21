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
		
		void CullLights(Camera *camera, Light **lights, size_t lightCount, GLuint indicesBuffer, GLuint offsetBuffer);
		int CreatePointLightList(Camera *camera);
		int CreateSpotLightList(Camera *camera);
		int CreateDirectionalLightList(Camera *camera);
		
	protected:
		int *_lightIndicesBuffer;
		int *_tempLightIndicesBuffer;
		int *_lightOffsetBuffer;
		size_t _lightIndicesBufferSize;
		size_t _lightOffsetBufferSize;
		
		size_t _lightPointDataSize;
		GLuint _lightPointTextures[3];
		GLuint _lightPointBuffers[3];
		
		size_t _lightSpotDataSize;
		GLuint _lightSpotTextures[3];
		GLuint _lightSpotBuffers[3];
		
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
		
	private:
		void AllocateLightBufferStorage(size_t indicesSize, size_t offsetSize);
	};
}

#endif /* __RAYNE_LIGHT_MANAGER_H__ */
