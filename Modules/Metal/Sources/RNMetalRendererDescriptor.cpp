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
#include "RNMetalDevice.h"

namespace RN
{
	RNDefineMeta(MetalRendererDescriptor, RendererDescriptor)

	void MetalRendererDescriptor::InitialWakeUp(MetaClass *meta)
	{
		if(meta == MetalRendererDescriptor::GetMetaClass())
		{
			MetalRendererDescriptor *descriptor = new MetalRendererDescriptor();
			GetExtensionPoint()->AddExtension(descriptor, 0);
			descriptor->Release();
		}
	}

	MetalRendererDescriptor::MetalRendererDescriptor() :
		RN::RendererDescriptor(RNCSTR("net.uberpixel.rendering.metal"), RNCSTR("Metal")),
		_devices(new Array())
	{}


	void MetalRendererDescriptor::PrepareWithSettings(const Dictionary *settings)
	{
		// Check for settings that need to be activated before interacting with the Metal API
		if(settings)
		{
			Number *apiValidation = const_cast<Dictionary *>(settings)->GetObjectForKey<Number>(RNCSTR("api_validation"));
			if(apiValidation && apiValidation->GetBoolValue())
				setenv("METAL_DEVICE_WRAPPER_TYPE", "1", 1);
		}

		NSArray *devices = MTLCopyAllDevices();

		for(id<MTLDevice> device in devices)
		{
			if(![device isHeadless])
			{
				MetalDevice *temp = new MetalDevice(device);
				_devices->AddObject(temp);
				temp->Release();
			}
		}

		[devices release];

		_devices->Sort<MetalDevice>([](const MetalDevice *deviceA, const MetalDevice *deviceB) -> bool {

			if(deviceA->GetType() == MetalDevice::Type::Discrete)
				return true;

			return false;

		});
	}

	Renderer *MetalRendererDescriptor::CreateRenderer(RenderingDevice *device)
	{
		MetalRenderer *renderer = new MetalRenderer(this, static_cast<MetalDevice *>(device));
		return renderer;
	}

	bool MetalRendererDescriptor::CanCreateRenderer() const
	{
		return (_devices->GetCount() > 0);
	}
}
