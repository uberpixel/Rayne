//
//  RND3D12RendererDescriptor.cpp
//  Rayne
//
//  Copyright 2015 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RND3D12RendererDescriptor.h"
#include "RND3D12Renderer.h"

namespace RN
{
	RNDefineMeta(D3D12RendererDescriptor, RendererDescriptor)

	void D3D12RendererDescriptor::InitialWakeUp(MetaClass *meta)
	{
		if(meta == D3D12RendererDescriptor::GetMetaClass())
		{
			D3D12RendererDescriptor *descriptor = new D3D12RendererDescriptor();
			RendererManager::GetSharedInstance()->AddDescriptor(descriptor);
			descriptor->Release();
		}
	}

	D3D12RendererDescriptor::D3D12RendererDescriptor() :
		RN::RendererDescriptor(RNCSTR("net.uberpixel.rendering.d3d12"), RNCSTR("D3D12"))
	{}


	Renderer *D3D12RendererDescriptor::CreateRenderer(const Dictionary *parameters)
	{
		D3D12Renderer *renderer = new D3D12Renderer(parameters);
		return renderer;
	}

	bool D3D12RendererDescriptor::CanConstructWithSettings(const Dictionary *parameters) const
	{
		// Check for parameters that need to be activated before interacting with the D3D12 API
/*		if(parameters)
		{
			Number *apiValidation = const_cast<Dictionary *>(parameters)->GetObjectForKey<Number>(RNCSTR("api_validation"));
			if(apiValidation && apiValidation->GetBoolValue())
				setenv("D3D12_DEVICE_WRAPPER_TYPE", "1", 1);
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

		return hasHighPoweredDevice;*/

		return true;
	}
}
