//
//  RNVulkanDevice.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanDevice.h"
#include "RNVulkanDebug.h"

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
		_workQueue(kRNNotFound),
		_deviceExtensions(nullptr),
		_maxMultiviewViewCount(1),
		_supportsFragmentDensityMaps(false)
	{
		std::vector<VkQueueFamilyProperties> queues;
		GetQueueProperties(queues);

		for(uint32_t i = 0; i < queues.size(); i++)
		{
			const VkQueueFamilyProperties &queue = queues[i];
			const VkFlags flags = VK_QUEUE_GRAPHICS_BIT;

#if RN_PLATFORM_WINDOWS
			bool result = (vk::GetPhysicalDeviceWin32PresentationSupportKHR(_physicalDevice, i) == VK_TRUE);
#endif
#if RN_PLATFORM_LINUX
			const xcb_screen_t *screen = Screen::GetMainScreen()->GetXCBScreen();
			bool result = (vk::GetPhysicalDeviceXcbPresentationSupportKHR(_physicalDevice, i, Kernel::GetSharedInstance()->GetXCBConnection(), screen->root_visual) == VK_TRUE);
#endif
#if RN_PLATFORM_ANDROID
			bool result = true;
#endif

			if(result && (queue.queueFlags & flags) == flags)
			{
				_workQueue = i;
				break;
			}
		}

		vk::GetPhysicalDeviceMemoryProperties(device, &_memoryProperties);
	}

	VulkanDevice::~VulkanDevice()
	{
		SafeRelease(_deviceExtensions);
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
		RNVulkanValidate(vk::GetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, surface, &count, nullptr));

		formats.resize(count);
		return vk::GetPhysicalDeviceSurfaceFormatsKHR(_physicalDevice, surface, &count, formats.data());
	}

	VkResult VulkanDevice::GetPresentModes(VkSurfaceKHR surface, std::vector<VkPresentModeKHR> &modes)
	{
		uint32_t count = 0;
		RNVulkanValidate(vk::GetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, surface, &count, nullptr));

		modes.resize(count);
		return vk::GetPhysicalDeviceSurfacePresentModesKHR(_physicalDevice, surface, &count, modes.data());
	}

	VkBool32 VulkanDevice::GetMemoryWithType(uint32_t typeBits, VkFlags properties, uint32_t &typeIndex) const
	{
		for(uint32_t i = 0; i < _memoryProperties.memoryTypeCount; i++)
		{
			if((typeBits & 1) == 1)
			{
				if((_memoryProperties.memoryTypes[i].propertyFlags & properties) == properties)
				{
					typeIndex = i;

					return VK_TRUE;
				}
			}

			typeBits >>= 1;
		}

		return VK_FALSE;
	}

	size_t VulkanDevice::GetMemoryWithType(VkMemoryPropertyFlagBits required) const
	{
		for(size_t i = 0; i < _memoryProperties.memoryTypeCount; i ++)
		{
			uint32_t flags = _memoryProperties.memoryTypes[i].propertyFlags;

			if((flags & required) == required)
				return i;
		}

		return kRNNotFound;
	}

	bool VulkanDevice::CreateDevice(const std::vector<const char *> &extensions)
	{
		std::vector<const char *> deviceExtensions(extensions);
		if(_deviceExtensions)
		{
			_deviceExtensions->Enumerate<String>([&](String *extension, size_t index, bool &stop){
				deviceExtensions.push_back(extension->GetUTF8String());
			});
		}

		//Check if optional extensions are available and add to extensions list if they are
		std::vector<VkExtensionProperties> rawDeviceExtensions;
		_instance->EnumerateDeviceExtensions(_physicalDevice, nullptr, rawDeviceExtensions);
		for(const auto &extension : rawDeviceExtensions)
		{
			//check for multiview extension
			if(std::strcmp(extension.extensionName, VK_KHR_MULTIVIEW_EXTENSION_NAME) == 0)
			{
				deviceExtensions.push_back(extension.extensionName);

				VkPhysicalDeviceMultiviewPropertiesKHR multiviewProperties;
				multiviewProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES_KHR;
				multiviewProperties.pNext = NULL;
				
				VkPhysicalDeviceProperties2 deviceProperties;
				deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
				deviceProperties.pNext = &multiviewProperties;

				vk::GetPhysicalDeviceProperties2KHR(_physicalDevice, &deviceProperties);

				_maxMultiviewViewCount = multiviewProperties.maxMultiviewViewCount;

				RNDebug("Maximum number of multiviews: " << _maxMultiviewViewCount << " instances: " << multiviewProperties.maxMultiviewInstanceIndex);
			}
			else if(std::strcmp(extension.extensionName, VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME) == 0)
			{
				deviceExtensions.push_back(extension.extensionName);

				VkPhysicalDeviceFragmentDensityMapPropertiesEXT fragmentDensityMapProperties;
				fragmentDensityMapProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_PROPERTIES_EXT;
				fragmentDensityMapProperties.pNext = NULL;

				VkPhysicalDeviceProperties2 deviceProperties;
				deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
				deviceProperties.pNext = &fragmentDensityMapProperties;

				vk::GetPhysicalDeviceProperties2KHR(_physicalDevice, &deviceProperties);
				_minFragmentDensityTexelSize.x = fragmentDensityMapProperties.minFragmentDensityTexelSize.width;
				_minFragmentDensityTexelSize.y = fragmentDensityMapProperties.minFragmentDensityTexelSize.height;
				_maxFragmentDensityTexelSize.x = fragmentDensityMapProperties.maxFragmentDensityTexelSize.width;
				_maxFragmentDensityTexelSize.y = fragmentDensityMapProperties.maxFragmentDensityTexelSize.height;
				_supportsFragmentDensityMaps = true;
			}
		}

		VkPhysicalDeviceFragmentDensityMapFeaturesEXT fragmentDensityMapFeatures = {};
		fragmentDensityMapFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT;

		VkPhysicalDeviceMultiviewFeaturesKHR multiviewFeatures = {};
		multiviewFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES_KHR;
		multiviewFeatures.pNext = &fragmentDensityMapFeatures;

		VkPhysicalDeviceFeatures2KHR features = {};
		features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
		features.pNext = &multiviewFeatures;

		vk::GetPhysicalDeviceFeatures2KHR(_physicalDevice, &features);

		const std::vector<float> queuePriorities(1, 0.0f);
		VkDeviceQueueCreateInfo queueInfo = {};
		queueInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueInfo.queueFamilyIndex = static_cast<uint32_t>(_workQueue);
		queueInfo.queueCount = 1;
		queueInfo.pQueuePriorities = queuePriorities.data();

		std::vector<const char *> layers = DebugDeviceLayers();

		VkDeviceCreateInfo deviceInfo = {};
		deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		deviceInfo.queueCreateInfoCount = 1;
		deviceInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
		deviceInfo.ppEnabledExtensionNames = deviceExtensions.data();
		deviceInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
		deviceInfo.ppEnabledLayerNames = layers.data();
		deviceInfo.pEnabledFeatures = nullptr; //Using VkPhysicalDeviceFeatures2KHR as pNext pointer instead to enable all available features, including extensions, which this one doesn't support
		deviceInfo.pQueueCreateInfos = &queueInfo;
		deviceInfo.pNext = &features;

		VkResult result = vk::CreateDevice(_physicalDevice, &deviceInfo, nullptr, &_device);
		RNVulkanValidate(result);

/*		VkPhysicalDeviceProperties properties;
		vk::GetPhysicalDeviceProperties(_physicalDevice, &properties);
		RNDebug("Max uniform buffer size: " << properties.limits.maxUniformBufferRange);*/

		return (result == VK_SUCCESS);
	}

	void VulkanDevice::SetExtensions(Array *extensions)
	{
		_deviceExtensions = extensions;
		SafeRetain(_deviceExtensions);
	}
}
