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
#include "RNMatrix.h"
#include "RNModel.h"

namespace RN
{
	class Entity;
	class LightEntity;
	class RenderingGroup : public RenderingResource
	{
	public:
		RenderingGroup() :
			RenderingResource("Rendering Group")
		{
		}
		
		Camera *camera;
		std::vector<Entity *> entities;
		std::vector<LightEntity *> lights;
	};
}

#endif /* __RAYNE_RENDERING_H__ */
