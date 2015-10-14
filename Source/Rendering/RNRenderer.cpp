//
//  RNRenderer.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNRenderer.h"
#include "../Base/RNSettings.h"

namespace RN
{
	RNDefineMeta(Renderer, Object)

	static Renderer *_activeRenderer = nullptr;

	Renderer::Renderer()
	{}

	Renderer *Renderer::GetActiveRenderer()
	{
		return _activeRenderer;
	}

	Dictionary *Renderer::GetParameters() const
	{
		Dictionary *renderer = Settings::GetSharedInstance()->GetEntryForKey<Dictionary>(RNCSTR("RNRenderer"));
		if(renderer)
		{
			Dictionary *parameters = renderer->GetObjectForKey<Dictionary>(RNCSTR("parameters"));
			return parameters;
		}

		return nullptr;
	}

	void Renderer::Activate()
	{
		RN_ASSERT(!_activeRenderer, "Rayne only supports one active renderer at a time");
		_activeRenderer = this;
	}

	void Renderer::Deactivate()
	{
		_activeRenderer = nullptr;
	}
}
