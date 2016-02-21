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
	RNDefineMeta(VulkanDevice, RenderingDevice)

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
		_physicalDevice(device),
		_gameQueue(kRNNotFound),
		_presentQueue(kRNNotFound)
	{
		std::vector<VkQueueFamilyProperties> queues;
		GetQueueProperties(queues);

		for (uint32_t i = 0; i < queues.size(); i++)
		{
			const VkQueueFamilyProperties &queue = queues[i];
			const VkFlags flags = VK_QUEUE_GRAPHICS_BIT;

			if(_gameQueue == kRNNotFound && (queue.queueFlags & flags) == flags)
				_gameQueue = i;

			if(_presentQueue == kRNNotFound)
			{
#if RN_PLATFORM_WINDOWS
				bool result = vk::GetPhysicalDeviceWin32PresentationSupportKHR(_physicalDevice, i);
#endif

				if(result)
					_presentQueue = i;
			}

			if(_gameQueue >= 0 && _presentQueue >= 0)
				break;
		}
	}

	void VulkanDevice::GetQueueProperties(std::vector<VkQueueFamilyProperties> &queues)
	{
		uint32_t count = 0;
		vk::GetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &count, nullptr);

		queues.resize(count);
		vk::GetPhysicalDeviceQueueFamilyProperties(_physicalDevice, &count, queues.data());
	}

	VkResult VulkanDevice::GetSurfaceFormats(VkSurfaceKHR surface, std::vector<VkSurfaceFormatKHR> &formats)
	{
		uint32_t count = 0;
		vk::GetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, surface, &count, nullptr);

		formats.resize(count);
		return vk::GetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, surface, &count, formats.data());
	}

	VkResult VulkanDevice::GetPresentModes(VkSurfaceKHR surface, std::vector<VkPresentModeKHR> &modes)
	{
		uint32_t count = 0;
		vk::GetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, surface, &count, nullptr);

		modes.resize(count);
		return vk::GetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, surface, &count, modes.data());
	}

	bool VulkanDevice::CreateDevice(const std::vector<const char *> &extensions)
	{
		VkPhysicalDeviceFeatures features;
		vk::GetPhysicalDeviceFeatures(_physicalDevice, &features);


		VkDeviceCreateInfo deviceInfo = {};
		deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceInfo.queueCreateInfoCount = 1;
		deviceInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
		deviceInfo.ppEnabledExtensionNames = extensions.data();
		deviceInfo.pEnabledFeatures = &features;


		const std::vector<float> queuePriorities(1, 0.0f);
		std::array<VkDeviceQueueCreateInfo, 2> queueInfo = {};
		queueInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo[0].queueFamilyIndex = static_cast<uint32_t>(_gameQueue);
		queueInfo[0].queueCount = 1;
		queueInfo[0].pQueuePriorities = queuePriorities.data();

		if(_gameQueue != _presentQueue)
		{
			queueInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			queueInfo[1].queueFamilyIndex = static_cast<uint32_t>(_presentQueue);
			queueInfo[1].queueCount = 1;
			queueInfo[1].pQueuePriorities = queuePriorities.data();

			deviceInfo.queueCreateInfoCount ++;
		}

		deviceInfo.pQueueCreateInfos = queueInfo.data();

		return (vk::CreateDevice(_physicalDevice, &deviceInfo, nullptr, &_device) == VK_SUCCESS);
	}
}
