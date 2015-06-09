//
//  RNRenderer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRenderer.h"

namespace RN
{
	static Renderer *_activeRenderer = nullptr;

	Renderer::Renderer()
	{}

	Renderer *Renderer::GetActiveRenderer()
	{
		return _activeRenderer;
	}

	void Renderer::Activate()
	{
		RN_ASSERT(_activeRenderer, "Rayne only supports one active renderer at a time");
		_activeRenderer = this;
	}
}
