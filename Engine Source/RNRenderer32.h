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
		RNAPI void DrawMesh(Mesh *mesh, uint32 offset, uint32 count);
		RNAPI void DrawMeshInstanced(const RenderingObject& object);
		
		RNAPI void FlushCamera(Camera *camera, Shader *drawShader) override;
		RNAPI void DrawCameraStage(Camera *camera, Camera *stage) override;
		RNAPI void DrawCamera(Camera *camera, Camera *source, uint32 skyCubeMeshes) override;
		
		RNAPI void AdjustDrawBuffer(Camera *camera, Camera *target);
		
		GLuint _copyVAO;
		GLuint _copyVBO;
		Vector4 _copyVertices[4];
		
		uint32 _maxLightFastPath;
	};
}

#endif /* __RAYNE_RENDERER32_H__ */
