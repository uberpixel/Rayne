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
	class RenderingIntent : public RenderingResource
	{
	public:
		RenderingIntent(Model *_model) :
			RenderingResource("Rendering Intent")
		{
			model = _model->Retain<Model>();
		}
		
		RenderingIntent(const RenderingIntent& other) :
			RenderingResource(other._name)
		{
			model = other.model->Retain<Model>();
			transform = other.transform;
		}
		
		~RenderingIntent()
		{
			model->Release();
		}
		
		
		Matrix transform;
		Model *model;
	};
	
	class RenderingGroup : public RenderingResource
	{
	public:
		RenderingGroup() :
			RenderingResource("Rendering Group")
		{
		}
		
		Camera *camera;
		
		Matrix viewMatrix;
		Matrix inverseViewMatrix;
		Matrix projectionMatrix;
		Matrix inverseProjectionMatrix;
		
		std::vector<RenderingIntent> intents;
	};
}

#endif /* __RAYNE_RENDERING_H__ */
