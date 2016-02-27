//
//  RNVulkanDevice.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANDEVICE_H_
#define __RAYNE_VULKANDEVICE_H_

#include "RNVulkan.h"
#include "RNVulkanDispatchTable.h"
#include "RNVulkanInstance.h"

namespace RN
{
	class VulkanDevice : public RenderingDevice
	{
	public:
		friend class VulkanInstance;

		VKAPI bool CreateDevice(const std::vector<const char *> &extensions);
		bool IsValidDevice() const { return (_workQueue != kRNNotFound); }

		VKAPI void GetQueueProperties(std::vector<VkQueueFamilyProperties> &queues);
		VKAPI VkResult GetSurfaceFormats(VkSurfaceKHR surface, std::vector<VkSurfaceFormatKHR> &formats);
		VKAPI VkResult GetPresentModes(VkSurfaceKHR surface, std::vector<VkPresentModeKHR> &modes);

		VkDevice GetDevice() const { return _device; }
		VkPhysicalDevice GetPhysicalDevice() const { return _physicalDevice; }

		uint32_t GetWorkQueue() const { return static_cast<uint32_t>(_workQueue); }

		VKAPI VkBool32 GetMemoryWithType(uint32_t typeBits, VkFlags properties, uint32_t &typeIndex) const;
		VKAPI size_t GetMemoryWithType(VkMemoryPropertyFlagBits required) const;

	private:
		VulkanDevice(VulkanInstance *instance, VkPhysicalDevice device);

		static Descriptor DescriptorForDevice(VkPhysicalDevice device);

		VulkanInstance *_instance;
		VkPhysicalDevice _physicalDevice;
		VkDevice _device;

		size_t _workQueue;

		VkPhysicalDeviceMemoryProperties _memoryProperties;

		RNDeclareMetaAPI(VulkanDevice, VKAPI)
	};
}


#endif /* __RAYNE_VULKANDEVICE_H_ */
