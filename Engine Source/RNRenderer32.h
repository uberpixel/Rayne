//
//  RNRenderer32.h
//  Rayne
//
//  Copyright 2014 by Überpixel. All rights reserved.
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
	
	protected:
		RNAPI void DrawMesh(Mesh *mesh, uint32 offset, uint32 count);
		RNAPI void DrawMeshInstanced(const RenderingObject &object);
		
		RNAPI void FlushCamera(Camera *camera, Shader *drawShader) override;
		RNAPI void DrawCameraStage(Camera *camera, Camera *stage) override;
		RNAPI void DrawCamera(Camera *camera, Camera *source, size_t skyCubeMeshes) override;
		
		RNAPI void AdjustDrawBuffer(Camera *camera, Camera *target);
		RNAPI void BeginFrame(float delta) override;
		
		GLuint _copyVAO;
		GLuint _copyVBO;
		Vector4 _copyVertices[4];
		Vector4 _copyAtlas;
	};
}

#endif /* __RAYNE_RENDERER32_H__ */
