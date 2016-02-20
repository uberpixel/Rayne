//
//  RNMetalDevice.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNMetalDevice.h"

namespace RN
{
	RNDefineMeta(MetalDevice, RenderingDevice)

	MetalDevice::MetalDevice(id<MTLDevice> device) :
		RenderingDevice(RNSTR([[device name] UTF8String]), DescriptorFromDevice(device)),
		_device(device)
	{}

	RenderingDevice::Descriptor MetalDevice::DescriptorFromDevice(id<MTLDevice> device)
	{
		Descriptor descriptor;

		descriptor.type = Type::Discrete;

		if([[device name] containsString:@"Intel"])
			descriptor.type = Type::Integrated;

		descriptor.vendorID = 0;
		descriptor.driverVersion = 0;

		if([device supportsFeatureSet:MTLFeatureSet_OSX_GPUFamily1_v1])
			descriptor.apiVersion = RNVersionMake(1, 1, 0);

		return descriptor;
	}
}
