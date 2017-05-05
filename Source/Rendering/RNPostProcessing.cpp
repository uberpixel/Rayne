//
//  RNCamera.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNPostProcessing.h"
#include "../Rendering/RNRenderer.h"
#include "../Rendering/RNWindow.h"

namespace RN
{
	RNDefineMeta(PostProcessingAPIStage, RenderPass)
	RNDefineMeta(PostProcessingStage, RenderPass)

	PostProcessingAPIStage::PostProcessingAPIStage(Type type) : _type(type)
	{

	}

	PostProcessingAPIStage::~PostProcessingAPIStage()
	{
		
	}



	PostProcessingStage::PostProcessingStage() : _material(nullptr)
	{

	}

	PostProcessingStage::~PostProcessingStage()
	{
		SafeRelease(_material);
	}

	// Setter
	void PostProcessingStage::SetMaterial(Material *material)
	{
		SafeRelease(_material);
		_material = SafeRetain(material);
	}
}
