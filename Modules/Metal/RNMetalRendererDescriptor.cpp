//
//  RNMetalRendererDescriptor.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#import <Metal/Metal.h>
#include "RNMetalRendererDescriptor.h"
#include "RNMetalRenderer.h"

namespace RN
{
	RNDefineMeta(MetalRendererDescriptor, RendererDescriptor)

	void MetalRendererDescriptor::InitialWakeUp(MetaClass *meta)
	{
		if(meta == MetalRendererDescriptor::GetMetaClass())
		{
			MetalRendererDescriptor *descriptor = new MetalRendererDescriptor();
			RendererCoordinator::GetSharedInstance()->AddDescriptor(descriptor);
			descriptor->Release();
		}
	}

	MetalRendererDescriptor::MetalRendererDescriptor() :
		RN::RendererDescriptor(RNCSTR("net.uberpixel.rendering.metal"), RNCSTR("Metal"))
	{}


	Renderer *MetalRendererDescriptor::CreateRenderer(const Dictionary *parameters)
	{
		MetalRenderer *renderer = new MetalRenderer(parameters);
		return renderer;
	}

	bool MetalRendererDescriptor::CanConstructWithSettings(const Dictionary *parameters) const
	{
		bool hasHighPoweredDevice = false;
		NSArray *devices = MTLCopyAllDevices();

		for(id<MTLDevice> device in devices)
		{
			if(![device isLowPower])
			{
				hasHighPoweredDevice = true;
				break;
			}
		}

		[devices release];

		return hasHighPoweredDevice;
	}
}
