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

	VulkanDevice::Descriptor VulkanDevice::DescriptorForDevice(VkPhysicalDevice device)
	{
		VkPhysicalDeviceProperties properties;
		vk::GetPhysicalDeviceProperties(device, &properties);

		Descriptor descriptor;

		descriptor.apiVersion = RNVersionMake(VK_VERSION_MAJOR(properties.apiVersion), VK_VERSION_MINOR(properties.apiVersion), VK_VERSION_PATCH(properties.apiVersion));
		descriptor.driverVersion = RNVersionMake(VK_VERSION_MAJOR(properties.driverVersion), VK_VERSION_MINOR(properties.driverVersion), VK_VERSION_PATCH(properties.driverVersion));
		descriptor.vendorID = properties.vendorID;

		switch(properties.deviceType)
		{
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
				descriptor.type = Type::Integrated;
				break;
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
				descriptor.type = Type::Discrete;
				break;
			case VK_PHYSICAL_DEVICE_TYPE_CPU:
				descriptor.type = Type::CPU;
				break;
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
				descriptor.type = Type::Virtual;
				break;
			default:
				descriptor.type = Type::Other;
				break;
	}

		return descriptor;
	}

	String *GetNameForDevice(VkPhysicalDevice device)
	{
		VkPhysicalDeviceProperties properties;
		vk::GetPhysicalDeviceProperties(device, &properties);

		return RNSTR(properties.deviceName);
	}

	VulkanDevice::VulkanDevice(VulkanInstance *instance, VkPhysicalDevice device) :
		RenderingDevice(GetNameForDevice(device), DescriptorForDevice(device)),
		_instance(instance),
		_device(device)
	{}
}
