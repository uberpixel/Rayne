//
//  RNRenderer32.h
//  Rayne
//
//  Copyright 2013 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RENDERER32_H__
#define __RAYNE_RENDERER32_H__

#include "RNBase.h"
#include "RNRenderer.h"

namespace RN
{
	class Renderer32 : public Renderer
	{
	public:
		RNAPI Renderer32();
		RNAPI ~Renderer32() override;
		
		RNAPI void SetMaxLightFastPathCount(uint32 maxLights);
		uint32 GetMaxLightFastPathCount() const { return _maxLightFastPath; }
	
	protected:
		RNAPI void AllocateLightBufferStorage(size_t indicesSize, size_t offsetSize);
		RNAPI void DrawMesh(Mesh *mesh, uint32 offset, uint32 count);
		RNAPI void DrawMeshInstanced(const RenderingObject& object);
		
		RNAPI virtual void CullLights(Camera *camera, Light **lights, size_t lightCount, GLuint indicesBuffer, GLuint offsetBuffer);
		RNAPI virtual int CreatePointLightList(Camera *camera);
		RNAPI virtual int CreateSpotLightList(Camera *camera);
		RNAPI virtual int CreateDirectionalLightList(Camera *camera);
		
		RNAPI void FlushCamera(Camera *camera, Shader *drawShader) override;
		RNAPI void DrawCameraStage(Camera *camera, Camera *stage) override;
		RNAPI void DrawCamera(Camera *camera, Camera *source, uint32 skyCubeMeshes) override;
		
		RNAPI void AdjustDrawBuffer(Camera *camera, Camera *target);
		
		GLuint _copyVAO;
		GLuint _copyVBO;
		Vector4 _copyVertices[4];
		
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
		
		uint32 _maxLightFastPath;
		
		std::vector<Vector3> _lightDirectionalDirection;
		std::vector<Vector4> _lightDirectionalColor;
		std::vector<Matrix> _lightDirectionalMatrix;
		std::vector<Texture *> _lightDirectionalDepth;
		
		std::vector<Vector4> _lightSpotPosition;
		std::vector<Vector4> _lightSpotDirection;
		std::vector<Vector4> _lightSpotColor;
		std::vector<float> _lightSpotRanges;
		std::vector<Texture *> _lightSpotDepth;
		
		std::vector<Vector4> _lightPointPosition;
		std::vector<Vector4> _lightPointColor;
		std::vector<float> _lightPointRanges;
		std::vector<Texture *> _lightPointDepth;
	};
}

#endif /* __RAYNE_RENDERER32_H__ */
