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
		// Check for parameters that need to be activated before interacting with the Metal API
		if(parameters)
		{
			Number *apiValidation = const_cast<Dictionary *>(parameters)->GetObjectForKey<Number>(RNCSTR("api_validation"));
			if(apiValidation && apiValidation->GetBoolValue())
				setenv("METAL_DEVICE_WRAPPER_TYPE", "1", 1);
		}

		// Probe the provided devices
		bool hasHighPoweredDevice = false;
		NSArray *devices = MTLCopyAllDevices();

		for(id<MTLDevice> device in devices)
		{
			if(![device isLowPower] && ![device isHeadless])
			{
				hasHighPoweredDevice = true;
				break;
			}
		}

		[devices release];

		return hasHighPoweredDevice;
	}
}
