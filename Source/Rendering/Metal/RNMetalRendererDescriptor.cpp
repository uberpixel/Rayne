//
//  RNMetalRendererDescriptor.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMetalRendererDescriptor.h"
#include "RNMetalRenderer.h"

namespace RN
{
	MetalRendererDescriptor::MetalRendererDescriptor() :
		RenderingDescriptor(RNCSTR("net.uberpixel.rendering.metal"), RNCSTR("Metal"))
	{}

	Renderer *MetalRendererDescriptor::CreateAndSetActiveRenderer()
	{
		MetalRenderer *renderer = new MetalRenderer();
		renderer->Activate();

		return renderer;
	}
}
