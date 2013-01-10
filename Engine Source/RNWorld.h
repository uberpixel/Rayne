//
//  RNWorld.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_WORLD_H__
#define __RAYNE_WORLD_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNCamera.h"
#include "RNArray.h"
#include "RNRenderingResource.h"
#include "RNRendererFrontend.h"

namespace RN
{
	class Kernel;
	class World : public Object
	{
	public:
		World(Kernel *kernel);
		virtual ~World();
		
		void Update(float delta);
		
	private:
		Kernel *_kernel;
		ObjectArray *_cameras;
		RendererFrontend *_renderer;
		
		void CreateTestMesh();
		
		Mesh *mesh;
		Texture *texture;
		Material *material;
		Shader *shader;
		Matrix transform;
	};
}

#endif /* __RAYNE_WORLD_H__ */
