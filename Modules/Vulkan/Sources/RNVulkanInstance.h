//
//  RNVulkanInstance.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANINSTANCE_H_
#define __RAYNE_VULKANINSTANCE_H_

#include "RNVulkan.h"
#include "RNVulkanDispatchTable.h"

namespace RN
{
	class VulkanInstance
	{
	public:
		VulkanInstance();
		~VulkanInstance();

		bool LoadVulkan();

		Array *GetDevices() const { return _devices; }

		VkInstance GetInstance() const { return _instance; }

		VkResult EnumerateExtensions(const char *layer, std::vector<VkExtensionProperties> &extensions) const;
		VkResult EnumerateDeviceExtensions(VkPhysicalDevice device, const char *layer, std::vector<VkExtensionProperties> &extensions) const;

		VkResult EnumerateDevices(std::vector<VkPhysicalDevice> &devices) const;
		VkResult EnumerateDevicesWithExtensions(std::vector<VkPhysicalDevice> &devices, const std::vector<const char *> &extensions) const;

		const std::vector<const char *> &GetDeviceExtensions() { return _requiredDeviceExtensions; }

	private:
		bool DeviceSupportsExtensions(VkPhysicalDevice device, const std::vector<const char *> &extensions) const;

		HMODULE _module;
		VkInstance _instance;
		VkAllocationCallbacks *_allocationCallbacks;

		Array *_devices;

		std::vector<const char *> _requiredExtensions;
		std::vector<const char *> _requiredDeviceExtensions;
	};

	extern String *GetStringForVulkanResult(VkResult result);

#define RNDumpVulkanResult(result) \
	RNWarning("Vulkan: Encountered " << GetStringForVulkanResult(result));
}


#endif /* __RAYNE_VULKANINSTANCE_H_ */
