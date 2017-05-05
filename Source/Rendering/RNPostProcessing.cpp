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
	RNDefineMeta(PostProcessingStage, Object)

	PostProcessingStage::PostProcessingStage()
	{
		Initialize();
	}

	PostProcessingStage::~PostProcessingStage()
	{
		SafeRelease(_inputFramebuffer);
		SafeRelease(_outputFramebuffer);
		SafeRelease(_material);
	}

	void PostProcessingStage::Initialize()
	{
		_inputFramebuffer = nullptr;
		_outputFramebuffer = nullptr;
		_material   = nullptr;
	}

	// Setter
	void PostProcessingStage::SetFrame(const Rect &frame)
	{
		_frame = std::move(frame.GetIntegral());
	}

	void PostProcessingStage::SetInputFramebuffer(Framebuffer *framebuffer)
	{
		SafeRelease(_inputFramebuffer);
		_inputFramebuffer = framebuffer->Retain();
	}

	void PostProcessingStage::SetOutputFramebuffer(Framebuffer *framebuffer)
	{
		SafeRelease(_outputFramebuffer);
		_outputFramebuffer = framebuffer->Retain();
	}

	void PostProcessingStage::SetMaterial(Material *material)
	{
		SafeRelease(_material);
		_material = SafeRetain(material);
	}
}
