//
//  RNRendering.h
//  Rayne
//
//  Copyright 2013 by Felix Pohl, Nils Daumann and Sidney Just. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_RENDERING_H__
#define __RAYNE_RENDERING_H__

#include "RNBase.h"
#include "RNObject.h"
#include "RNSpinLock.h"
#include "RNRenderingResource.h"

#include "RNCamera.h"
#include "RNVector.h"
#include "RNMatrix.h"
#include "RNQuaternion.h"
#include "RNTexture.h"
#include "RNMaterial.h"
#include "RNMesh.h"

namespace RN
{
	class RenderingIntent : public RenderingResource
	{
	public:
		RenderingIntent(Mesh *_mesh, Material *_material) :
			RenderingResource("Rendering Intent")
		{
			mesh = _mesh->Retain<Mesh>();
			material = _material->Retain<Material>();
		}
		
		RenderingIntent(const RenderingIntent& other) :
			RenderingResource(other._name)
		{
			mesh = other.mesh->Retain<Mesh>();
			material = other.material->Retain<Material>();
			
			transform = other.transform;
		}
		
		~RenderingIntent()
		{
			mesh->Release();
			material->Release();
		}
		
		
		Matrix transform;
		Material *material;
		Mesh *mesh;
	};
	
	class RenderingGroup : public RenderingResource
	{
	public:
		RenderingGroup() :
			RenderingResource("Rendering Group")
		{
		}
		
		Camera *camera;
		std::vector<RenderingIntent> intents;
	};
}

#endif /* __RAYNE_RENDERING_H__ */
