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
		_type = descriptor.type;
	}

	RenderingDevice::~RenderingDevice()
	{
		SafeRelease(_name);
	}
	
	bool RenderingDevice::IsEqual(const Object *other) const
	{
		const RenderingDevice *device = other->Downcast<RenderingDevice>();
		return (device && _name->IsEqual(device->_name) && _apiVersion == device->_apiVersion && _driverVersion == device->_driverVersion && _vendorID == device->_vendorID && _type == device->_type);
	}

	const String *RenderingDevice::GetDescription() const
	{
		String *apiString = GetVersionString(_apiVersion);
		String *driverString = GetVersionString(_driverVersion);

		return RNSTR("<" << GetClass()->GetFullname() << ":" << (void *)this << "> (" << _name << " (Vendor: " << std::hex << _vendorID << "), " << "API: " << apiString << ", Driver: " << driverString << ")");
	}
}
