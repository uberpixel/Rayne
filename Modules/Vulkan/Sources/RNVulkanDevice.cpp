//
//  RNVulkanDevice.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanDevice.h"

namespace RN
{
	RNDefineMeta(VulkanDevice, Object)

	VulkanDevice::VulkanDevice(VulkanInstance *instance, VkPhysicalDevice device) :
		_instance(instance),
		_device(device),
		_name(nullptr)
	{

	}

	const String *VulkanDevice::GetDescription() const
	{
		String *apiString = RNSTR(RNVersionGetMajor(_apiVersion) << "." << RNVersionGetMinor(_apiVersion) << "." << RNVersionGetPatch(_apiVersion));
		String *driverString = RNSTR(RNVersionGetMajor(_driverVersion) << "." << RNVersionGetMinor(_driverVersion) << "." << RNVersionGetPatch(_driverVersion));

		return RNSTR("<VulkanDevice::" << (void *)this << "> (" << _name << ", " << "API: " << apiString << ", Driver: " << driverString << ")");
	}

	bool VulkanDevice::ParseDevice()
	{
		VkPhysicalDeviceProperties properties;
		vk::GetPhysicalDeviceProperties(_device, &properties);

		_name = new String(properties.deviceName);
		_type = properties.deviceType;
		_apiVersion = properties.apiVersion;
		_driverVersion = properties.driverVersion;
		_vendorID = properties.vendorID;
		_deviceID = properties.deviceID;

		return true;
	}
}
