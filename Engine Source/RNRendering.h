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
#include "RNRenderingResource.h"
#include "RNCamera.h"
#include "RNEntity.h"
#include "RNLightEntity.h"

namespace RN
{
	class RenderingGroup : public RenderingResource
	{
	public:
		RenderingGroup(Camera *tcamera) :
			RenderingResource("Rendering Group")
		{
			camera = tcamera->Retain<Camera>();
		}
		
		RenderingGroup(const RenderingGroup& other) :
			RenderingResource(other._name),
			entities(other.entities),
			pointLights(other.pointLights),
			spotLights(other.spotLights),
			directionalLights(other.directionalLights)
		{
			camera = other.camera->Retain<Camera>();
		}
		
		~RenderingGroup()
		{
			camera->Release();
		}
		
		Camera *camera;
		Array<Entity> entities;
		Array<LightEntity> pointLights;
		Array<LightEntity> spotLights;
		Array<LightEntity> directionalLights;
	};
}

#endif /* __RAYNE_RENDERING_H__ */
