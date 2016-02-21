//
//  RNVulkanDevice.h
//  Rayne
//
//  Copyright 2016 by Überpixel. All rights reserved.
//  Unauthorized use is punishable by torture, mutilation, and vivisection.
//

#ifndef __RAYNE_VULKANDEVICE_H_
#define __RAYNE_VULKANDEVICE_H_

#include <Rayne.h>
#include "RNVulkanDispatchTable.h"
#include "RNVulkanInstance.h"

namespace RN
{
	class VulkanDevice : public RenderingDevice
	{
	public:
		friend class VulkanInstance;

		bool CreateDevice(const std::vector<const char *> &extensions);
		bool IsValidDevice() const { return (_gameQueue != kRNNotFound && _presentQueue != kRNNotFound); }

		void GetQueueProperties(std::vector<VkQueueFamilyProperties> &queues);
		VkResult GetSurfaceFormats(VkSurfaceKHR surface, std::vector<VkSurfaceFormatKHR> &formats);
		VkResult GetPresentModes(VkSurfaceKHR surface, std::vector<VkPresentModeKHR> &modes);

		VkDevice GetDevice() const { return _device; }
		VkPhysicalDevice GetPhysicalDevice() const { return _physicalDevice; }

		uint32_t GetGameQueue() const { return static_cast<uint32_t>(_gameQueue); }
		uint32_t GetPresentQueue() const { return static_cast<uint32_t>(_presentQueue); }

	private:
		VulkanDevice(VulkanInstance *instance, VkPhysicalDevice device);

		static Descriptor DescriptorForDevice(VkPhysicalDevice device);

		VulkanInstance *_instance;
		VkPhysicalDevice _physicalDevice;
		VkDevice _device;

		size_t _gameQueue;
		size_t _presentQueue;

		RNDeclareMeta(VulkanDevice)
	};
}


#endif /* __RAYNE_VULKANDEVICE_H_ */
