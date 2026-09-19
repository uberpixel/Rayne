//
//  RNVulkanDevice.cpp
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#include "RNVulkanDevice.h"
#include "RNVulkanDebug.h"
#include "RNVulkanGraphicsProvider.h"

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
		descriptor.deviceID = properties.deviceID;

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
		_supportsFragmentDensityMaps(false),
		_supportsFragmentDensityMaps2(false),
		_hasFragmentDensitySubsampledLoads(false),
		_hasFragmentDensitySubsampledCoarseReconstructionEarlyAccess(false),
		_maxFragmentDensitySubsampledLayers(0),
		_maxFragmentDensitySubsampledSamplers(0),
		_supportsTileProperties(false),
		_supportsSamplerAnisotropy(false),
		_supportsFullscreenExclusive(false),
		_supportsMultiDrawIndirect(false),
		_supportsShaderFloat16(false),
		_supportsShaderInt8(false),
		_maxSamplerAnisotropy(1.0f),
		_supportsExternalTextureImport(false),
		_supportsExternalTextureSynchronization(false),
		_hasDeviceLUID(false)
	{
		for(size_t i = 0; i < VK_LUID_SIZE; i++)
			_deviceLUID[i] = 0;

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

		VkPhysicalDeviceIDProperties idProperties = {};
		idProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_ID_PROPERTIES;

		VkPhysicalDeviceProperties2 deviceProperties = {};
		deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
		deviceProperties.pNext = &idProperties;

		vk::GetPhysicalDeviceProperties2(device, &deviceProperties);
		_maxSamplerAnisotropy = deviceProperties.properties.limits.maxSamplerAnisotropy;
		for(int i = 0; i < VK_UUID_SIZE; i++)
		{
			pipelineCacheUUID[i] = deviceProperties.properties.pipelineCacheUUID[i];
		}

		if(idProperties.deviceLUIDValid == VK_TRUE)
		{
			_hasDeviceLUID = true;
			for(size_t i = 0; i < VK_LUID_SIZE; i++)
				_deviceLUID[i] = idProperties.deviceLUID[i];
		}
	}

	VulkanDevice::~VulkanDevice()
	{
		SafeRelease(_deviceExtensions);
	}

	void VulkanDevice::GetPipelineCacheUUID(uint8 *uuid)
	{
		for(int i = 0; i < VK_UUID_SIZE; i++)
		{
			uuid[i] = pipelineCacheUUID[i];
		}
	}

	bool VulkanDevice::AddDeviceExtensionIfAvailable(std::vector<const char *> &enabledExtensions, const std::vector<VkExtensionProperties> &availableExtensions, const char *name)
	{
		for(const VkExtensionProperties &extension : availableExtensions)
		{
			if(std::strcmp(extension.extensionName, name) == 0)
			{
				enabledExtensions.push_back(name);
				return true;
			}
		}

		return false;
	}

	bool VulkanDevice::GetDeviceLUID(uint8 *luid) const
	{
		if(!_hasDeviceLUID || !luid) return false;

		for(size_t i = 0; i < VK_LUID_SIZE; i++)
			luid[i] = _deviceLUID[i];

		return true;
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

	bool VulkanDevice::CreateDevice(const std::vector<const char *> &extensions, VulkanGraphicsProvider *graphicsProvider)
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

#if RN_PLATFORM_WINDOWS
		if(AddDeviceExtensionIfAvailable(deviceExtensions, rawDeviceExtensions, VK_EXT_FULL_SCREEN_EXCLUSIVE_EXTENSION_NAME))
			_supportsFullscreenExclusive = true;
#endif

		bool supportsMultiview = AddDeviceExtensionIfAvailable(deviceExtensions, rawDeviceExtensions, VK_KHR_MULTIVIEW_EXTENSION_NAME);
		bool supportsFragmentDensityMap = AddDeviceExtensionIfAvailable(deviceExtensions, rawDeviceExtensions, VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME);
		bool supportsFragmentDensityMap2 = AddDeviceExtensionIfAvailable(deviceExtensions, rawDeviceExtensions, VK_EXT_FRAGMENT_DENSITY_MAP_2_EXTENSION_NAME);
		bool supportsShaderFloat16Int8 = AddDeviceExtensionIfAvailable(deviceExtensions, rawDeviceExtensions, VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME);

		VkPhysicalDeviceMultiviewPropertiesKHR multiviewProperties = {};
		multiviewProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_PROPERTIES_KHR;

		VkPhysicalDeviceFragmentDensityMapPropertiesEXT fragmentDensityMapProperties = {};
		fragmentDensityMapProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_PROPERTIES_EXT;

		VkPhysicalDeviceFragmentDensityMap2PropertiesEXT fragmentDensityMap2Properties = {};
		fragmentDensityMap2Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_2_PROPERTIES_EXT;

		void *optionalProperties = nullptr;
		if(supportsMultiview)
		{
			multiviewProperties.pNext = optionalProperties;
			optionalProperties = &multiviewProperties;
		}
		if(supportsFragmentDensityMap)
		{
			fragmentDensityMapProperties.pNext = optionalProperties;
			optionalProperties = &fragmentDensityMapProperties;
		}
		if(supportsFragmentDensityMap2)
		{
			fragmentDensityMap2Properties.pNext = optionalProperties;
			optionalProperties = &fragmentDensityMap2Properties;
		}

		if(optionalProperties)
		{
			VkPhysicalDeviceProperties2 deviceProperties = {};
			deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			deviceProperties.pNext = optionalProperties;

			vk::GetPhysicalDeviceProperties2(_physicalDevice, &deviceProperties);
		}

		if(supportsMultiview)
		{
			_maxMultiviewViewCount = multiviewProperties.maxMultiviewViewCount;

			RNDebug("Maximum number of multiviews: " << _maxMultiviewViewCount << " instances: " << multiviewProperties.maxMultiviewInstanceIndex);
		}

		if(supportsFragmentDensityMap)
		{
			_minFragmentDensityTexelSize.x = fragmentDensityMapProperties.minFragmentDensityTexelSize.width;
			_minFragmentDensityTexelSize.y = fragmentDensityMapProperties.minFragmentDensityTexelSize.height;
			_maxFragmentDensityTexelSize.x = fragmentDensityMapProperties.maxFragmentDensityTexelSize.width;
			_maxFragmentDensityTexelSize.y = fragmentDensityMapProperties.maxFragmentDensityTexelSize.height;
			_supportsFragmentDensityMaps = true;
		}

		if(supportsFragmentDensityMap2)
		{
			_hasFragmentDensitySubsampledLoads = fragmentDensityMap2Properties.subsampledLoads;
			_hasFragmentDensitySubsampledCoarseReconstructionEarlyAccess = fragmentDensityMap2Properties.subsampledCoarseReconstructionEarlyAccess;
			_maxFragmentDensitySubsampledLayers = fragmentDensityMap2Properties.maxSubsampledArrayLayers;
			_maxFragmentDensitySubsampledSamplers = fragmentDensityMap2Properties.maxDescriptorSetSubsampledSamplers;
			_supportsFragmentDensityMaps2 = true;
		}

		if(AddDeviceExtensionIfAvailable(deviceExtensions, rawDeviceExtensions, VK_QCOM_TILE_PROPERTIES_EXTENSION_NAME))
		{
			_supportsTileProperties = true;
		}

#if RN_PLATFORM_WINDOWS
		_supportsExternalTextureImport = AddDeviceExtensionIfAvailable(deviceExtensions, rawDeviceExtensions, VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
		_supportsExternalTextureSynchronization = AddDeviceExtensionIfAvailable(deviceExtensions, rawDeviceExtensions, VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME);
#endif

		VkPhysicalDeviceTilePropertiesFeaturesQCOM tilePropertiesFeatures = {};
		tilePropertiesFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_TILE_PROPERTIES_FEATURES_QCOM;

		VkPhysicalDeviceFragmentDensityMap2FeaturesEXT fragmentDensityMap2Features = {};
		fragmentDensityMap2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_2_FEATURES_EXT;
		fragmentDensityMap2Features.pNext = &tilePropertiesFeatures;

		VkPhysicalDeviceFragmentDensityMapFeaturesEXT fragmentDensityMapFeatures = {};
		fragmentDensityMapFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FRAGMENT_DENSITY_MAP_FEATURES_EXT;
		fragmentDensityMapFeatures.pNext = &fragmentDensityMap2Features;

		VkPhysicalDeviceMultiviewFeaturesKHR multiviewFeatures = {};
		multiviewFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_MULTIVIEW_FEATURES_KHR;
		multiviewFeatures.pNext = &fragmentDensityMapFeatures;

		VkPhysicalDeviceFeatures2KHR features = {};
		features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2_KHR;
		features.pNext = &multiviewFeatures;

		VkPhysicalDeviceShaderFloat16Int8FeaturesKHR shaderFloat16Int8Features = {};
		shaderFloat16Int8Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES_KHR;
		if(supportsShaderFloat16Int8)
		{
			shaderFloat16Int8Features.pNext = features.pNext;
			features.pNext = &shaderFloat16Int8Features;
		}

		vk::GetPhysicalDeviceFeatures2(_physicalDevice, &features);

		_supportsSamplerAnisotropy = (features.features.samplerAnisotropy == VK_TRUE);
		_supportsMultiDrawIndirect = (features.features.multiDrawIndirect == VK_TRUE);
		_supportsShaderFloat16 = supportsShaderFloat16Int8 && (shaderFloat16Int8Features.shaderFloat16 == VK_TRUE);
		_supportsShaderInt8 = supportsShaderFloat16Int8 && (shaderFloat16Int8Features.shaderInt8 == VK_TRUE);

		if(_maxMultiviewViewCount <= 1 || (multiviewFeatures.multiview != VK_TRUE))
		{
			_maxMultiviewViewCount = 1;
			deviceExtensions.erase(std::remove_if(deviceExtensions.begin(), deviceExtensions.end(), [](const char *name){
				return std::strcmp(name, VK_KHR_MULTIVIEW_EXTENSION_NAME) == 0;
			}), deviceExtensions.end());
		}

		_supportsFragmentDensityMaps = _supportsFragmentDensityMaps && (fragmentDensityMapFeatures.fragmentDensityMap == VK_TRUE);
		if(!_supportsFragmentDensityMaps)
		{
			deviceExtensions.erase(std::remove_if(deviceExtensions.begin(), deviceExtensions.end(), [](const char *name){
				return std::strcmp(name, VK_EXT_FRAGMENT_DENSITY_MAP_EXTENSION_NAME) == 0;
			}), deviceExtensions.end());

			multiviewFeatures.pNext = fragmentDensityMap2Features.pNext;
		}

		//fragment density maps 2 doesn't have a bool for the extension itself, only for the deferred feature that is part of it, but not used by Rayne right now

		_supportsTileProperties = _supportsTileProperties && (tilePropertiesFeatures.tileProperties == VK_TRUE);
		if(!_supportsTileProperties)
		{
			deviceExtensions.erase(std::remove_if(deviceExtensions.begin(), deviceExtensions.end(), [](const char *name){
				return std::strcmp(name, VK_QCOM_TILE_PROPERTIES_EXTENSION_NAME) == 0;
			}), deviceExtensions.end());

			if(multiviewFeatures.pNext == &fragmentDensityMapFeatures)
			{
				fragmentDensityMap2Features.pNext = nullptr;
			}
			else
			{
				multiviewFeatures.pNext = nullptr;
			}
		}

		std::vector<const char *> deduplicatedDeviceExtensions;
		deduplicatedDeviceExtensions.reserve(deviceExtensions.size());
		std::unordered_set<std::string> uniqueDeviceExtensions;
		for(const char *name : deviceExtensions)
		{
			if(uniqueDeviceExtensions.insert(name).second)
			{
				deduplicatedDeviceExtensions.push_back(name);
			}
		}
		deviceExtensions.swap(deduplicatedDeviceExtensions);

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

		VkResult result = graphicsProvider? graphicsProvider->CreateVulkanDevice(vk::GetInstanceProcAddr, _physicalDevice, &deviceInfo, nullptr, &_device) : vk::CreateDevice(_physicalDevice, &deviceInfo, nullptr, &_device);
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
