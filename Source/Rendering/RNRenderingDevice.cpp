//
//  RNRenderingDevice.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "../Objects/RNString.h"
#include "RNRenderingDevice.h"

namespace RN
{
	RNDefineMeta(RenderingDevice, Object)

	RenderingDevice::RenderingDevice(const String *name, const Descriptor &descriptor) :
		_name(SafeCopy(name))
	{
		_apiVersion = descriptor.apiVersion;
		_driverVersion = descriptor.driverVersion;
		_vendorID = descriptor.vendorID;
	}

	RenderingDevice::~RenderingDevice()
	{
		SafeRelease(_name);
	}

	const String *RenderingDevice::GetDescription() const
	{
		String *apiString = RNSTR(RNVersionGetMajor(_apiVersion) << "." << RNVersionGetMinor(_apiVersion) << "." << RNVersionGetPatch(_apiVersion));
		String *driverString = RNSTR(RNVersionGetMajor(_driverVersion) << "." << RNVersionGetMinor(_driverVersion) << "." << RNVersionGetPatch(_driverVersion));

		return RNSTR("<" << GetMetaClass()->GetFullname() << ":" << (void *)this << "> (" << _name << ", " << "API: " << apiString << ", Driver: " << driverString << ")");
	}
}
